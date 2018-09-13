/******************************
Intel Confidential
******************************/

// MACH
/* define the following for WindowsNT only */
#define WIN32_OR_WIN64

#ifdef WIN32_OR_WIN64
#define LX "%I64x"
#define LD "%I64d"
#else
#define LX "%Lx"
#define LD "%Ld"
#endif

/******************************************************************
Due to differences in format of the constants we need to wrap is
with a macro to help add the LL on all platforms other than NT
*******************************************************************/
#ifndef WIN32_OR_WIN64
#define CONST_FORMAT(num) num ## LL
#else
#define CONST_FORMAT(num) ((EM_uint64_t)(num))
#endif

#ifndef _EM_TYPES_H
#define _EM_TYPES_H

// MACH
// Exception Codes for Emulation Traps and Faults
#define STATUS_EMULATION_FAULT   0x0E0010000
#define STATUS_EMULATION_TRAP    0x0E0020000

// MACH
#define LITTLE_ENDIAN

#define MAX_REAL_GR_INDEX             1
#define MAX_REAL_FR_INDEX             128

#define EM_NUM_PR        64
#define EM_NUM_MEM        1


#ifndef INLINE
#define INLINE
#endif

#if !(defined(BIG_ENDIAN) || defined(LITTLE_ENDIAN))
    #error Endianness not established; define BIG_ENDIAN or LITTLE_ENDIAN
#endif



/***********************/
/* Basic Integer Types */
/***********************/
 
typedef int                EM_int_t;      /* 32-bit signed integer */
typedef unsigned int       EM_uint_t;     /* 32-bit unsigned integer */

typedef short              EM_short_t;    /* 16-bit signed integer */
typedef unsigned short     EM_ushort_t;   /* 16-bit unsigned integer */

typedef unsigned int       EM_boolean_t;  /* true (1) or false (0) */

#ifdef WIN32_OR_WIN64
typedef __int64            EM_int64_t;    /* 64-bit signed integer */
typedef unsigned __int64   EM_uint64_t;   /* 64-bit unsigned integer */

#else

// MACH
#ifndef unix
typedef long long          EM_int64_t;    // 64-bit signed integer
typedef unsigned long long EM_uint64_t;   // 64-bit unsigned integer
#else
// MACH: for SCO UNIX
typedef long               EM_int64_t;    // 64-bit signed integer
typedef unsigned long      EM_uint64_t;   // 64-bit unsigned integer
#endif

#endif
 
/*******************************************************************************
FP_EXCP_xxx - Define a typedef for function pointers to override exceptions.
*****************************************************************************/

typedef void (*FP_EXCP_FAULT)(void*, EM_uint_t);
typedef void (*FP_EXCP_TRAP)(void*, EM_uint_t);
typedef void (*FP_EXCP_SPEC)(void*, EM_uint_t, EM_uint64_t);


typedef struct uint128_struct {
#ifdef BIG_ENDIAN
    EM_uint64_t hi;
    EM_uint64_t lo;
#endif
#ifdef LITTLE_ENDIAN
    EM_uint64_t lo;
    EM_uint64_t hi;
#endif
} EM_uint128_t;                          /* 128-bit unsigned integer */
 
typedef struct uint256_struct {
#ifdef BIG_ENDIAN
    EM_uint64_t hh;
    EM_uint64_t hl;
    EM_uint64_t lh;
    EM_uint64_t ll;
#endif
#ifdef LITTLE_ENDIAN
    EM_uint64_t ll;
    EM_uint64_t lh;
    EM_uint64_t hl;
    EM_uint64_t hh;
#endif
} EM_uint256_t;                          /* 256-bit unsigned integer */

/******************/
/* Support enum's */
/******************/

typedef enum {
    pr_00 =  0, pr_01 =  1, pr_02 =  2, pr_03 =  3,
    pr_04 =  4, pr_05 =  5, pr_06 =  6, pr_07 =  7,
    pr_08 =  8, pr_09 =  9, pr_10 = 10, pr_11 = 11,
    pr_12 = 12, pr_13 = 13, pr_14 = 14, pr_15 = 15,

    pr_16 = 16, pr_17 = 17, pr_18 = 18, pr_19 = 19,
    pr_20 = 20, pr_21 = 21, pr_22 = 22, pr_23 = 23,
    pr_24 = 24, pr_25 = 25, pr_26 = 26, pr_27 = 27,
    pr_28 = 28, pr_29 = 29, pr_30 = 30, pr_31 = 31,
    pr_32 = 32, pr_33 = 33, pr_34 = 34, pr_35 = 35,
    pr_36 = 36, pr_37 = 37, pr_38 = 38, pr_39 = 39,
    pr_40 = 40, pr_41 = 41, pr_42 = 42, pr_43 = 43,
    pr_44 = 44, pr_45 = 45, pr_46 = 46, pr_47 = 47,
    pr_48 = 48, pr_49 = 49, pr_50 = 50, pr_51 = 51,
    pr_52 = 52, pr_53 = 53, pr_54 = 54, pr_55 = 55,
    pr_56 = 56, pr_57 = 57, pr_58 = 58, pr_59 = 59,
    pr_60 = 60, pr_61 = 61, pr_62 = 62, pr_63 = 63
} EM_pred_reg_specifier;

typedef enum {
    gr_000 =   0, gr_001 =   1, gr_002 =   2, gr_003 =   3,
    gr_004 =   4, gr_005 =   5, gr_006 =   6, gr_007 =   7,
    gr_008 =   8, gr_009 =   9, gr_010 =  10, gr_011 =  11,
    gr_012 =  12, gr_013 =  13, gr_014 =  14, gr_015 =  15,
    gr_016 =  16, gr_017 =  17, gr_018 =  18, gr_019 =  19,
    gr_020 =  20, gr_021 =  21, gr_022 =  22, gr_023 =  23,
    gr_024 =  24, gr_025 =  25, gr_026 =  26, gr_027 =  27,
    gr_028 =  28, gr_029 =  29, gr_030 =  30, gr_031 =  31,

    gr_032 =  32, gr_033 =  33, gr_034 =  34, gr_035 =  35,
    gr_036 =  36, gr_037 =  37, gr_038 =  38, gr_039 =  39,
    gr_040 =  40, gr_041 =  41, gr_042 =  42, gr_043 =  43,
    gr_044 =  44, gr_045 =  45, gr_046 =  46, gr_047 =  47,
    gr_048 =  48, gr_049 =  49, gr_050 =  50, gr_051 =  51,
    gr_052 =  52, gr_053 =  53, gr_054 =  54, gr_055 =  55,
    gr_056 =  56, gr_057 =  57, gr_058 =  58, gr_059 =  59,
    gr_060 =  60, gr_061 =  61, gr_062 =  62, gr_063 =  63,
    gr_064 =  64, gr_065 =  65, gr_066 =  66, gr_067 =  67,
    gr_068 =  68, gr_069 =  69, gr_070 =  70, gr_071 =  71,
    gr_072 =  72, gr_073 =  73, gr_074 =  74, gr_075 =  75,
    gr_076 =  76, gr_077 =  77, gr_078 =  78, gr_079 =  79,
    gr_080 =  80, gr_081 =  81, gr_082 =  82, gr_083 =  83,
    gr_084 =  84, gr_085 =  85, gr_086 =  86, gr_087 =  87,
    gr_088 =  88, gr_089 =  89, gr_090 =  90, gr_091 =  91,
    gr_092 =  92, gr_093 =  93, gr_094 =  94, gr_095 =  95,
    gr_096 =  96, gr_097 =  97, gr_098 =  98, gr_099 =  99,
    gr_100 = 100, gr_101 = 101, gr_102 = 102, gr_103 = 103,
    gr_104 = 104, gr_105 = 105, gr_106 = 106, gr_107 = 107,
    gr_108 = 108, gr_109 = 109, gr_110 = 110, gr_111 = 111,
    gr_112 = 112, gr_113 = 113, gr_114 = 114, gr_115 = 115,
    gr_116 = 116, gr_117 = 117, gr_118 = 118, gr_119 = 119,
    gr_120 = 120, gr_121 = 121, gr_122 = 122, gr_123 = 123,
    gr_124 = 124, gr_125 = 125, gr_126 = 126, gr_127 = 127
} EM_general_register_specifier;


typedef enum {
    fr_000 =   0, fr_001 =   1, fr_002 =   2, fr_003 =   3,
    fr_004 =   4, fr_005 =   5, fr_006 =   6, fr_007 =   7,
    fr_008 =   8, fr_009 =   9, fr_010 =  10, fr_011 =  11,
    fr_012 =  12, fr_013 =  13, fr_014 =  14, fr_015 =  15,
    fr_016 =  16, fr_017 =  17, fr_018 =  18, fr_019 =  19,
    fr_020 =  20, fr_021 =  21, fr_022 =  22, fr_023 =  23,
    fr_024 =  24, fr_025 =  25, fr_026 =  26, fr_027 =  27,
    fr_028 =  28, fr_029 =  29, fr_030 =  30, fr_031 =  31,

    fr_032 =  32, fr_033 =  33, fr_034 =  34, fr_035 =  35,
    fr_036 =  36, fr_037 =  37, fr_038 =  38, fr_039 =  39,
    fr_040 =  40, fr_041 =  41, fr_042 =  42, fr_043 =  43,
    fr_044 =  44, fr_045 =  45, fr_046 =  46, fr_047 =  47,
    fr_048 =  48, fr_049 =  49, fr_050 =  50, fr_051 =  51,
    fr_052 =  52, fr_053 =  53, fr_054 =  54, fr_055 =  55,
    fr_056 =  56, fr_057 =  57, fr_058 =  58, fr_059 =  59,
    fr_060 =  60, fr_061 =  61, fr_062 =  62, fr_063 =  63,
    fr_064 =  64, fr_065 =  65, fr_066 =  66, fr_067 =  67,
    fr_068 =  68, fr_069 =  69, fr_070 =  70, fr_071 =  71,
    fr_072 =  72, fr_073 =  73, fr_074 =  74, fr_075 =  75,
    fr_076 =  76, fr_077 =  77, fr_078 =  78, fr_079 =  79,
    fr_080 =  80, fr_081 =  81, fr_082 =  82, fr_083 =  83,
    fr_084 =  84, fr_085 =  85, fr_086 =  86, fr_087 =  87,
    fr_088 =  88, fr_089 =  89, fr_090 =  90, fr_091 =  91,
    fr_092 =  92, fr_093 =  93, fr_094 =  94, fr_095 =  95,
    fr_096 =  96, fr_097 =  97, fr_098 =  98, fr_099 =  99,
    fr_100 = 100, fr_101 = 101, fr_102 = 102, fr_103 = 103,
    fr_104 = 104, fr_105 = 105, fr_106 = 106, fr_107 = 107,
    fr_108 = 108, fr_109 = 109, fr_110 = 110, fr_111 = 111,
    fr_112 = 112, fr_113 = 113, fr_114 = 114, fr_115 = 115,
    fr_116 = 116, fr_117 = 117, fr_118 = 118, fr_119 = 119,
    fr_120 = 120, fr_121 = 121, fr_122 = 122, fr_123 = 123,
    fr_124 = 124, fr_125 = 125, fr_126 = 126, fr_127 = 127
} EM_fp_reg_specifier;

typedef enum {
    mem_real_form    = 0,
    mem_integer_form = 1
} EM_mem_fr_format_type;


typedef enum {
    pc_s    = 0,
    pc_d    = 2,
    pc_sf   = 3,
    pc_none = 4,
    pc_simd = 5
} EM_opcode_pc_type;

typedef enum {
    sfS0   = 0,
    sfS1   = 1,
    sfS2   = 2,
    sfS3   = 3,
    sf_none = 4
} EM_opcode_sf_type;

typedef enum {
    fctypeUNC      = 0,
    ctype_or       = 1,
    ctype_and      = 2,
    ctype_or_andcm = 3,
    ctype_orcm     = 4,
    ctype_andcm    = 5,
    ctype_and_orcm = 6,
    ctype_none     = 7
} EM_opcode_ctype_type;

typedef enum {
    crel_eq  = 0,
    crel_ne  = 1,
    crel_lt  = 2,
    crel_le  = 3,
    crel_gt  = 4,
    crel_ge  = 5,
    crel_ltu = 6,
    crel_leu = 7,
    crel_gtu = 8,
    crel_geu = 9
} EM_opcode_crel_type;


typedef enum {
    fcrel_nm  = 0,
    fcrel_m   = 1
} EM_opcode_fcrel_type;

typedef enum {
    frelEQ    =  0,
    frelLT    =  1,
    frelLE    =  2,
    frelUNORD =  3,
    frelNEQ   =  4,
    frelNLT   =  5,
    frelNLE   =  6,
    frelORD   =  7,
    frelGT    =  8,
    frelGE    =  9,
    frelNGT   = 10,
    frelNGE   = 11
} EM_opcode_frel_type;

typedef enum {
    sf_single             = 0,
    SF_PC_RESERVED        = 1,
    sf_double             = 2,
    sf_double_extended    = 3
} EM_sf_pc_type;

typedef enum {
    rc_rn = 0,
    rc_rm = 1,
    rc_rp = 2,
    rc_rz = 3
} EM_sf_rc_type;

typedef enum {
    ar_fpsr = 0
} EM_ar_index;

typedef enum {
    high = 1,
    low  = 0
} EM_simd_hilo;

typedef enum {
    op_fcmp        = 0,
    op_fpcmp       = 1,
    op_fcvt_fx     = 2,
    op_fpcvt_fx    = 3,
    op_fcvt_fxu    = 4,
    op_fpcvt_fxu   = 5,
    op_fma         = 6,
    op_fpma        = 7,
    op_fminmax     = 8,
    op_fpminmax    = 9,
    op_fms_fnma    = 10,
    op_fpms_fpnma  = 11,
    op_frcpa       = 12,
    op_fprcpa      = 13,
    op_frsqrta     = 14,
    op_fprsqrta    = 15,
    op_fnorm       = 16,
    op_fsetc       = 17
 } EM_opcode_type;

    
typedef struct EM_limits_check_fprcpa_struct {
   EM_uint_t  hi_fr3;
   EM_uint_t  hi_fr2_or_quot;
   EM_uint_t  lo_fr3;
   EM_uint_t  lo_fr2_or_quot;
} EM_limits_check_fprcpa;

typedef struct EM_limits_check_fprsqrta_struct {
   EM_uint_t  hi;
   EM_uint_t  lo;
} EM_limits_check_fprsqrta;

/***************************/
/* Support Structure Types */
/***************************/


typedef struct trap_control_struct {
    EM_uint_t vd:1;
    EM_uint_t dd:1;
    EM_uint_t zd:1;
    EM_uint_t od:1;
    EM_uint_t ud:1;
    EM_uint_t id:1;
} EM_trap_control_type;

typedef enum {
    ss_single_24          = 24,
    ss_double_53          = 53,
    ss_double_extended_64 = 64
} EM_significand_size_type;

typedef enum {
    es_eight_bits     =  8,
    es_eleven_bits    = 11,
    es_fifteen_bits   = 15,
    es_seventeen_bits = 17
} EM_exponent_size_type;

typedef struct controls_struct {
    EM_uint_t ftz:1;
    EM_uint_t wre:1;
    EM_sf_pc_type pc;
    EM_sf_rc_type rc;
    EM_uint_t td:1;
} EM_controls_type;

typedef struct flags_struct {
    EM_uint_t v:1;
    EM_uint_t d:1;
    EM_uint_t z:1;
    EM_uint_t o:1;
    EM_uint_t un:1; // MACH
    EM_uint_t i:1;
} EM_flags_type;

typedef struct tmp_fp_env_struct { // MACH
    long                     dummy;
    EM_trap_control_type     controls;
    EM_significand_size_type ss;
    EM_exponent_size_type    es;

    EM_sf_rc_type            rc;
    EM_uint_t                ftz:1;

    EM_flags_type            flags;
    EM_flags_type            hi_flags;
    EM_flags_type            lo_flags;

    EM_uint_t                ebc:1;
    EM_uint_t                fpa:1;
    EM_uint_t                hi_fpa:1;
    EM_uint_t                lo_fpa:1;
    EM_uint_t                mdl:1;
    EM_uint_t                mdh:1;
    EM_uint_t                simd:1;

    struct em_faults_struct {
        EM_uint_t v:1;
        EM_uint_t d:1;
        EM_uint_t z:1;
        EM_uint_t swa:1;
    } em_faults;
    struct hi_faults_struct {
        EM_uint_t v:1;
        EM_uint_t d:1;
        EM_uint_t z:1;
        EM_uint_t swa:1;
    } hi_faults;
    struct lo_faults_struct {
        EM_uint_t v:1;
        EM_uint_t d:1;
        EM_uint_t z:1;
        EM_uint_t swa:1;
    } lo_faults;

    struct em_traps_struct {
        EM_uint_t o:1;
        EM_uint_t un:1; // MACH
        EM_uint_t i:1;
    } em_traps;
    struct hi_traps_struct {
        EM_uint_t o:1;
        EM_uint_t un:1; // MACH
        EM_uint_t i:1;
    } hi_traps;
    struct lo_traps_struct {
        EM_uint_t o:1;
        EM_uint_t un:1; // MACH
        EM_uint_t i:1;
    } lo_traps;
} EM_tmp_fp_env_type;

typedef struct fp_dp_struct {
    EM_uint_t sticky:1;
    EM_uint128_t significand;
    EM_uint_t exponent:19;
    EM_uint_t sign:1;
} EM_fp_dp_type;

/*************************/
/* Register Definitions */
/*************************/

typedef struct gr_reg_struct {
    EM_uint64_t value;
    EM_uint_t nat:1;
} EM_gr_reg_type;

typedef struct fp_reg_struct {
    EM_uint64_t significand;
    EM_uint_t exponent:17;
    EM_uint_t sign:1;
} EM_fp_reg_type;

typedef struct pair_fp_reg_struct {
    EM_fp_reg_type hi;
    EM_fp_reg_type lo;
} EM_pair_fp_reg_type;

typedef struct sf_struct {
    EM_controls_type controls;
    EM_flags_type    flags;
} EM_sf_type;

typedef struct cfm_struct {
#ifdef BIG_ENDIAN
    EM_uint_t reserved         :26;
    EM_uint_t rrb_pr           : 6;
    EM_uint_t sof              : 7;
    EM_uint_t sol              : 7;
    EM_uint_t sor              : 4;
    EM_uint_t rrb_gr           : 7;
    EM_uint_t rrb_fr           : 7;
#endif
#ifdef LITTLE_ENDIAN
    EM_uint_t rrb_fr           : 7;
    EM_uint_t rrb_gr           : 7;
    EM_uint_t sor              : 4;
    EM_uint_t sol              : 7;
    EM_uint_t sof              : 7;
    EM_uint_t rrb_pr           : 6;
    EM_uint_t reserved         :26;
#endif
} EM_cfm_type;

typedef struct fpsr_struct {
#ifdef BIG_ENDIAN
    EM_uint_t reserved         : 6;
    EM_uint_t sf3_flags_i      : 1;
    EM_uint_t sf3_flags_u      : 1;
    EM_uint_t sf3_flags_o      : 1;
    EM_uint_t sf3_flags_z      : 1;
    EM_uint_t sf3_flags_d      : 1;
    EM_uint_t sf3_flags_v      : 1;
    EM_uint_t sf3_controls_td  : 1;
    EM_uint_t sf3_controls_rc  : 2;
    EM_uint_t sf3_controls_pc  : 2;
    EM_uint_t sf3_controls_wre : 1;
    EM_uint_t sf3_controls_ftz : 1;
    EM_uint_t sf2_flags_i      : 1;
    EM_uint_t sf2_flags_u      : 1;
    EM_uint_t sf2_flags_o      : 1;
    EM_uint_t sf2_flags_z      : 1;
    EM_uint_t sf2_flags_d      : 1;
    EM_uint_t sf2_flags_v      : 1;
    EM_uint_t sf2_controls_td  : 1;
    EM_uint_t sf2_controls_rc  : 2;
    EM_uint_t sf2_controls_pc  : 2;
    EM_uint_t sf2_controls_wre : 1;
    EM_uint_t sf2_controls_ftz : 1;
    EM_uint_t sf1_flags_i      : 1;
    EM_uint_t sf1_flags_u      : 1;
    EM_uint_t sf1_flags_o      : 1;
    EM_uint_t sf1_flags_z      : 1;
    EM_uint_t sf1_flags_d      : 1;
    EM_uint_t sf1_flags_v      : 1;
    EM_uint_t sf1_controls_td  : 1;
    EM_uint_t sf1_controls_rc  : 2;
    EM_uint_t sf1_controls_pc  : 2;
    EM_uint_t sf1_controls_wre : 1;
    EM_uint_t sf1_controls_ftz : 1;
    EM_uint_t sf0_flags_i      : 1;
    EM_uint_t sf0_flags_u      : 1;
    EM_uint_t sf0_flags_o      : 1;
    EM_uint_t sf0_flags_z      : 1;
    EM_uint_t sf0_flags_d      : 1;
    EM_uint_t sf0_flags_v      : 1;
    EM_uint_t sf0_controls_td  : 1;
    EM_uint_t sf0_controls_rc  : 2;
    EM_uint_t sf0_controls_pc  : 2;
    EM_uint_t sf0_controls_wre : 1;
    EM_uint_t sf0_controls_ftz : 1;
    EM_uint_t traps_id         : 1;
    EM_uint_t traps_ud         : 1;
    EM_uint_t traps_od         : 1;
    EM_uint_t traps_zd         : 1;
    EM_uint_t traps_dd         : 1;
    EM_uint_t traps_vd         : 1;
#endif
#ifdef LITTLE_ENDIAN
    EM_uint_t traps_vd         : 1;
    EM_uint_t traps_dd         : 1;
    EM_uint_t traps_zd         : 1;
    EM_uint_t traps_od         : 1;
    EM_uint_t traps_ud         : 1;
    EM_uint_t traps_id         : 1;
    EM_uint_t sf0_controls_ftz : 1;
    EM_uint_t sf0_controls_wre : 1;
    EM_uint_t sf0_controls_pc  : 2;
    EM_uint_t sf0_controls_rc  : 2;
    EM_uint_t sf0_controls_td  : 1;
    EM_uint_t sf0_flags_v      : 1;
    EM_uint_t sf0_flags_d      : 1;
    EM_uint_t sf0_flags_z      : 1;
    EM_uint_t sf0_flags_o      : 1;
    EM_uint_t sf0_flags_u      : 1;
    EM_uint_t sf0_flags_i      : 1;
    EM_uint_t sf1_controls_ftz : 1;
    EM_uint_t sf1_controls_wre : 1;
    EM_uint_t sf1_controls_pc  : 2;
    EM_uint_t sf1_controls_rc  : 2;
    EM_uint_t sf1_controls_td  : 1;
    EM_uint_t sf1_flags_v      : 1;
    EM_uint_t sf1_flags_d      : 1;
    EM_uint_t sf1_flags_z      : 1;
    EM_uint_t sf1_flags_o      : 1;
    EM_uint_t sf1_flags_u      : 1;
    EM_uint_t sf1_flags_i      : 1;
    EM_uint_t sf2_controls_ftz : 1;
    EM_uint_t sf2_controls_wre : 1;
    EM_uint_t sf2_controls_pc  : 2;
    EM_uint_t sf2_controls_rc  : 2;
    EM_uint_t sf2_controls_td  : 1;
    EM_uint_t sf2_flags_v      : 1;
    EM_uint_t sf2_flags_d      : 1;
    EM_uint_t sf2_flags_z      : 1;
    EM_uint_t sf2_flags_o      : 1;
    EM_uint_t sf2_flags_u      : 1;
    EM_uint_t sf2_flags_i      : 1;
    EM_uint_t sf3_controls_ftz : 1;
    EM_uint_t sf3_controls_wre : 1;
    EM_uint_t sf3_controls_pc  : 2;
    EM_uint_t sf3_controls_rc  : 2;
    EM_uint_t sf3_controls_td  : 1;
    EM_uint_t sf3_flags_v      : 1;
    EM_uint_t sf3_flags_d      : 1;
    EM_uint_t sf3_flags_z      : 1;
    EM_uint_t sf3_flags_o      : 1;
    EM_uint_t sf3_flags_u      : 1;
    EM_uint_t sf3_flags_i      : 1;
    EM_uint_t reserved         : 6;
#endif
} EM_fpsr_type;


typedef struct psr_struct {
#ifdef BIG_ENDIAN
    EM_uint_t reserved_field_4:19;
    EM_uint_t bn:1;
    EM_uint_t ed:1;
    EM_uint_t ri:2;
    EM_uint_t ss:1;
    EM_uint_t dd:1;
    EM_uint_t da:1;
    EM_uint_t id:1;
    EM_uint_t it:1;
    EM_uint_t mc:1;
    EM_uint_t is:1;
    EM_uint_t cpl:2;
    EM_uint_t reserved_field_3:4;
    EM_uint_t rt:1;
    EM_uint_t tb:1;
    EM_uint_t lp:1;
    EM_uint_t db:1;
    EM_uint_t si:1;
    EM_uint_t di:1;
    EM_uint_t pp:1;
    EM_uint_t sp:1;
    EM_uint_t dfh:1;
    EM_uint_t dfl:1;
    EM_uint_t dt:1;
    EM_uint_t reserved_field_2:1;
    EM_uint_t pk:1;
    EM_uint_t i:1;
    EM_uint_t ic:1;
    EM_uint_t reserved_field_1:7;
    EM_uint_t mfh:1;
    EM_uint_t mfl:1;
    EM_uint_t ac:1;
    EM_uint_t up:1;
    EM_uint_t be:1;
    EM_uint_t or:1;
#endif
#ifdef LITTLE_ENDIAN
    EM_uint_t or:1;
    EM_uint_t be:1;
    EM_uint_t up:1;
    EM_uint_t ac:1;
    EM_uint_t mfl:1;
    EM_uint_t mfh:1;
    EM_uint_t reserved_field_1:7;
    EM_uint_t ic:1;
    EM_uint_t i:1;
    EM_uint_t pk:1;
    EM_uint_t reserved_field_2:1;
    EM_uint_t dt:1;
    EM_uint_t dfl:1;
    EM_uint_t dfh:1;
    EM_uint_t sp:1;
    EM_uint_t pp:1;
    EM_uint_t di:1;
    EM_uint_t si:1;
    EM_uint_t db:1;
    EM_uint_t lp:1;
    EM_uint_t tb:1;
    EM_uint_t rt:1;
    EM_uint_t reserved_field_3:4;
    EM_uint_t cpl:2;
    EM_uint_t is:1;
    EM_uint_t mc:1;
    EM_uint_t it:1;
    EM_uint_t id:1;
    EM_uint_t da:1;
    EM_uint_t dd:1;
    EM_uint_t ss:1;
    EM_uint_t ri:2;
    EM_uint_t ed:1;
    EM_uint_t bn:1;
    EM_uint_t reserved_field_4:19;
#endif
} EM_psr_type;

typedef union ar_union {
    EM_fpsr_type fpsr;
    EM_uint64_t  uint_value;
} EM_ar_type;

/*****************************/
/* memory format definitions */
/*****************************/

typedef union memory_union {
        struct int_8_struct {
            EM_int_t ivalue:8;
        } int_8;
        struct int_16_struct {
            EM_int_t ivalue:16;
        } int_16;
        struct int_32_struct {
            EM_int_t ivalue:32;
        } int_32;
        struct int_64_struct {
            EM_int64_t ivalue;
        } int_64;
        struct uint_8_struct {
            EM_uint_t uvalue:8;
        } uint_8;
        struct uint_16_struct {
            EM_uint_t uvalue:16;
        } uint_16;
        struct uint_32_struct {
            EM_uint_t uvalue:32;
        } uint_32;
        struct uint_64_struct {
            EM_uint64_t uvalue;
        } uint_64;
        struct c_float_struct {
            float fvalue;
        } c_float;
        struct c_double_struct {
            double fvalue;
        } c_double;
        struct c_long_double_struct {
            long double fvalue;
        } c_long_double;

        struct fp_single_struct {
#ifdef BIG_ENDIAN
            EM_uint_t sign:1;
            EM_uint_t exponent:8;
            EM_uint_t significand:23;
#endif
#ifdef LITTLE_ENDIAN
            EM_uint_t significand:23;
            EM_uint_t exponent:8;
            EM_uint_t sign:1;
#endif
        } fp_single;

        struct fp_double_struct {
#ifdef BIG_ENDIAN
            EM_uint_t sign:1;
            EM_uint_t exponent:11;
            EM_uint_t significand_hi:20;
            EM_uint_t significand_lo:32;
#endif
#ifdef LITTLE_ENDIAN
            EM_uint_t significand_lo:32;
            EM_uint_t significand_hi:20;
            EM_uint_t exponent:11;
            EM_uint_t sign:1;

#endif
        } fp_double;

        struct fp_double_extended_struct {
#ifdef BIG_ENDIAN
            EM_uint_t sign:1;
            EM_uint_t exponent:15;
            EM_ushort_t significand[4];
#endif
#ifdef LITTLE_ENDIAN
            EM_ushort_t significand[4];
            EM_uint_t exponent:15;
            EM_uint_t sign:1;
#endif
        } fp_double_extended;

        struct fp_spill_fill_struct {
#ifdef BIG_ENDIAN
            EM_uint_t reserved2:32;
            EM_uint_t reserved1:14;
            EM_uint_t sign:1;
            EM_uint_t exponent:17;
            EM_uint64_t significand;
#endif
#ifdef LITTLE_ENDIAN
            EM_uint64_t significand;
            EM_uint_t exponent:17;
            EM_uint_t sign:1;
            EM_uint_t reserved1:14;
            EM_uint_t reserved2:32;
#endif
        } fp_spill_fill;


        struct uint_32_pair_struct {
#ifdef BIG_ENDIAN
            EM_uint_t hi:32;
            EM_uint_t lo:32;
#endif
#ifdef LITTLE_ENDIAN
            EM_uint_t lo:32;
            EM_uint_t hi:32;
#endif
        } uint_32_pair;
        struct uint_64_pair_struct {
#ifdef BIG_ENDIAN
            EM_uint64_t hi;
            EM_uint64_t lo;
#endif
#ifdef LITTLE_ENDIAN
            EM_uint64_t lo;
            EM_uint64_t hi;
#endif
        } uint_64_pair;

} EM_memory_type;

typedef struct EM_form_struct {
  EM_boolean_t advanced_form;
  EM_boolean_t clear_form;
  EM_boolean_t double_form;
  EM_boolean_t exponent_form;

  EM_boolean_t fp82_floating_form;
  EM_boolean_t fcheck_branch_implemented;
  EM_boolean_t general_form;
  EM_boolean_t data_form;
  EM_boolean_t control_form;
  EM_boolean_t high_form;
  EM_boolean_t high_unsigned_form;

  EM_boolean_t low_form;
  EM_boolean_t mix_l_form;
  EM_boolean_t mix_r_form;
  EM_boolean_t mix_lr_form;

  EM_boolean_t neg_sign_form;
  EM_boolean_t no_clear_form;
  EM_boolean_t pack_form;

  EM_boolean_t no_base_update_form;
  EM_boolean_t immediate_base_update_form;
  EM_boolean_t register_base_update_form;

  EM_boolean_t sign_form;
  EM_boolean_t sign_exp_form;
  EM_boolean_t signed_form;
  EM_boolean_t significand_form;
  EM_boolean_t single_form;
  EM_boolean_t spill_form;
  EM_boolean_t swap_form;
  EM_boolean_t swap_nl_form;
  EM_boolean_t swap_nr_form;
  EM_boolean_t sxt_l_form;
  EM_boolean_t sxt_r_form;

  EM_boolean_t trunc_form;
  EM_boolean_t unsigned_form;
} EM_form_type;

typedef void (*EM_EXCP_FAULT)(void*, EM_uint_t);
typedef void (*EM_EXCP_TRAP)(void*, EM_uint_t);
typedef void (*EM_EXCP_SPEC)(void*, EM_uint_t, EM_uint64_t);


typedef struct EM_state_struct {
  EM_psr_type            state_PSR;
  EM_uint64_t            state_IP;
  EM_ar_type             state_AR[1];
  EM_fp_reg_type         state_FR[MAX_REAL_FR_INDEX];
  EM_gr_reg_type         state_GR[MAX_REAL_GR_INDEX];
  EM_boolean_t           state_PR[EM_NUM_PR];
  EM_memory_type         state_MEM[EM_NUM_MEM];
  EM_form_type           state_form;
  EM_uint_t              state_MERCED_RTL;
  EM_EXCP_FAULT          state_fp82_fp_exception_fault;
  EM_EXCP_TRAP           state_fp82_fp_exception_trap;
  void                   *state_user_context;
  EM_uint_t              trap_type; // 0 - fault; 1 - trap
} EM_state_type;


#endif /* _EM_TYPES_H */

