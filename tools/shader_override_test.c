// SPDX-License-Identifier: GPL-3.0-or-later
#include "wiiu_window.h"
#include "botw_shader_override.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static const char red[] = "float4 main():SV_Target0{return float4(1,0,0,1);}";
static const char green[] = "float4 main():SV_Target0{return float4(0,1,0,1);}";
static bool write_text(const char* directory, unsigned key, const char* suffix, const char* text) {
    char path[4096]; snprintf(path, sizeof(path), "%s/%08X.%s.hlsl", directory, key, suffix);
    FILE* file = fopen(path, "wb"); if (!file) return false;
    bool ok = fwrite(text, 1, strlen(text), file) == strlen(text);
    return fclose(file) == 0 && ok;
}
static bool draw(unsigned key) {
    WiiULattePixelProgram* program = calloc(1, sizeof(*program));
    if (!program) return false;
    strcpy(program->hlsl, red); program->key = key; program->target_mask = 1;
    WiiUWindowSceneMaterial material = {0}; material.program = program;
    WiiUWindowPostVertex vertices[3] = {0};
    const float positions[3][2] = {{-1,1},{3,1},{-1,-3}};
    for (unsigned i = 0; i < 3; ++i) {
        vertices[i].position[0] = positions[i][0]; vertices[i].position[1] = positions[i][1];
        vertices[i].position[3] = 1;
    }
    float viewport[4] = {0,0,4,4};
    WiiUWindowGpuBlendControl blend = {true,1,0,0,true,1,0,0};
    bool result = wiiu_window_gpu_draw_scene(99,4,4,&material,vertices,3,viewport,&blend,false,0,0,0,0);
    free(program); return result;
}
int main(void) {
    char temp[2048], directory[4096];
    CHECK(GetTempPathA(sizeof(temp), temp) > 0);
    snprintf(directory, sizeof(directory), "%sbotw-shaders-%lu", temp, (unsigned long)GetCurrentProcessId());
    CHECK(CreateDirectoryA(directory, NULL));
    CHECK(_putenv_s("BOTW_SHADER_OVERRIDES", directory) == 0);
    wiiu_window_show("shader override GPU regression");
    CHECK(wiiu_window_gpu_clear(99,4,4,0,0,255,255));
    CHECK(draw(1));
    unsigned char pixels[64]; CHECK(wiiu_window_gpu_readback(99,pixels,4,4,16));
    CHECK(pixels[0] == 0 && pixels[1] == 0 && pixels[2] == 255);
    CHECK(write_text(directory, 2, "original", red));
    CHECK(write_text(directory, 2, "override", green));
    CHECK(draw(2)); CHECK(wiiu_window_gpu_readback(99,pixels,4,4,16));
    CHECK(pixels[0] == 0 && pixels[1] == 255 && pixels[2] == 0);
    CHECK(write_text(directory, 3, "original", green));
    CHECK(write_text(directory, 3, "override", green));
    CHECK(!draw(3));
    CHECK(write_text(directory, 4, "original", red));
    CHECK(write_text(directory, 4, "override", "invalid shader source"));
    CHECK(!draw(4));
    CHECK(wiiu_window_gpu_readback(99,pixels,4,4,16));
    CHECK(pixels[0] == 0 && pixels[1] == 255 && pixels[2] == 0);
    CHECK(write_text(directory, 5, "original", red));
    char* replacement = NULL;
    CHECK(write_text(directory, 5, "override", ""));
    CHECK(!botw_shader_override_load(directory,5,red,&replacement));
    CHECK(!replacement);
    CHECK(botw_shader_override_load(NULL,99,red,&replacement) && !replacement);
    for (unsigned key = 1; key <= 5; ++key) {
        char path[4096]; snprintf(path,sizeof(path),"%s/%08X.original.hlsl",directory,key); DeleteFileA(path);
        snprintf(path,sizeof(path),"%s/%08X.override.hlsl",directory,key); DeleteFileA(path);
    }
    RemoveDirectoryA(directory);
    puts("Original and overridden shaders verified with GPU pixel readback.");
    return 0;
}
