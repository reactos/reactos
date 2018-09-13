//--------------------------------------------------------------------------;
//
//  File: perfpage.h
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved
//
//
//--------------------------------------------------------------------------;

#ifndef PERFPAGE_HEADER
#define PERFPAGE_HEADER


#define MAX_HW_LEVEL			(3)
#define MAX_SRC_LEVEL			(2)
#define DEFAULT_HW_LEVEL		(2)
#define DEFAULT_SRC_LEVEL		(0)


BOOL CALLBACK PerformanceHandler(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);


#endif // PERFPAGE_HEADER