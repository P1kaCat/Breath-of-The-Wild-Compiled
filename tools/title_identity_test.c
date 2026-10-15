// SPDX-License-Identifier: GPL-3.0-or-later
#include "wiiu_imports.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static RPXFile rpx;
static void call(CPUState* cpu, unsigned symbol) {
    cpu->pc = rpx.symbols[symbol].value;
    cpu->lr = 0x02001000u;
    if (!wiiu_imports_host_call(cpu, cpu->pc)) cpu->exception = 1;
}
int main(int argc, char** argv) {
    CHECK(argc == 2);
    rpx.symbol_count = 3;
    const char* names[] = {"SYSGetCallerTitleId", "MCP_GetSysProdSettings", "AOC_ListTitle"};
    for (unsigned i = 0; i < 3; ++i) {
        strcpy(rpx.symbols[i].name, names[i]);
        rpx.symbols[i].value = 0x0434D000u + i * 8u;
        rpx.symbols[i].section_index = RPX_SYMBOL_SECTION_ALIAS;
    }
    wiiu_imports_attach_rpx(&rpx);
    wiiu_imports_set_title_paths(argv[1], argv[1], argv[1]);
    WiiUMemory memory;
    wiiu_memory_init(&memory);
    const u32 base = WIIU_GUEST_HEAP_BASE;
    CHECK(wiiu_memory_add_segment(&memory, "identity_test", base, 65536, NULL, true));
    CPUState cpu = {0}; wiiu_memory_bind_cpu(&memory, &cpu);
    const u64 ids[] = {0x00050000101C9400ull,0x00050000101C9500ull};
    const u32 regions[] = {2,4}, groups[] = {0x1C94,0x1C95};
    const char* mounts[] = {"/vol/aoc0005000c101c9400","/vol/aoc0005000c101c9500"};
    for (unsigned i = 0; i < 2; ++i) {
        CHECK(wiiu_imports_set_title_identity(ids[i],groups[i],regions[i]));
        call(&cpu,0); CHECK(!cpu.exception);
        CHECK(cpu.gpr[3] == (u32)(ids[i] >> 32) && cpu.gpr[4] == (u32)ids[i]);
        cpu.gpr[4] = base + 0x100; call(&cpu,1); CHECK(!cpu.exception);
        CHECK(mem_read8(&cpu,base+0x103) == regions[i]);
        CHECK(mem_read8(&cpu,base+0x10B) == regions[i]);
        cpu.gpr[3] = base; cpu.gpr[4] = base + 0x200; cpu.gpr[5] = 1;
        call(&cpu,2); CHECK(!cpu.exception);
        CHECK(mem_read32(&cpu,base) == 1);
        CHECK(mem_read32(&cpu,base+0x200) == 0x0005000C);
        CHECK(mem_read32(&cpu,base+0x204) == (u32)ids[i]);
        CHECK(mem_read32(&cpu,base+0x208) == groups[i]);
        for (unsigned c = 0; c <= strlen(mounts[i]); ++c)
            CHECK(mem_read8(&cpu,base + 0x20E + c) == (unsigned char)mounts[i][c]);
    }
    CHECK(!wiiu_imports_set_title_identity(ids[1],groups[0],regions[0]));
    call(&cpu,0); CHECK(cpu.gpr[4] == (u32)ids[1]);
    wiiu_imports_shutdown(); wiiu_memory_free(&memory);
    return 0;
}
