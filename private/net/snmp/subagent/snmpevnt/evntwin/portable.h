//****************************************************************************
//
//  Copyright (c) 1992, Microsoft Corporation
//
//  File:  PORTABLE.H
//
//  Definitions to simplify portability between WIN31 and WIN32
//
//****************************************************************************

#ifndef _PORTABLE_H_
#define _PORTABLE_H_

#ifdef _NTWIN
#ifndef WIN32
#define WIN32
#endif
#endif

#ifdef WIN32

#define GET_WM_COMMAND_ID(wp, lp)       LOWORD(wp)
#define GET_WM_COMMAND_CMD(wp, lp)      HIWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)     (HWND)(lp)

#define GET_WINDOW_ID(hwnd)         (UINT)GetWindowLong(hwnd, GWL_ID)
#define GET_WINDOW_INSTANCE(hwnd)   (HINSTANCE)GetWindowLong(hwnd, \
                                        GWL_HINSTANCE)

#define LONG2POINT(l, pt)  ((pt).x=(SHORT)LOWORD(l), (pt).y=(SHORT)HIWORD(l))

#ifdef __cplusplus

#define NOTIFYPARENT(hwnd,code) { \
                                    UINT nID; \
                                    nID = GET_WINDOW_ID(hwnd); \
                                    (::SendMessage)(::GetParent(hwnd), \
                                        WM_COMMAND, MAKEWPARAM(nID, code), \
                                        (LPARAM)hwnd); \
                                }

#define SENDCOMMAND(hwnd, cmd)  { \
                                    (::SendMessage)(hwnd, WM_COMMAND, \
                                        MAKEWPARAM(cmd, 0), (LPARAM)0); \
                                }

#define POSTCOMMAND(hwnd, cmd)  { \
                                    (::PostMessage)(hwnd, WM_COMMAND, \
                                        MAKEWPARAM(cmd, 0), (LPARAM)0); \
                                }

#else // !__cplusplus

#define NOTIFYPARENT(hwnd,code) { \
                                    UINT nID; \
                                    nID = GET_WINDOW_ID(hwnd); \
                                    SendMessage(GetParent(hwnd), WM_COMMAND, \
                                        MAKEWPARAM(nID, code), \
                                        (LPARAM)hwnd); \
                                }

#define SENDCOMMAND(hwnd, cmd)  { \
                                    SendMessage(hwnd, WM_COMMAND, \
                                        MAKEWPARAM(cmd, 0), (LPARAM)0); \
                                }

#define POSTCOMMAND(hwnd, cmd)  { \
                                    PostMessage(hwnd, WM_COMMAND, \
                                        MAKEWPARAM(cmd, 0), (LPARAM)0); \
                                }

#endif // __cplusplus

#else // !WIN32

// Some type definitions excluded from win31 sdk...
// Should these ever be defined for win31, GET RID OF THESE!
typedef float FLOAT;
typedef char TCHAR;

#define GET_WM_COMMAND_ID(wp, lp)       LOWORD(wp)
#define GET_WM_COMMAND_CMD(wp, lp)      HIWORD(lp)
#define GET_WM_COMMAND_HWND(wp, lp)     (HWND)LOWORD(lp)

#define GET_WINDOW_ID(hwnd)         (UINT)GetWindowWord(hwnd, GWW_ID)
#define GET_WINDOW_INSTANCE(hwnd)   (HINSTANCE)GetWindowWord(hwnd, \
                                        GWW_HINSTANCE)

#define LONG2POINT(l, pt)  ((pt).x = (int)LOWORD(l), (pt).y = (int)HIWORD(l))

#ifdef __cplusplus

#define NOTIFYPARENT(hwnd,code) { \
                                    UINT nID; \
                                    nID = GET_WINDOW_ID(hwnd); \
                                    (::SendMessage)(::GetParent(hwnd), \
                                        WM_COMMAND, nID, \
                                        MAKELPARAM(hwnd, code)); \
                                }

#define SENDCOMMAND(hwnd, cmd)  { \
                                    (::SendMessage)(hwnd,WM_COMMAND,cmd,0); \
                                }

#define POSTCOMMAND(hwnd, cmd)  { \
                                    (::PostMessage)(hwnd,WM_COMMAND,cmd,0); \
                                }

#else // !__cplusplus

#define NOTIFYPARENT(hwnd,code) { \
                                    UINT nID; \
                                    nID = GET_WINDOW_ID(hwnd); \
                                    SendMessage(GetParent(hwnd), WM_COMMAND, \
                                        nID, MAKELPARAM(hwnd, code)); \
                                }

#define SENDCOMMAND(hwnd, cmd)  { \
                                    SendMessage(hwnd, WM_COMMAND, cmd, 0); \
                                }

#define POSTCOMMAND(hwnd, cmd)  { \
                                    PostMessage(hwnd, WM_COMMAND, cmd, 0); \
                                }

#endif // __cplusplus

#endif // WIN32

#endif // _PORTABLE_H_
