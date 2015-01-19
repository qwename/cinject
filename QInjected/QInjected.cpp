// Todo - Run process and inject
// Auto-detect and inject

#include "QInjected.h"

void InitHook();
void CreateLoggingWindow(HWND);
void HookedFunctionCalled(int);
void HookFunction(int, HANDLE);
void UnhookFunction(int, HANDLE);
int __stdcall MonitorConnect(SOCKET, sockaddr *, int);
int __stdcall MonitorSend(SOCKET, char *, int, int);
int __stdcall MonitorRecv(SOCKET, char *, int, int);
int __stdcall MonitorWSASend(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int __stdcall MonitorWSARecv(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int __stdcall MonitorWSAConnect(SOCKET s, const struct sockaddr *name, int namelen, LPWSABUF lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS);
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);
int GetX(LPARAM x) { return x & 0xFFFF; }
int GetY(LPARAM y) { return (y >> 0x10) & 0xFFFF; }
BYTE GetKeyCode(TCHAR c) { return LOBYTE(VkKeyScan(c)); }

LONG oldWndProc;
HINSTANCE currentDLLModule;
HWND editLog = NULL;

const enum hFunctions {hconnect, hsend, hrecv, hWSAConnect, hWSASend, hWSARecv, hEND};
const unsigned hookFunctionCount = hEND;
const char *hookFunctionAddresses[hookFunctionCount] = { (char *)MonitorConnect, (char *)MonitorSend, (char *)MonitorRecv, (char *)MonitorWSAConnect, (char *)MonitorWSASend, (char *)MonitorWSARecv };
const std::wstring hookFunctionNames[hookFunctionCount] = { L"connect" ,L"send", L"recv", L"WSAConnect", L"WSASend", L"WSARecv" };
char *patchAddresses[hookFunctionCount];
char patchBytes[] = { 0xE9, 0, 0, 0, 0, 0xC3 };
char backupBytes[hookFunctionCount][sizeof(patchBytes)];
long long functionCalledCount[hookFunctionCount] = {0};
std::wstring callStatTemplate = L"";

void InitHook(HWND hwnd)
{
    HMODULE winsockModule = GetModuleHandle(L"ws2_32");
    if (winsockModule)
    {
        for (int i = 0; i < hookFunctionCount; ++i)
        {
            patchAddresses[i] = (char *)GetProcAddress(winsockModule, StringToWString(hookFunctionNames[i]).c_str());
            if (!ReadProcessMemory(GetCurrentProcess(), patchAddresses[i], backupBytes[i], sizeof(backupBytes[i]), NULL))
            {
                MessageBox(NULL, hookFunctionNames[i].c_str(), L"ReadProcessMemory - Failed", MB_OK);
                break;
            }
            HookFunction(i, GetCurrentProcess());

            callStatTemplate += hookFunctionNames[i] + L": 0\r\n";
        }
    }
}

void CreateLoggingWindow(HWND parentHwnd)
{
    if (editLog)
        return;
    editLog = CreateWindowEx(NULL, L"EDIT", L"Hooking functions", WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, parentHwnd, NULL, (HINSTANCE)GetWindowLong(parentHwnd, GWL_HINSTANCE), NULL);
    if (!editLog)
    {
        MessageBox(NULL, std::to_wstring((long long)GetLastError()).c_str(), L"CreateWindowEx - Failed", MB_OK);
        return;
    }
    ShowWindow(editLog, SW_SHOWNOACTIVATE);
    UpdateWindow(editLog);
    SetWindowText(editLog, callStatTemplate.c_str());
}

void HookFunction(int func, HANDLE processHandle)
{
    *((int*)&patchBytes[1]) = (int)(hookFunctionAddresses[func]) - (int)patchAddresses[func] - (sizeof(patchBytes) - 1);
    WriteProcessMemory(processHandle, patchAddresses[func], patchBytes, sizeof(patchBytes), NULL);
    FlushInstructionCache(processHandle, NULL, NULL);
}

void UnhookFunction(int func, HANDLE processHandle)
{
    WriteProcessMemory(processHandle, patchAddresses[func], backupBytes[func], sizeof(backupBytes[func]), NULL);
    FlushInstructionCache(processHandle, NULL, NULL);
}

void HookedFunctionCalled(int func)
{
    ++(functionCalledCount[func]);
    if (!editLog)
        return;

    int editLogTextLength = GetWindowTextLength(editLog);
    TCHAR *editLogText = new TCHAR[editLogTextLength+1];
    GetWindowText(editLog, editLogText, editLogTextLength);
    editLogText[editLogTextLength] = '\0';
    std::wstring callStats = std::wstring(editLogText);
    delete[] editLogText;

    std::wstring originalEditLogText = hookFunctionNames[func] + L": " + std::to_wstring((long long)functionCalledCount[func]-1);
    std::wstring replaceEditLogText = hookFunctionNames[func] + L": " + std::to_wstring((long long)functionCalledCount[func]);
    std::string::size_type found = callStats.find(hookFunctionNames[func]);
    callStats.replace(found, originalEditLogText.length(), replaceEditLogText);
    SetWindowText(editLog, callStats.c_str());
}

int __stdcall MonitorConnect(SOCKET s, sockaddr *name, int namelen)
{
    HookedFunctionCalled(hconnect);
    UnhookFunction(hconnect, GetCurrentProcess());
    int result = connect(s, name, namelen);
    HookFunction(hconnect, GetCurrentProcess());

    if (result != SOCKET_ERROR)
    {
        sockaddr_in *name_in = (sockaddr_in *)name;
        WriteToTextFile(StringToWString(hookFunctionNames[hconnect]) + std::string(".txt"), std::string(inet_ntoa(name_in->sin_addr)) + ':' + std::to_string((long long)ntohs(name_in->sin_port)));
    }
    return result;
}

int __stdcall MonitorSend(SOCKET s, char *buf, int len, int flags)
{
    HookedFunctionCalled(hsend);
    UnhookFunction(hsend, GetCurrentProcess());
    int result = send(s, buf, len, flags);
    HookFunction(hsend, GetCurrentProcess());

    if (result != SOCKET_ERROR && result > 0)
    {
        WriteToBinaryFile(hookFunctionNames[hsend], buf, result);
    }
    return result;
}

int __stdcall MonitorRecv(SOCKET s, char *buff, int buffSize, int flags)
{
    HookedFunctionCalled(hrecv);
    UnhookFunction(hrecv, GetCurrentProcess());
    int result = recv(s, buff, buffSize, flags);
    HookFunction(hrecv, GetCurrentProcess());
    if (result != SOCKET_ERROR && result > 0)
    {
        /*if (editLog)
        {
            char *packetString = new char[result*3];
            bytesToHexString(buff, result, packetString);
            int ndx = GetWindowTextLength(editLog);
            SendMessage(editLog, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
            SendMessageA(editLog, EM_REPLACESEL, 0, (LPARAM)packetString);
            SendMessageA(editLog, EM_REPLACESEL, 0, (LPARAM)"\r\n");
            delete[] packetString;
        }*/
        //out.write(packetString, result*3-1);
        WriteToBinaryFile(hookFunctionNames[hrecv], buff, result);
    }
    return result;
}

int __stdcall MonitorWSAConnect(SOCKET s, const struct sockaddr *name, int namelen, LPWSABUF lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS)
{
    HookedFunctionCalled(hWSAConnect);
    UnhookFunction(hWSAConnect, GetCurrentProcess());
    int result = WSAConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
    HookFunction(hWSAConnect, GetCurrentProcess());

    if (result != SOCKET_ERROR)
    {
        sockaddr_in *name_in = (sockaddr_in *)name;
        WriteToTextFile(StringToWString(hookFunctionNames[hWSAConnect]) + std::string(".txt"), std::string(inet_ntoa(name_in->sin_addr)) + ':' + std::to_string((long long)ntohs(name_in->sin_port)));
    }
    return result;
}

int __stdcall MonitorWSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    HookedFunctionCalled(hWSASend);
    UnhookFunction(hWSASend, GetCurrentProcess());
    int result = WSASend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
    HookFunction(hWSASend, GetCurrentProcess());

    if (result != SOCKET_ERROR && *lpNumberOfBytesSent > 0)
    {
        WriteToBinaryFile(hookFunctionNames[hWSASend], lpBuffers[0].buf, *lpNumberOfBytesSent);
    }
    return result;
}

int __stdcall MonitorWSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    HookedFunctionCalled(hWSARecv);
    UnhookFunction(hWSARecv, GetCurrentProcess());
    int result = WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
    HookFunction(hWSARecv, GetCurrentProcess());

    if (result != SOCKET_ERROR && *lpNumberOfBytesRecvd > 0)
    {
        WriteToBinaryFile(hookFunctionNames[hWSARecv], lpBuffers[0].buf, *lpNumberOfBytesRecvd);
    }
    else if (result == SOCKET_ERROR && WSAGetLastError() == WSA_IO_PENDING)
    {
    }
    return result;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
        case WM_KEYDOWN:
            if (wParam == GetKeyCode('q'))
            {
                CreateLoggingWindow(hwnd);
            }
            else if (wParam == GetKeyCode('i'))
            {
            }
            break;
        case WM_CLOSE:
            if (MessageBox(hwnd, L"Exit the program?", L"WM_CLOSE", MB_ICONWARNING | MB_OKCANCEL) == 2)
                return 0;
            if (hwnd != GetCurrentProcessHwnd())
                SetFocus(GetCurrentProcessHwnd());
            break;
        default:
            break;
    }

    return CallWindowProc((WNDPROC)oldWndProc, hwnd, message, wParam, lParam);
}

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            {
                currentDLLModule = hinstDLL;
                // attach to process
                // return FALSE to fail DLL load
                oldWndProc = SetWindowLongPtr(GetCurrentProcessHwnd(), GWL_WNDPROC, (LONG)WindowProcedure);
                InitHook(GetCurrentProcessHwnd());
                break;
            }
        case DLL_PROCESS_DETACH:
            // detach from process
            // SetWindowLongPtr(GetCurrentProcessHwnd(), GWL_WNDPROC, (LONG)oldWndProc);
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
}
