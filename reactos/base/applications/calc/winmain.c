#include "calc.h"

#define HTMLHELP_PATH(_pt)  TEXT("%systemroot%\\Help\\calc.chm::") TEXT(_pt)

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

#define CALC_CLR_RED        0x000000FF
#define CALC_CLR_BLUE       0x00FF0000
#define CALC_CLR_PURP       0x00FF00FF

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

static void run_pow(calc_number_t *number);
static void run_sqr(calc_number_t *number);
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
    { IDC_BUTTON_AVE,  0,                         0, rpn_ave,     NULL,        NULL,     NULL      },
    { IDC_BUTTON_SUM,  0,                         0, rpn_sum,     NULL,        NULL,     NULL      },
    { IDC_BUTTON_S,    MODIFIER_INV,              0, rpn_s_m1,    rpn_s,       NULL,     NULL      },
    { IDC_BUTTON_XeY,  MODIFIER_INV,              1, run_pow,     run_sqr,     NULL,     NULL      },
    { IDC_BUTTON_SQRT, MODIFIER_INV,              1, rpn_sqrt,    NULL,        NULL,     NULL      },
    { IDC_BUTTON_DMS,  MODIFIER_INV,              1, rpn_dec2dms, rpn_dms2dec, NULL,     NULL      },
    { IDC_BUTTON_FE,   0,                         1, run_fe,      NULL,        NULL,     NULL      },
    { IDC_BUTTON_DAT,  0,                         1, run_dat_sta, NULL,        NULL,     NULL,     },
    { IDC_BUTTON_MP,   MODIFIER_INV,              1, run_mp,      run_mm,      NULL,     NULL,     },
    { IDC_BUTTON_MS,   MODIFIER_INV,              1, run_ms,      run_mw,      NULL,     NULL,     },
    { IDC_BUTTON_CANC, NO_CHAIN,                  0, run_canc,    NULL,        NULL,     NULL,     },
    { IDC_BUTTON_RIGHTPAR, NO_CHAIN,              1, run_rpar,    NULL,        NULL,     NULL,     },
    { IDC_BUTTON_LEFTPAR,  NO_CHAIN,              0, run_lpar,    NULL,        NULL,     NULL,     },
};

/*
*/

calc_t calc;

static void load_config(void)
{
    TCHAR buf[32];
    DWORD tmp;
#if _WIN32_WINNT >= 0x0500
    HKEY hKey;
#endif

    /* Try to load last selected layout */
    GetProfileString(TEXT("SciCalc"), TEXT("layout"), TEXT("0"), buf, SIZEOF(buf));
    if (_stscanf(buf, TEXT("%ld"), &calc.layout) != 1)
        calc.layout = CALC_LAYOUT_STANDARD;

    /* Try to load last selected formatting option */
    GetProfileString(TEXT("SciCalc"), TEXT("UseSep"), TEXT("0"), buf, SIZEOF(buf));
    if (_stscanf(buf, TEXT("%ld"), &tmp) != 1)
        calc.usesep = FALSE;
    else
        calc.usesep = (tmp == 1) ? TRUE : FALSE;

    /* memory is empty at startup */
    calc.is_memory = FALSE;

#if _WIN32_WINNT >= 0x0500
    /* empty these values */
    calc.sDecimal[0] = TEXT('\0');
    calc.sThousand[0] = TEXT('\0');

    /* try to open the registry */
    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                     TEXT("Control Panel\\International"),
                     0,
                     KEY_QUERY_VALUE,
                     &hKey) == ERROR_SUCCESS) {
        /* get these values (ignore errors) */
        tmp = sizeof(calc.sDecimal);
        RegQueryValueEx(hKey, TEXT("sDecimal"), NULL, NULL, (LPBYTE)calc.sDecimal, &tmp);

        tmp = sizeof(calc.sThousand);
        RegQueryValueEx(hKey, TEXT("sThousand"), NULL, NULL, (LPBYTE)calc.sThousand, &tmp);

        /* close the key */
        RegCloseKey(hKey);
    }
    /* if something goes wrong, let's apply the defaults */
    if (calc.sDecimal[0] == TEXT('\0'))
        _tcscpy(calc.sDecimal, TEXT("."));

    if (calc.sThousand[0] == TEXT('\0'))
        _tcscpy(calc.sThousand, TEXT(","));

    /* get the string lengths */
    calc.sDecimal_len = _tcslen(calc.sDecimal);
    calc.sThousand_len = _tcslen(calc.sThousand);
#else
    /* acquire regional settings */
    calc.sDecimal_len  = GetProfileString(TEXT("intl"), TEXT("sDecimal"), TEXT("."), calc.sDecimal, SIZEOF(calc.sDecimal));
    calc.sThousand_len = GetProfileString(TEXT("intl"), TEXT("sThousand"), TEXT(","), calc.sThousand, SIZEOF(calc.sThousand));
#endif
}

static void save_config(void)
{
    TCHAR buf[32];

    _stprintf(buf, TEXT("%lu"), calc.layout);
    WriteProfileString(TEXT("SciCalc"), TEXT("layout"), buf);
    WriteProfileString(TEXT("SciCalc"), TEXT("UseSep"), (calc.usesep==TRUE) ? TEXT("1") : TEXT("0"));
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

    if (!_tcscmp(ClassName, TEXT("Button"))) {
        DWORD dwStyle = GetWindowLong(hCtlWnd, GWL_STYLE) & 0xF;

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
     * muliply size of calc.buffer by 2 because it may
     * happen that separator is used between each digit.
     * Also added little additional space for dot and '\0'.
     */
    TCHAR *tmp = (TCHAR *)alloca(sizeof(calc.buffer)*2+2*sizeof(TCHAR));

    if (calc.buffer[0] == TEXT('\0'))
        _tcscpy(tmp, TEXT("0"));
    else
        _tcscpy(tmp, calc.buffer);
    /* add final '.' in decimal mode (if it's missing) */
    if (calc.base == IDC_RADIO_DEC) {
        if (_tcschr(tmp, TEXT('.')) == NULL)
            _tcscat(tmp, TEXT("."));
    }
    /* if separator mode is on, let's add an additional space */
    if (calc.usesep && !calc.sci_in && !calc.sci_out && !calc.is_nan) {
        /* go to the integer part of the string */
        TCHAR *p = _tcschr(tmp, TEXT('.'));
        TCHAR *e = _tcschr(tmp, TEXT('\0'));
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
            if (++n == t && *(p-1) != TEXT('-')) {
                memmove(p+1, p, (e-p+1)*sizeof(TCHAR));
                e++;
                *p = TEXT(' ');
                n = 0;
            }
        }
        /* if decimal mode, apply regional settings */
        if (calc.base == IDC_RADIO_DEC) {
            TCHAR *p = tmp;
            TCHAR *e = _tcschr(tmp, TEXT('.'));

            /* searching for thousands default separator */
            while (p < e) {
                if (*p == TEXT(' ')) {
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
        TCHAR *p = _tcschr(tmp, TEXT('.'));

        /* update decimal point when usesep is false */
        if (p != NULL) {
            memmove(p+calc.sDecimal_len, p+1, _tcslen(p)*sizeof(TCHAR));
            memcpy(p, calc.sDecimal, calc.sDecimal_len*sizeof(TCHAR));
        }
    }
    SendDlgItemMessage(hwnd, IDC_TEXT_OUTPUT, WM_SETTEXT, (WPARAM)0, (LPARAM)tmp);
}

static void update_parent_display(HWND hWnd)
{
    TCHAR str[8];
    int   n = eval_parent_count();

    if (!n)
        str[0] = TEXT('\0');
    else
        _stprintf(str,TEXT("(=%d"), n);
    SendDlgItemMessage(hWnd, IDC_TEXT_PARENT, WM_SETTEXT, 0, (LPARAM)str);
}

static void build_operand(HWND hwnd, DWORD idc)
{
    unsigned int i = 0, n;

    if (idc == IDC_BUTTON_DOT) {
        /* if dot is the first char, it's added automatically */
        if (calc.buffer == calc.ptr) {
            *calc.ptr++ = TEXT('0');
            *calc.ptr++ = TEXT('.');
            *calc.ptr   = TEXT('\0');
            update_lcd_display(hwnd);
            return;
        }
        /* if pressed dot and it's already in the string, then return */
        if (_tcschr(calc.buffer, TEXT('.')) != NULL)
            return;
    }
    if (idc != IDC_STATIC) {
        while (idc != key2code[i].idc) i++;
    }
    n = calc.ptr - calc.buffer;
    if (idc == IDC_BUTTON_0 && n == 0) {
        /* no need to put the dot because it's handled by update_lcd_display() */
        calc.buffer[0] = TEXT('0');
        calc.buffer[1] = TEXT('\0');
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
                _stprintf(calc.ptr, TEXT("0.e%+d"), calc.esp);
            else {
                /* adds the dot at the end if the number has no decimal part */
                if (!_tcschr(calc.buffer, TEXT('.')))
                    *calc.ptr++ = TEXT('.');
                _stprintf(calc.ptr, TEXT("e%+d"), calc.esp);
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
    calc.ptr += _stprintf(calc.ptr, TEXT("%C"), key2code[i].key);
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

static void display_rpn_result(HWND hwnd, calc_number_t *rpn)
{
    calc.sci_in = FALSE;
    prepare_rpn_result(rpn, calc.buffer, SIZEOF(calc.buffer), calc.base);
    calc.ptr = calc.buffer + _tcslen(calc.buffer);
    update_lcd_display(hwnd);
    calc.ptr = calc.buffer;
    update_parent_display(hwnd);
}

static int get_modifiers(HWND hwnd)
{
    int modifiers = 0;

    if (SendDlgItemMessage(hwnd, IDC_CHECK_INV, BM_GETCHECK, 0, 0))
        modifiers |= MODIFIER_INV;
    if (SendDlgItemMessage(hwnd, IDC_CHECK_HYP, BM_GETCHECK, 0, 0))
        modifiers |= MODIFIER_HYP;

    return modifiers;
}

static void convert_text2number(calc_number_t *a)
{
    /* if the screen output buffer is empty, then */
    /* the operand is taken from the last input */
    if (calc.buffer == calc.ptr) {
        /* if pushed valued is ZERO then we should grab it */
        if (!_tcscmp(calc.buffer, TEXT("0.")) ||
            !_tcscmp(calc.buffer, TEXT("0")))
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

static void update_menu(HWND hwnd)
{
    HMENU        hMenu = GetSubMenu(GetMenu(hwnd), 1);
    unsigned int x;

    for (x=0; x<SIZEOF(upd); x++) {
        if (*(upd[x].sel) != upd[x].idc) {
            CheckMenuItem(hMenu, upd[x].idm, MF_BYCOMMAND|MF_UNCHECKED);
            SendMessage((HWND)GetDlgItem(hwnd,upd[x].idc),BM_SETCHECK,FALSE,0L);
        } else {
            CheckMenuItem(hMenu, upd[x].idm, MF_BYCOMMAND|MF_CHECKED);
            SendMessage((HWND)GetDlgItem(hwnd,upd[x].idc),BM_SETCHECK,TRUE,0L);
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

    SendDlgItemMessage(hwnd, calc.base, BM_SETCHECK, BST_CHECKED, 0);
    if (base == IDC_RADIO_DEC)
        SendDlgItemMessage(hwnd, calc.degr, BM_SETCHECK, BST_CHECKED, 0);
    else
        SendDlgItemMessage(hwnd, calc.size, BM_SETCHECK, BST_CHECKED, 0);
}

static void update_memory_flag(HWND hWnd, BOOL mem_flag)
{
    calc.is_memory = mem_flag;
    SendDlgItemMessage(hWnd, IDC_TEXT_MEMORY, WM_SETTEXT, 0, (LPARAM)(mem_flag ? TEXT("M") : TEXT("")));
}

static void update_n_stats_items(HWND hWnd, TCHAR *buffer)
{
    unsigned int n = SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_GETCOUNT, 0, 0); 

    _stprintf(buffer, TEXT("n=%d"), n);
    SendDlgItemMessage(hWnd, IDC_TEXT_NITEMS, WM_SETTEXT, 0, (LPARAM)buffer);
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
    int len = strlen(formula);
    char *str = (char *)malloc(len+3);

    if (str == NULL)
        return NULL;

    str[0] = '(';
    memcpy(str+1, formula, len);
    str[len+1] = ')';
    str[len+2] = '\0';

    _tcscpy(calc.source, (*calc.buffer == _T('\0')) ? _T("0") : calc.buffer);

    /* clear display content before proceeding */
    calc.ptr = calc.buffer;
    calc.buffer[0] = TEXT('\0');

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
            if (n == (DWORD)-1)
                return TRUE;
			PostMessage(GetParent(hWnd), WM_LOAD_STAT, (WPARAM)n, 0);
            return TRUE;
        case IDC_BUTTON_CD:
            n = SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_GETCURSEL, 0, 0);
            if (n == (DWORD)-1)
                return TRUE;
			SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_DELETESTRING, (WPARAM)n, 0);
            update_n_stats_items(hWnd, buffer);
            delete_stat_item(n);
            return TRUE;
        case IDC_BUTTON_CAD:
			SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_RESETCONTENT, 0, 0);
            clean_stat_list();
            update_n_stats_items(hWnd, buffer);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        clean_stat_list();
        DestroyWindow(hWnd);
        return TRUE;
    case WM_DESTROY:
        PostMessage(GetParent(hWnd), WM_CLOSE_STATS, 0, 0);
        return TRUE;
    case WM_INSERT_STAT:
        prepare_rpn_result(&(((statistic_t *)lp)->num),
                           buffer, SIZEOF(buffer),
                           ((statistic_t *)lp)->base);
        SendDlgItemMessage(hWnd, IDC_LIST_STAT, LB_ADDSTRING, 0, (LPARAM)buffer);
        update_n_stats_items(hWnd, buffer);
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

	    EmptyClipboard();
	    clipbuffer = GlobalAlloc(GMEM_DDESHARE, (_tcslen(ptr)+1)*sizeof(TCHAR));
	    buffer = (TCHAR *)GlobalLock(clipbuffer);
	    _tcscpy(buffer, ptr);
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
    TCHAR display[sizeof(calc.buffer)];

    SendDlgItemMessage(hWnd, IDC_TEXT_OUTPUT, WM_GETTEXT, (WPARAM)SIZEOF(display), (LPARAM)display);
    if (calc.base == IDC_RADIO_DEC && _tcschr(calc.buffer, _T('.')) == NULL)
        display[_tcslen(display)-calc.sDecimal_len] = TEXT('\0');
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
            if (strlen(fromClipboard))
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
        calc.ptr =
        _tcscpy(calc.buffer, calc.source) +
        _tcslen(calc.source);
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
    seq->ptr = ptr;
    if (*ptr != '\0')
        PostMessage(hwnd, seq->wm_msg, 0, 0);
    else {
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

static void run_pow(calc_number_t *number)
{
    exec_infix2postfix(number, RPN_OPERATOR_POW);
}

static void run_sqr(calc_number_t *number)
{
    exec_infix2postfix(number, RPN_OPERATOR_SQR);
}

static void run_fe(calc_number_t *number)
{
    calc.sci_out = ((calc.sci_out == TRUE) ? FALSE : TRUE);
}

static void handle_context_menu(HWND hWnd, WPARAM wp, LPARAM lp)
{
    TCHAR text[64];
    HMENU hMenu = CreatePopupMenu();
    DWORD idm;

    LoadString(calc.hInstance, IDS_QUICKHELP, text, SIZEOF(text));
    AppendMenu(hMenu, MF_STRING | MF_ENABLED, IDM_HELP_HELP, text);
    idm = (DWORD)TrackPopupMenu(hMenu,
                                TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                LOWORD(lp),
                                HIWORD(lp),
                                0,
                                hWnd,
                                NULL);
    DestroyMenu(hMenu);
#ifndef DISABLE_HTMLHELP_SUPPORT
    if (idm != 0) {
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
        popup.idString = GetWindowLong((HWND)wp, GWL_ID);
        HtmlHelp((HWND)wp, HTMLHELP_PATH("/popups.txt"), HH_DISPLAY_TEXT_POPUP, (DWORD_PTR)&popup);
    }
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
    SendDlgItemMessage(calc.hWnd, IDC_CHECK_INV, BM_SETCHECK, 0, 0);
    SendDlgItemMessage(calc.hWnd, IDC_CHECK_HYP, BM_SETCHECK, 0, 0);
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
    DWORD            dwStyle;
    UINT             dwText;
    TCHAR            text[64];
    int              dx, dy, len;
    SIZE             size;
    POINT            pt;

    if(dis->CtlType == ODT_BUTTON) {
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
        /* button text to write */
        len = GetWindowText(dis->hwndItem, text, SIZEOF(text));
        /* default state: unpushed & enabled */
        dwStyle = 0;
        dwText = 0;
        if ((dis->itemState & ODS_DISABLED))
            dwText = DSS_DISABLED;
        if ((dis->itemState & ODS_SELECTED))
            dwStyle = DFCS_PUSHED;

        DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | dwStyle);
        GetTextExtentPoint32(dis->hDC, text, len, &size);
        dx = ((dis->rcItem.right-dis->rcItem.left) - size.cx) >> 1;
        dy = ((dis->rcItem.bottom-dis->rcItem.top) - size.cy) >> 1;
        if ((dwStyle & DFCS_PUSHED)) {
            dx++;
            dy++;
        }
        pt.x = dis->rcItem.left + dx;
        pt.y = dis->rcItem.top + dy;
        DrawState(dis->hDC, NULL, NULL, (LPARAM)text, 0, pt.x, pt.y, size.cx, size.cy, DST_TEXT | dwText);
    }
    return 1L;
}

static INT_PTR CALLBACK DlgMainProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    unsigned int x;
    RECT         rc;

    switch (msg) {
    case WM_DRAWITEM:
        return SubclassButtonProc(hWnd, wp, lp);

    case WM_INITDIALOG:
        calc.hWnd=hWnd;

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
        SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(calc.hInstance, MAKEINTRESOURCE(IDI_CALC_BIG)));
        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(calc.hInstance, MAKEINTRESOURCE(IDI_CALC_SMALL)));
        /* update text for decimal button */
        SendDlgItemMessage(hWnd, IDC_BUTTON_DOT, WM_SETTEXT, (WPARAM)0, (LPARAM)calc.sDecimal);
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
         * updatethe content of the "from/to" combo boxes.
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
            DialogBox(calc.hInstance,MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hWnd, AboutDlgProc);
            return TRUE;
        case IDM_HELP_HELP:
#ifndef DISABLE_HTMLHELP_SUPPORT
            HtmlHelp(hWnd, HTMLHELP_PATH("/general_information.htm"), HH_DISPLAY_TOPIC, (DWORD_PTR)NULL);
#endif
            return TRUE;
        case IDM_VIEW_STANDARD:
            calc.layout = CALC_LAYOUT_STANDARD;
            calc.action = IDM_VIEW_STANDARD;
            DestroyWindow(hWnd);
            save_config();
            return TRUE;
        case IDM_VIEW_SCIENTIFIC:
            calc.layout = CALC_LAYOUT_SCIENTIFIC;
            calc.action = IDM_VIEW_SCIENTIFIC;
            DestroyWindow(hWnd);
            save_config();
            return TRUE;
        case IDM_VIEW_CONVERSION:
            calc.layout = CALC_LAYOUT_CONVERSION;
            calc.action = IDM_VIEW_CONVERSION;
            DestroyWindow(hWnd);
            save_config();
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
            save_config();
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
            calc.is_nan = FALSE;
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
            if (calc.is_nan) break;
            /*
             * LSH button holds the RSH function too with INV modifier,
             * but since it's a two operand operator, it must be handled here.
             */
            if (LOWORD(wp) == IDC_BUTTON_LSH &&
                (get_modifiers(hWnd) & MODIFIER_INV)) {
                PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_BUTTON_RSH, BN_CLICKED), 0);
                SendDlgItemMessage(hWnd, IDC_CHECK_INV, BM_SETCHECK, 0, 0);
                break;
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
            if (calc.sci_in) {
                if (calc.esp == 0) {
                    TCHAR *ptr;

                    calc.sci_in = FALSE;
                    ptr = _tcschr(calc.ptr, TEXT('e'));
                    if (ptr)
                        *ptr = TEXT('\0');
                    update_lcd_display(hWnd);
                } else {
                    calc.esp /= 10;
                    build_operand(hWnd, IDC_STATIC);
                }
            } else
            if (calc.ptr != calc.buffer) {
                *--calc.ptr = TEXT('\0');
                if (!_tcscmp(calc.buffer, TEXT("-")) ||
                    !_tcscmp(calc.buffer, TEXT("-0")) ||
                    !_tcscmp(calc.buffer, TEXT("0"))) {
                    calc.ptr = calc.buffer;
                    calc.buffer[0] = TEXT('\0');
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
                if (calc.is_nan || calc.buffer[0] == TEXT('\0'))
                    break;

                if (calc.buffer[0] == TEXT('-')) {
                    /* make the number positive */
                    memmove(calc.buffer, calc.buffer+1, sizeof(calc.buffer)-1);
                    if (calc.buffer != calc.ptr)
                        calc.ptr--;
                } else {
                    /* if first char is '0' and no dot, it isn't valid */
                    if (calc.buffer[0] == TEXT('0') &&
                        calc.buffer[1] != TEXT('.'))
                        break;
                    /* make the number negative */
                    memmove(calc.buffer+1, calc.buffer, sizeof(calc.buffer)-1);
                    calc.buffer[0] = TEXT('-');
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
        case IDC_BUTTON_XeY:
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
                        display_rpn_result(hWnd, &calc.code);
                        if (!(function_table[x].range & NO_CHAIN))
                            exec_infix2postfix(&calc.code, RPN_OPERATOR_NONE);
                        if (function_table[x].range & MODIFIER_INV)
                            SendDlgItemMessage(hWnd, IDC_CHECK_INV, BM_SETCHECK, 0, 0);
                        if (function_table[x].range & MODIFIER_HYP)
                            SendDlgItemMessage(hWnd, IDC_CHECK_HYP, BM_SETCHECK, 0, 0);
                    }
                }
            }
            return TRUE;
        case IDC_BUTTON_STA:
            if (IsWindow(calc.hStatWnd))
                break;
            calc.hStatWnd = CreateDialog(calc.hInstance,
                                    MAKEINTRESOURCE(IDD_DIALOG_STAT), hWnd, (DLGPROC)DlgStatProc);
            if (calc.hStatWnd != NULL) {
                enable_allowed_controls(hWnd, calc.base);
                SendMessage(calc.hStatWnd, WM_SETFOCUS, 0, 0);
            }
            return TRUE;
        }
        break;
    case WM_CLOSE_STATS:
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

    calc.hInstance = hInstance;

    calc.x_coord = -1;
    calc.y_coord = -1;

    load_config();
    start_rpn_engine();

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
        if (NULL == CreateDialog(hInstance, MAKEINTRESOURCE(dwLayout), NULL, (DLGPROC)DlgMainProc))
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
    } while (calc.action != IDC_STATIC);

    stop_rpn_engine();

    return 0;
}
