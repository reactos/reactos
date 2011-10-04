#pragma once

#include <internal/kbd.h>

typedef struct _KBL
{
  LIST_ENTRY List;
  DWORD Flags;
  WCHAR Name[KL_NAMELENGTH];    // used w GetKeyboardLayoutName same as wszKLID.
  struct _KBDTABLES* KBTables;  // KBDTABLES in ntoskrnl/include/internal/kbd.h
  HANDLE hModule;
  ULONG RefCount;
  HKL hkl;
  DWORD klid; // Low word - language id. High word - device id.
} KBL, *PKBL;

typedef struct _ATTACHINFO
{
  struct _ATTACHINFO* paiNext;
  PTHREADINFO pti1;
  PTHREADINFO pti2;
} ATTACHINFO, *PATTACHINFO;

extern PATTACHINFO gpai;

#define KBL_UNLOAD 1
#define KBL_PRELOAD 2
#define KBL_RESET 4

/* Key States */
#define KS_DOWN_BIT      0x80
#define KS_LOCK_BIT      0x01
/* Lock modifiers */
#define CAPITAL_BIT   0x80000000
#define NUMLOCK_BIT   0x40000000
#define MOD_BITS_MASK 0x3fffffff
#define MOD_KCTRL     0x02
/* Scan Codes */
#define SC_KEY_UP        0x8000
/* lParam bits */
#define LP_EXT_BIT       (1<<24)
/* From kbdxx.c -- Key changes with numlock */
#define KNUMP         0x400

INIT_FUNCTION NTSTATUS NTAPI InitInputImpl(VOID);
INIT_FUNCTION NTSTATUS NTAPI InitKeyboardImpl(VOID);
PKBL W32kGetDefaultKeyLayout(VOID);
VOID FASTCALL W32kKeyProcessMessage(LPMSG Msg, PKBDTABLES KeyLayout, BYTE Prefix);
BOOL FASTCALL IntBlockInput(PTHREADINFO W32Thread, BOOL BlockIt);
BOOL FASTCALL IntMouseInput(MOUSEINPUT *mi, BOOL Injected);
BOOL UserInitDefaultKeyboardLayout(VOID);
PKBL UserHklToKbl(HKL hKl);
BOOL FASTCALL UserAttachThreadInput(PTHREADINFO,PTHREADINFO,BOOL);
VOID FASTCALL DoTheScreenSaver(VOID);
WORD FASTCALL get_key_state(void);
#define ThreadHasInputAccess(W32Thread) \
  (TRUE)

extern PTHREADINFO ptiRawInput;
