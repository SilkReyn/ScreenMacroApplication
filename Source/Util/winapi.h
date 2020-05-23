#pragma once

#include <map>
#include <string>


namespace WinOS {

void listDsktParentWindows(std::map<const void*, std::wstring>* rOutHwnd_TitleMap);

bool checkIsValidWindow(const void* hWnd);

void clickWindowHere(const void* hWnd, int xPos, int yPos, bool returnAfter=false, unsigned long durationMs=0);

void moveMouseAbsolute(unsigned short xPxl, unsigned short yPxl);

void mouseClick(unsigned long durationMs);

}// Namespace WinOS