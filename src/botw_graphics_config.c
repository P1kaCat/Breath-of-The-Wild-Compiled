// SPDX-License-Identifier: GPL-3.0-or-later
#include "botw_graphics_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool decimal(const char* text, uint32_t minimum, uint32_t maximum,
                    uint32_t* result) {
    if (!text || !text[0]) return true;
    uint32_t value = 0;
    for (const char* p = text; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
        uint32_t digit = (uint32_t)(*p - '0');
        if (value > maximum / 10u ||
            (value == maximum / 10u && digit > maximum % 10u)) return false;
        value = value * 10u + digit;
    }
    if (value < minimum || value > maximum) return false;
    *result = value;
    return true;
}

bool botw_graphics_config_parse(BotwGraphicsConfig* config, const char* width,
    const char* height, const char* vsync, const char* filter,
    char* error, size_t error_size) {
    BotwGraphicsConfig candidate = {1280u, 720u, 0u, false};
    const char* invalid = NULL;
    if (error && error_size) error[0] = 0;
    if (!config) invalid = "missing configuration";
    else if (!decimal(width, 320u, 7680u, &candidate.window_width))
        invalid = "BOTW_WINDOW_WIDTH must be between 320 and 7680";
    else if (!decimal(height, 180u, 4320u, &candidate.window_height))
        invalid = "BOTW_WINDOW_HEIGHT must be between 180 and 4320";
    else if (!decimal(vsync, 0u, 1u, &candidate.vsync))
        invalid = "BOTW_VSYNC must be 0 or 1";
    else if (filter && filter[0]) {
        if (strcmp(filter, "nearest") == 0) candidate.presentation_nearest = true;
        else if (strcmp(filter, "linear") != 0)
            invalid = "BOTW_PRESENT_FILTER must be nearest or linear";
    }
    if (invalid) {
        if (error && error_size) snprintf(error, error_size, "%s", invalid);
        return false;
    }
    *config = candidate;
    return true;
}

bool botw_graphics_config_from_environment(BotwGraphicsConfig* config,
    char* error, size_t error_size) {
    return botw_graphics_config_parse(config, getenv("BOTW_WINDOW_WIDTH"),
        getenv("BOTW_WINDOW_HEIGHT"), getenv("BOTW_VSYNC"),
        getenv("BOTW_PRESENT_FILTER"), error, error_size);
}
