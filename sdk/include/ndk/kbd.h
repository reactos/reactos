#ifndef __KBD_H
#define __KBD_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Virtual key flags */
#define KBDEXT     0x100  /* Extended key code */
#define KBDMULTIVK 0x200  /* Multi-key */
#define KBDSPECIAL 0x400  /* Special key */
#define KBDNUMPAD  0x800  /* Number-pad */

/* Modifier bits */
#define KBDSHIFT   0x001  /* Shift modifier */
#define KBDCTRL    0x002  /* Ctrl modifier */
#define KBDALT     0x004  /* Alt modifier */

/* Invalid shift */
#define SHFT_INVALID 0x0F

  typedef struct _VK_TO_BIT {
    BYTE Vk;
    BYTE ModBits;
  } VK_TO_BIT, *PVK_TO_BIT;

  typedef struct _MODIFIERS {
    PVK_TO_BIT pVkToBit;
    WORD wMaxModBits;
    BYTE ModNumber[];
  } MODIFIERS, *PMODIFIERS;

#define TYPEDEF_VK_TO_WCHARS(i) \
  typedef struct _VK_TO_WCHARS ## i { \
    BYTE VirtualKey; \
    BYTE Attributes; \
    WCHAR wch[i]; \
  } VK_TO_WCHARS ## i, *PVK_TO_WCHARS ## i;

  TYPEDEF_VK_TO_WCHARS(1)
  TYPEDEF_VK_TO_WCHARS(2)
  TYPEDEF_VK_TO_WCHARS(3)
  TYPEDEF_VK_TO_WCHARS(4)
  TYPEDEF_VK_TO_WCHARS(5)
  TYPEDEF_VK_TO_WCHARS(6)
  TYPEDEF_VK_TO_WCHARS(7)
  TYPEDEF_VK_TO_WCHARS(8)
  TYPEDEF_VK_TO_WCHARS(9)
  TYPEDEF_VK_TO_WCHARS(10)

  typedef struct _VK_TO_WCHAR_TABLE {
    PVK_TO_WCHARS1 pVkToWchars;
    BYTE nModifications;
    BYTE cbSize;
  } VK_TO_WCHAR_TABLE, *PVK_TO_WCHAR_TABLE;

  typedef struct _DEADKEY {
    DWORD dwBoth;
    WCHAR wchComposed;
    USHORT uFlags;
  } DEADKEY, *PDEADKEY;

  typedef WCHAR *DEADKEY_LPWSTR;

#define DKF_DEAD 1

  typedef struct _VSC_LPWSTR {
    BYTE vsc;
    LPWSTR pwsz;
  } VSC_LPWSTR, *PVSC_LPWSTR;

  typedef struct _VSC_VK {
    BYTE Vsc;
    USHORT Vk;
  } VSC_VK, *PVSC_VK;

#define TYPEDEF_LIGATURE(i) \
typedef struct _LIGATURE ## i { \
  BYTE VirtualKey; \
  WORD ModificationNumber; \
  WCHAR wch[i]; \
} LIGATURE ## i, *PLIGATURE ## i;

  TYPEDEF_LIGATURE(1)
  TYPEDEF_LIGATURE(2)
  TYPEDEF_LIGATURE(3)
  TYPEDEF_LIGATURE(4)
  TYPEDEF_LIGATURE(5)

#define KBD_VERSION 1
#define GET_KBD_VERSION(p) (HIWORD((p)->fLocaleFlags))
#define KLLF_ALTGR     0x1
#define KLLF_SHIFTLOCK 0x2
#define KLLF_LRM_RLM   0x4

  typedef struct _KBDTABLES {
    PMODIFIERS pCharModifiers;
    PVK_TO_WCHAR_TABLE pVkToWcharTable;
    PDEADKEY pDeadKey;
    VSC_LPWSTR *pKeyNames;
    VSC_LPWSTR *pKeyNamesExt;
    LPWSTR *pKeyNamesDead;
    USHORT *pusVSCtoVK;
    BYTE bMaxVSCtoVK;
    PVSC_VK pVSCtoVK_E0;
    PVSC_VK pVSCtoVK_E1;
    DWORD fLocaleFlags;
    BYTE nLgMaxd;
    BYTE cbLgEntry;
    PLIGATURE1 pLigature;
  } KBDTABLES, *PKBDTABLES;

/* Constants that help table decoding */
#define WCH_NONE  0xf000
#define WCH_DEAD  0xf001
#define WCH_LGTR  0xf002

/* VK_TO_WCHARS attributes */
#define CAPLOK       0x01
#define SGCAPS       0x02
#define CAPLOKALTGR  0x04
#define KANALOK      0x08
#define GRPSELTAP    0x80

#define VK_ABNT_C1  0xC1
#define VK_ABNT_C2  0xC2

/* Useful scancodes */
#define SCANCODE_LSHIFT  0x2A
#define SCANCODE_RSHIFT  0x36
#define SCANCODE_CTRL    0x1D
#define SCANCODE_ALT     0x38

/* ------------------------------------------------------------------ */
/* Multi-table definitions                                            */

typedef struct tagKBDTABLE_DESC
{
    WCHAR wszDllName[32];
    DWORD dwType;
    DWORD dwSubType;
} KBDTABLE_DESC, *PKBDTABLE_DESC;

#define KBDTABLE_MULTI_MAX 8

typedef struct tagKBDTABLE_MULTI
{
    UINT nTables;
    KBDTABLE_DESC aKbdTables[KBDTABLE_MULTI_MAX];
} KBDTABLE_MULTI, *PKBDTABLE_MULTI;

/* ------------------------------------------------------------------ */
/* NLS (Far-East) definitions                                         */

#define KBDNLS_TYPE_NULL    0
#define KBDNLS_TYPE_NORMAL  1
#define KBDNLS_TYPE_TOGGLE  2

#define KBDNLS_INDEX_NORMAL 1
#define KBDNLS_INDEX_ALT    2

/* NLSFEProcIndex values */
#define KBDNLS_NULL             0  /* invalid */
#define KBDNLS_NOEVENT          1  /* swallow the key */
#define KBDNLS_SEND_BASE_VK     2  /* send the base VK */
#define KBDNLS_SEND_PARAM_VK    3  /* send the VK in the parameter */
#define KBDNLS_KANALOCK         4
#define KBDNLS_ALPHANUM         5
#define KBDNLS_HIRAGANA         6
#define KBDNLS_KATAKANA         7
#define KBDNLS_SBCSDBCS         8
#define KBDNLS_ROMAN            9
#define KBDNLS_CODEINPUT       10
#define KBDNLS_HELP_OR_END     11
#define KBDNLS_HOME_OR_CLEAR   12
#define KBDNLS_NUMPAD          13
#define KBDNLS_KANAEVENT       14
#define KBDNLS_CONV_OR_NONCONV 15

typedef struct tagVK_FPARAM
{
    BYTE  NLSFEProcIndex;
    ULONG NLSFEProcParam;
} VK_FPARAM, *PVK_FPARAM;

typedef struct tagVK_F
{
    BYTE      Vk;
    BYTE      NLSFEProcType;
    BYTE      NLSFEProcCurrent;
    BYTE      NLSFEProcSwitch;
    VK_FPARAM NLSFEProc[8];
    VK_FPARAM NLSFEProcAlt[8];
} VK_F, *PVK_F;

typedef struct tagKBDNLSTABLES
{
    USHORT OEMIdentifier;
    USHORT LayoutInformation;
    ULONG  NumOfVkToF;
    PVK_F  pVkToF;
    INT    NumOfMouseVKey;
    PUSHORT pusMouseVKey;
} KBDNLSTABLES, *PKBDNLSTABLES;

/* KBDTABLES as used by the FE layouts: dwType/dwSubType follow pLigature */
typedef struct tagKBDTABLES_FE
{
    KBDTABLES Base;
    DWORD     dwType;
    DWORD     dwSubType;
} KBDTABLES_FE, *PKBDTABLES_FE;

#if (NTDDI_VERSION >= NTDDI_WINXP)
#define VK_DBE_ALPHANUMERIC           0xF0
#define VK_DBE_KATAKANA               0xF1
#define VK_DBE_HIRAGANA               0xF2
#define VK_DBE_SBCSCHAR               0xF3
#define VK_DBE_DBCSCHAR               0xF4
#define VK_DBE_ROMAN                  0xF5
#define VK_DBE_NOROMAN                0xF6
#define VK_DBE_ENTERWORDREGISTERMODE  0xF7
#define VK_DBE_ENTERIMECONFIGMODE     0xF8
#define VK_DBE_FLUSHSTRING            0xF9
#define VK_DBE_CODEINPUT              0xFA
#define VK_DBE_NOCODEINPUT            0xFB
#define VK_DBE_DETERMINESTRING        0xFC
#define VK_DBE_ENTERDLGCONVERSIONMODE 0xFD
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __KBD_H */
