/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
// debug.cxx

#include "core.hxx"
#pragma hdrstop
#include "engine.h"

#ifdef _DEBUG

bool g_fLogDebugOutput = true;

void DebugAssert(LPSTR szFilename, DWORD dwLine)
{
    const char szAssertString[] = "%s, line %lu\n(Terminate\\Continue\\Debug)";
    char rgchOutput[_MAX_PATH + sizeof(szAssertString)/sizeof(szAssertString[0]) + 10];
    int iRet = 0;

    wsprintfA(rgchOutput, szAssertString, szFilename, dwLine);

    iRet = MessageBoxA(NULL, rgchOutput, "Assertion failed !", MB_YESNOCANCEL | MB_DEFBUTTON3);

    switch (iRet)
    {
    case IDYES:
        TerminateProcess(GetCurrentProcess(), 1);
        break;

    case IDNO:
        break;

    case IDCANCEL:
        DebugBreak();
        break;
    }

    return;
}
#endif // _DEBUG

