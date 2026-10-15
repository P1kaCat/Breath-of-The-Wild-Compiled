#ifndef WIIU_LATTE_EXECUTE_H
#define WIIU_LATTE_EXECUTE_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* A bounded, fail-closed path for Latte vertex ALU, structured forward branches,
   and indexed VFETCH reads of packed uniform-block vectors.
   Programs and uniform blocks contain GPU-native little-endian words.
   Register words preserve bits (including integer constants). Masks are per
   component: reading an uninitialized lane fails instead of inventing data. */
typedef struct {
    uint32_t gpr[128][4];
    uint8_t valid[128];
    const uint32_t* uniforms;
    const uint8_t* uniform_valid;
    uint32_t uniform_count;
    const uint8_t* blocks[16];
    uint32_t block_sizes[16];
    bool (*sample_texture)(void* user,unsigned texture,unsigned sampler,
        float u,float v,float lod,float result[4]);
    void* texture_user;
    bool (*texture_info)(void* user,unsigned texture,uint32_t lod,uint32_t result[4]);
    /* Z is an unnormalized layer for a 2D array. Preserve it rather than
       silently sampling layer zero through the legacy 2D callback. */
    bool (*sample_texture_layer)(void* user,unsigned texture,unsigned sampler,
        float u,float v,float layer,float lod,float result[4]);
} WiiULatteVertexInputs;

typedef struct {
    float position[4];
    float parameters[32][4];
    uint8_t position_mask;
    uint8_t parameter_mask[32];
} WiiULatteVertexOutputs;

/* First rejection only. UINT32_MAX marks context not reached by execution. */
typedef struct {
    uint32_t line, cf_slot, instruction_slot, word0, word1, opcode;
    uint32_t source_index, source_select, source_channel, detail0, detail1;
    char reason[96];
} WiiULatteVertexFailure;

bool wiiu_latte_execute_vertex(const uint8_t* program, size_t size,
    const WiiULatteVertexInputs* inputs, WiiULatteVertexOutputs* outputs);
bool wiiu_latte_execute_vertex_diagnostic(const uint8_t* program, size_t size,
    const WiiULatteVertexInputs* inputs, WiiULatteVertexOutputs* outputs,
    WiiULatteVertexFailure* failure);
#endif
