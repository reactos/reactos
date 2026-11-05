/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Keyboard layout definitions
 * COPYRIGHT:   Copyright 2003 James Tabor
 *              Copyright 2011 Rafal Harabien
 *              Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#ifndef __KBD_H
#define __KBD_H
#pragma once

/* ------------------------------------------------------------------ */
/* Pointer slots inside the layout tables                             */
/*                                                                    */
/* Native layout   : slots are ordinary pointers.                     */
/* WOW64 layout    : (BUILD_WOW6432, 32-bit image) every pointer slot */
/*                   is 64 bits wide, as in the Windows DDK kbd.h.    */
/*                                                                    */
/* KBD_PTR(T) declares a table pointer slot of type T.                */
/*   MSVC / clang-cl : T __ptr64                                      */
/*   GCC / clang     : __ptr64 does not exist, so the slot is a       */
/*                     8-byte aligned union whose first member is the */
/*                     pointer. Because the first member of a union   */
/*                     takes the initializer (C99 6.7.8p20) and the   */
/*                     remaining bytes of a static object are zero,   */
/*                     table sources such as                          */
/*                         { aVkToBits, 7, {...} }                    */
/*                     compile unchanged and the upper 32 bits of the */
/*                     slot are 0. This relies on little endian (x86).*/
/*                     Code that *reads* a table built this way must  */
/*                     use ->p; only layout DLLs are built like this. */
#if defined(BUILD_WOW6432) && !defined(_WIN64)
# if defined(_MSC_VER)
#  define KBD_LONG_POINTER __ptr64
#  define KBD_PTR(T) T KBD_LONG_POINTER
# else
#  if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#   error "WOW64 keyboard tables without __ptr64 require a little-endian target"
#  endif
#  define KBD_LONG_POINTER
#  define KBD_PTR(T) union { T p; ULONG64 u64; } __attribute__((aligned(8)))
#  define KBD_PTR_IS_UNION 1
#  pragma GCC diagnostic ignored "-Wmissing-braces" /* brace elision is intended */
# endif
#else
# define KBD_LONG_POINTER
# define KBD_PTR(T) T
#endif

#ifdef KBD_PTR_IS_UNION
# define KBD_P(x) ((x).p)
#else
# define KBD_P(x) (x)
#endif

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
    KBD_PTR(PVK_TO_BIT) pVkToBit;
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
    KBD_PTR(PVK_TO_WCHARS1) pVkToWchars;
    BYTE nModifications;
    BYTE cbSize;
  } VK_TO_WCHAR_TABLE, *PVK_TO_WCHAR_TABLE;

  typedef struct _DEADKEY {
    DWORD dwBoth;
    WCHAR wchComposed;
    USHORT uFlags;
  } DEADKEY, *PDEADKEY;

  /* One slot of the dead-key name array (pKeyNamesDead points at these) */
  typedef KBD_PTR(LPWSTR) DEADKEY_LPWSTR;

#define DKF_DEAD 1

  typedef struct _VSC_LPWSTR {
    BYTE vsc;
    KBD_PTR(LPWSTR) pwsz;
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
    KBD_PTR(PMODIFIERS) pCharModifiers;
    KBD_PTR(PVK_TO_WCHAR_TABLE) pVkToWcharTable;
    KBD_PTR(PDEADKEY) pDeadKey;
    KBD_PTR(VSC_LPWSTR *) pKeyNames;
    KBD_PTR(VSC_LPWSTR *) pKeyNamesExt;
    KBD_PTR(DEADKEY_LPWSTR *) pKeyNamesDead;
    KBD_PTR(USHORT *) pusVSCtoVK;
    BYTE bMaxVSCtoVK;
    KBD_PTR(PVSC_VK) pVSCtoVK_E0;
    KBD_PTR(PVSC_VK) pVSCtoVK_E1;
    DWORD fLocaleFlags;
    BYTE nLgMaxd;
    BYTE cbLgEntry;
    KBD_PTR(PLIGATURE1) pLigature;
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

typedef struct _VK_FUNCTION_PARAM
{
    BYTE  NLSFEProcIndex;
    ULONG NLSFEProcParam;
} VK_FPARAM, *PVK_FPARAM;

typedef struct _VK_TO_FUNCTION_TABLE
{
    BYTE      Vk;
    BYTE      NLSFEProcType;
    BYTE      NLSFEProcCurrent;
    BYTE      NLSFEProcSwitch;
    VK_FPARAM NLSFEProc[8];
    VK_FPARAM NLSFEProcAlt[8];
} VK_F, *PVK_F;

typedef struct tagKbdNlsLayer
{
    USHORT OEMIdentifier;
    USHORT LayoutInformation;
    ULONG  NumOfVkToF;
    KBD_PTR(PVK_F) pVkToF;
    INT    NumOfMouseVKey;
    KBD_PTR(PUSHORT) pusMouseVKey;
} KBDNLSTABLES, *PKBDNLSTABLES;

/* KBDTABLES as used by the FE layouts: dwType/dwSubType follow pLigature */
typedef struct tagKBDTABLES_FE
{
    KBDTABLES Base;
    DWORD     dwType;
    DWORD     dwSubType;
} KBDTABLES_FE, *PKBDTABLES_FE;

/* ------------------------------------------------------------------ */
/* Layout self-checks: the WOW64 layout must equal the x64 layout.    */
#if defined(BUILD_WOW6432) && defined(C_ASSERT)
C_ASSERT(sizeof(MODIFIERS)         == 16);
C_ASSERT(sizeof(VK_TO_WCHAR_TABLE) == 16);
C_ASSERT(sizeof(VSC_LPWSTR)        == 16);
C_ASSERT(sizeof(DEADKEY_LPWSTR)    == 8);
C_ASSERT(sizeof(KBDTABLES)         == 96);
C_ASSERT(sizeof(KBDNLSTABLES)      == 32);
#endif

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
#endif // (NTDDI_VERSION >= NTDDI_WINXP)

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __KBD_H */
