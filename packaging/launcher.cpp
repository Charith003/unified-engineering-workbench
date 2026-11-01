// ═══════════════════════════════════════════════════════════════════════════
// Unified Engineering Workbench — Native Windows One-Click Launcher
// PASS Science Works R&D Internship 2026–2027
// ═══════════════════════════════════════════════════════════════════════════

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    // 1. Resolve current directory
    wchar_t exe_path_buf[MAX_PATH];
    GetModuleFileNameW(NULL, exe_path_buf, MAX_PATH);
    fs::path base_dir = fs::path(exe_path_buf).parent_path();

    // 2. Locate backend server and frontend dist
    fs::path server_exe = base_dir / "uew_server.exe";
    if (!fs::exists(server_exe)) {
        server_exe = base_dir / "bin" / "uew_server.exe";
    }

    if (!fs::exists(server_exe)) {
        MessageBoxW(NULL,
            L"Could not locate uew_server.exe.\nPlease ensure all extracted files are kept together.",
            L"Unified Engineering Workbench - Error",
            MB_ICONERROR | MB_OK);
        return 1;
    }

    fs::path frontend_dir = base_dir / "frontend_dist";
    if (!fs::exists(frontend_dir)) {
        frontend_dir = base_dir / "frontend" / "dist";
    }

    // Remove any stale port file
    fs::path port_file = base_dir / "uew_port.txt";
    fs::remove(port_file);

    // 3. Launch backend process
    std::wstring cmd = L"\"" + server_exe.wstring() + L"\"";
    if (fs::exists(frontend_dir)) {
        cmd += L" --frontend=\"" + frontend_dir.wstring() + L"\"";
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    std::wcout << L"Starting Unified Engineering Workbench backend..." << std::endl;

    BOOL ok = CreateProcessW(
        NULL,
        &cmd[0],
        NULL, NULL, FALSE,
        0, // Inherit console
        NULL,
        base_dir.wstring().c_str(),
        &si, &pi
    );

    if (!ok) {
        MessageBoxW(NULL,
            L"Failed to start uew_server.exe backend process.",
            L"Unified Engineering Workbench - Launch Error",
            MB_ICONERROR | MB_OK);
        return 1;
    }

    // 4. Wait for uew_port.txt to appear (up to 5 seconds)
    int port = 0;
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::steady_clock::now() - start_time).count() < 6) {
        std::ifstream pf(port_file);
        if (pf && (pf >> port) && port > 0) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (port == 0) {
        port = 7800; // default fallback
    }

    // 5. Open in default browser
    std::wstring url = L"http://127.0.0.1:" + std::to_wstring(port);
    std::wcout << L"Opening workbench interface: " << url << std::endl;
    ShellExecuteW(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);

    std::wcout << L"\nUnified Engineering Workbench is active." << std::endl;
    std::wcout << L"Keep this console window open while using the application." << std::endl;
    std::wcout << L"Close this window or press Ctrl+C to terminate UEW cleanly.\n" << std::endl;

    // 6. Monitor child process until exit
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}
