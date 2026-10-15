/* Double-click Windows entry point. Starts the native runtime directly,
   using only the project's isolated game copy and save directory. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include <stdio.h>

#ifndef BOTW_LAUNCH_RUNTIME
#define BOTW_LAUNCH_RUNTIME L"build/research-full/bin/botw_recomp.exe"
#endif

enum { PATH_CAPACITY = 32768 };

static int fail(const wchar_t *message, DWORD code) {
    wchar_t detail[2048];
    wchar_t system[1024] = L"";
    if (code)
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, code, 0, system, 1024, NULL);
    _snwprintf_s(detail, 2048, _TRUNCATE, L"%ls\n\n%ls\nWindows error: %lu",
                 message, system, code);
    MessageBoxW(NULL, detail, L"BOTW Compiled", MB_OK | MB_ICONERROR);
    return 1;
}

static BOOL join(wchar_t *out, const wchar_t *root, const wchar_t *part) {
    return _snwprintf_s(out, PATH_CAPACITY, _TRUNCATE, L"%ls\\%ls", root, part) >= 0;
}

static BOOL exists(const wchar_t *path, BOOL directory) {
    DWORD attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           !!(attributes & FILE_ATTRIBUTE_DIRECTORY) == !!directory;
}

static BOOL make_directory(const wchar_t *path) {
    return CreateDirectoryW(path, NULL) ||
           (GetLastError() == ERROR_ALREADY_EXISTS && exists(path, TRUE));
}

/* Keep buffers off the small default Windows stack. */
static wchar_t root[PATH_CAPACITY], runtime[PATH_CAPACITY];
static wchar_t base[PATH_CAPACITY], update[PATH_CAPACITY], dlc[PATH_CAPACITY];
static wchar_t work[PATH_CAPACITY], path[PATH_CAPACITY], run[PATH_CAPACITY];
static wchar_t command[PATH_CAPACITY], log_path[PATH_CAPACITY];

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous,
                    PWSTR arguments, int show) {
    (void)instance; (void)previous; (void)arguments; (void)show;
    DWORD length = GetModuleFileNameW(NULL, root, PATH_CAPACITY);
    if (!length || length >= PATH_CAPACITY)
        return fail(L"Cannot determine the launcher directory.", GetLastError());
    wchar_t *separator = wcsrchr(root, L'\\');
    if (!separator) return fail(L"Invalid launcher directory.", 0);
    *separator = 0;

    /* The launcher can live at the project root or beside botw_recomp.exe
       in the normal CMake output directory. */
    if (!join(path, root, L"workspace") || !exists(path, TRUE)) {
        if (!join(path, root, L"botw_recomp.exe") || !exists(path, FALSE))
            return fail(L"Place BOTW Compiled.exe in the project directory.", 0);
        wcscpy_s(runtime, PATH_CAPACITY, path);
        for (;;) {
            separator = wcsrchr(root, L'\\');
            if (!separator) return fail(L"Cannot find the project workspace.", 0);
            *separator = 0;
            if (!join(path, root, L"workspace"))
                return fail(L"Project path is too long.", 0);
            if (exists(path, TRUE)) break;
        }
    } else if (!join(runtime, root, BOTW_LAUNCH_RUNTIME)) {
        return fail(L"Runtime path is too long.", 0);
    }
    if (!exists(runtime, FALSE))
        return fail(L"The native runtime is missing. Build the project before launching.", 0);
    if (!join(work, root, L"workspace") ||
        !join(base, work, L"game\\base") ||
        !join(update, work, L"game\\update") ||
        !join(dlc, work, L"game\\dlc"))
        return fail(L"Game path is too long.", 0);
    if (!join(path, work, L"snapshot.complete.json") || !exists(path, FALSE) ||
        !join(path, base, L"content") || !exists(path, TRUE) ||
        !join(path, update, L"content") || !exists(path, TRUE) ||
        !join(path, update, L"code\\U-King.rpx") || !exists(path, FALSE))
        return fail(L"The isolated game copy is incomplete. Finish game preparation first.", 0);

    if (!join(run, work, L"run") || !make_directory(run) ||
        !join(path, work, L"logs") || !make_directory(path) ||
        !join(path, work, L"temp") || !make_directory(path) ||
        !SetEnvironmentVariableW(L"TEMP", path) ||
        !SetEnvironmentVariableW(L"TMP", path) ||
        !join(path, work, L"system") || !make_directory(path) ||
        !join(path, work, L"system\\mlc01") || !make_directory(path) ||
        !SetEnvironmentVariableW(L"BOTW_WIIU_MLC_ROOT", path) ||
        !join(path, work, L"logs\\runtime.log") ||
        !SetEnvironmentVariableW(L"BOTW_LOG_PATH", path))
        return fail(L"Cannot prepare the local runtime directories.", GetLastError());

    /* Normal interactive defaults; diagnostic input injection must not leak
       into a double-click launch from an initialized development shell. */
    const wchar_t *clear[] = {
        L"BOTW_CAPTURE_DIR", L"BOTW_SHADER_OVERRIDES", L"BOTW_TEST_A_SAMPLES",
        L"BOTW_TEST_START_SAMPLE", L"BOTW_TEST_START_PRO", L"BOTW_TRACE_DRAW_FRAME"
    };
    for (unsigned i = 0; i < sizeof(clear) / sizeof(clear[0]); ++i)
        SetEnvironmentVariableW(clear[i], NULL);
    if (!SetEnvironmentVariableW(L"BOTW_HEADLESS", L"0") ||
        !SetEnvironmentVariableW(L"BOTW_MUTE", L"0") ||
        !SetEnvironmentVariableW(L"BOTW_WINDOW_WIDTH", L"1280") ||
        !SetEnvironmentVariableW(L"BOTW_WINDOW_HEIGHT", L"720") ||
        !SetEnvironmentVariableW(L"BOTW_VSYNC", L"0") ||
        !SetEnvironmentVariableW(L"BOTW_PRESENT_FILTER", L"linear"))
        return fail(L"Cannot configure the native runtime.", GetLastError());

    if (_snwprintf_s(command, PATH_CAPACITY, _TRUNCATE,
                    L"\"%ls\" --base \"%ls\" --update \"%ls\" --dlc \"%ls\" --blocks 0",
                    runtime, base, update, dlc) < 0 ||
        !join(log_path, work, L"logs\\launcher-output.log"))
        return fail(L"Launch command is too long.", 0);

    SECURITY_ATTRIBUTES security = { sizeof(security), NULL, TRUE };
    HANDLE log = CreateFileW(log_path, GENERIC_WRITE, FILE_SHARE_READ,
                            &security, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (log == INVALID_HANDLE_VALUE)
        return fail(L"Cannot open launcher-output.log. The game may already be running.", GetLastError());
    HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (input == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        CloseHandle(log);
        return fail(L"Cannot initialize runtime input.", error);
    }
    STARTUPINFOW startup = { 0 };
    PROCESS_INFORMATION process = { 0 };
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input;
    startup.hStdOutput = log;
    startup.hStdError = log;
    BOOL started = CreateProcessW(runtime, command, NULL, NULL, TRUE,
                                  CREATE_NO_WINDOW, NULL, run, &startup, &process);
    DWORD error = GetLastError();
    CloseHandle(input);
    CloseHandle(log);
    if (!started) return fail(L"Cannot start the native runtime.", error);
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD result = 1;
    GetExitCodeProcess(process.hProcess, &result);
    CloseHandle(process.hProcess);
    if (result)
        return fail(L"The runtime stopped with an error. See workspace\\logs\\launcher-output.log.", result);
    return 0;
}
