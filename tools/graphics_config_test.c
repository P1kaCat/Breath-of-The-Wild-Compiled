// SPDX-License-Identifier: GPL-3.0-or-later
#include "botw_graphics_config.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void) {
    BotwGraphicsConfig config;
    char error[160];
    CHECK(botw_graphics_config_parse(&config, NULL, NULL, NULL, NULL, error, sizeof(error)));
    CHECK(config.window_width == 1280 && config.window_height == 720 && config.vsync == 0);
    CHECK(botw_graphics_config_parse(&config, "3840", "2160", "1", "nearest", error, sizeof(error)));
    CHECK(config.window_width == 3840 && config.window_height == 2160 && config.vsync == 1 && config.presentation_nearest);
    const char* invalid[] = {"-1", "12x", "999999999999999999999", "0", "319", "7681", " 1280"};
    for (unsigned i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        CHECK(!botw_graphics_config_parse(&config, invalid[i], "720", "0", "linear", error, sizeof(error)));
        CHECK(config.window_width == 3840 && config.vsync == 1 && config.presentation_nearest);
        CHECK(error[0]);
    }
    CHECK(!botw_graphics_config_parse(&config, "1280", "720", "2", "linear", error, sizeof(error)));
    CHECK(!botw_graphics_config_parse(&config, "1280", "720", "0", "fake", error, sizeof(error)));
    CHECK(!botw_graphics_config_parse(&config, "1280", "179", "0", "linear", error, sizeof(error)));
    return 0;
}
