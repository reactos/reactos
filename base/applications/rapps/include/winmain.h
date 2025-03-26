#pragma once

#include "settings.h"

extern LPCWSTR szWindowClass;

extern HWND hMainWnd;
extern HINSTANCE hInst;

// integrity.cpp
BOOL VerifyInteg(LPCWSTR lpSHA1Hash, LPCWSTR lpFileName);
