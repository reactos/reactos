#pragma once

#include <ndk/kbd.h>

typedef struct _KBL
{
  LIST_ENTRY List;
  DWORD Flags;
  WCHAR Name[KL_NAMELENGTH];    // used w GetKeyboardLayoutName same as wszKLID.
  struct _KBDTABLES* KBTables;  // KBDTABLES in ndk/kbd.h
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
/* Scan Codes */
#define SC_KEY_UP        0x8000
/* lParam bits */
#define LP_DO_NOT_CARE_BIT (1<<25) // for GetKeyNameText


INIT_FUNCTION NTSTATUS NTAPI InitInputImpl(VOID);
INIT_FUNCTION NTSTATUS NTAPI InitKeyboardImpl(VOID);
VOID NTAPI UserInitKeyboard(HANDLE hKeyboardDevice);
PKBL W32kGetDefaultKeyLayout(VOID);
VOID NTAPI UserProcessKeyboardInput(PKEYBOARD_INPUT_DATA pKeyInput);
BOOL NTAPI UserSendKeyboardInput(KEYBDINPUT *pKbdInput, BOOL bInjected);
VOID NTAPI UserProcessMouseInput(PMOUSE_INPUT_DATA Data, ULONG InputCount);
BOOL FASTCALL IntBlockInput(PTHREADINFO W32Thread, BOOL BlockIt);
BOOL FASTCALL IntMouseInput(MOUSEINPUT *mi, BOOL Injected);
BOOL UserInitDefaultKeyboardLayout(VOID);
PKBL UserHklToKbl(HKL hKl);
VOID NTAPI KeyboardThreadMain(PVOID StartContext);
DWORD NTAPI CreateSystemThreads(UINT Type);
BOOL FASTCALL UserAttachThreadInput(PTHREADINFO,PTHREADINFO,BOOL);
VOID FASTCALL DoTheScreenSaver(VOID);
#define ThreadHasInputAccess(W32Thread) (TRUE)

extern HANDLE ghKeyboardDevice;
extern PTHREADINFO ptiRawInput;
extern BYTE gafAsyncKeyState[256 * 2 / 8]; // 2 bits per key

#define GET_KS_BYTE(vk) ((vk) * 2 / 8)
#define GET_KS_DOWN_BIT(vk) (1 << (((vk) % 4)*2))
#define GET_KS_LOCK_BIT(vk) (1 << (((vk) % 4)*2 + 1))
#define IS_KEY_DOWN(ks, vk) (((ks)[GET_KS_BYTE(vk)] & GET_KS_DOWN_BIT(vk)) ? TRUE : FALSE)
#define IS_KEY_LOCKED(ks, vk) (((ks)[GET_KS_BYTE(vk)] & GET_KS_LOCK_BIT(vk)) ? TRUE : FALSE)
#define SET_KEY_DOWN(ks, vk, down) (ks)[GET_KS_BYTE(vk)] = ((down) ? \
                                                            ((ks)[GET_KS_BYTE(vk)] | GET_KS_DOWN_BIT(vk)) : \
                                                            ((ks)[GET_KS_BYTE(vk)] & ~GET_KS_DOWN_BIT(vk)))
#define SET_KEY_LOCKED(ks, vk, down) (ks)[GET_KS_BYTE(vk)] = ((down) ? \
                                                              ((ks)[GET_KS_BYTE(vk)] | GET_KS_LOCK_BIT(vk)) : \
                                                              ((ks)[GET_KS_BYTE(vk)] & ~GET_KS_LOCK_BIT(vk)))

