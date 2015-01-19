#ifndef Q_TOOLS_H
#define Q_TOOLS_H

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <string>

std::string StringToWString(std::wstring wstr);
void WriteToBinaryFile(std::string path, char *buf, int len);
void WriteToBinaryFile(std::wstring path, char *buf, int len);
void WriteToTextFile(std::string path, std::string str);
void WriteToTextFile(std::wstring path, std::wstring str);
void bytesToHexString(char *src, int len, char *dest);
HWND GetCurrentProcessHwnd();
DWORD GetProcessIdFromWindowName(LPCWSTR windowName);
DWORD GetProcessIdFromImageName(LPCWSTR imageName);

#endif // Q_TOOLS_H
