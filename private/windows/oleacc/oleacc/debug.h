// Copyright (c) 1996-1999 Microsoft Corporation

// Macros and function prototypes for debugging
#ifdef DEBUG
  #define _DEBUG
#endif
#ifdef _DEBUG
  void FAR CDECL PrintIt(LPTSTR strFmt, ...);
  #define DBPRINTF PrintIt
#else
  #define DBPRINTF        1 ? (void)0 : (void)
#endif
