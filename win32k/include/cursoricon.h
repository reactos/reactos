#ifndef _WIN32K_CURSORICON_H
#define _WIN32K_CURSORICON_H

#define MAXCURICONHANDLES 4096

typedef struct tagCURICON_PROCESS
{
  LIST_ENTRY ListEntry;
  PW32PROCESS Process;
} CURICON_PROCESS, *PCURICON_PROCESS;

typedef struct _CURICON_OBJECT
{
   union
   {
      USER_OBJECT_HDR hdr;
      struct
      {
         /*---------- USER_OBJECT_HDR --------------*/
         HCURSOR hSelf; /* want typesafe handle */
         BYTE flags_placeholder;
         /*---------- USER_OBJECT_HDR --------------*/

         LIST_ENTRY ListEntry;
         LIST_ENTRY ProcessList;
         HMODULE hModule;
         HRSRC hRsrc;
         HRSRC hGroupRsrc;
         SIZE Size;
         BYTE Shadow;
         ICONINFO IconInfo;
      };
   };
} CURICON_OBJECT, *PCURICON_OBJECT;

typedef struct _CURSORCLIP_INFO
{
  BOOL IsClipped;
  UINT Left;
  UINT Top;
  UINT Right;
  UINT Bottom;
} CURSORCLIP_INFO, *PCURSORCLIP_INFO;

typedef struct _SYSTEM_CURSORINFO
{
  BOOL Enabled;
  BOOL SwapButtons;
  UINT ButtonsDown;
  CURSORCLIP_INFO CursorClipInfo;
  PCURICON_OBJECT CurrentCursorObject;
  BYTE ShowingCursor;
  UINT DblClickSpeed;
  UINT DblClickWidth;
  UINT DblClickHeight;
  DWORD LastBtnDown;
  LONG LastBtnDownX;
  LONG LastBtnDownY;
  HANDLE LastClkWnd; //FIXME
} SYSTEM_CURSORINFO, *PSYSTEM_CURSORINFO;


#endif /* _WIN32K_CURSORICON_H */

/* EOF */

