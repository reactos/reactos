#ifndef __NCPA_H
#define __NCPA_H

typedef LONG (CALLBACK *CPLAPPLET_PROC)(VOID);

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  CPLAPPLET_PROC AppletProc;
} APPLET, *PAPPLET;

typedef struct
{
  WCHAR CurrentAdapterName[MAX_ADAPTER_NAME];
  UINT_PTR hStatsUpdateTimer;
  PMIB_IFTABLE pIfTable;
  DWORD IfTableSize;
  PIP_ADAPTER_INFO pFirstAdapterInfo;
  PIP_ADAPTER_INFO pCurrentAdapterInfo;
} GLOBAL_NCPA_DATA, *PGLOBAL_NCPA_DATA;

extern HINSTANCE hApplet;

extern ULONG DbgPrint(PCCH Fmt, ...);

#endif // __NCPA_H

/* EOF */
