// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BOTW_GRAPHICS_CONFIG_H
#define BOTW_GRAPHICS_CONFIG_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t window_width;
    uint32_t window_height;
    uint32_t vsync;
    bool presentation_nearest;
} BotwGraphicsConfig;

/* Presentation controls only. Guest render targets and samplers stay intact. */
bool botw_graphics_config_parse(BotwGraphicsConfig* config, const char* width,
    const char* height, const char* vsync, const char* filter,
    char* error, size_t error_size);
bool botw_graphics_config_from_environment(BotwGraphicsConfig* config,
    char* error, size_t error_size);
#endif
