#include "wiiu_latte_execute.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Usage: prefix is the absolute path ending in botw_vertex_failure_00.
   Captured files stay local; this tool contains no game bytes. */
static int load(const char* prefix,const char* suffix,void* bytes,size_t size) {
    char path[4096];
    int n=snprintf(path,sizeof(path),"%s_%s",prefix,suffix);
    if(n<0 || (size_t)n>=sizeof(path))return 0;
    FILE* f=fopen(path,"rb");if(!f)return 0;
    size_t got=fread(bytes,1,size,f);int tail=fgetc(f);fclose(f);
    return got==size && tail==EOF;
}
static bool missing_sample(void* user,unsigned texture,unsigned sampler,
    float u,float v,float lod,float result[4]) {
    (void)sampler;(void)u;(void)v;(void)lod;(void)result;
    *(bool*)user=true;
    fprintf(stderr,"replay needs unrecorded texture sample %u\n",texture);return false;
}
static bool missing_layer(void* user,unsigned texture,unsigned sampler,
    float u,float v,float layer,float lod,float result[4]) {
    (void)result;*(bool*)user=true;
    fprintf(stderr,"replay needs unrecorded texture=%u sampler=%u uv=(%.9g,%.9g) layer=%.9g lod=%.9g\n",
        texture,sampler,u,v,layer,lod);return false;
}
static bool missing_info(void* user,unsigned texture,uint32_t lod,uint32_t result[4]) {
    (void)lod;(void)result;*(bool*)user=true;
    fprintf(stderr,"replay needs unrecorded texture info %u\n",texture);return false;
}
static int self_test(void) {
    /* MOV R1.x (undefined) -> R2.x, in a one-instruction ALU clause. */
    uint32_t words[]={1,0x20000000u,0x80000001u,0x00400c90u};
    uint8_t program[sizeof(words)];
    for(unsigned i=0;i<sizeof(words)/4;++i)
        for(unsigned b=0;b<4;++b)program[i*4+b]=(uint8_t)(words[i]>>(b*8));
    WiiULatteVertexInputs in={0};WiiULatteVertexOutputs out;
    WiiULatteVertexFailure failure;
    if(wiiu_latte_execute_vertex_diagnostic(program,sizeof(program),&in,&out,&failure) ||
       !failure.line || failure.cf_slot!=0 || failure.instruction_slot!=1 ||
       failure.opcode!=0x19 || failure.source_select!=1 || failure.source_channel!=0 ||
       strcmp(failure.reason,"uninitialized GPR component")) {
        fprintf(stderr,"failure context self-test failed: %s cf=%u instruction=%u op=%X\n",
            failure.reason,failure.cf_slot,failure.instruction_slot,failure.opcode);return 1;
    }
    /* An effect may initialize XYZ for culling but never initialize W. Keep
       that visible as a precise rejection; do not invent position data. */
    words[0]=0xA03C;words[1]=0x94200688u;
    for(unsigned i=0;i<2;++i)
        for(unsigned b=0;b<4;++b)program[i*4+b]=(uint8_t)(words[i]>>(b*8));
    in.valid[1]=7;
    if(wiiu_latte_execute_vertex_diagnostic(program,8,&in,&out,&failure) ||
       failure.cf_slot!=0 || failure.source_select!=1 || failure.source_channel!=3 ||
       failure.detail0!=7 || failure.detail1!=3 ||
       strcmp(failure.reason,"uninitialized exported position component")) {
        fprintf(stderr,"undefined exported W diagnostic failed\n");return 1;
    }
    puts("first failure preserves instruction/source context and undefined exported W");return 0;
}
int main(int argc,char** argv) {
    if(argc==2 && !strcmp(argv[1],"--self-test"))return self_test();
    if(argc!=2) {fprintf(stderr,"usage: latte_vertex_failure_replay capture-file-prefix | --self-test\n");return 2;}
    char path[4096],line[512];
    int n=snprintf(path,sizeof(path),"%s_meta.txt",argv[1]);
    if(n<0 || (size_t)n>=sizeof(path))return 2;
    FILE* meta=fopen(path,"rb");if(!meta){fprintf(stderr,"cannot open failure metadata\n");return 2;}
    unsigned version=0,size=0,uniform_count=0,complete=0,seen=0;
    unsigned expected_line=0,expected_cf=UINT32_MAX,expected_instruction=UINT32_MAX;
    unsigned requested[16]={0},captured[16]={0},present[16]={0};
    while(fgets(line,sizeof(line),meta)) {
        unsigned slot,bytes,saved,mapped,address;
        if(sscanf(line,"version %u",&version)==1)continue;
        if(sscanf(line,"program_size %u",&size)==1)continue;
        if(sscanf(line,"uniform_count %u",&uniform_count)==1)continue;
        if(sscanf(line,"capture_complete %u",&complete)==1)continue;
        if(sscanf(line,"failure_line %u",&expected_line)==1)continue;
        if(sscanf(line,"cf_slot %u",&expected_cf)==1)continue;
        if(sscanf(line,"instruction_slot %u",&expected_instruction)==1)continue;
        if(sscanf(line,"block %u %u %u %u %x",&slot,&bytes,&saved,&mapped,&address)==5) {
            if(slot>=16 || bytes>4u*1024u*1024u || saved>bytes || mapped>1) {fclose(meta);return 2;}
            requested[slot]=bytes;captured[slot]=saved;present[slot]=mapped;seen|=1u<<slot;
        }
    }
    fclose(meta);
    if(version!=1 || !complete || !size || size>1048576 || size%8 ||
       uniform_count>1024 || seen!=65535 || !expected_line) {
        fprintf(stderr,"unsupported or incomplete capture\n");return 2;
    }
    uint8_t* program=malloc(size);uint32_t uniforms[1024]={0};uint8_t valid[1024]={0};
    WiiULatteVertexInputs in={0};WiiULatteVertexOutputs out;WiiULatteVertexFailure failure;
    int status=2;bool needs_texture=false;
    if(!program || !load(argv[1],"program.bin",program,size) ||
       !load(argv[1],"gpr.bin",in.gpr,sizeof(in.gpr)) ||
       !load(argv[1],"valid.bin",in.valid,sizeof(in.valid)))goto done;
    if(uniform_count && (!load(argv[1],"uniforms.bin",uniforms,uniform_count*4u) ||
        !load(argv[1],"uniform_valid.bin",valid,uniform_count)))goto done;
    in.uniforms=uniforms;in.uniform_valid=valid;in.uniform_count=uniform_count;
    for(unsigned slot=0;slot<16;++slot) {
        in.block_sizes[slot]=requested[slot];
        if(!present[slot])continue;
        if(captured[slot]!=requested[slot])goto done;
        if(!captured[slot])continue;
        uint8_t* block=malloc(captured[slot]);in.blocks[slot]=block;
        char suffix[64];snprintf(suffix,sizeof(suffix),"block_%02u.bin",slot);
        if(!block || !load(argv[1],suffix,block,captured[slot]))goto done;
    }
    in.sample_texture=missing_sample;in.texture_info=missing_info;in.texture_user=&needs_texture;
    in.sample_texture_layer=missing_layer;
    if(wiiu_latte_execute_vertex_diagnostic(program,size,&in,&out,&failure)) {
        printf("captured vertex now executes: clip=(%g,%g,%g,%g)\n",
            out.position[0],out.position[1],out.position[2],out.position[3]);status=0;
    } else {
        printf("replay rejection: line=%u cf=%u instruction=%u opcode=%X words=%08X,%08X source=%u/%u/%u detail=%08X,%08X reason=%s\n",
            failure.line,failure.cf_slot,failure.instruction_slot,failure.opcode,
            failure.word0,failure.word1,failure.source_index,failure.source_select,
            failure.source_channel,failure.detail0,failure.detail1,failure.reason);
        if(!needs_texture && failure.line==expected_line && failure.cf_slot==expected_cf &&
           failure.instruction_slot==expected_instruction) {puts("original rejection reproduced");status=0;}
        else fprintf(stderr,"replay differs from saved rejection or needs texture data\n");
    }
done:
    for(unsigned slot=0;slot<16;++slot)free((void*)in.blocks[slot]);free(program);
    return status;
}
