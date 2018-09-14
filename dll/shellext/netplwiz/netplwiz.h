#pragma once

#include "resource.h"

#include <objbase.h>
#include <stdio.h>
#include <strsafe.h>
#include <windows.h>
#include <winnetwk.h>
#include <commctrl.h>

extern HINSTANCE hInstance;

HRESULT WINAPI SHDisconnectNetDrives(PVOID Unused);
