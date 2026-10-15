// SPDX-License-Identifier: GPL-3.0-or-later
#include "botw_shader_override.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static char* read_source(FILE* file) {
    if (fseek(file, 0, SEEK_END) != 0) return NULL;
    long bytes = ftell(file);
    if (bytes <= 0 || bytes >= 65536 || fseek(file, 0, SEEK_SET) != 0) return NULL;
    char* source = malloc((size_t)bytes + 1u);
    if (!source) return NULL;
    if (fread(source, 1u, (size_t)bytes, file) != (size_t)bytes ||
        memchr(source, 0, (size_t)bytes)) { free(source); return NULL; }
    source[bytes] = 0;
    return source;
}

bool botw_shader_override_load(const char* directory, uint32_t key,
                              const char* original, char** replacement) {
    if (!replacement || !original) return false;
    *replacement = NULL;
    if (!directory || !directory[0]) return true;
    char baseline_path[4096], override_path[4096];
    int baseline_size = snprintf(baseline_path, sizeof(baseline_path),
        "%s/%08X.original.hlsl", directory, key);
    int override_size = snprintf(override_path, sizeof(override_path),
        "%s/%08X.override.hlsl", directory, key);
    if (baseline_size < 0 || baseline_size >= sizeof(baseline_path) ||
        override_size < 0 || override_size >= sizeof(override_path)) return false;

    /* Never overwrite a user's baseline; a hash collision must not select an
       override for a different shader. Compare its complete original source. */
    FILE* baseline = fopen(baseline_path, "rb");
    if (!baseline && errno == ENOENT) {
        FILE* output = fopen(baseline_path, "wbx");
        if (!output) return false;
        size_t size = strlen(original);
        bool written = fwrite(original, 1u, size, output) == size;
        if (fclose(output) != 0) written = false;
        if (!written) return false;
        baseline = fopen(baseline_path, "rb");
    }
    if (!baseline) return false;
    char* baseline_source = read_source(baseline);
    fclose(baseline);
    bool match = baseline_source && strcmp(baseline_source, original) == 0;
    free(baseline_source);
    if (!match) {
        fprintf(stderr, "graphics: shader %08X baseline mismatch; override rejected\n", key);
        return false;
    }
    FILE* file = fopen(override_path, "rb");
    if (!file) return errno == ENOENT;
    *replacement = read_source(file);
    fclose(file);
    if (!*replacement) {
        fprintf(stderr, "graphics: shader %08X override must be nonempty text below 64 KiB\n", key);
        return false;
    }
    fprintf(stderr, "graphics: using local pixel shader %08X override\n", key);
    return true;
}
