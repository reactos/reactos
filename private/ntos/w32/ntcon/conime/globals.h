// Copyright (c) 1985 - 1999, Microsoft Corporation
//
//  MODULE:   Globals.h
//
//  PURPOSE:   Contains declarations for all globally scoped names in the program.
//
//  PLATFORMS: Windows NT-J 3.51
//
//  FUNCTIONS:
//
//  History:
//
//  27.Jul.1995 v-HirShi (Hirotoshi Shimizu)    created
//
//  COMMENTS:
//

extern HANDLE    LastConsole ;
extern HIMC      ghDefaultIMC ;




extern WCHAR           szTitle[];         // The title bar text

#ifdef DEBUG_MODE
extern int       cxMetrics ;
extern int       cyMetrics ;
extern int       cxOverTypeCaret ;
extern int       xPos ;
extern int       xPosLast ;
extern int       CaretWidth;               // insert/overtype mode caret width

extern WCHAR           ConvertLine[CVMAX] ;
extern unsigned char   ConvertLineAtr[CVMAX] ;
#endif
