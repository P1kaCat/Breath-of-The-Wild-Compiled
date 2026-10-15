// SPDX-License-Identifier: GPL-3.0-or-later
#include "wiiu_window.h"
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static BOOL CALLBACK own_window(HWND window, LPARAM result) {
    DWORD process = 0;
    GetWindowThreadProcessId(window, &process);
    if (process == GetCurrentProcessId()) {
        char name[64];
        GetClassNameA(window, name, sizeof(name));
        if (lstrcmpA(name, "BOTWRecompWindow") == 0) {
            *(HWND*)result = window;
            return FALSE;
        }
    }
    return TRUE;
}

int main(void) {
    wiiu_window_show("presentation settings regression");
    HWND window = NULL;
    EnumWindows(own_window, (LPARAM)&window);
    RECT client;
    if (!window || !GetClientRect(window, &client) ||
        client.right - client.left != 640 || client.bottom - client.top != 360 ||
        IsWindowVisible(window)) {
        fprintf(stderr, "Expected a hidden 640x360 client window.\n");
        return 1;
    }
    /* Exercise the final presenter with a small, asymmetric test image. */
    const unsigned char bgra[16] = {0,0,255,255, 0,255,0,255,
                                    255,0,0,255, 255,255,255,255};
    wiiu_window_present_bgra(2, 2, bgra, 8);
    if (wiiu_window_is_closed()) return 1;
    return 0;
}
