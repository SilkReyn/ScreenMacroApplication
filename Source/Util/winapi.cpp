#include <Windows.h>
#include <vector>
#include "winapi.h"


// Anonymous namespace
namespace {
    const int WND_TXT_SZ = 50;
    const unsigned long CLICK_TIME(GetDoubleClickTime() + 5);
    template<typename T> T limToPos(T val) {
        return val < 0 ? 0 : val;
    }
}

namespace WinOS {
using namespace std;

BOOL CALLBACK EnumWindowsProc(_In_ HWND hWnd, _In_ LPARAM lParam)
{
    if (!lParam)
        return false;
    if (!hWnd)
        return true;
    
    reinterpret_cast<vector<HWND>*>(lParam)->push_back(hWnd);

    return true;
}


void listDsktParentWindows(map<const void*, wstring>* rOutHwnd_TitleMap)
{
    if (!rOutHwnd_TitleMap)
        return;

    vector<HWND> handles;
    int srcSz, dstSz;
    wchar_t* wcharBuff = new wchar_t[WND_TXT_SZ];

    dstSz = WND_TXT_SZ;
    EnumWindows(EnumWindowsProc, LONG_PTR(&handles));
    for(auto hdl : handles)  // auto it = Handles.cbegin(); it != Handles.cend(); it++)
    {
        if (IsWindowVisible(hdl) && (!GetWindow(hdl, GW_OWNER)))
        {
            srcSz = GetWindowTextLength(hdl);
            if (srcSz >= dstSz)
            {
                wcharBuff = new wchar_t[++srcSz];
                dstSz = srcSz;
            }
            if (0 < GetWindowTextW(hdl, wcharBuff, dstSz))
            {
                rOutHwnd_TitleMap->emplace(hdl, wstring(wcharBuff));
            }
        }
    }
    delete[] wcharBuff;
}


bool checkIsValidWindow(const void* hWnd)
{
    return hWnd ? IsWindow(HWND(hWnd)) : false;
}


void clickWindowHere(
    const void* hWnd,
    int xPos,
    int yPos,
    bool returnAfter,
    unsigned long durationMs
    )
{
    if (!hWnd || !checkIsValidWindow(hWnd))
        return;
    
    RECT wndRect;
    if (GetWindowRect(HWND(hWnd), &wndRect))  // returns virtual desktop coords
    {
        // TODO: limit wndRect
        POINT msePos;
        GetCursorPos(&msePos);
        long x, y;
        x = limToPos(wndRect.left + xPos);
        y = limToPos(wndRect.top + yPos);
        if ((x | y) > 0xFFFF)  // x, y < short_max
            return;  //throw overflow_error("Mouse coordinates too big");
        // can click on a desktop spread over max 34 FullHD monitors...
        moveMouseAbsolute(
            (unsigned short)x,
            (unsigned short)y
        );
        mouseClick(durationMs ? durationMs : CLICK_TIME);
        if (returnAfter)
        {
            x = limToPos(msePos.x);
            y = limToPos(msePos.y);
            if ((x | y) > 0xFFFF)  // x, y < short_max
                return;  //throw overflow_error("Unsupported desktop resolution");;
            moveMouseAbsolute((unsigned short)x, (unsigned short)y);
        }
    }
}


void moveMouseAbsolute(unsigned short xPxl, unsigned short yPxl)
{
    // Alternatives:
    // GetDeviceCaps( hdcPrimaryMonitor, HORZRES)
    // GetMonitorInfoA( MonitorFromWindow( hwnd, MONITOR_DEFAULTTONEAREST),&MONITORINFO() )
    INPUT inpt = {
        (DWORD)INPUT_MOUSE,  // type
        MOUSEINPUT {         // DUMMYUNIONNAME.mi
            // dx, dy, mouseData, dwFlags, time, dwExtraInfo
            (long)((float)xPxl * (65535.f / (GetSystemMetrics(SM_CXVIRTUALSCREEN)-1))),  //SM_CXSCREEN
            (long)((float)yPxl * (65535.f / (GetSystemMetrics(SM_CYVIRTUALSCREEN)-1))),  //SM_CYSCREEN
            0,
            MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE| MOUSEEVENTF_VIRTUALDESK,
            0,
            0
        }
    };
    SendInput(1, &inpt, sizeof(inpt));
}


void mouseClick(unsigned long durationMs)
{
    INPUT inpt = {
        (DWORD)INPUT_MOUSE,  // type
        MOUSEINPUT {         // DUMMYUNIONNAME.mi
            // dx, dy, mouseData, dwFlags, time, dwExtraInfo
            0, 0, 0, MOUSEEVENTF_LEFTDOWN, 0, 0
        }
    };
    SendInput(1, &inpt, sizeof(inpt));
    Sleep(durationMs);
    inpt.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &inpt, sizeof(inpt));
}

}// Namespace WinOS