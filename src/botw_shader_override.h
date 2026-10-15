// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BOTW_SHADER_OVERRIDE_H
#define BOTW_SHADER_OVERRIDE_H
#include <stdbool.h>
#include <stdint.h>
/* With no directory, no filesystem access. The caller owns *replacement. */
bool botw_shader_override_load(const char* directory, uint32_t key,
                              const char* original, char** replacement);
#endif
