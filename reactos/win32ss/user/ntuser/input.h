#pragma once

#include <ndk/kbd.h>
 
typedef struct tagKBDNLSLAYER
{
    USHORT OEMIdentifier;
    USHORT LayoutInformation;
    UINT NumOfVkToF;
    struct _VK_TO_FUNCTION_TABLE *pVkToF;
    INT NumOfMouseVKey;
    PUSHORT pusMouseVKey;
} KBDNLSLAYER, *PKBDNLSLAYER;
 
typedef struct tagKBDFILE
{
    HEAD head;
    struct tagKBDFILE *pkfNext;
    WCHAR awchKF[20];
    HANDLE hBase;
    struct _KBDTABLES *pKbdTbl;
    ULONG Size;
    PKBDNLSLAYER pKbdNlsTbl;
} KBDFILE, *PKBDFILE;

typedef struct tagKL
{
    HEAD head;
    struct tagKL *pklNext;
    struct tagKL *pklPrev;
    DWORD dwKL_Flags;
    HKL hkl;
    PKBDFILE spkf;
    DWORD dwFontSigs;
    UINT iBaseCharset;
    USHORT CodePage;
    WCHAR wchDiacritic;
    PIMEINFOEX piiex;
} KL, *PKL;

typedef struct _ATTACHINFO
{
  struct _ATTACHINFO *paiNext;
  PTHREADINFO pti1;
  PTHREADINFO pti2;
} ATTACHINFO, *PATTACHINFO;

extern PATTACHINFO gpai;

/* Keyboard layout undocumented flags */
#define KLF_UNLOAD 0x20000000

/* Key States */
#define KS_DOWN_BIT      0x80
#define KS_LOCK_BIT      0x01
/* Scan Codes */
#define SC_KEY_UP        0x8000
/* lParam bits */
#define LP_DO_NOT_CARE_BIT (1<<25) // For GetKeyNameText

/* General */
INIT_FUNCTION NTSTATUS NTAPI InitInputImpl(VOID);
BOOL FASTCALL IntBlockInput(PTHREADINFO W32Thread, BOOL BlockIt);
DWORD NTAPI CreateSystemThreads(UINT Type);
NTSTATUS FASTCALL UserAttachThreadInput(PTHREADINFO,PTHREADINFO,BOOL);
BOOL FASTCALL IsRemoveAttachThread(PTHREADINFO);
VOID FASTCALL DoTheScreenSaver(VOID);
#define ThreadHasInputAccess(W32Thread) (TRUE)

/* Keyboard */
INIT_FUNCTION NTSTATUS NTAPI InitKeyboardImpl(VOID);
VOID NTAPI UserInitKeyboard(HANDLE hKeyboardDevice);
PKL W32kGetDefaultKeyLayout(VOID);
VOID NTAPI UserProcessKeyboardInput(PKEYBOARD_INPUT_DATA pKeyInput);
BOOL NTAPI UserSendKeyboardInput(KEYBDINPUT *pKbdInput, BOOL bInjected);
PKL NTAPI UserHklToKbl(HKL hKl);
BOOL NTAPI UserSetDefaultInputLang(HKL hKl);

/* Mouse */
WORD FASTCALL UserGetMouseButtonsState(VOID);
VOID NTAPI UserProcessMouseInput(PMOUSE_INPUT_DATA pMouseInputData);
BOOL NTAPI UserSendMouseInput(MOUSEINPUT *pMouseInput, BOOL bInjected);

/* IMM */
UINT FASTCALL IntImmProcessKey(PUSER_MESSAGE_QUEUE, PWND, UINT, WPARAM, LPARAM);

extern DWORD gSystemFS;
extern UINT gSystemCPCharSet; 
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

extern PKL gspklBaseLayout;
extern KEYBOARD_ATTRIBUTES gKeyboardInfo;
