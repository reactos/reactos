/*
 * ReactOS Calc (main program)
 *
 * Copyright 2007-2017, Carlo Bramini
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "calc.h"

#define HTMLHELP_PATH(_pt)  _T("%systemroot%\\Help\\calc.chm::") _T(_pt)

#define MAKE_BITMASK4(_show_b16, _show_b10, _show_b8, _show_b2) \
    (((_show_b2)  << 0) | \
     ((_show_b8)  << 1) | \
     ((_show_b10) << 2) | \
     ((_show_b16) << 3))

#define MAKE_BITMASK5(_transl, _is_stats, _is_ctrl, _show_b16, _show_b10, _show_b8, _show_b2) \
    (((_show_b2)  << 0) | \
     ((_show_b8)  << 1) | \
     ((_show_b10) << 2) | \
     ((_show_b16) << 3) | \
     ((_is_ctrl)  << 5) | \
     ((_is_stats) << 6) | \
     ((_transl)   << 7))

#define KEY_IS_UP       0x80000000
#define KEY_WAS_DOWN    0x40000000

#define BITMASK_IS_ASCII    0x80
#define BITMASK_IS_STATS    0x40
#define BITMASK_IS_CTRL     0x20
#define BITMASK_HEX_MASK    0x08
#define BITMASK_DEC_MASK    0x04
#define BITMASK_OCT_MASK    0x02
#define BITMASK_BIN_MASK    0x01

#define CALC_CLR_RED        RGB(0xFF, 0x00, 0x00)
#define CALC_CLR_BLUE       RGB(0x00, 0x00, 0xFF)
#define CALC_CLR_PURP       RGB(0xFF, 0x00, 0xFF)

typedef struct {
    CHAR key; // Virtual key identifier
    WORD idc; // IDC for posting message
} key2code_t;

typedef struct {
    WORD     idc;  // IDC for posting message
    CHAR     key;  // Virtual key identifier
    BYTE     mask; // enable/disable into the various modes.
    COLORREF col;  // color used for drawing the text
} key3code_t;

#define CTRL_FLAG   0x100
#define ALT_FLAG    0x200

#define CTRL_A  (0x0001+'A'-'A')
#define CTRL_C  (0x0001+'C'-'A')
#define CTRL_D  (0x0001+'D'-'A')
#define CTRL_L  (0x0001+'L'-'A')
#define CTRL_M  (0x0001+'M'-'A')
#define CTRL_P  (0x0001+'P'-'A')
#define CTRL_R  (0x0001+'R'-'A')
#define CTRL_S  (0x0001+'S'-'A')
#define CTRL_T  (0x0001+'T'-'A')
#define CTRL_V  (0x0001+'V'-'A')
#define CTRL_Z  (0x0001+'Z'-'A')

static const key3code_t key2code[] = {
    /* CONTROL-ID          Key                    asc sta ctl hex dec oct bin */
    { IDC_BUTTON_STA,      CTRL_S,  MAKE_BITMASK5(  1,  0,  1,  1,  1,  1,  1), CALC_CLR_BLUE, },
    { IDC_BUTTON_AVE,      CTRL_A,  MAKE_BITMASK5(  1,  1,  1,  1,  1,  1,  1), CALC_CLR_BLUE, },
    { IDC_BUTTON_SUM,      CTRL_T,  MAKE_BITMASK5(  1,  1,  1,  1,  1,  1,  1), CALC_CLR_BLUE, },
    { IDC_BUTTON_S,        CTRL_D,  MAKE_BITMASK5(  1,  1,  1,  1,  1,  1,  1), CALC_CLR_BLUE, },
    { IDC_BUTTON_MS,       CTRL_M,  MAKE_BITMASK5(  1,  0,  1,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_MR,       CTRL_R,  MAKE_BITMASK5(  1,  0,  1,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_MP,       CTRL_P,  MAKE_BITMASK5(  1,  0,  1,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_MC,       CTRL_L,  MAKE_BITMASK5(  1,  0,  1,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_0,        '0',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_BLUE, },
    { IDC_BUTTON_1,        '1',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_BLUE, },
    { IDC_BUTTON_2,        '2',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_3,        '3',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_4,        '4',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_5,        '5',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_6,        '6',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_7,        '7',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_8,        '8',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  0,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_9,        '9',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  0,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_DOT,      '.',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_DOT,      ',',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), -1,            },
    { IDC_BUTTON_ADD,      '+',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_SUB,      '-',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_MULT,     '*',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_DIV,      '/',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_AND,      '&',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_OR,       '|',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_XOR,      '^',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_LSH,      '<',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_NOT,      '~',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_INT,      ';',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_RED,  },
    { IDC_BUTTON_EQU,      '=',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_A,        'A',     MAKE_BITMASK5(  1,  0,  0,  1,  0,  0,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_B,        'B',     MAKE_BITMASK5(  1,  0,  0,  1,  0,  0,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_C,        'C',     MAKE_BITMASK5(  1,  0,  0,  1,  0,  0,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_D,        'D',     MAKE_BITMASK5(  1,  0,  0,  1,  0,  0,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_E,        'E',     MAKE_BITMASK5(  1,  0,  0,  1,  0,  0,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_F,        'F',     MAKE_BITMASK5(  1,  0,  0,  1,  0,  0,  0), CALC_CLR_BLUE, },
    { IDC_CHECK_HYP,       'H',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), -1,            },
    { IDC_CHECK_INV,       'I',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), -1,            },
    { IDC_BUTTON_LOG,      'L',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_PURP, },
    { IDC_BUTTON_DMS,      'M',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_PURP, },
    { IDC_BUTTON_LN,       'N',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_PURP, },
    { IDC_BUTTON_PI,       'P',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_RX,       'R',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_PURP, },
    { IDC_BUTTON_SIN,      'S',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_PURP, },
    { IDC_BUTTON_COS,      'O',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_PURP, },
    { IDC_BUTTON_TAN,      'T',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_PURP, },
    { IDC_BUTTON_FE,       'V',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_PURP, },
    { IDC_BUTTON_EXP,      'X',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_PURP, },
    { IDC_BUTTON_XeY,      'Y',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_PURP, },
    { IDC_BUTTON_SQRT,     '@',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_BLUE, },
    { IDC_BUTTON_Xe2,      '@',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_PURP, },
    { IDC_BUTTON_Xe3,      '#',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_PURP, },
    { IDC_BUTTON_NF,       '!',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_PURP, },
    { IDC_BUTTON_LEFTPAR,  '(',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_PURP, },
    { IDC_BUTTON_RIGHTPAR, ')',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_PURP, },
    { IDC_BUTTON_MOD,      '%',     MAKE_BITMASK5(  1,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_PERCENT,  '%',     MAKE_BITMASK5(  1,  0,  0,  0,  1,  0,  0), CALC_CLR_BLUE, },
    /*----------------------------------------------------------------------*/
    { IDC_BUTTON_DAT,  VK_INSERT,   MAKE_BITMASK5(  0,  1,  0,  1,  1,  1,  1), CALC_CLR_BLUE, },
    { IDC_BUTTON_EQU,  VK_RETURN,   MAKE_BITMASK5(  0,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_CANC, VK_ESCAPE,   MAKE_BITMASK5(  0,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_CE,   VK_DELETE,   MAKE_BITMASK5(  0,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_BUTTON_BACK, VK_BACK,     MAKE_BITMASK5(  0,  0,  0,  1,  1,  1,  1), CALC_CLR_RED,  },
    { IDC_RADIO_HEX,   VK_F5,       MAKE_BITMASK5(  0,  0,  0,  1,  1,  1,  1), -1,            },
    { IDC_RADIO_DEC,   VK_F6,       MAKE_BITMASK5(  0,  0,  0,  1,  1,  1,  1), -1,            },
    { IDC_RADIO_OCT,   VK_F7,       MAKE_BITMASK5(  0,  0,  0,  1,  1,  1,  1), -1,            },
    { IDC_RADIO_BIN,   VK_F8,       MAKE_BITMASK5(  0,  0,  0,  1,  1,  1,  1), -1,            },
    { IDC_BUTTON_SIGN, VK_F9,       MAKE_BITMASK5(  0,  0,  0,  1,  1,  1,  1), CALC_CLR_BLUE, },
};

static const key2code_t key2code_base16[] = {
    { VK_F2,       IDC_RADIO_DWORD, },
    { VK_F3,       IDC_RADIO_WORD, },
    { VK_F4,       IDC_RADIO_BYTE, },
    { VK_F12,      IDC_RADIO_QWORD, },
};

static const key2code_t key2code_base10[] = {
    { VK_F2,       IDC_RADIO_DEG, },
    { VK_F3,       IDC_RADIO_RAD, },
    { VK_F4,       IDC_RADIO_GRAD, },
};

static const WORD operator_codes[] = {
    /* CONTROL-ID       operator */
    (WORD)IDC_STATIC,   // RPN_OPERATOR_PARENT
    IDC_BUTTON_PERCENT, // RPN_OPERATOR_PERCENT
    IDC_BUTTON_EQU,     // RPN_OPERATOR_EQUAL
    IDC_BUTTON_OR,      // RPN_OPERATOR_OR
    IDC_BUTTON_XOR,     // RPN_OPERATOR_XOR
    IDC_BUTTON_AND,     // RPN_OPERATOR_AND
    IDC_BUTTON_LSH,     // RPN_OPERATOR_LSH
    IDC_BUTTON_RSH,     // RPN_OPERATOR_RSH
    IDC_BUTTON_ADD,     // RPN_OPERATOR_ADD
    IDC_BUTTON_SUB,     // RPN_OPERATOR_SUB
    IDC_BUTTON_MULT,    // RPN_OPERATOR_MULT
    IDC_BUTTON_DIV,     // RPN_OPERATOR_DIV
    IDC_BUTTON_MOD,     // RPN_OPERATOR_MOD
    IDC_BUTTON_XeY,     // RPN_OPERATOR_POW
    IDC_BUTTON_XrY,     // RPN_OPERATOR_SQR
};

typedef void (*rpn_callback1)(calc_number_t *);

typedef struct {
    WORD            idc;
    BYTE            range;
    BYTE            check_nan;
    rpn_callback1   direct;
    rpn_callback1   inverse;
    rpn_callback1   hyperb;
    rpn_callback1   inv_hyp;
} function_table_t;

static void run_fe(calc_number_t *number);
static void run_dat_sta(calc_number_t *number);
static void run_mp(calc_number_t *c);
static void run_mm(calc_number_t *c);
static void run_ms(calc_number_t *c);
static void run_mw(calc_number_t *c);
static void run_canc(calc_number_t *c);
static void run_rpar(calc_number_t *c);
static void run_lpar(calc_number_t *c);

static const function_table_t function_table[] = {
    { IDC_BUTTON_SIN,  MODIFIER_INV|MODIFIER_HYP, 1, rpn_sin,     rpn_asin,    rpn_sinh, rpn_asinh },
    { IDC_BUTTON_COS,  MODIFIER_INV|MODIFIER_HYP, 1, rpn_cos,     rpn_acos,    rpn_cosh, rpn_acosh },
    { IDC_BUTTON_TAN,  MODIFIER_INV|MODIFIER_HYP, 1, rpn_tan,     rpn_atan,    rpn_tanh, rpn_atanh },
    { IDC_BUTTON_INT,  MODIFIER_INV,              1, rpn_int,     rpn_frac,    NULL,     NULL      },
    { IDC_BUTTON_RX,   0,                         1, rpn_reci,    NULL,        NULL,     NULL      },
    { IDC_BUTTON_NOT,  0,                         1, rpn_not,     NULL,        NULL,     NULL      },
    { IDC_BUTTON_PI,   MODIFIER_INV,              0, rpn_pi,      rpn_2pi,     NULL,     NULL      },
    { IDC_BUTTON_Xe2,  MODIFIER_INV,              1, rpn_exp2,    rpn_sqrt,    NULL,     NULL      },
    { IDC_BUTTON_Xe3,  MODIFIER_INV,              1, rpn_exp3,    rpn_cbrt,    NULL,     NULL      },
    { IDC_BUTTON_LN,   MODIFIER_INV,              1, rpn_ln,      rpn_exp,     NULL,     NULL      },
    { IDC_BUTTON_LOG,  MODIFIER_INV,              1, rpn_log,     rpn_exp10,   NULL,     NULL      },
    { IDC_BUTTON_NF,   0,                         1, rpn_fact,    NULL,        NULL,     NULL      },
    { IDC_BUTTON_AVE,  MODIFIER_INV,              0, rpn_ave,     rpn_ave2,    NULL,     NULL      },
    { IDC_BUTTON_SUM,  MODIFIER_INV,              0, rpn_sum,     rpn_sum2,    NULL,     NULL      },
    { IDC_BUTTON_S,    MODIFIER_INV,              0, rpn_s_m1,    rpn_s,       NULL,     NULL      },
    { IDC_BUTTON_SQRT, MODIFIER_INV,              1, rpn_sqrt,    NULL,        NULL,     NULL      },
    { IDC_BUTTON_DMS,  MODIFIER_INV,              1, rpn_dec2dms, rpn_dms2dec, NULL,     NULL      },
    { IDC_BUTTON_FE,   0,                         1, run_fe,      NULL,        NULL,     NULL      },
    { IDC_BUTTON_DAT,  0,                         1, run_dat_sta, NULL,        NULL,     NULL,     },
    { IDC_BUTTON_MP,   MODIFIER_INV|NO_CHAIN,     1, run_mp,      run_mm,      NULL,     NULL,     },
    { IDC_BUTTON_MS,   MODIFIER_INV|NO_CHAIN,     1, run_ms,      run_mw,      NULL,     NULL,     },
    { IDC_BUTTON_CANC, NO_CHAIN,                  0, run_canc,    NULL,        NULL,     NULL,     },
    { IDC_BUTTON_RIGHTPAR, NO_CHAIN,              1, run_rpar,    NULL,        NULL,     NULL,     },
    { IDC_BUTTON_LEFTPAR,  NO_CHAIN,              0, run_lpar,    NULL,        NULL,     NULL,     },
};

/* Sub-classing information for theming support */
typedef struct{
    BOOL    bHover;
    WNDPROC oldProc;
} BTNINFO,*LPBTNINFO;


/*
 * Global variable declaration
 */

calc_t  calc;

/* Hot-state info for theming support */
BTNINFO BtnInfo[255];
UINT    BtnCount;

static void UpdateNumberIntl(void)
{
    /* Get current user defaults */
    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, calc.sDecimal, SIZEOF(calc.sDecimal)))
        StringCbCopy(calc.sDecimal, sizeof(calc.sDecimal), _T("."));

    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, calc.sThousand, SIZEOF(calc.sThousand)))
        StringCbCopy(calc.sThousand, sizeof(calc.sThousand), _T(","));

    /* get the string lengths */
    calc.sDecimal_len = _tcslen(calc.sDecimal);
    calc.sThousand_len = _tcslen(calc.sThousand);
}

static int LoadRegInt(LPCTSTR lpszApp, LPCTSTR lpszKey, int iDefault)
{
    HKEY  hKey;
    int   iValue;
    DWORD tmp;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, lpszApp, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        /* Try to load integer value */
        tmp = sizeof(int);

        if (RegQueryValueEx(hKey, lpszKey, NULL, NULL, (LPBYTE)&iValue, &tmp) == ERROR_SUCCESS)
            iDefault = iValue;

        /* close the key */
        RegCloseKey(hKey);
    }

    return iDefault;
}

static void SaveRegInt(LPCTSTR lpszApp, LPCTSTR lpszKey, int iValue)
{
    HKEY hKey;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, lpszApp, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, lpszKey, 0, REG_DWORD, (const BYTE*)&iValue, sizeof(int));

        /* close the key */
        RegCloseKey(hKey);
    }
}

static void load_config(void)
{
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    switch (osvi.dwPlatformId) {
    case VER_PLATFORM_WIN32s:
    case VER_PLATFORM_WIN32_WINDOWS:
        /* Try to load last selected layout */
        calc.layout = GetProfileInt(_T("SciCalc"), _T("layout"), CALC_LAYOUT_STANDARD);

        /* Try to load last selected formatting option */
        calc.usesep = (GetProfileInt(_T("SciCalc"), _T("UseSep"), FALSE)) ? TRUE : FALSE;
        break;

    default: /* VER_PLATFORM_WIN32_NT */
        /* Try to load last selected layout */
        calc.layout = LoadRegInt(_T("SOFTWARE\\Microsoft\\Calc"), _T("layout"), CALC_LAYOUT_STANDARD);

        /* Try to load last selected formatting option */
        calc.usesep = (LoadRegInt(_T("SOFTWARE\\Microsoft\\Calc"), _T("UseSep"), FALSE)) ? TRUE : FALSE;
        break;
    }

    /* memory is empty at startup */
    calc.is_memory = FALSE;

    /* Get locale info for numbers */
    UpdateNumberIntl();
}

static void save_config(void)
{
    TCHAR buf[32];
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    switch (osvi.dwPlatformId) {
    case VER_PLATFORM_WIN32s:
    case VER_PLATFORM_WIN32_WINDOWS:
        StringCbPrintf(buf, sizeof(buf), _T("%lu"), calc.layout);
        WriteProfileString(_T("SciCalc"), _T("layout"), buf);
        WriteProfileString(_T("SciCalc"), _T("UseSep"), (calc.usesep==TRUE) ? _T("1") : _T("0"));
        break;

    default: /* VER_PLATFORM_WIN32_NT */
        SaveRegInt(_T("SOFTWARE\\Microsoft\\Calc"), _T("layout"), calc.layout);
        SaveRegInt(_T("SOFTWARE\\Microsoft\\Calc"), _T("UseSep"), calc.usesep);
        break;
    }
}

static LRESULT post_key_press(LPARAM lParam, WORD idc)
{
    HWND  hCtlWnd = GetDlgItem(calc.hWnd,idc);
    TCHAR ClassName[64];

    /* check if the key is enabled! */
    if (!IsWindowEnabled(hCtlWnd))
        return 1;

    if (!GetClassName(hCtlWnd, ClassName, SIZEOF(ClassName)))
        return 1;

    if (!_tcscmp(ClassName, WC_BUTTON)) {
        DWORD dwStyle = GetWindowLongPtr(hCtlWnd, GWL_STYLE) & 0xF;

        /* Set states for press/release, but only for push buttons */
        if (dwStyle == BS_PUSHBUTTON || dwStyle == BS_DEFPUSHBUTTON || dwStyle == BS_OWNERDRAW) {
            if (!(lParam & KEY_WAS_DOWN)) {
                PostMessage(hCtlWnd, BM_SETSTATE, 1, 0);
            } else
            if ((lParam & KEY_IS_UP)) {
                PostMessage(hCtlWnd, BM_SETSTATE, 0, 0);
                PostMessage(hCtlWnd, BM_CLICK, 0, 0);
            }
            return 1;
        }
    }
    /* default action: simple click event at key release */
    if ((lParam & KEY_IS_UP)) {
        PostMessage(hCtlWnd, BM_CLICK, 0, 0);
    }
    return 1;
}

static int vk2ascii(unsigned int vk)
{
    unsigned short int s;
    int                scan;
    BYTE               state[256];
    HKL                layout=GetKeyboardLayout(0);

    if(!GetKeyboardState(state))
        return 0;

    scan=MapVirtualKeyEx(vk, 0, layout);
    s = 0;
    if (ToAsciiEx(vk, scan, state, &s, 0, layout)>0) {
        /* convert to upper case */
        if (s >= 'a' && s <= 'z')
            s = s - 'a' + 'A';
        /* add check to CTRL key */
        if (vk >= 'A' && vk <= 'Z' &&
            s >= CTRL_A && s <= CTRL_Z)
            s |= CTRL_FLAG;
        else
        if (GetAsyncKeyState(VK_MENU) < 0)
            s |= ALT_FLAG;
        return s;
    }
    return 0;
}

static int process_vk_key(WPARAM wParam, LPARAM lParam)
{
    const key2code_t *k;
    unsigned int x;
    unsigned short int ch;

    ch = vk2ascii(LOWORD(wParam));
    if ((lParam & KEY_IS_UP)) {
        /* Test for "copy" to clipboard */
        if (ch == (CTRL_C|CTRL_FLAG)) {
            SendMessage(calc.hWnd, WM_COMMAND, IDM_EDIT_COPY, 0);
            return 1;
        }
        /* Test for "paste" from clipboard */
        if (ch == (CTRL_V|CTRL_FLAG)) {
            SendMessage(calc.hWnd, WM_COMMAND, IDM_EDIT_PASTE, 0);
            return 1;
        }
        /* Test of help menu */
        if (LOWORD(wParam) == VK_F1) {
            SendMessage(calc.hWnd, WM_COMMAND, IDM_HELP_HELP, 0);
            return 1;
        }
    }

    for (x=0; x<SIZEOF(key2code); x++) {
        int key = key2code[x].key;
        if (key2code[x].mask & BITMASK_IS_CTRL)
            key |= CTRL_FLAG;
        if ((key == ch             &&  (key2code[x].mask & BITMASK_IS_ASCII)) ||
            (key == LOWORD(wParam) && !(key2code[x].mask & BITMASK_IS_ASCII))
           ) {
            if (GetDlgItem(calc.hWnd, key2code[x].idc) == NULL)
                continue;
            return post_key_press(lParam, key2code[x].idc);
        }
    }
    if (calc.layout == CALC_LAYOUT_SCIENTIFIC) {
        if (calc.base == IDC_RADIO_DEC) {
            k = key2code_base10;
            x = SIZEOF(key2code_base10);
        } else {
            k = key2code_base16;
            x = SIZEOF(key2code_base16);
        }
        do {
            if (k->key == LOWORD(wParam)) {
                return post_key_press(lParam, k->idc);
            }
            k++;
        } while (--x);
    }
    return 0;
}

#ifdef USE_KEYBOARD_HOOK
static LRESULT CALLBACK
KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if(nCode<0 || calc.is_menu_on)
        return CallNextHookEx(calc.hKeyboardHook,nCode,wParam,lParam);

    if(nCode==HC_ACTION)
        if (process_vk_key(wParam, lParam))
            return;

    return CallNextHookEx(calc.hKeyboardHook,nCode,wParam,lParam);
}
#endif

static void update_lcd_display(HWND hwnd)
{
    /*
     * multiply size of calc.buffer by 2 because it may
     * happen that separator is used between each digit.
     * Also added little additional space for dot and '\0'.
     */
    TCHAR tmp[MAX_CALC_SIZE * 2 + 2];

    if (calc.buffer[0] == _T('\0'))
        StringCbCopy(tmp, sizeof(tmp), _T("0"));
    else
        StringCbCopy(tmp, sizeof(tmp), calc.buffer);

    /* Add final '.' in decimal mode (if it's missing), but
     * only if it's a result: no append if it prints "ERROR".
     */
    if (calc.base == IDC_RADIO_DEC && !calc.is_nan) {
        if (_tcschr(tmp, _T('.')) == NULL)
            StringCbCat(tmp, sizeof(tmp), _T("."));
    }
    /* if separator mode is on, let's add an additional space */
    if (calc.usesep && !calc.sci_in && !calc.sci_out && !calc.is_nan) {
        /* go to the integer part of the string */
        TCHAR *p = _tcschr(tmp, _T('.'));
        TCHAR *e = _tcschr(tmp, _T('\0'));
        int    n=0, t;

        if (p == NULL) p = e;
        switch (calc.base) {
        case IDC_RADIO_HEX:
        case IDC_RADIO_BIN:
            t = 4;
            break;
        default:
        /* fall here for:
             IDC_RADIO_DEC:
             IDC_RADIO_OCT: */
            t = 3;
            break;
        }
        while (--p > tmp) {
            if (++n == t && *(p-1) != _T('-')) {
                memmove(p+1, p, (e-p+1)*sizeof(TCHAR));
                e++;
                *p = _T(' ');
                n = 0;
            }
        }
        /* if decimal mode, apply regional settings */
        if (calc.base == IDC_RADIO_DEC) {
            TCHAR *p = tmp;
            TCHAR *e = _tcschr(tmp, _T('.'));

            /* searching for thousands default separator */
            while (p < e) {
                if (*p == _T(' ')) {
                    memmove(p+calc.sThousand_len, p+1, _tcslen(p)*sizeof(TCHAR));
                    memcpy(p, calc.sThousand, calc.sThousand_len*sizeof(TCHAR));
                    p += calc.sThousand_len;
                } else
                    p++;
            }
            /* update decimal point too. */
            memmove(p+calc.sDecimal_len, p+1, _tcslen(p)*sizeof(TCHAR));
            memcpy(p, calc.sDecimal, calc.sDecimal_len*sizeof(TCHAR));
        }
    } else {
        TCHAR *p = _tcschr(tmp, _T('.'));

        /* update decimal point when usesep is false */
        if (p != NULL) {
            memmove(p+calc.sDecimal_len, p+1, _tcslen(p)*sizeof(TCHAR));
            memcpy(p, calc.sDecimal, calc.sDecimal_len*sizeof(TCHAR));
        }
    }
    SetDlgItemText(hwnd, IDC_TEXT_OUTPUT, tmp);
}

static void update_parent_display(HWND hWnd)
{
    TCHAR str[8];
    int   n = eval_parent_count();

    if (!n)
        str[0] = _T('\0');
    else
        StringCbPrintf(str, sizeof(str), _T("(=%d"), n);
    SetDlgItemText(hWnd, IDC_TEXT_PARENT, str);
}

static void build_operand(HWND hwnd, DWORD idc)
{
    unsigned int i = 0, n;
    size_t cbPtr;

    if (idc == IDC_BUTTON_DOT) {
        /* if dot is the first char, it's added automatically */
        if (calc.buffer == calc.ptr) {
            *calc.ptr++ = _T('0');
            *calc.ptr++ = _T('.');
            *calc.ptr   = _T('\0');
            update_lcd_display(hwnd);
            return;
        }
        /* if pressed dot and it's already in the string, then return */
        if (_tcschr(calc.buffer, _T('.')) != NULL)
            return;
    }
    if (idc != IDC_STATIC) {
        while (idc != key2code[i].idc) i++;
    }
    n = calc.ptr - calc.buffer;
    if (idc == IDC_BUTTON_0 && n == 0) {
        /* no need to put the dot because it's handled by update_lcd_display() */
        calc.buffer[0] = _T('0');
        calc.buffer[1] = _T('\0');
        update_lcd_display(hwnd);
        return;
    }
    switch (calc.base) {
    case IDC_RADIO_HEX:
        if (n >= 16)
            return;
        break;
    case IDC_RADIO_DEC:
        if (n >= SIZEOF(calc.buffer)-1)
            return;
        if (calc.sci_in) {
            if (idc != IDC_STATIC)
                calc.esp = (calc.esp * 10 + (key2code[i].key-'0')) % LOCAL_EXP_SIZE;
            if (calc.ptr == calc.buffer)
                StringCbPrintf(calc.ptr, sizeof(calc.buffer), _T("0.e%+d"), calc.esp);
            else {
                /* adds the dot at the end if the number has no decimal part */
                if (!_tcschr(calc.buffer, _T('.')))
                    *calc.ptr++ = _T('.');

                cbPtr = sizeof(calc.buffer) - ((BYTE*)calc.ptr - (BYTE*)calc.buffer);
                StringCbPrintf(calc.ptr, cbPtr, _T("e%+d"), calc.esp);
            }
            update_lcd_display(hwnd);
            return;
        }
        break;
    case IDC_RADIO_OCT:
        if (n >= 22)
            return;
        break;
    case IDC_RADIO_BIN:
        if (n >= 64)
            return;
        break;
    }

    cbPtr = sizeof(calc.buffer) - ((BYTE*)calc.ptr - (BYTE*)calc.buffer);
    StringCbPrintfEx(calc.ptr, cbPtr, &calc.ptr, NULL, STRSAFE_FILL_ON_FAILURE,
                     _T("%C"), key2code[i].key);

    update_lcd_display(hwnd);
}

static void prepare_rpn_result(calc_number_t *rpn, TCHAR *buffer, int size, int base)
{
    if (calc.is_nan) {
        rpn_zero(&calc.code);
        LoadString(calc.hInstance, IDS_MATH_ERROR, buffer, size);
        return;
    }
    prepare_rpn_result_2(rpn, buffer, size, base);
}

static void set_rpn_result(HWND hwnd, calc_number_t *rpn)
{
    calc.sci_in = FALSE;
    prepare_rpn_result(rpn, calc.buffer, SIZEOF(calc.buffer), calc.base);
    calc.ptr = calc.buffer + _tcslen(calc.buffer);
    update_lcd_display(hwnd);
    update_parent_display(hwnd);
}

static void display_rpn_result(HWND hwnd, calc_number_t *rpn)
{
    set_rpn_result(hwnd, rpn);
    calc.ptr = calc.buffer;
}

static int get_modifiers(HWND hWnd)
{
    int modifiers = 0;

    if (IsDlgButtonChecked(hWnd, IDC_CHECK_INV) == BST_CHECKED)
        modifiers |= MODIFIER_INV;
    if (IsDlgButtonChecked(hWnd, IDC_CHECK_HYP) == BST_CHECKED)
        modifiers |= MODIFIER_HYP;

    return modifiers;
}

static void convert_text2number(calc_number_t *a)
{
    /* if the screen output buffer is empty, then */
    /* the operand is taken from the last input */
    if (calc.buffer == calc.ptr) {
        /* if pushed valued is ZERO then we should grab it */
        if (!_tcscmp(calc.buffer, _T("0.")) ||
            !_tcscmp(calc.buffer, _T("0")))
            /* this zero is good for both integer and decimal */
            rpn_zero(a);
        else
            rpn_copy(a, &calc.code);
        return;
    }
    /* ZERO is the default value for all numeric bases */
    rpn_zero(a);
    convert_text2number_2(a);
}

static const struct _update_check_menus {
    DWORD  *sel;
    WORD    idm;
    WORD    idc;
} upd[] = {
    { &calc.layout, IDM_VIEW_STANDARD,   CALC_LAYOUT_STANDARD },
    { &calc.layout, IDM_VIEW_SCIENTIFIC, CALC_LAYOUT_SCIENTIFIC },
    { &calc.layout, IDM_VIEW_CONVERSION, CALC_LAYOUT_CONVERSION },
    /*-----------------------------------------*/
    { &calc.base, IDM_VIEW_HEX, IDC_RADIO_HEX, },
    { &calc.base, IDM_VIEW_DEC, IDC_RADIO_DEC, },
    { &calc.base, IDM_VIEW_OCT, IDC_RADIO_OCT, },
    { &calc.base, IDM_VIEW_BIN, IDC_RADIO_BIN, },
    /*-----------------------------------------*/
    { &calc.degr, IDM_VIEW_DEG,  IDC_RADIO_DEG, },
    { &calc.degr, IDM_VIEW_RAD,  IDC_RADIO_RAD, },
    { &calc.degr, IDM_VIEW_GRAD, IDC_RADIO_GRAD, },
    /*-----------------------------------------*/
    { &calc.size, IDM_VIEW_QWORD, IDC_RADIO_QWORD, },
    { &calc.size, IDM_VIEW_DWORD, IDC_RADIO_DWORD, },
    { &calc.size, IDM_VIEW_WORD,  IDC_RADIO_WORD, },
    { &calc.size, IDM_VIEW_BYTE,  IDC_RADIO_BYTE, },
};

static void update_menu(HWND hWnd)
{
    HMENU        hMenu = GetSubMenu(GetMenu(hWnd), 1);
    unsigned int x;

    for (x=0; x<SIZEOF(upd); x++) {
        if (*(upd[x].sel) != upd[x].idc) {
            CheckMenuItem(hMenu, upd[x].idm, MF_BYCOMMAND|MF_UNCHECKED);
            CheckDlgButton(hWnd, upd[x].idc, BST_UNCHECKED);
        } else {
            CheckMenuItem(hMenu, upd[x].idm, MF_BYCOMMAND|MF_CHECKED);
            CheckDlgButton(hWnd, upd[x].idc, BST_CHECKED);
        }
    }
    CheckMenuItem(hMenu, IDM_VIEW_GROUP, MF_BYCOMMAND|(calc.usesep ? MF_CHECKED : MF_UNCHECKED));
}

typedef struct {
    WORD   idc;
    WORD   mask;
} radio_config_t;

static const radio_config_t radio_setup[] = {
    /* CONTROL-ID                     hex dec oct bin */
    { IDC_RADIO_QWORD,  MAKE_BITMASK4(  1,  0,  1,  1) },
    { IDC_RADIO_DWORD,  MAKE_BITMASK4(  1,  0,  1,  1) },
    { IDC_RADIO_WORD,   MAKE_BITMASK4(  1,  0,  1,  1) },
    { IDC_RADIO_BYTE,   MAKE_BITMASK4(  1,  0,  1,  1) },
    { IDC_RADIO_DEG,    MAKE_BITMASK4(  0,  1,  0,  0) },
    { IDC_RADIO_RAD,    MAKE_BITMASK4(  0,  1,  0,  0) },
    { IDC_RADIO_GRAD,   MAKE_BITMASK4(  0,  1,  0,  0) },
};

static void enable_allowed_controls(HWND hwnd, DWORD base)
{
    BYTE mask;
    int  n;

    switch (base) {
    case IDC_RADIO_DEC:
        mask = BITMASK_DEC_MASK;
        break;
    case IDC_RADIO_HEX:
        mask = BITMASK_HEX_MASK;
        break;
    case IDC_RADIO_OCT:
        mask = BITMASK_OCT_MASK;
        break;
    case IDC_RADIO_BIN:
        mask = BITMASK_BIN_MASK;
        break;
    default:
        return;
    }
    for (n=0; n<SIZEOF(key2code); n++) {
        if (key2code[n].mask != 0) {
            HWND hCtlWnd = GetDlgItem(hwnd, key2code[n].idc);
            BOOL current;

            if ((key2code[n].mask & BITMASK_IS_STATS))
                current = IsWindow(calc.hStatWnd) ? TRUE : FALSE;
            else
                current = (key2code[n].mask & mask) ? TRUE : FALSE;
            if (IsWindowEnabled(hCtlWnd) != current)
                EnableWindow(hCtlWnd, current);
        }
    }
}

static void update_radio(HWND hwnd, unsigned int base)
{
    HMENU   hMenu;
    LPCTSTR lpMenuId;
    WORD    mask;
    int     n;

    switch (base) {
    case IDC_RADIO_DEC:
        lpMenuId = MAKEINTRESOURCE(IDR_MENU_SCIENTIFIC_1);
        mask = BITMASK_DEC_MASK;
        break;
    case IDC_RADIO_HEX:
        lpMenuId = MAKEINTRESOURCE(IDR_MENU_SCIENTIFIC_2);
        mask = BITMASK_HEX_MASK;
        break;
    case IDC_RADIO_OCT:
        lpMenuId = MAKEINTRESOURCE(IDR_MENU_SCIENTIFIC_2);
        mask = BITMASK_OCT_MASK;
        break;
    case IDC_RADIO_BIN:
        lpMenuId = MAKEINTRESOURCE(IDR_MENU_SCIENTIFIC_2);
        mask = BITMASK_BIN_MASK;
        break;
    default:
        return;
    }

    if (calc.base != base) {
        convert_text2number(&calc.code);
        convert_real_integer(base);
        calc.base = base;
        display_rpn_result(hwnd, &calc.code);

        hMenu = GetMenu(hwnd);
        DestroyMenu(hMenu);
        hMenu = LoadMenu(calc.hInstance, lpMenuId);
        SetMenu(hwnd, hMenu);
        update_menu(hwnd);

        for (n=0; n<SIZEOF(radio_setup); n++)
            ShowWindow(GetDlgItem(hwnd, radio_setup[n].idc), (radio_setup[n].mask & mask) ? SW_SHOW : SW_HIDE);

        enable_allowed_controls(hwnd, base);
    }

    CheckRadioButton(hwnd, IDC_RADIO_HEX, IDC_RADIO_BIN, calc.base);

    if (base == IDC_RADIO_DEC)
        CheckRadioButton(hwnd, IDC_RADIO_DEG, IDC_RADIO_GRAD, calc.degr);
    else
        CheckRadioButton(hwnd, IDC_RADIO_QWORD, IDC_RADIO_BYTE, calc.size);
}

static void update_memory_flag(HWND hWnd, BOOL mem_flag)
{
    calc.is_memory = mem_flag;
    SetDlgItemText(hWnd, IDC_TEXT_MEMORY, mem_flag ? _T("M") : _T(""));
}

static void update_n_stats_items(HWND hWnd, TCHAR *buffer, size_t cbBuffer)
{
    unsigned int n = SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_GETCOUNT, 0, 0);

    StringCbPrintf(buffer, cbBuffer, _T("n=%u"), n);
    SetDlgItemText(hWnd, IDC_TEXT_NITEMS, buffer);
}

static void clean_stat_list(void)
{
    statistic_t *p = calc.stat;

    while (p != NULL) {
        statistic_t *s = p;
        p = (statistic_t *)(p->next);
        rpn_free(&s->num);
        free(s);
    }
    calc.stat = p;
}

static void delete_stat_item(int n)
{
    statistic_t *p = calc.stat;
    statistic_t *s;

    if (n == 0) {
        calc.stat = (statistic_t *)p->next;
        rpn_free(&p->num);
        free(p);
    } else {
        s = (statistic_t *)p->next;
        while (--n) {
            p = s;
            s = (statistic_t *)p->next;
        }
        p->next = s->next;
        rpn_free(&s->num);
        free(s);
    }
}

static char *ReadConversion(const char *formula)
{
    size_t len = strlen(formula);
    char *str = (char *)malloc(len+3);

    if (str == NULL)
        return NULL;

    str[0] = '(';
    memcpy(str+1, formula, len);
    str[len+1] = ')';
    str[len+2] = '\0';

    StringCbCopy(calc.source, sizeof(calc.source), (*calc.buffer == _T('\0')) ? _T("0") : calc.buffer);

    /* clear display content before proceeding */
    calc.ptr = calc.buffer;
    calc.buffer[0] = _T('\0');

    return str;
}

static INT_PTR CALLBACK DlgStatProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TCHAR buffer[SIZEOF(calc.buffer)];
    DWORD n;

    switch (msg) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_LIST_STAT:
            if (HIWORD(wp) == CBN_DBLCLK)
                SendMessage(hWnd, WM_COMMAND, (WPARAM)IDC_BUTTON_LOAD, 0);
            return TRUE;
        case IDC_BUTTON_RET:
            SetFocus(GetDlgItem(GetParent(hWnd), IDC_BUTTON_FOCUS));
            return TRUE;
        case IDC_BUTTON_LOAD:
            n = SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_GETCURSEL, 0, 0);
            if (n == LB_ERR)
                return TRUE;
            PostMessage(GetParent(hWnd), WM_LOAD_STAT, (WPARAM)n, 0);
            return TRUE;
        case IDC_BUTTON_CD:
            n = SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_GETCURSEL, 0, 0);
            if (n == LB_ERR)
                return TRUE;
            SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_DELETESTRING, (WPARAM)n, 0);
            update_n_stats_items(hWnd, buffer, sizeof(buffer));
            delete_stat_item(n);
            return TRUE;
        case IDC_BUTTON_CAD:
            SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_RESETCONTENT, 0, 0);
            clean_stat_list();
            update_n_stats_items(hWnd, buffer, sizeof(buffer));
            return TRUE;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return TRUE;
    case WM_DESTROY:
        clean_stat_list();
        PostMessage(GetParent(hWnd), WM_CLOSE_STATS, 0, 0);
        return TRUE;
    case WM_INSERT_STAT:
        prepare_rpn_result(&(((statistic_t *)lp)->num),
                           buffer, SIZEOF(buffer),
                           ((statistic_t *)lp)->base);
        SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_ADDSTRING, 0, (LPARAM)buffer);
        update_n_stats_items(hWnd, buffer, sizeof(buffer));
        return TRUE;
    }
    return FALSE;
}

static WPARAM idm_2_idc(int idm)
{
    int x;

    for (x=0; x<SIZEOF(upd); x++) {
        if (upd[x].idm == idm)
            break;
    }
    return (WPARAM)(upd[x].idc);
}

static void CopyMemToClipboard(void *ptr)
{
    if(OpenClipboard(NULL)) {
        HGLOBAL  clipbuffer;
        TCHAR   *buffer;
        size_t cbBuffer;

        EmptyClipboard();
        cbBuffer = (_tcslen(ptr) + 1) * sizeof(TCHAR);
        clipbuffer = GlobalAlloc(GMEM_DDESHARE, cbBuffer);
        buffer = (TCHAR *)GlobalLock(clipbuffer);
        StringCbCopy(buffer, cbBuffer, ptr);
        GlobalUnlock(clipbuffer);
#ifdef UNICODE
        SetClipboardData(CF_UNICODETEXT,clipbuffer);
#else
        SetClipboardData(CF_TEXT,clipbuffer);
#endif
        CloseClipboard();
    }
}

static void handle_copy_command(HWND hWnd)
{
    TCHAR display[MAX_CALC_SIZE];
    UINT  n;

    // Read current text from output display
    n = GetDlgItemText(hWnd, IDC_TEXT_OUTPUT, display, SIZEOF(display));

    // Check if result is a true number
    if (!calc.is_nan)
    {
        // Remove trailing decimal point if no decimal digits exist
        if (calc.base == IDC_RADIO_DEC && _tcschr(calc.buffer, _T('.')) == NULL)
            display[n - calc.sDecimal_len] = _T('\0');
    }

    CopyMemToClipboard(display);
}

static char *ReadClipboard(void)
{
    char *buffer = NULL;

    if (OpenClipboard(NULL)) {
        HANDLE  hData = GetClipboardData(CF_TEXT);
        char   *fromClipboard;

        if (hData != NULL) {
            fromClipboard = (char *)GlobalLock(hData);
            if (fromClipboard[0])
                buffer = _strupr(_strdup(fromClipboard));
            GlobalUnlock( hData );
        }
        CloseClipboard();
    }
    return buffer;
}

static char *handle_sequence_input(HWND hwnd, sequence_t *seq)
{
    char *ptr = seq->ptr;
    int ch, x;

    ch = *ptr++;
    if (ch == '\\')
        PostMessage(hwnd, WM_COMMAND, (WPARAM)IDC_BUTTON_DAT, 0);
    else
    if (ch == ':') {
        ch = *ptr;
        if (ch != '\0')
            ptr++;
        switch (ch) {
        case 'C': PostMessage(hwnd, WM_COMMAND, (WPARAM)IDC_BUTTON_MC, 0); break;
        case 'E': PostMessage(hwnd, WM_COMMAND, (WPARAM)IDC_BUTTON_EXP,0); break;
        case 'M': PostMessage(hwnd, WM_COMMAND, (WPARAM)IDC_BUTTON_MS, 0); break;
        case 'P': PostMessage(hwnd, WM_COMMAND, (WPARAM)IDC_BUTTON_MP, 0); break;
        case 'Q': PostMessage(hwnd, WM_COMMAND, (WPARAM)IDC_BUTTON_CANC, 0); break;
        case 'R': PostMessage(hwnd, WM_COMMAND, (WPARAM)IDC_BUTTON_MR, 0); break;
        }
    } else
    if (ch == '$') {
        StringCbCopyEx(calc.buffer, sizeof(calc.buffer), calc.source, &calc.ptr, NULL,
                       STRSAFE_FILL_ON_FAILURE);
    } else {
        for (x=0; x<SIZEOF(key2code); x++) {
            if (!(key2code[x].mask & BITMASK_IS_ASCII) ||
                (key2code[x].mask & BITMASK_IS_CTRL))
                continue;
            if (key2code[x].key == ch) {
                PostMessage(hwnd, WM_COMMAND, (WPARAM)key2code[x].idc, 0);
                break;
            }
        }
    }

    if (*ptr != '\0')
    {
        seq->ptr = ptr;
        PostMessage(hwnd, seq->wm_msg, 0, 0);
    } else {
        free(seq->data);
        seq->data = seq->ptr = ptr = NULL;
    }
    return ptr;
}

static void run_dat_sta(calc_number_t *a)
{
    statistic_t *s = (statistic_t *)malloc(sizeof(statistic_t));
    statistic_t *p = calc.stat;

    rpn_alloc(&s->num);
    rpn_copy(&s->num, a);
    s->base = calc.base;
    s->next = NULL;
    if (p == NULL)
        calc.stat = s;
    else {
        while (p->next != NULL)
            p = (statistic_t *)(p->next);
        p->next = s;
    }
    PostMessage(calc.hStatWnd, WM_INSERT_STAT, 0, (LPARAM)s);
}

static void run_mp(calc_number_t *c)
{
    calc_node_t cn;

    cn.number = *c;
    cn.base = calc.base;
    run_operator(&calc.memory, &calc.memory, &cn, RPN_OPERATOR_ADD);
    update_memory_flag(calc.hWnd, TRUE);
}

static void run_mm(calc_number_t *c)
{
    calc_node_t cn;

    cn.number = *c;
    cn.base = calc.base;
    run_operator(&calc.memory, &calc.memory, &cn, RPN_OPERATOR_SUB);
    update_memory_flag(calc.hWnd, TRUE);
}

static void run_ms(calc_number_t *c)
{
    rpn_copy(&calc.memory.number, c);
    calc.memory.base = calc.base;
    update_memory_flag(calc.hWnd, rpn_is_zero(&calc.memory.number) ? FALSE : TRUE);
}

static void run_mw(calc_number_t *c)
{
    calc_number_t tmp;

    rpn_copy(&tmp, &calc.memory.number);
    rpn_copy(&calc.memory.number, c);
    calc.memory.base = calc.base;
    if (calc.is_memory)
        rpn_copy(c, &tmp);
    update_memory_flag(calc.hWnd, rpn_is_zero(&calc.memory.number) ? FALSE : TRUE);
}

static statistic_t *upload_stat_number(int n)
{
    statistic_t *p = calc.stat;

    if (p == NULL)
        return p;

    while (n--) {
        p = (statistic_t *)(p->next);
        if (p == NULL)
            return p;
    }

#ifndef ENABLE_MULTI_PRECISION
    if (calc.base != p->base) {
        if (calc.base == IDC_RADIO_DEC)
            calc.code.f = (double)p->num.i;
        else {
            calc.code.i = (__int64)p->num.f;
            apply_int_mask(&calc.code);
        }
    } else
#endif
        rpn_copy(&calc.code, &p->num);

    calc.is_nan = FALSE;

    return p;
}

static void run_fe(calc_number_t *number)
{
    calc.sci_out = ((calc.sci_out != FALSE) ? FALSE : TRUE);
}

static void handle_context_menu(HWND hWnd, WPARAM wp, LPARAM lp)
{
    TCHAR text[64];
    HMENU hMenu = CreatePopupMenu();
    BOOL idm;

    LoadString(calc.hInstance, IDS_QUICKHELP, text, SIZEOF(text));
    AppendMenu(hMenu, MF_STRING | MF_ENABLED, IDM_HELP_HELP, text);
    idm = TrackPopupMenu( hMenu,
                          TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
                          LOWORD(lp),
                          HIWORD(lp),
                          0,
                          hWnd,
                          NULL);
    DestroyMenu(hMenu);
#ifndef DISABLE_HTMLHELP_SUPPORT
    if (idm) {
        HH_POPUP popup;

        memset(&popup, 0, sizeof(popup));
        popup.cbStruct = sizeof(HH_POPUP);
        popup.clrForeground = 1;
        popup.clrBackground = -1;
        popup.pt.x = LOWORD(lp);
        popup.pt.y = HIWORD(lp);
        popup.rcMargins.top    = -1;
        popup.rcMargins.bottom = -1;
        popup.rcMargins.left   = -1;
        popup.rcMargins.right  = -1;
        popup.idString = GetWindowLongPtr((HWND)wp, GWL_ID);
        calc_HtmlHelp((HWND)wp, HTMLHELP_PATH("/popups.txt"), HH_DISPLAY_TEXT_POPUP, (DWORD_PTR)&popup);
    }
#else
    (void)idm;
#endif
}

static void run_canc(calc_number_t *c)
{
    flush_postfix();
    rpn_zero(c);

    /* clear also scientific display modes */
    calc.sci_out = FALSE;
    calc.sci_in  = FALSE;

    /* clear state of inv and hyp flags */
    CheckDlgButton(calc.hWnd, IDC_CHECK_INV, BST_UNCHECKED);
    CheckDlgButton(calc.hWnd, IDC_CHECK_HYP, BST_UNCHECKED);
}

static void run_rpar(calc_number_t *c)
{
    exec_closeparent(c);
}

static void run_lpar(calc_number_t *c)
{
    exec_infix2postfix(c, RPN_OPERATOR_PARENT);
}

static LRESULT CALLBACK SubclassButtonProc(HWND hWnd, WPARAM wp, LPARAM lp)
{
    LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lp;
    UINT             dwText;
    TCHAR            text[64];
    int              dx, dy, len;
    SIZE             size;
    POINT            pt;

    if(dis->CtlType == ODT_BUTTON)
    {
        HTHEME hTheme = NULL;
        LPBTNINFO lpBtnInfo;

        if (calc_IsAppThemed() && calc_IsThemeActive())
            hTheme = calc_OpenThemeData(hWnd, L"Button");

        if (hTheme)
        {
            int iState = 0;

            if ((dis->itemState & ODS_DISABLED))
                iState |= PBS_DISABLED;
            if ((dis->itemState & ODS_SELECTED))
                iState |= PBS_PRESSED;

            lpBtnInfo = (LPBTNINFO)GetWindowLongPtr(dis->hwndItem, GWLP_USERDATA);
            if (lpBtnInfo != NULL)
            {
                if (lpBtnInfo->bHover)
                    iState |= PBS_HOT;
            }

            if (calc_IsThemeBackgroundPartiallyTransparent(hTheme, BP_PUSHBUTTON, iState))
            {
                calc_DrawThemeParentBackground(dis->hwndItem, dis->hDC, &dis->rcItem);
            }

            // Draw the frame around the control
            calc_DrawThemeBackground(hTheme, dis->hDC, BP_PUSHBUTTON, iState, &dis->rcItem, NULL);

            calc_CloseThemeData(hTheme);
        } else {
            /* default state: unpushed */
            DWORD dwStyle = 0;

            if ((dis->itemState & ODS_SELECTED))
                dwStyle = DFCS_PUSHED;

            DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | dwStyle);
        }

        /* button text to write */
        len = GetWindowText(dis->hwndItem, text, SIZEOF(text));

        /*
         * little exception: 1/x has different color
         * in standard and scientific modes
         */
        if ((calc.layout == CALC_LAYOUT_STANDARD ||
             calc.layout == CALC_LAYOUT_CONVERSION) &&
            IDC_BUTTON_RX == dis->CtlID) {
            SetTextColor(dis->hDC, CALC_CLR_BLUE);
        } else
        for (dx=0; dx<SIZEOF(key2code); dx++) {
            if (key2code[dx].idc == dis->CtlID) {
                SetTextColor(dis->hDC, key2code[dx].col);
                break;
            }
        }

        /* No background, to avoid corruption of the texture */
        SetBkMode(dis->hDC, TRANSPARENT);

        /* Default state: enabled */
        dwText = 0;
        if ((dis->itemState & ODS_DISABLED))
            dwText = DSS_DISABLED;

        /* Draw the text in the button */
        GetTextExtentPoint32(dis->hDC, text, len, &size);
        dx = ((dis->rcItem.right-dis->rcItem.left) - size.cx) >> 1;
        dy = ((dis->rcItem.bottom-dis->rcItem.top) - size.cy) >> 1;
        if ((dis->itemState & ODS_SELECTED)) {
            dx++;
            dy++;
        }
        pt.x = dis->rcItem.left + dx;
        pt.y = dis->rcItem.top + dy;
        DrawState(dis->hDC, NULL, NULL, (LPARAM)text, 0, pt.x, pt.y, size.cx, size.cy, DST_TEXT | dwText);
    }
    return 1L;
}

static INT_PTR CALLBACK HotButtonProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LPBTNINFO lpBtnInfo = (LPBTNINFO)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    TRACKMOUSEEVENT mouse_event;

    switch (msg) {
    case WM_MOUSEMOVE:
        mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
        mouse_event.dwFlags = TME_QUERY;
        if (!TrackMouseEvent(&mouse_event) || !(mouse_event.dwFlags & (TME_HOVER|TME_LEAVE)))
        {
            mouse_event.dwFlags = TME_HOVER|TME_LEAVE;
            mouse_event.hwndTrack = hWnd;
            mouse_event.dwHoverTime = 1;
            TrackMouseEvent(&mouse_event);
        }
        break;

    case WM_MOUSEHOVER:
        lpBtnInfo->bHover = TRUE;
        InvalidateRect(hWnd, NULL, FALSE);
        break;

    case WM_MOUSELEAVE:
        lpBtnInfo->bHover = FALSE;
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }

    return CallWindowProc(lpBtnInfo->oldProc, hWnd, msg, wp, lp);
}

static BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam)
{
    TCHAR szClass[64];

    if (!GetClassName(hWnd, szClass, SIZEOF(szClass)))
        return TRUE;

    if (!_tcscmp(szClass, WC_BUTTON))
    {
        int *pnCtrls = (int *)lParam;
        int nCtrls = *pnCtrls;

        BtnInfo[nCtrls].oldProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
        BtnInfo[nCtrls].bHover  = FALSE;

        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)&BtnInfo[nCtrls]);
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)HotButtonProc);

        *pnCtrls = ++nCtrls;
    }
    return TRUE;
}

static INT_PTR CALLBACK OnSettingChange(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    /* Check for user policy and area string valid */
    if (wParam == 0 && lParam != 0)
    {
        LPTSTR lpArea = (LPTSTR)lParam;

        /* Check if a parameter has been changed into the locale settings */
        if (!_tcsicmp(lpArea, _T("intl")))
        {
            /* Re-load locale parameters */
            UpdateNumberIntl();

            /* Update text for decimal button */
            SetDlgItemText(hWnd, IDC_BUTTON_DOT, calc.sDecimal);

            /* Update text into the output display */
            update_lcd_display(hWnd);
        }
    }
    return 0;
}

static INT_PTR CALLBACK DlgMainProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    unsigned int x;
    RECT         rc;

    switch (msg) {
    case WM_DRAWITEM:
        return SubclassButtonProc(hWnd, wp, lp);

    case WM_INITDIALOG:
#ifdef DISABLE_HTMLHELP_SUPPORT
        EnableMenuItem(GetMenu(hWnd), IDM_HELP_HELP, MF_BYCOMMAND | MF_GRAYED);
#endif
        calc.hWnd=hWnd;
        /* Enumerate children and apply hover function */
        BtnCount = 0;
        EnumChildWindows(hWnd, EnumChildProc, (LPARAM)&BtnCount);

#ifdef USE_KEYBOARD_HOOK
        calc.hKeyboardHook=SetWindowsHookEx(
                                       WH_KEYBOARD,
                                       KeyboardHookProc,
                                       NULL,
                                       GetCurrentThreadId()
                                      );
#endif
        rpn_zero(&calc.code);
        calc.sci_out = FALSE;
        calc.base = IDC_RADIO_DEC;
        calc.size = IDC_RADIO_QWORD;
        calc.degr = IDC_RADIO_DEG;
        calc.ptr  = calc.buffer;
        calc.is_nan = FALSE;
        enable_allowed_controls(hWnd, IDC_RADIO_DEC);
        update_radio(hWnd, IDC_RADIO_DEC);
        update_menu(hWnd);
        display_rpn_result(hWnd, &calc.code);
        update_memory_flag(hWnd, calc.is_memory);
        /* remove keyboard focus */
        SetFocus(GetDlgItem(hWnd, IDC_BUTTON_FOCUS));
        /* set our calc icon */
        SendMessage(hWnd, WM_SETICON, ICON_BIG,   (LPARAM)calc.hBgIcon);
        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)calc.hSmIcon);
        /* update text for decimal button */
        SetDlgItemText(hWnd, IDC_BUTTON_DOT, calc.sDecimal);
        /* Fill combo box for conversion */
        if (calc.layout == CALC_LAYOUT_CONVERSION)
            ConvInit(hWnd);
        /* Restore the window at the same position it was */
        if (calc.x_coord >= 0 && calc.y_coord >= 0) {
            int w, h, sw, sh;

            GetWindowRect(hWnd, &rc);
            w = rc.right-rc.left;
            h = rc.bottom-rc.top;
            sw = GetSystemMetrics(SM_CXSCREEN);
            sh = GetSystemMetrics(SM_CYSCREEN);
            if (calc.x_coord+w > sw) calc.x_coord = sw - w;
            if (calc.y_coord+h > sh) calc.y_coord = sh - h;
            MoveWindow(hWnd, calc.x_coord, calc.y_coord, w, h, FALSE);
        }
        break;
    case WM_CTLCOLORSTATIC:
        if ((HWND)lp == GetDlgItem(hWnd, IDC_TEXT_OUTPUT))
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        break;
    case WM_HANDLE_CLIPBOARD:
        handle_sequence_input(hWnd, &calc.Clipboard);
        return TRUE;
    case WM_COMMAND:
        /*
         * if selection of category is changed, we must
         * update the content of the "from/to" combo boxes.
         */
        if (wp == MAKEWPARAM(IDC_COMBO_CATEGORY, CBN_SELCHANGE)) {
            ConvAdjust(hWnd, SendDlgItemMessage(hWnd, IDC_COMBO_CATEGORY, CB_GETCURSEL, 0, 0));
            return TRUE;
        }
        if (HIWORD(wp) != BN_CLICKED && HIWORD(wp) != BN_DBLCLK)
            break;
        /* avoid flicker if the user selects from keyboard */
        if (GetFocus() != GetDlgItem(hWnd, IDC_BUTTON_FOCUS))
            SetFocus(GetDlgItem(hWnd, IDC_BUTTON_FOCUS));
        switch (LOWORD(wp)) {
        case IDM_HELP_ABOUT:
        {
            TCHAR infotitle[100];
            TCHAR infotext[200];
            LoadString(calc.hInstance, IDS_CALC_NAME, infotitle, SIZEOF(infotitle));
            LoadString(calc.hInstance, IDS_AUTHOR, infotext, SIZEOF(infotext));
            ShellAbout(hWnd, infotitle, infotext, calc.hBgIcon);
            return TRUE;
        }
        case IDM_HELP_HELP:
#ifndef DISABLE_HTMLHELP_SUPPORT
            calc_HtmlHelp(hWnd, HTMLHELP_PATH("/general_information.htm"), HH_DISPLAY_TOPIC, (DWORD_PTR)NULL);
#endif
            return TRUE;
        case IDM_VIEW_STANDARD:
            calc.layout = CALC_LAYOUT_STANDARD;
            calc.action = IDM_VIEW_STANDARD;
            DestroyWindow(hWnd);
            return TRUE;
        case IDM_VIEW_SCIENTIFIC:
            calc.layout = CALC_LAYOUT_SCIENTIFIC;
            calc.action = IDM_VIEW_SCIENTIFIC;
            DestroyWindow(hWnd);
            return TRUE;
        case IDM_VIEW_CONVERSION:
            calc.layout = CALC_LAYOUT_CONVERSION;
            calc.action = IDM_VIEW_CONVERSION;
            DestroyWindow(hWnd);
            return TRUE;
        case IDM_VIEW_HEX:
        case IDM_VIEW_DEC:
        case IDM_VIEW_OCT:
        case IDM_VIEW_BIN:
        case IDM_VIEW_DEG:
        case IDM_VIEW_RAD:
        case IDM_VIEW_GRAD:
        case IDM_VIEW_QWORD:
        case IDM_VIEW_DWORD:
        case IDM_VIEW_WORD:
        case IDM_VIEW_BYTE:
            SendMessage(hWnd, WM_COMMAND, idm_2_idc(LOWORD(wp)), 0);
            return TRUE;
        case IDM_EDIT_COPY:
            handle_copy_command(hWnd);
            return TRUE;
        case IDM_EDIT_PASTE:
            if (calc.Clipboard.data != NULL)
                break;
            calc.Clipboard.data = ReadClipboard();
            if (calc.Clipboard.data != NULL) {
                /* clear the content of the display before pasting */
                PostMessage(hWnd, WM_COMMAND, IDC_BUTTON_CE, 0);
                calc.Clipboard.ptr = calc.Clipboard.data;
                calc.Clipboard.wm_msg = WM_HANDLE_CLIPBOARD;
                handle_sequence_input(hWnd, &calc.Clipboard);
            }
            return TRUE;
        case IDM_VIEW_GROUP:
            calc.usesep = (calc.usesep ? FALSE : TRUE);
            update_menu(hWnd);
            update_lcd_display(hWnd);
            return TRUE;
        case IDC_BUTTON_CONVERT:
            ConvExecute(hWnd);
            return TRUE;
        case IDC_BUTTON_CE: {
            calc_number_t tmp;
            rpn_zero(&tmp);
            display_rpn_result(hWnd, &tmp);
            }
            return TRUE;
        case IDC_RADIO_DEC:
        case IDC_RADIO_HEX:
        case IDC_RADIO_OCT:
        case IDC_RADIO_BIN:
/* GNU WINDRES is bugged so I must always force radio update */
/* (Fix for Win95/98) */
#ifdef _MSC_VER
            if (calc.base == LOWORD(wp))
                break;
#endif
            calc.is_nan = FALSE;
            update_radio(hWnd, LOWORD(wp));
            return TRUE;
        case IDC_RADIO_DEG:
        case IDC_RADIO_RAD:
        case IDC_RADIO_GRAD:
/* GNU WINDRES is bugged so I must always force radio update */
/* (Fix for Win95/98) */
#ifdef _MSC_VER
            if (calc.degr == LOWORD(wp))
                break;
#endif
            calc.degr = LOWORD(wp);
            calc.is_nan = FALSE;
            update_menu(hWnd);
            return TRUE;
        case IDC_RADIO_QWORD:
        case IDC_RADIO_DWORD:
        case IDC_RADIO_WORD:
        case IDC_RADIO_BYTE:
/* GNU WINDRES is bugged so I must always force radio update */
/* (Fix for Win95/98) */
#ifdef _MSC_VER
            if (calc.size == LOWORD(wp))
                break;
#endif
            calc.size = LOWORD(wp);
            calc.is_nan = FALSE;
            update_menu(hWnd);
            /*
             * update the content of the display
             */
            convert_text2number(&calc.code);
            apply_int_mask(&calc.code);
            display_rpn_result(hWnd, &calc.code);
            return TRUE;
        case IDC_BUTTON_1:
        case IDC_BUTTON_2:
        case IDC_BUTTON_3:
        case IDC_BUTTON_4:
        case IDC_BUTTON_5:
        case IDC_BUTTON_6:
        case IDC_BUTTON_7:
        case IDC_BUTTON_8:
        case IDC_BUTTON_9:
        case IDC_BUTTON_0:
        case IDC_BUTTON_DOT:
        case IDC_BUTTON_A:
        case IDC_BUTTON_B:
        case IDC_BUTTON_C:
        case IDC_BUTTON_D:
        case IDC_BUTTON_E:
        case IDC_BUTTON_F:
            if (calc.is_nan) break;
            build_operand(hWnd, LOWORD(wp));
            return TRUE;
        case IDC_BUTTON_PERCENT:
        case IDC_BUTTON_ADD:
        case IDC_BUTTON_SUB:
        case IDC_BUTTON_MULT:
        case IDC_BUTTON_DIV:
        case IDC_BUTTON_MOD:
        case IDC_BUTTON_AND:
        case IDC_BUTTON_OR:
        case IDC_BUTTON_XOR:
        case IDC_BUTTON_LSH:
        case IDC_BUTTON_RSH:
        case IDC_BUTTON_EQU:
        case IDC_BUTTON_XeY:
        case IDC_BUTTON_XrY:
            if (calc.is_nan) break;
            /*
             * LSH and XeY buttons hold also the RSH and XrY functions with INV modifier,
             * but since they are two operand operators, they must be handled here.
             */
            if ((get_modifiers(hWnd) & MODIFIER_INV))
            {
                WPARAM IdcSim = IDC_STATIC;

                switch (LOWORD(wp)) {
                case IDC_BUTTON_LSH: IdcSim = MAKEWPARAM(IDC_BUTTON_RSH, BN_CLICKED); break;
                case IDC_BUTTON_XeY: IdcSim = MAKEWPARAM(IDC_BUTTON_XrY, BN_CLICKED); break;
                }

                if (IdcSim != IDC_STATIC)
                {
                    PostMessage(hWnd, WM_COMMAND, IdcSim, 0);
                    CheckDlgButton(hWnd, IDC_CHECK_INV, BST_UNCHECKED);
                    break;
                }
            }

            for (x=0; x<SIZEOF(operator_codes); x++) {
                if (LOWORD(wp) == operator_codes[x]) {
                    convert_text2number(&calc.code);

                    if (calc.ptr == calc.buffer) {
                        if (calc.last_operator != x) {
                            if (x != RPN_OPERATOR_EQUAL)
                                exec_change_infix();
                        } else
                        if (x == RPN_OPERATOR_EQUAL) {
                            exec_infix2postfix(&calc.code, calc.prev_operator);
                            rpn_copy(&calc.code, &calc.prev);
                        } else
                            break;
                    }

                    /* if no change then quit silently, */
                    /* without display updates */
                    if (!exec_infix2postfix(&calc.code, x))
                        break;

                    display_rpn_result(hWnd, &calc.code);
                    break;
                }
            }
            return TRUE;
        case IDC_BUTTON_BACK:
            if (calc.is_nan) break;
            if (calc.sci_in) {
                if (calc.esp == 0) {
                    TCHAR *ptr;

                    calc.sci_in = FALSE;
                    ptr = _tcschr(calc.ptr, _T('e'));
                    if (ptr)
                        *ptr = _T('\0');
                    update_lcd_display(hWnd);
                } else {
                    calc.esp /= 10;
                    build_operand(hWnd, IDC_STATIC);
                }
            } else
            if (calc.ptr != calc.buffer) {
                *--calc.ptr = _T('\0');
                if (!_tcscmp(calc.buffer, _T("-")) ||
                    !_tcscmp(calc.buffer, _T("-0")) ||
                    !_tcscmp(calc.buffer, _T("0"))) {
                    calc.ptr = calc.buffer;
                    calc.buffer[0] = _T('\0');
                }
                update_lcd_display(hWnd);
            }
            return TRUE;
        case IDC_BUTTON_MC:
            rpn_zero(&calc.memory.number);
            update_memory_flag(hWnd, FALSE);
            return TRUE;
        case IDC_BUTTON_MR:
            if (calc.is_memory) {
                calc.is_nan = FALSE;
                rpn_copy(&calc.code, &calc.memory.number);
                display_rpn_result(hWnd, &calc.code);
            }
            return TRUE;
        case IDC_BUTTON_EXP:
            if (calc.sci_in || calc.is_nan || calc.buffer == calc.ptr)
                break;
            calc.sci_in = TRUE;
            calc.esp = 0;
            build_operand(hWnd, IDC_STATIC);
            return TRUE;
        case IDC_BUTTON_SIGN:
            if (calc.sci_in) {
                calc.esp = 0-calc.esp;
                build_operand(hWnd, IDC_STATIC);
            } else {
                if (calc.is_nan || calc.buffer[0] == _T('\0'))
                    break;

                if (calc.buffer[0] == _T('-')) {
                    /* make the number positive */
                    memmove(calc.buffer, calc.buffer+1, sizeof(calc.buffer)-1);
                    if (calc.buffer != calc.ptr)
                        calc.ptr--;
                } else {
                    /* if first char is '0' and no dot, it isn't valid */
                    if (calc.buffer[0] == _T('0') &&
                        calc.buffer[1] != _T('.'))
                        break;
                    /* make the number negative */
                    memmove(calc.buffer+1, calc.buffer, sizeof(calc.buffer)-1);
                    calc.buffer[0] = _T('-');
                    if (calc.buffer != calc.ptr)
                        calc.ptr++;
                }
                /* If the input buffer is empty, then
                   we change also the sign of calc.code
                   because it could be the result of a
                   previous calculation. */
                if (calc.buffer == calc.ptr)
                    rpn_sign(&calc.code);
                update_lcd_display(hWnd);
            }
            return TRUE;
        case IDC_BUTTON_RIGHTPAR:
        case IDC_BUTTON_LEFTPAR:
        case IDC_BUTTON_CANC:
        case IDC_BUTTON_MP:
        case IDC_BUTTON_DAT:
        case IDC_BUTTON_FE:
        case IDC_BUTTON_DMS:
        case IDC_BUTTON_SQRT:
        case IDC_BUTTON_S:
        case IDC_BUTTON_SUM:
        case IDC_BUTTON_AVE:
        case IDC_BUTTON_NF:
        case IDC_BUTTON_LN:
        case IDC_BUTTON_LOG:
        case IDC_BUTTON_Xe2:
        case IDC_BUTTON_Xe3:
        case IDC_BUTTON_PI:
        case IDC_BUTTON_NOT:
        case IDC_BUTTON_RX:
        case IDC_BUTTON_INT:
        case IDC_BUTTON_SIN:
        case IDC_BUTTON_COS:
        case IDC_BUTTON_TAN:
        case IDC_BUTTON_MS:
            for (x=0; x<SIZEOF(function_table); x++) {
                if (LOWORD(wp) == function_table[x].idc) {
                    rpn_callback1 cb = NULL;

                    /* test if NaN state is important or not */
                    if (calc.is_nan && function_table[x].check_nan) break;
                    /* otherwise, it's cleared */
                    calc.is_nan = FALSE;

                    switch (get_modifiers(hWnd) & function_table[x].range) {
                    case 0:
                        cb = function_table[x].direct;
                        break;
                    case MODIFIER_INV:
                        cb = function_table[x].inverse;
                        break;
                    case MODIFIER_HYP:
                        cb = function_table[x].hyperb;
                        break;
                    case MODIFIER_INV|MODIFIER_HYP:
                        cb = function_table[x].inv_hyp;
                        break;
                    }
                    if (cb != NULL) {
                        convert_text2number(&calc.code);
                        cb(&calc.code);
//                        display_rpn_result(hWnd, &calc.code);
                        set_rpn_result(hWnd, &calc.code);

                        if ((function_table[x].range & NO_CHAIN))
                            calc.ptr = calc.buffer;

//                        if (!(function_table[x].range & NO_CHAIN))
//                            exec_infix2postfix(&calc.code, RPN_OPERATOR_NONE);
                        if (function_table[x].range & MODIFIER_INV)
                            CheckDlgButton(hWnd, IDC_CHECK_INV, BST_UNCHECKED);
                        if (function_table[x].range & MODIFIER_HYP)
                            CheckDlgButton(hWnd, IDC_CHECK_HYP, BST_UNCHECKED);
                    }
                    break;
                }
            }
            return TRUE;
        case IDC_BUTTON_STA:
            if (IsWindow(calc.hStatWnd))
                break;
            calc.hStatWnd = CreateDialog(calc.hInstance,
                                    MAKEINTRESOURCE(IDD_DIALOG_STAT), hWnd, DlgStatProc);
            if (calc.hStatWnd != NULL) {
                enable_allowed_controls(hWnd, calc.base);
                SendMessage(calc.hStatWnd, WM_SETFOCUS, 0, 0);
            }
            return TRUE;
        }
        break;
    case WM_CLOSE_STATS:
        calc.hStatWnd = NULL;
        enable_allowed_controls(hWnd, calc.base);
        return TRUE;
    case WM_LOAD_STAT:
        if (upload_stat_number((int)LOWORD(wp)) != NULL)
            display_rpn_result(hWnd, &calc.code);
        return TRUE;
    case WM_START_CONV:
        x = LOWORD(lp);
        calc.Convert[x].data = ReadConversion(calc.Convert[x].data);
        if (calc.Convert[x].data != NULL) {
            calc.Convert[x].ptr = calc.Convert[x].data;
            PostMessage(hWnd, HIWORD(lp), 0, 0);
        }
        return TRUE;
    case WM_HANDLE_FROM:
        if (calc.is_nan)
            break;
        if (handle_sequence_input(hWnd, &calc.Convert[0]) == NULL)
            PostMessage(hWnd, WM_START_CONV, 0,
                        MAKELPARAM(0x0001, WM_HANDLE_TO));
        return TRUE;
    case WM_HANDLE_TO:
        if (!calc.is_nan)
            handle_sequence_input(hWnd, &calc.Convert[1]);
        return TRUE;
    case WM_CLOSE:
        calc.action = IDC_STATIC;
        DestroyWindow(hWnd);
        return TRUE;

    case WM_DESTROY:
        /* Get (x,y) position of the calculator */
        GetWindowRect(hWnd, &rc);
        calc.x_coord = rc.left;
        calc.y_coord = rc.top;
#ifdef USE_KEYBOARD_HOOK
        UnhookWindowsHookEx(calc.hKeyboardHook);
#endif
        PostQuitMessage(0);
        return TRUE;
    case WM_CONTEXTMENU:
        if ((HWND)wp != hWnd)
            handle_context_menu(hWnd, wp, lp);
        return TRUE;
    case WM_ENTERMENULOOP:
        calc.is_menu_on = TRUE;
        /* Check if a valid format is available in the clipboard */
        EnableMenuItem(GetSubMenu(GetMenu(hWnd), 0),
                       IDM_EDIT_PASTE,
                       MF_BYCOMMAND|
                       (IsClipboardFormatAvailable(CF_TEXT) ?
                       MF_ENABLED : MF_GRAYED));
        break;
    case WM_EXITMENULOOP:
        calc.is_menu_on = FALSE;
        break;

    case WM_SETTINGCHANGE:
        return OnSettingChange(hWnd, wp, lp);

    case WM_THEMECHANGED:
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    return FALSE;
}

#if defined(__GNUC__) && !defined(__REACTOS__)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
#endif
{
    MSG msg;
    DWORD dwLayout;

    /* Initialize controls for theming & manifest support */
    InitCommonControls();

    calc.hInstance = hInstance;

    calc.x_coord = -1;
    calc.y_coord = -1;

    load_config();
    start_rpn_engine();

    HtmlHelp_Start(hInstance);

    Theme_Start(hInstance);

    calc.hBgIcon = LoadImage(
                    hInstance,
                    MAKEINTRESOURCE(IDI_CALC),
                    IMAGE_ICON,
                    0,
                    0,
                    LR_DEFAULTSIZE | LR_SHARED);

    calc.hSmIcon = LoadImage(
                    hInstance,
                    MAKEINTRESOURCE(IDI_CALC),
                    IMAGE_ICON,
                    GetSystemMetrics(SM_CXSMICON),
                    GetSystemMetrics(SM_CYSMICON),
                    LR_SHARED);

    do {
        /* ignore hwnd: dialogs are already visible! */
        if (calc.layout == CALC_LAYOUT_SCIENTIFIC)
            dwLayout = IDD_DIALOG_SCIENTIFIC;
        else
        if (calc.layout == CALC_LAYOUT_CONVERSION)
            dwLayout = IDD_DIALOG_CONVERSION;
        else
            dwLayout = IDD_DIALOG_STANDARD;

        /* This call will always fail if UNICODE for Win9x */
        if (NULL == CreateDialog(hInstance, MAKEINTRESOURCE(dwLayout), NULL, DlgMainProc))
            break;

        while (GetMessage(&msg, NULL, 0, 0)) {
#ifndef USE_KEYBOARD_HOOK
            if ((msg.message == WM_KEYUP ||
                msg.message == WM_KEYDOWN) &&
                !calc.is_menu_on)
                process_vk_key(msg.wParam, msg.lParam);
#endif
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        save_config();
    } while (calc.action != IDC_STATIC);

    stop_rpn_engine();

    Theme_Stop();
    HtmlHelp_Stop();

    return 0;
}
