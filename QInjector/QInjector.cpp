#include "Tools.h"
#include <TCHAR.h>

LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);
bool CreateControls(HWND hwnd);
void InjectDLL(DWORD remoteProcessId);
int GetX(LPARAM x) { return x & 0xFFFF; }
int GetY(LPARAM y) { return (y >> 0x10) & 0xFFFF; }

HWND editProcessName = NULL;
TCHAR editText[MAX_PATH];
HWND buttonInjectDll = NULL;
HWND checkBoxProcessId = NULL;
char relativePath[_MAX_PATH] = "QInjected.dll";
char libPath[_MAX_PATH];
HANDLE remoteProcess = NULL;
DWORD remoteProcessId = NULL;
HANDLE remoteThread = NULL;
LPVOID remoteLibPathAddress = NULL;
DWORD baseAddressModule = NULL;
HMODULE hKernel32 = GetModuleHandle(L"Kernel32");

void InjectDLL(DWORD remoteProcessId)
{
    if (!remoteProcessId)
    {
        MessageBox(NULL, L"Failed to find process", L"Error", MB_OK);
        return;
    }
    // 1. Allocate memory in the remote process for libPath
    // 2. Write libPath to the allocated memory
    remoteProcess = OpenProcess(0x2|0x400|0x8|0x10|0x20, false, remoteProcessId);
    if (!remoteProcess)
    {
        MessageBox(NULL, (std::wstring(L"Failed to open process. Error: ") + std::to_wstring((long long)GetLastError())).c_str(), L"Error - OpenProcess", MB_ICONERROR | MB_OK);
        return;
    }
    if (GetFileAttributesA(relativePath) == INVALID_FILE_ATTRIBUTES)
    {
        DWORD errorCode = GetLastError();
        if (errorCode == ERROR_FILE_NOT_FOUND)
            MessageBox(NULL, L"Failed to locate DLL", L"Error", MB_OK);
        else
            MessageBox(NULL, std::to_wstring((long long)GetLastError()).c_str(), L"Error", MB_OK);
        return;
    }
    GetFullPathNameA(relativePath, sizeof(relativePath), libPath, NULL);
    remoteLibPathAddress = VirtualAllocEx(remoteProcess, NULL, sizeof(libPath), 0x1000, 0x04);
    if (!WriteProcessMemory(remoteProcess, remoteLibPathAddress, (LPVOID)libPath, sizeof(libPath), NULL))
    {
        MessageBox(NULL, (std::to_wstring((long long)GetLastError())).c_str(), L"Error - WriteProcessMemory", MB_OK);
        return;
    }

    // Load "LibSpy.dll" into the remote process
    // (via CreateRemoteThread & LoadLibrary)
    remoteThread = CreateRemoteThread(remoteProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryA"),
                                      remoteLibPathAddress, 0, NULL);
    if (!remoteThread)
    {
        MessageBox(NULL, (std::to_wstring((long long)GetLastError())).c_str(), L"Error - CreateRemoteThread", MB_OK);
        return;
    }

    WaitForSingleObject(remoteThread, INFINITE);

    // Get handle of the loaded module
    GetExitCodeThread(remoteThread, &baseAddressModule);

    // Clean up
    VirtualFreeEx(remoteProcess, remoteLibPathAddress, 0, 0x8000);
    CloseHandle(remoteThread);
    CloseHandle(remoteProcess);

    if (!baseAddressModule)
    {
        MessageBox(NULL, L"Failed.", L"Fail", MB_OK);
        return;
    }

    MessageBox(NULL, L"DLL Injection complete.", L"Success", MB_OK);

    return;

    remoteThread = CreateRemoteThread(remoteProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "FreeLibrary"),
                                      (LPVOID)baseAddressModule, 0, NULL);
    if (!remoteThread)
    {
        MessageBox(NULL, (std::to_wstring((long long)GetLastError())).c_str(), L"CreateRemoteThread", MB_OK);
        return;
    }
    WaitForSingleObject(remoteThread, INFINITE);
    GetExitCodeThread(remoteThread, &baseAddressModule);
    CloseHandle(remoteThread);
}

/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = L"QInjector";

int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 1;

    /* The class is registered, let's create the program*/
    DWORD dwStyle = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);
    hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           L"DLL Injector",       /* Title Text */
           dwStyle, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           600,                 /* The programs width */
           375,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );

    if (!hwnd)
        return 2;

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nCmdShow);
    UpdateWindow(hwnd);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}


/*  This function is called by the Windows function DispatchMessage()  */
LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
                if ((HWND)lParam == buttonInjectDll)
                {
                    GetWindowText(editProcessName, editText, MAX_PATH);
                    if (SendMessage(checkBoxProcessId, BM_GETCHECK, NULL, NULL) == BST_CHECKED)
                        InjectDLL(_tstoi(editText));
                    else
                        InjectDLL(GetProcessIdFromImageName(editText));
                }
                else if ((HWND)lParam == checkBoxProcessId)
                {
                    if (SendMessage(checkBoxProcessId, BM_GETCHECK, NULL, NULL) == BST_UNCHECKED)
                        SendMessage(checkBoxProcessId, BM_SETCHECK, BST_CHECKED, NULL);
                    else
                        SendMessage(checkBoxProcessId, BM_SETCHECK, BST_UNCHECKED, NULL);
                }
                break;
            }
            break;
        case WM_KEYDOWN:
            if (wParam == (VkKeyScan('q') & 0xFF))
            {
                //ExitProcess(0);
            }
            break;
        case WM_CREATE:
            CreateControls(hwnd);
            break;
        case WM_CLOSE:
            if (MessageBox(hwnd, L"Exit the program?", L"WM_CLOSE", MB_ICONWARNING | MB_OKCANCEL) == 2);
            else
                DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

bool CreateControls(HWND hwnd)
{
    TEXTMETRIC tm;
    GetTextMetrics(GetDC(NULL), &tm);
    RECT wndRect;
    GetWindowRect(GetCurrentProcessHwnd(), &wndRect);
    int wndWidth = wndRect.right - wndRect.left;
    int wndHeight = wndRect.bottom - wndRect.top;
    int editWidth = wndWidth*3/4;
    int editHeight = tm.tmHeight + tm.tmDescent;
    int x = (wndWidth - editWidth)/2;
    int y = wndHeight/3;
    int buttonWidth = 100;
    int buttonHeight = 50;
    editProcessName = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | ES_AUTOHSCROLL, x, y, editWidth, editHeight, hwnd, (HMENU)101, (HINSTANCE) GetWindowLong (hwnd, GWL_HINSTANCE), NULL);
    if (!editProcessName)
        return false;
    SendMessage(editProcessName, EM_SETLIMITTEXT, MAX_PATH, NULL);
    buttonInjectDll = CreateWindow(TEXT("BUTTON"), TEXT("Inject DLL"), WS_CHILD | WS_VISIBLE, x + (editWidth/2) - (buttonWidth/2), y + editHeight + (editHeight/2), buttonWidth, buttonHeight, hwnd, NULL, (HINSTANCE) GetWindowLong (hwnd, GWL_HINSTANCE), NULL);
    if (!buttonInjectDll)
        return false;
    checkBoxProcessId = CreateWindow(TEXT("BUTTON"), NULL, WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP | WS_GROUP | BS_MULTILINE, 0, 0, tm.tmMaxCharWidth, tm.tmHeight + tm.tmDescent, hwnd, NULL, (HINSTANCE) GetWindowLong (hwnd, GWL_HINSTANCE), NULL);
    if (!checkBoxProcessId)
        return false;
    CreateWindow(TEXT("STATIC"), L"ASD", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_GROUP, 30, 0, 100, 30, hwnd, NULL, NULL, NULL);
    return true;
}
