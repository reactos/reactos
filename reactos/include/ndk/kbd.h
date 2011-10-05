#define __KBD_H

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
#define GET_KBD_VERSION(p) (HIWORD((p)->fLocalFlags))
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

#ifdef __cplusplus
};
#endif//__KBD_H
