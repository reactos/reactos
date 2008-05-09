#ifndef __CALC_H__
#define __CALC_H__

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <malloc.h>
#include <htmlhelp.h>
#include <limits.h>

#ifdef ENABLE_MULTI_PRECISION
#include <mpfr.h>

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

#ifdef UNICODE
#define CF_TCHAR    CF_UNICODETEXT
#else
#define CF_TCHAR    CF_TEXT
#endif

#define CALC_VERSION        TEXT("1.06")

/*#define USE_KEYBOARD_HOOK*/

#define SIZEOF(_ar)     (sizeof(_ar)/sizeof(_ar[1]))

// RPN.C
/*
typedef struct _postfix_item_t {
    unsigned int type;
    union {
#ifdef ENABLE_MULTI_PRECISION
        mpfr_t       mf;
#else
        double       f;
        INT64        i;
#endif
        struct {
            unsigned short int code;
            unsigned short int elem;
        } action;
    } number;
    struct _postfix_item_t *next;
} postfix_item_t;

void flush_postfix(void);
void infix2postfix(char *in_str);
postfix_item_t *exec_postfix(void);
*/

//

enum {
    RPN_OPERATOR_PARENT,
    RPN_OPERATOR_PERCENT,
    RPN_OPERATOR_EQUAL,

    RPN_OPERATOR_OR,
    RPN_OPERATOR_XOR,
    RPN_OPERATOR_AND,
    RPN_OPERATOR_LSH,
    RPN_OPERATOR_ADD,
    RPN_OPERATOR_SUB,
    RPN_OPERATOR_MULT,
    RPN_OPERATOR_DIV,
    RPN_OPERATOR_MOD,
    RPN_OPERATOR_POW,
    RPN_OPERATOR_SQR,
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

void run_operator(calc_number_t *, calc_number_t *,
                  calc_number_t *, unsigned int);
int  exec_infix2postfix(calc_number_t *, unsigned int);
void exec_closeparent(calc_number_t *);
int  eval_parent_count(void);
void flush_postfix(void);
void exec_change_infix(void);
void start_rpn_engine(void);
void stop_rpn_engine(void);

typedef struct {
    calc_number_t    num;
    DWORD            base;
    void            *next;
} statistic_t;

enum {
    CALC_LAYOUT_SCIENTIFIC=0,
    CALC_LAYOUT_STANDARD,
};

typedef struct {
    HINSTANCE     hInstance;
#ifdef USE_KEYBOARD_HOOK
    HHOOK         hKeyboardHook;
#endif
    HWND          hWnd;
    DWORD         layout;
    TCHAR         buffer[256];
    TCHAR        *ptr;
    calc_number_t code;
    calc_number_t prev;
    calc_number_t memory;
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
    TCHAR        *Clipboard;
    TCHAR        *ClipPtr;
    unsigned int  last_operator;
    unsigned int  prev_operator;
    TCHAR         sDecimal[8];
    TCHAR         sThousand[8];
    unsigned int  sDecimal_len;
    unsigned int  sThousand_len;
} calc_t;

extern calc_t calc;

//
#define CALC_E      2.7182818284590452354
#define CALC_PI     3.14159265358979323846

#define MODIFIER_INV    0x01
#define MODIFIER_HYP    0x02

void apply_int_mask(calc_number_t *a);
#ifdef ENABLE_MULTI_PRECISION
void validate_rad2angle(calc_number_t *c);
void validate_angle2rad(calc_number_t *c);
#else
__int64 logic_dbl2int(calc_number_t *a);
double validate_rad2angle(double a);
double validate_angle2rad(calc_number_t *c);
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
void rpn_sum(calc_number_t *c);
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

#endif
