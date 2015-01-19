#include "Tools.h"
#include <TlHelp32.h>
#include <cwchar>
#include <fstream>

BOOL CALLBACK EnumWindowsProc(HWND, LPARAM);
const char hexChars[] = "0123456789ABCDEF";

std::string StringToWString(std::wstring str)
{
    return std::string(str.begin(), str.end());
}

void WriteToBinaryFile(std::string path, char *buf, int len)
{
    std::ofstream out(path, std::ios::app | std::ios::binary);
    out.write(buf, len);
    out.close();
}
void WriteToBinaryFile(std::wstring path, char *buf, int len)
{
    std::ofstream out(path, std::ios::app | std::ios::binary);
    out.write(buf, len);
    out.close();
}

void WriteToTextFile(std::string path, std::string str)
{
    std::ofstream out(path, std::ios::app);
    out << str << std::endl;
    out.close();
}

void WriteToTextFile(std::wstring path, std::wstring str)
{
    std::ofstream out(path, std::ios::app);
    out << std::string(str.begin(), str.end()) << std::endl;
    out.close();
}

void bytesToHexString(char *src, int len, char *dest)
{
    int i = 0;
    for (int i = 0; i < len; ++i)
    {
        dest[i*3] = hexChars[(src[i] & 0xF0) >> 4];
        dest[i*3+1] = hexChars[src[i] & 0x0F];
        dest[i*3+2] = ' ';
    }
    dest[len*3-1] = '\0';
}

HWND GetCurrentProcessHwnd()
{
    HWND hwnd = NULL;
    while (EnumWindows(EnumWindowsProc, (LPARAM)&hwnd));
    return hwnd;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD procId = 0;
    GetWindowThreadProcessId(hwnd, &procId);
    if (procId == GetCurrentProcessId())
    {
        *(HWND *)lParam = hwnd;
        return false;
    }
    return true;
}

DWORD GetProcessIdFromWindowName(LPCWSTR windowName)
{
    DWORD processId = 0;
    HWND hwnd = FindWindow(NULL, windowName);
    GetWindowThreadProcessId(hwnd, &processId);
    return processId;
}

DWORD GetProcessIdFromImageName(LPCWSTR imageName)
{
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    PROCESSENTRY32 processSnapshot = {0};
    processSnapshot.dwSize = sizeof(PROCESSENTRY32);
    if (Process32Next(snapshot, &processSnapshot))
    {
        while (Process32Next(snapshot, &processSnapshot))
        {
            if (CompareString(LOCALE_SYSTEM_DEFAULT, NULL, imageName, wcslen(imageName), processSnapshot.szExeFile, wcslen(processSnapshot.szExeFile)) == CSTR_EQUAL)
            {
                processId = processSnapshot.th32ProcessID;
                break;
            }
        }
    }
    CloseHandle(snapshot);
    return processId;
}
