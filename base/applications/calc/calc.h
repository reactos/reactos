#ifndef __CALC_H__
#define __CALC_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <shellapi.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <malloc.h>
#ifndef DISABLE_HTMLHELP_SUPPORT
#include <htmlhelp.h>
#endif
#include <limits.h>

/* Messages reserved for the main dialog */
#define WM_CLOSE_STATS      (WM_APP+1)
#define WM_HANDLE_CLIPBOARD (WM_APP+2)
#define WM_INSERT_STAT      (WM_APP+3)
#define WM_LOAD_STAT        (WM_APP+4)
#define WM_START_CONV       (WM_APP+5)
#define WM_HANDLE_FROM      (WM_APP+6)
#define WM_HANDLE_TO        (WM_APP+7)

/* GNU MULTI-PRECISION LIBRARY support */
#ifdef ENABLE_MULTI_PRECISION
#include "mpfr.h"

#ifndef MPFR_DEFAULT_RND
#define MPFR_DEFAULT_RND mpfr_get_default_rounding_mode ()
#endif

#define LOCAL_EXP_SIZE  100000000L
#else

#define LOCAL_EXP_SIZE  10000L

#endif

#include "resource.h"

#ifndef IDC_STATIC
#define IDC_STATIC  ((DWORD)-1)
#endif

#define CALC_VERSION        _T("1.12")

#define MAX_CALC_SIZE       256

/* HTMLHELP SUPPORT */
typedef HWND (WINAPI* type_HtmlHelpA)(HWND, LPCSTR, UINT, DWORD);
typedef HWND (WINAPI* type_HtmlHelpW)(HWND, LPCWSTR, UINT, DWORD);

extern type_HtmlHelpA calc_HtmlHelpA;
extern type_HtmlHelpW calc_HtmlHelpW;

#ifndef UNICODE
#define calc_HtmlHelp   calc_HtmlHelpA
#else
#define calc_HtmlHelp   calc_HtmlHelpW
#endif

void HtmlHelp_Start(HINSTANCE hInstance);
void HtmlHelp_Stop(void);

/*#define USE_KEYBOARD_HOOK*/

#define SIZEOF(_ar)     (sizeof(_ar)/sizeof(_ar[1]))

// RPN.C

enum {
    RPN_OPERATOR_PARENT,
    RPN_OPERATOR_PERCENT,
    RPN_OPERATOR_EQUAL,

    RPN_OPERATOR_OR,
    RPN_OPERATOR_XOR,
    RPN_OPERATOR_AND,
    RPN_OPERATOR_LSH,
    RPN_OPERATOR_RSH,
    RPN_OPERATOR_ADD,
    RPN_OPERATOR_SUB,
    RPN_OPERATOR_MULT,
    RPN_OPERATOR_DIV,
    RPN_OPERATOR_MOD,
    RPN_OPERATOR_POW,
    RPN_OPERATOR_SQR,

    RPN_OPERATOR_NONE
};

typedef union {
#ifdef ENABLE_MULTI_PRECISION
    mpfr_t  mf;
#else
    double  f;
    INT64   i;
    UINT64  u;
#endif
} calc_number_t;

typedef struct {
    calc_number_t number;
    unsigned int  operation;
    DWORD         base;
} calc_node_t;

void run_operator(calc_node_t *result, calc_node_t *a,
                  calc_node_t *b, unsigned int operation);
int  exec_infix2postfix(calc_number_t *, unsigned int);
void exec_closeparent(calc_number_t *);
int  eval_parent_count(void);
void flush_postfix(void);
void exec_change_infix(void);
void start_rpn_engine(void);
void stop_rpn_engine(void);

typedef struct {
    char *data;
    char *ptr;
    UINT  wm_msg;
} sequence_t;

typedef struct {
    calc_number_t    num;
    DWORD            base;
    void            *next;
} statistic_t;

enum {
    CALC_LAYOUT_SCIENTIFIC=0,
    CALC_LAYOUT_STANDARD,
    CALC_LAYOUT_CONVERSION,
};

typedef struct {
    HINSTANCE     hInstance;
#ifdef USE_KEYBOARD_HOOK
    HHOOK         hKeyboardHook;
#endif
    HWND          hWnd;
    DWORD         layout;
    TCHAR         buffer[MAX_CALC_SIZE];
    TCHAR         source[MAX_CALC_SIZE];
    TCHAR        *ptr;
    calc_number_t code;
    calc_number_t prev;
    calc_node_t   memory;
    statistic_t  *stat;
    BOOL          is_memory;
    BOOL          is_nan;
    BOOL          sci_out;
    BOOL          sci_in;
    BOOL          usesep;
    BOOL          is_menu_on;
    signed int    esp;
    DWORD         base;
    DWORD         size;
    DWORD         degr;
    DWORD         action;
    HWND          hStatWnd;
    HWND          hConvWnd;
    sequence_t    Clipboard;
    sequence_t    Convert[2];
    unsigned int  last_operator;
    unsigned int  prev_operator;
    TCHAR         sDecimal[8];
    TCHAR         sThousand[8];
    unsigned int  sDecimal_len;
    unsigned int  sThousand_len;
    signed int    x_coord;
    signed int    y_coord;
} calc_t;

extern calc_t calc;

/* IEEE constants */
#define CALC_E      2.718281828459045235360
#define CALC_PI_2   1.570796326794896619231
#define CALC_PI     3.141592653589793238462
#define CALC_3_PI_2 4.712388980384689857694
#define CALC_2_PI   6.283185307179586476925

#define MODIFIER_INV    0x01
#define MODIFIER_HYP    0x02
#define NO_CHAIN        0x04

void apply_int_mask(calc_number_t *a);
#ifndef ENABLE_MULTI_PRECISION
__int64 logic_dbl2int(calc_number_t *a);
double logic_int2dbl(calc_number_t *a);
#endif
void rpn_sin(calc_number_t *c);
void rpn_cos(calc_number_t *c);
void rpn_tan(calc_number_t *c);
void rpn_asin(calc_number_t *c);
void rpn_acos(calc_number_t *c);
void rpn_atan(calc_number_t *c);
void rpn_sinh(calc_number_t *c);
void rpn_cosh(calc_number_t *c);
void rpn_tanh(calc_number_t *c);
void rpn_asinh(calc_number_t *c);
void rpn_acosh(calc_number_t *c);
void rpn_atanh(calc_number_t *c);
BOOL rpn_validate_result(calc_number_t *c);
void rpn_int(calc_number_t *c);
void rpn_frac(calc_number_t *c);
void rpn_reci(calc_number_t *c);
void rpn_fact(calc_number_t *c);
void rpn_not(calc_number_t *c);
void rpn_pi(calc_number_t *c);
void rpn_2pi(calc_number_t *c);
void rpn_sign(calc_number_t *c);
void rpn_exp2(calc_number_t *c);
void rpn_exp3(calc_number_t *c);
void rpn_sqrt(calc_number_t *c);
void rpn_cbrt(calc_number_t *c);
void rpn_exp(calc_number_t *c);
void rpn_exp10(calc_number_t *c);
void rpn_ln(calc_number_t *c);
void rpn_log(calc_number_t *c);
void rpn_ave(calc_number_t *c);
void rpn_ave2(calc_number_t *c);
void rpn_sum(calc_number_t *c);
void rpn_sum2(calc_number_t *c);
void rpn_s(calc_number_t *c);
void rpn_s_m1(calc_number_t *c);
void rpn_dms2dec(calc_number_t *c);
void rpn_dec2dms(calc_number_t *c);
void rpn_zero(calc_number_t *c);
void rpn_copy(calc_number_t *dst, calc_number_t *src);
int  rpn_is_zero(calc_number_t *c);
void rpn_alloc(calc_number_t *c);
void rpn_free(calc_number_t *c);

//

void prepare_rpn_result_2(calc_number_t *rpn, TCHAR *buffer, int size, int base);
void convert_text2number_2(calc_number_t *a);
void convert_real_integer(unsigned int base);

//

INT_PTR CALLBACK AboutDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

//

void ConvExecute(HWND hWnd);
void ConvAdjust(HWND hWnd, int n_cat);
void ConvInit(HWND hWnd);

#endif /* __CALC_H__ */
