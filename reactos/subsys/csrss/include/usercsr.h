/* $Id: usercsr.h,v 1.1 2003/10/20 18:02:04 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/usercsr/usercsr.h
 * PURPOSE:         Interface to usercsr.dll
 */

#include <windows.h>

typedef BOOL STDCALL (*USERCSRINITCONSOLEPROC)(PCSRSS_CONSOLE Console);
typedef VOID STDCALL (*USERCSRDRAWREGIONPROC)(PCSRSS_CONSOLE Console, SMALL_RECT Region);
typedef VOID STDCALL (*USERCSRCOPYREGIONPROC)(PCSRSS_CONSOLE Console,
                                              RECT *Source,
                                              RECT *Dest);
typedef VOID STDCALL (*USERCSRCHANGETITLEPROC)(PCSRSS_CONSOLE Console);
typedef VOID STDCALL (*USERCSRDELETECONSOLEPROC)(PCSRSS_CONSOLE Console);
typedef VOID STDCALL (*USERCSRPROCESSKEYCALLBACK)(MSG *msg, PCSRSS_CONSOLE Console);

typedef struct
{
  USERCSRINITCONSOLEPROC InitConsole;
  USERCSRDRAWREGIONPROC DrawRegion;
  USERCSRCOPYREGIONPROC CopyRegion;
  USERCSRCHANGETITLEPROC ChangeTitle;
  USERCSRDELETECONSOLEPROC DeleteConsole;
} USERCSRFUNCS, *PUSERCSRFUNCS;

typedef BOOL STDCALL (*USERCSRINITIALIZEPROC)(PUSERCSRFUNCS Funcs,
                                              HANDLE CsrssApiHeap,
                                              USERCSRPROCESSKEYCALLBACK ProcessKey);

extern HANDLE UserCsrApiHeap;
extern USERCSRPROCESSKEYCALLBACK UserCsrProcessKey;
