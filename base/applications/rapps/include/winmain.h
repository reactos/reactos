#pragma once

#include "settings.h"

#define MAINWINDOWCLASSNAME L"ROSAPPMGR2"
#define MAINWINDOWMUTEX szWindowClass
#define UPDATEDBMUTEX ( MAINWINDOWCLASSNAME L":UpDB" )

extern LPCWSTR szWindowClass;

extern HWND hMainWnd;
extern HINSTANCE hInst;

// integrity.cpp
BOOL VerifyInteg(LPCWSTR lpSHA1Hash, LPCWSTR lpFileName);
