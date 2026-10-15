#include "wiiu_gx2.h"
#include "wiiu_gx2_software.h"
#include "wiiu_latte_execute.h"
#include "wiiu_latte_fetch.h"
#include "wiiu_memory.h"
#include "wiiu_window.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void le(CPUState* cpu,u32 a,u32 v){for(u32 c=0;c<4;++c)mem_write8(cpu,a+c,(u8)(v>>(c*8)));}
static u32 bits(float f){u32 b;memcpy(&b,&f,4);return b;}
static void pair(u8* p,u32 index,u32 a,u32 b){for(u32 c=0;c<4;++c){p[index*8+c]=(u8)(a>>(c*8));p[index*8+4+c]=(u8)(b>>(c*8));}}
int main(void) {
    WiiUMemory memory;CPUState cpu={0};wiiu_memory_init(&memory);
    const u32 base=0x11000000,texture=base,sampler=base+256,data=base+4096;
    CHECK(wiiu_memory_add_segment(&memory,"vertex texture fixture",base,65536,NULL,true));
    wiiu_memory_bind_cpu(&memory,&cpu);wiiu_gx2_reset();
    const u32 point=2|(2<<3),linear=point|(1<<9)|(1<<12);
    /* Standard DXT endpoints put red in the high bits. The uncompressed
       Latte R5_G6_B5 format deliberately uses the opposite order. */
    WiiUGX2TextureView color_view={data,16,4,4,1,0x31,16,0,1,0x00010203,0};
    u8 color_pixels[64];
    for(u32 format=0x31;format<=0x33;++format) {
        color_view.format=format;color_view.image_size=format==0x31?8:16;
        u32 color_data=data+(format==0x31?0:8);
        le(&cpu,data,0xffffffff);le(&cpu,data+4,0xffffffff);
        if(format==0x33){le(&cpu,data,0x0000ffff);le(&cpu,data+4,0);}
        le(&cpu,color_data,0x001ff800);le(&cpu,color_data+4,0xe4e4e4e4);
        CHECK(wiiu_gx2_software_blit(&cpu,&color_view,color_pixels,4,4,16,0,0,4,4,false));
        CHECK(color_pixels[0]==0 && color_pixels[2]==255 && color_pixels[3]==255);
        CHECK(color_pixels[4]==255 && color_pixels[6]==0 && color_pixels[7]==255);
        CHECK(color_pixels[8]==85 && color_pixels[10]==170);
        CHECK(color_pixels[12]==170 && color_pixels[14]==85);
    }
    color_view.format=8;color_view.width=color_view.height=color_view.pitch=1;color_view.image_size=2;
    le(&cpu,data,0x001f);
    CHECK(wiiu_gx2_software_blit(&cpu,&color_view,color_pixels,1,1,4,0,0,1,1,false));
    CHECK(color_pixels[0]==0 && color_pixels[2]==255);
    /* sRGB is decoded before bilinear filtering or component selection;
       alpha and synthesized constants never get gamma corrected. */
    color_view.format=0x41a;color_view.image_size=8;color_view.width=color_view.pitch=2;
    le(&cpu,data,0x800a4080);le(&cpu,data+4,0xff0000ff);
    float color_value[4];
    CHECK(wiiu_gx2_software_sample_float(&cpu,&color_view,point,.25f,.5f,color_value));
    CHECK(fabsf(color_value[0]-.2158605f)<.00001f && fabsf(color_value[1]-.0512695f)<.00001f);
    CHECK(fabsf(color_value[2]-.00303527f)<.00001f && color_value[3]==128/255.0f);
    CHECK(wiiu_gx2_software_sample_float(&cpu,&color_view,linear,.5f,.5f,color_value));
    CHECK(fabsf(color_value[0]-(.2158605f+1)*.5f)<.00001f);
    color_view.comp_map=0x03050400;
    CHECK(wiiu_gx2_software_sample_float(&cpu,&color_view,point,.25f,.5f,color_value));
    CHECK(color_value[0]==128/255.0f && color_value[1]==1 && color_value[2]==0 &&
          fabsf(color_value[3]-.2158605f)<.00001f);
    /* Integer bone indices must reach MULLO_INT as integers, while existing
       layout consumers can still read numeric float attributes. */
    WiiULatteFetchStream integer_stream={0};
    integer_stream.location=3;integer_stream.format=0x1a;integer_stream.format_is_fetch=true;
    integer_stream.number_format=1;integer_stream.dest_sel=0x00010203;
    WiiULatteFetchBuffer integer_buffer={true,4,4,data};WiiULatteFetchedVertex integer_vertex;
    le(&cpu,data,0xff383130u);
    CHECK(wiiu_latte_fetch_vertex(&cpu,&integer_stream,1,&integer_buffer,1,0,0,&integer_vertex));
    CHECK(integer_vertex.gpr[3][0]==48 && integer_vertex.words[3][0]==48);
    CHECK(integer_vertex.words[3][1]==49 && integer_vertex.words[3][2]==56 && integer_vertex.words[3][3]==255);
    CHECK(integer_vertex.words[3][0]*3==144);
    integer_stream.format=0x0d;integer_stream.dest_sel=0x00040505;le(&cpu,data,0xffffffffu);
    CHECK(wiiu_latte_fetch_vertex(&cpu,&integer_stream,1,&integer_buffer,1,0,0,&integer_vertex));
    CHECK(integer_vertex.words[3][0]==0xffffffffu && integer_vertex.words[3][1]==0 && integer_vertex.words[3][3]==1);
    le(&cpu,data,bits(-2));le(&cpu,data+4,bits(4));
    WiiUGX2TextureView view={data,8,2,1,1,0x80e,16,0,2,0x00040505,0};float out[4];
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,point,.25f,.5f,out) && out[0]==-2);
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,point,.75f,.5f,out) && out[0]==4);
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,linear,.5f,.5f,out) && out[0]==1);
    CHECK(out[1]==0 && out[2]==1 && out[3]==1);
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,0,1.25f,.5f,out) && out[0]==-2);
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,1,1.25f,.5f,out) && out[0]==4);
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,6,-1,.5f,out) && out[0]==0 && out[3]==0);
    CHECK(!wiiu_gx2_software_sample_float(&cpu,&view,4,.5f,.5f,out));
    CHECK(!wiiu_gx2_software_sample_float(&cpu,&view,point,NAN,.5f,out));
    view.image_size=4;CHECK(!wiiu_gx2_software_sample_float(&cpu,&view,point,.75f,.5f,out));view.image_size=8;
    /* Half floats preserve HDR, negative and subnormal values. */
    le(&cpu,data,0x4C00C000);le(&cpu,data+4,0x3C000001);
    view.width=view.pitch=1;view.format=0x820;view.comp_map=0x00010203;
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,point,.5f,.5f,out));
    CHECK(out[0]==-2 && out[1]==16 && out[2]==ldexpf(1,-24) && out[3]==1);
    /* Packed unsigned HDR has 5-bit exponents, 6/6/5-bit mantissas and no
       alpha field. These known values also catch incorrect channel shifts. */
    le(&cpu,data,0x3c0u|(0x400u<<11)|(0x1c0u<<22));
    view.format=0x816;view.image_size=4;
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,point,.5f,.5f,out));
    CHECK(out[0]==1 && out[1]==2 && out[2]==.5f && out[3]==1);
    float decoded_numeric[4];
    CHECK(wiiu_gx2_software_decode_float(&cpu,&view,decoded_numeric));
    CHECK(!memcmp(decoded_numeric,out,sizeof(out)));
    le(&cpu,data,1u|((30u*64u+63u)<<11)|(1u<<22));
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,point,.5f,.5f,out));
    CHECK(out[0]==ldexpf(1,-20) && out[1]==65024 && out[2]==ldexpf(1,-19));
    le(&cpu,data,0xffff8000u);view.format=5;view.image_size=8;
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,point,.5f,.5f,out));
    CHECK(out[0]==32768/65535.0f && out[1]==0 && out[2]==0 && out[3]==1);
    le(&cpu,data+4,0x55550000u);view.format=0x1f;
    CHECK(wiiu_gx2_software_sample_float(&cpu,&view,point,.5f,.5f,out));
    CHECK(out[0]==32768/65535.0f && out[1]==1 && out[2]==0 && out[3]==1/3.0f);
    /* GPU readback sampling applies the view swizzle exactly once, preserves
       RG defaults, and uses the same address/filter rules as guest textures. */
    const u8 bgra[8]={200,64,0,17,100,128,255,33};
    view.width=2;view.format=7;view.comp_map=0x00010203;
    CHECK(wiiu_gx2_software_sample_bgra(bgra,&view,linear,.5f,.5f,out));
    CHECK(out[0]==.5f && fabsf(out[1]-96/255.0f)<.00001f && out[2]==0 && out[3]==1);
    view.comp_map=0x01000504;
    CHECK(wiiu_gx2_software_sample_bgra(bgra,&view,point,.25f,.5f,out));
    CHECK(out[0]==64/255.0f && out[1]==0 && out[2]==1 && out[3]==0);
    CHECK(wiiu_gx2_software_sample_bgra(bgra,&view,6,-1,.5f,out) && out[3]==0);
    view.format=0x820;CHECK(!wiiu_gx2_software_sample_bgra(bgra,&view,point,0,0,out));
    /* Real GX2 texture/sampler binding, recording and context restoration. */
    cpu.gpr[3]=base+0x2000;cpu.gpr[4]=0;wiiu_gx2_handle_import(&cpu,"GX2SetupContextStateEx");
    le(&cpu,data,bits(-2));le(&cpu,data+4,bits(4));
    mem_write32(&cpu,texture,1);mem_write32(&cpu,texture+4,2);mem_write32(&cpu,texture+8,1);
    mem_write32(&cpu,texture+12,1);mem_write32(&cpu,texture+0x14,0x80e);
    mem_write32(&cpu,texture+0x20,8);mem_write32(&cpu,texture+0x24,data);
    mem_write32(&cpu,texture+0x30,16);mem_write32(&cpu,texture+0x3c,2);
    mem_write32(&cpu,texture+0x84,0x00040505);
    cpu.gpr[3]=texture;cpu.gpr[4]=3;wiiu_gx2_handle_import(&cpu,"GX2SetVertexTexture");
    mem_write32(&cpu,sampler,linear);mem_write32(&cpu,sampler+4,14u<<16);
    cpu.gpr[3]=sampler;cpu.gpr[4]=5;wiiu_gx2_handle_import(&cpu,"GX2SetVertexSampler");
    CHECK(wiiu_gx2_sample_vertex_texture(&cpu,3,5,.5f,.5f,0,out) && out[0]==1);
    CHECK(!wiiu_gx2_sample_vertex_texture(&cpu,3,5,.5f,.5f,1,out));
    CHECK(!wiiu_gx2_sample_vertex_texture(&cpu,3,6,.5f,.5f,0,out));
    /* Captured scene opcode pattern: SAMPLE_L with separate texture/sampler. */
    u8 program[144]={0};pair(program,0,16,0x80800000);pair(program,1,0xA03C,0x94200688);
    pair(program,16,0x00000311,0xF00D1001);pair(program,17,0x68828000,0xDEADFEC);
    WiiULatteVertexInputs in={0};WiiULatteVertexOutputs vertex;
    in.valid[0]=15;in.gpr[0][0]=in.gpr[0][1]=bits(.5f);
    in.sample_texture=wiiu_gx2_sample_vertex_texture;in.texture_user=&cpu;
    CHECK(wiiu_latte_execute_vertex(program,sizeof(program),&in,&vertex));
    CHECK(vertex.position[0]==1 && vertex.position[1]==0 && vertex.position[2]==1 && vertex.position[3]==1);
    pair(program,16,0x00000310,0xF00D1001);
    CHECK(wiiu_latte_execute_vertex(program,sizeof(program),&in,&vertex) && vertex.position[0]==1);
    pair(program,16,0x00000311,0xF00D1001);
    in.sample_texture=NULL;CHECK(!wiiu_latte_execute_vertex(program,sizeof(program),&in,&vertex));
    in.sample_texture=wiiu_gx2_sample_vertex_texture;in.gpr[0][3]=bits(1);
    CHECK(!wiiu_latte_execute_vertex(program,sizeof(program),&in,&vertex));
    in.gpr[0][3]=0;
    cpu.gpr[3]=base+0x5000;cpu.gpr[4]=256;wiiu_gx2_handle_import(&cpu,"GX2BeginDisplayList");
    mem_write32(&cpu,sampler,point);cpu.gpr[3]=sampler;cpu.gpr[4]=5;
    wiiu_gx2_handle_import(&cpu,"GX2SetVertexSampler");
    cpu.gpr[3]=base+0x5000;wiiu_gx2_handle_import(&cpu,"GX2EndDisplayList");u32 used=cpu.gpr[3];
    mem_write32(&cpu,sampler,linear);
    CHECK(wiiu_gx2_sample_vertex_texture(&cpu,3,5,.5f,.5f,0,out) && out[0]==1);
    cpu.gpr[3]=base+0x5000;cpu.gpr[4]=used;wiiu_gx2_handle_import(&cpu,"GX2CallDisplayList");
    CHECK(wiiu_gx2_sample_vertex_texture(&cpu,3,5,.5f,.5f,0,out) && out[0]==4);
    /* The captured terrain shader uses SAMPLE_L with an unnormalized Z
       layer. Distinct array layers and a nonzero view start must survive
       the instruction, GX2 binding, and CPU sampler without layer-zero fallback. */
    le(&cpu,data,0xffff0000u);le(&cpu,data+4,0x80008000u);le(&cpu,data+8,0x0000ffffu);
    mem_write32(&cpu,texture,5);mem_write32(&cpu,texture+12,3);
    mem_write32(&cpu,texture+0x14,5);mem_write32(&cpu,texture+0x20,12);
    mem_write32(&cpu,texture+0x7c,1);mem_write32(&cpu,texture+0x80,2);
    cpu.gpr[3]=texture;cpu.gpr[4]=3;wiiu_gx2_handle_import(&cpu,"GX2SetVertexTexture");
    CHECK(wiiu_gx2_sample_vertex_texture_layer(&cpu,3,5,.25f,.5f,0,0,out) && out[0]==32768/65535.0f);
    CHECK(wiiu_gx2_sample_vertex_texture_layer(&cpu,3,5,.25f,.5f,1,0,out) && out[0]==1);
    CHECK(wiiu_gx2_sample_vertex_texture_layer(&cpu,3,5,.25f,.5f,100,0,out) && out[0]==1);
    CHECK(!wiiu_gx2_sample_vertex_texture_layer(&cpu,3,5,.25f,.5f,NAN,0,out));
    CHECK(!wiiu_gx2_sample_vertex_texture(&cpu,3,5,.25f,.5f,0,out));
    in.sample_texture_layer=wiiu_gx2_sample_vertex_texture_layer;
    in.gpr[0][0]=bits(.25f);in.gpr[0][2]=bits(1);in.gpr[0][3]=0;
    pair(program,16,0x00000311,0xB00D1001);
    CHECK(wiiu_latte_execute_vertex(program,sizeof(program),&in,&vertex) && vertex.position[0]==1);
    pair(program,16,0x00000311,0xF00D1001); /* normalized Z is not an array layer */
    CHECK(!wiiu_latte_execute_vertex(program,sizeof(program),&in,&vertex));
    /* Independent mip allocation: level zero is black, level one has a
       white layer followed by a black layer. Check level selection, layer
       retention, mip blending and a truncated allocation on real tiling. */
    const u32 array_data=base+0x100000,array_mips=array_data+65536;
    CHECK(wiiu_memory_add_segment(&memory,"array mip fixture",array_data,131072,NULL,true));
    for(u32 byte=0;byte<8192;++byte)mem_write8(&cpu,array_mips+byte,255);
    mem_write32(&cpu,texture+4,128);mem_write32(&cpu,texture+8,128);
    mem_write32(&cpu,texture+12,2);mem_write32(&cpu,texture+0x10,2);
    mem_write32(&cpu,texture+0x20,65536);mem_write32(&cpu,texture+0x24,array_data);
    mem_write32(&cpu,texture+0x28,16384);mem_write32(&cpu,texture+0x2c,array_mips);
    mem_write32(&cpu,texture+0x30,4);mem_write32(&cpu,texture+0x3c,128);
    mem_write32(&cpu,texture+0x40,0);mem_write32(&cpu,texture+0x7c,0);
    cpu.gpr[3]=texture;cpu.gpr[4]=3;wiiu_gx2_handle_import(&cpu,"GX2SetVertexTexture");
    mem_write32(&cpu,sampler,point|(1u<<17));cpu.gpr[3]=sampler;cpu.gpr[4]=5;
    wiiu_gx2_handle_import(&cpu,"GX2SetVertexSampler");
    CHECK(wiiu_gx2_sample_vertex_texture_layer(&cpu,3,5,.25f,.5f,0,0,out) && out[0]==0);
    CHECK(wiiu_gx2_sample_vertex_texture_layer(&cpu,3,5,.25f,.5f,0,1,out) && out[0]==1);
    CHECK(wiiu_gx2_sample_vertex_texture_layer(&cpu,3,5,.25f,.5f,1,1,out) && out[0]==0);
    mem_write32(&cpu,sampler,point|(2u<<17));cpu.gpr[3]=sampler;cpu.gpr[4]=5;
    wiiu_gx2_handle_import(&cpu,"GX2SetVertexSampler");
    CHECK(wiiu_gx2_sample_vertex_texture_layer(&cpu,3,5,.25f,.5f,0,.25f,out) && out[0]==.25f);
    mem_write32(&cpu,texture+0x28,1);cpu.gpr[3]=texture;cpu.gpr[4]=3;
    wiiu_gx2_handle_import(&cpu,"GX2SetVertexTexture");
    CHECK(!wiiu_gx2_sample_vertex_texture_layer(&cpu,3,5,.25f,.5f,0,1,out));
    mem_write32(&cpu,sampler,point);cpu.gpr[3]=sampler;cpu.gpr[4]=5;
    wiiu_gx2_handle_import(&cpu,"GX2SetVertexSampler");
    in.sample_texture_layer=NULL;
    le(&cpu,data,bits(-2));le(&cpu,data+4,bits(4));
    mem_write32(&cpu,texture,1);mem_write32(&cpu,texture+12,1);
    mem_write32(&cpu,texture+4,2);mem_write32(&cpu,texture+8,1);mem_write32(&cpu,texture+0x10,1);
    mem_write32(&cpu,texture+0x24,data);mem_write32(&cpu,texture+0x28,0);mem_write32(&cpu,texture+0x2c,0);
    mem_write32(&cpu,texture+0x30,16);mem_write32(&cpu,texture+0x3c,2);
    mem_write32(&cpu,texture+0x14,0x80e);mem_write32(&cpu,texture+0x20,8);
    mem_write32(&cpu,texture+0x7c,0);mem_write32(&cpu,texture+0x80,1);
    cpu.gpr[3]=texture;cpu.gpr[4]=3;wiiu_gx2_handle_import(&cpu,"GX2SetVertexTexture");
    cpu.gpr[3]=base+0x3000;cpu.gpr[4]=0;wiiu_gx2_handle_import(&cpu,"GX2SetupContextStateEx");
    CHECK(!wiiu_gx2_sample_vertex_texture(&cpu,3,5,.5f,.5f,0,out));
    cpu.gpr[3]=base+0x2000;wiiu_gx2_handle_import(&cpu,"GX2SetContextState");
    CHECK(wiiu_gx2_sample_vertex_texture(&cpu,3,5,.5f,.5f,0,out) && out[0]==4);
    /* Sample an actual GPU render target, then change it to check that the
       CPU readback cache never returns the previous render pass. */
    mem_write32(&cpu,texture+0x14,7);
    mem_write32(&cpu,texture+0x84,0x00010203);
    cpu.gpr[3]=texture;cpu.gpr[4]=3;wiiu_gx2_handle_import(&cpu,"GX2SetVertexTexture");
    for(unsigned pass=0;pass<2;++pass) {
        cpu.gpr[3]=texture;cpu.fpr[1]=pass;cpu.fpr[2]=1-pass;
        cpu.fpr[3]=.75;cpu.fpr[4]=.25;
        wiiu_gx2_handle_import(&cpu,"GX2ClearColor");
        for(unsigned repeat=0;repeat<2;++repeat) {
            CHECK(wiiu_gx2_sample_vertex_texture(&cpu,3,5,.5f,.5f,0,out));
            CHECK(out[0]==pass && out[1]==1-pass && out[2]==0 && out[3]==1);
        }
    }
    /* GX2 descriptor/clear routing must retain float values too. */
    mem_write32(&cpu,texture+0x14,0x820);
    cpu.gpr[3]=texture;cpu.fpr[1]=16;cpu.fpr[2]=-2;cpu.fpr[3]=.125;cpu.fpr[4]=.5;
    wiiu_gx2_handle_import(&cpu,"GX2ClearColor");
    float hdr[8];CHECK(wiiu_window_gpu_readback_float(data,hdr,2,1));
    for(unsigned p=0;p<2;++p)CHECK(hdr[p*4]==16 && hdr[p*4+1]==-2 && hdr[p*4+2]==.125f && hdr[p*4+3]==.5f);
    cpu.gpr[3]=texture;cpu.gpr[4]=3;wiiu_gx2_handle_import(&cpu,"GX2SetVertexTexture");
    CHECK(wiiu_gx2_sample_vertex_texture(&cpu,3,5,.5f,.5f,0,out));
    CHECK(out[0]==16 && out[1]==-2 && out[2]==.125f && out[3]==.5f);
    cpu.gpr[3]=texture;cpu.fpr[1]=32;wiiu_gx2_handle_import(&cpu,"GX2ClearColor");
    CHECK(wiiu_gx2_sample_vertex_texture(&cpu,3,5,.5f,.5f,0,out) && out[0]==32);
    /* Exercise host GX2 resource retention as well as the native GPU cache. */
    mem_write32(&cpu,texture+4,1);mem_write32(&cpu,texture+0x14,0x1a);
    mem_write32(&cpu,texture+0x3c,1);
    for(unsigned i=0;i<128;++i) {
        mem_write32(&cpu,texture+0x24,base+0x6000+i*16);
        cpu.gpr[3]=texture;cpu.fpr[1]=i/255.0;cpu.fpr[2]=cpu.fpr[3]=0;cpu.fpr[4]=1;
        wiiu_gx2_handle_import(&cpu,"GX2ClearColor");
    }
    for(unsigned i=0;i<128;++i) {
        u8 pixel[4];CHECK(wiiu_window_gpu_readback(base+0x6000+i*16,pixel,1,1,4));
        CHECK(pixel[2]==i && pixel[3]==255);
    }
    wiiu_gx2_reset();wiiu_memory_free(&memory);
    puts("Vertex TEX: float precision, filtering, bindings, replay and context verified");return 0;
}
