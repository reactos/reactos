/* {{{ Copyright */

/* Additional keyboard support routines.

   Copyright (C) 1998 the Free Software Foundation.

   Written by: 1998, Gyorgy Tamasi
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* }}} */

/*
 * PURPOSE:
 * 
 *   We would like to support the direct ALT-?/META-? and some other 'extra'
 *   keyboard functionality provided by some terminals under some OSes (and
 *   not supported by the 'learn keys...' facility of 'mc'.
 *   (First target platform: QNX.)
 * 
 * REMARK:
 * 
 *   Implementation strategy: we don't want to rely on a specific terminal
 *   information database management API (termcap,terminfo,SLang,...), so we
 *   try to define a superset of the possible key identifiers here.
 *
 */
#include <config.h>
#include "mouse.h" /* required before key.h */
#include "key.h"
#include "myslang.h"


#ifdef __QNX__

/* select implementation: use QNX/term interface */
#define __USE_QNX_TI

/* implementation specific _TE() definition */
#ifdef __USE_QNX_TI

/* include QNX/term.h (not NCURSES/term.h!) */
#if __WATCOMC__ > 1000
    #include <sys/term.h>
#else
    #include <term.h>
#endif
#include <stdlib.h> /* getenv() */

/* fieldname -> index conversion */
#define __QTISX(_qtisn) \
	(((int)(&((struct _strs*)0)->_qtisn))/sizeof(charoffset))

/* define the OS/implementation-specific __TK() format */
#define __TK(_tis,_tcs,_tisx,_qtisn)  __QTISX(_qtisn)
    
#endif /* __USE_QNX_TI */
    
#endif /* __QNX__ */


/* {{{ */
    
/* general key definitions:
 * 
 * format:
 * 
 *   terminfo name,
 *   termcap name,
 *   index in the terminfo string table (ncurses),
 *   field name in the QNX terminfo strings struct
 */
    
#define Key_backspace   __TK("kbs",   "kb",  55, _ky_backspace )
#define Key_catab       __TK("ktbc",  "ka",  56, _ky_catab )
#define Key_clear       __TK("kclr",  "kC",  57, _ky_clear )
#define Key_ctab        __TK("kctab", "kt",  58, _ky_ctab )
#define Key_dc          __TK("kdch1", "kD",  59, _ky_dc )
#define Key_dl          __TK("kdl1",  "kL",  60, _ky_dl )
#define Key_down        __TK("kcud1", "kd",  61, _ky_down )
#define Key_eic         __TK("krmir", "kM",  62, _ky_eic )
#define Key_eol         __TK("kel",   "kE",  63, _ky_eol )
#define Key_eos         __TK("ked",   "kS",  64, _ky_eos )
#define Key_f0          __TK("kf0",   "k0",  65, _ky_f0 )
#define Key_f1          __TK("kf1",   "k1",  66, _ky_f1 )
#define Key_f10         __TK("kf10",  "k;",  67, _ky_f10 )
#define Key_f2          __TK("kf2",   "k2",  68, _ky_f2 )
#define Key_f3          __TK("kf3",   "k3",  69, _ky_f3 )
#define Key_f4          __TK("kf4",   "k4",  70, _ky_f4 )
#define Key_f5          __TK("kf5",   "k5",  71, _ky_f5 )
#define Key_f6          __TK("kf6",   "k6",  72, _ky_f6 )
#define Key_f7          __TK("kf7",   "k7",  73, _ky_f7 )
#define Key_f8          __TK("kf8",   "k8",  74, _ky_f8 )
#define Key_f9          __TK("kf9",   "k9",  75, _ky_f9 )
#define Key_home        __TK("khome", "kh",  76, _ky_home )
#define Key_ic          __TK("kich1", "kI",  77, _ky_ic )
#define Key_il          __TK("kil1",  "kA",  78, _ky_il )
#define Key_left        __TK("kcub1", "kl",  79, _ky_left )
#define Key_ll          __TK("kll",   "kH",  80, _ky_ll )
#define Key_npage       __TK("knp",   "kN",  81, _ky_npage )
#define Key_ppage       __TK("kpp",   "kP",  82, _ky_ppage )
#define Key_right       __TK("kcuf1", "kr",  83, _ky_right )
#define Key_sf          __TK("kind",  "kF",  84, _ky_sf )
#define Key_sr          __TK("kri",   "kR",  85, _ky_sr )
#define Key_stab        __TK("khts",  "kT",  86, _ky_stab )
#define Key_up          __TK("kcuu1", "ku",  87, _ky_up )
#define Key_a1          __TK("ka1",   "K1", 139, _ky_a1 )
#define Key_a3          __TK("ka3",   "K3", 140, _ky_a3 )
#define Key_b2          __TK("kb2",   "K2", 141, _ky_b2 )
#define Key_c1          __TK("kc1",   "K4", 142, _ky_c1 )
#define Key_c3          __TK("kc3",   "K5", 143, _ky_c3 )
#define Key_btab        __TK("kcbt",  "kB", 148, _ky_btab )
#define Key_beg         __TK("kbeg",  "@1", 158, _ky_beg )
#define Key_cancel      __TK("kcan",  "@2", 159, _ky_cancel )
#define Key_close       __TK("kclo",  "@3", 160, _ky_close )
#define Key_command     __TK("kcmd",  "@4", 161, _ky_command )
#define Key_copy        __TK("kcpy",  "@5", 162, _ky_copy )
#define Key_create      __TK("kcrt",  "@6", 163, _ky_create )
#define Key_end         __TK("kend",  "@7", 164, _ky_end )
#define Key_enter       __TK("kent",  "@8", 165, _ky_enter )
#define Key_exit        __TK("kext",  "@9", 166, _ky_exit )
#define Key_find        __TK("kfnd",  "@0", 167, _ky_find )
#define Key_help        __TK("khlp",  "%1", 168, _ky_help )
#define Key_mark        __TK("kmrk",  "%2", 169, _ky_mark )
#define Key_message     __TK("kmsg",  "%3", 170, _ky_message )
#define Key_move        __TK("kmov",  "%4", 171, _ky_move )
#define Key_next        __TK("knxt",  "%5", 172, _ky_next )
#define Key_open        __TK("kopn",  "%6", 173, _ky_open )
#define Key_options     __TK("kopt",  "%7", 174, _ky_options )
#define Key_previous    __TK("kprv",  "%8", 175, _ky_previous )
#define Key_print       __TK("kprt",  "%9", 176, _ky_print )
#define Key_redo        __TK("krdo",  "%0", 177, _ky_redo )
#define Key_reference   __TK("kref",  "&1", 178, _ky_reference )
#define Key_refresh     __TK("krfr",  "&2", 179, _ky_refresh )
#define Key_replace     __TK("krpl",  "&3", 180, _ky_replace )
#define Key_restart     __TK("krst",  "&4", 181, _ky_restart )
#define Key_resume      __TK("kres",  "&5", 182, _ky_resume )
#define Key_save        __TK("ksav",  "&6", 183, _ky_save )
#define Key_suspend     __TK("kspd",  "&7", 184, _ky_suspend )
#define Key_undo        __TK("kund",  "&8", 185, _ky_undo )
#define Key_sbeg        __TK("kBEG",  "&9", 186, _ky_sbeg )
#define Key_scancel     __TK("kCAN",  "&0", 187, _ky_scancel )
#define Key_scommand    __TK("kCMD",  "*1", 188, _ky_scommand )
#define Key_scopy       __TK("kCPY",  "*2", 189, _ky_scopy )
#define Key_screate     __TK("kCRT",  "*3", 190, _ky_screate )
#define Key_sdc         __TK("kDC",   "*4", 191, _ky_sdc )
#define Key_sdl         __TK("kDL",   "*5", 192, _ky_sdl )
#define Key_select      __TK("kslt",  "*6", 193, _ky_select )
#define Key_send        __TK("kEND",  "*7", 194, _ky_send )
#define Key_seol        __TK("kEOL",  "*8", 195, _ky_seol )
#define Key_sexit       __TK("kEXT",  "*9", 196, _ky_sexit )
#define Key_sfind       __TK("kFND",  "*0", 197, _ky_sfind )
#define Key_shelp       __TK("kHLP",  "#1", 198, _ky_shelp )
#define Key_shome       __TK("kHOM",  "#2", 199, _ky_shome )
#define Key_sic         __TK("kIC",   "#3", 200, _ky_sic )
#define Key_sleft       __TK("kLFT",  "#4", 201, _ky_sleft )
#define Key_smessage    __TK("kMSG",  "%a", 202, _ky_smessage )
#define Key_smove       __TK("kMOV",  "%b", 203, _ky_smove )
#define Key_snext       __TK("kNXT",  "%c", 204, _ky_snext )
#define Key_soptions    __TK("kOPT",  "%d", 205, _ky_soptions )
#define Key_sprevious   __TK("kPRV",  "%e", 206, _ky_sprevious )
#define Key_sprint      __TK("kPRT",  "%f", 207, _ky_sprint )
#define Key_sredo       __TK("kRDO",  "%g", 208, _ky_sredo )
#define Key_sreplace    __TK("kRPL",  "%h", 209, _ky_sreplace )
#define Key_sright      __TK("kRIT",  "%i", 210, _ky_sright )
#define Key_srsume      __TK("kRES",  "%j", 211, _ky_srsume )
#define Key_ssave       __TK("kSAV",  "!1", 212, _ky_ssave )
#define Key_ssuspend    __TK("kSPD",  "!2", 213, _ky_ssuspend )
#define Key_sundo       __TK("kUND",  "!3", 214, _ky_sundo )
#define Key_f11         __TK("kf11",  "F1", 216, _ky_f11 )
#define Key_f12         __TK("kf12",  "F2", 217, _ky_f12 )
#define Key_f13         __TK("kf13",  "F3", 218, _ky_f13 )
#define Key_f14         __TK("kf14",  "F4", 219, _ky_f14 )
#define Key_f15         __TK("kf15",  "F5", 220, _ky_f15 )
#define Key_f16         __TK("kf16",  "F6", 221, _ky_f16 )
#define Key_f17         __TK("kf17",  "F7", 222, _ky_f17 )
#define Key_f18         __TK("kf18",  "F8", 223, _ky_f18 )
#define Key_f19         __TK("kf19",  "F9", 224, _ky_f19 )
#define Key_f20         __TK("kf20",  "FA", 225, _ky_f20 )
#define Key_f21         __TK("kf21",  "FB", 226, _ky_f21 )
#define Key_f22         __TK("kf22",  "FC", 227, _ky_f22 )
#define Key_f23         __TK("kf23",  "FD", 228, _ky_f23 )
#define Key_f24         __TK("kf24",  "FE", 229, _ky_f24 )
#define Key_f25         __TK("kf25",  "FF", 230, _ky_f25 )
#define Key_f26         __TK("kf26",  "FG", 231, _ky_f26 )
#define Key_f27         __TK("kf27",  "FH", 232, _ky_f27 )
#define Key_f28         __TK("kf28",  "FI", 233, _ky_f28 )
#define Key_f29         __TK("kf29",  "FJ", 234, _ky_f29 )
#define Key_f30         __TK("kf30",  "FK", 235, _ky_f30 )
#define Key_f31         __TK("kf31",  "FL", 236, _ky_f31 )
#define Key_f32         __TK("kf32",  "FM", 237, _ky_f32 )
#define Key_f33         __TK("kf33",  "FN", 238, _ky_f33 )
#define Key_f34         __TK("kf34",  "FO", 239, _ky_f34 )
#define Key_f35         __TK("kf35",  "FP", 240, _ky_f35 )
#define Key_f36         __TK("kf36",  "FQ", 241, _ky_f36 )
#define Key_f37         __TK("kf37",  "FR", 242, _ky_f37 )
#define Key_f38         __TK("kf38",  "FS", 243, _ky_f38 )
#define Key_f39         __TK("kf39",  "FT", 244, _ky_f39 )
#define Key_f40         __TK("kf40",  "FU", 245, _ky_f40 )
#define Key_f41         __TK("kf41",  "FV", 246, _ky_f41 )
#define Key_f42         __TK("kf42",  "FW", 247, _ky_f42 )
#define Key_f43         __TK("kf43",  "FX", 248, _ky_f43 )
#define Key_f44         __TK("kf44",  "FY", 249, _ky_f44 )
#define Key_f45         __TK("kf45",  "FZ", 250, _ky_f45 )
#define Key_f46         __TK("kf46",  "Fa", 251, _ky_f46 )
#define Key_f47         __TK("kf47",  "Fb", 252, _ky_f47 )
#define Key_f48         __TK("kf48",  "Fc", 253, _ky_f48 )
#define Key_f49         __TK("kf49",  "Fd", 254, _ky_f49 )
#define Key_f50         __TK("kf50",  "Fe", 255, _ky_f50 )
#define Key_f51         __TK("kf51",  "Ff", 256, _ky_f51 )
#define Key_f52         __TK("kf52",  "Fg", 257, _ky_f52 )
#define Key_f53         __TK("kf53",  "Fh", 258, _ky_f53 )
#define Key_f54         __TK("kf54",  "Fi", 259, _ky_f54 )
#define Key_f55         __TK("kf55",  "Fj", 260, _ky_f55 )
#define Key_f56         __TK("kf56",  "Fk", 261, _ky_f56 )
#define Key_f57         __TK("kf57",  "Fl", 262, _ky_f57 )
#define Key_f58         __TK("kf58",  "Fm", 263, _ky_f58 )
#define Key_f59         __TK("kf59",  "Fn", 264, _ky_f59 )
#define Key_f60         __TK("kf60",  "Fo", 265, _ky_f60 )
#define Key_f61         __TK("kf61",  "Fp", 266, _ky_f61 )
#define Key_f62         __TK("kf62",  "Fq", 267, _ky_f62 )
#define Key_f63         __TK("kf63",  "Fr", 268, _ky_f63 )

/* }}} */

#ifdef __QNX__

/* don't force pre-defining of base keys under QNX */
#define FORCE_BASE_KEY_DEFS 0
    
/* OS specific key aliases */
#define Key_alt_a       Key_clear
#define Key_alt_b       Key_stab
#define Key_alt_c       Key_close
#define Key_alt_d       Key_catab
#define Key_alt_e       Key_message
#define Key_alt_f       Key_find
#define Key_alt_g       Key_refresh
#define Key_alt_h       Key_help
#define Key_alt_i       Key_move
#define Key_alt_j       Key_restart
#define Key_alt_k       Key_options
#define Key_alt_l       Key_reference
#define Key_alt_m       Key_mark
#define Key_alt_n       Key_sbeg
#define Key_alt_o       Key_open
#define Key_alt_p       Key_resume
#define Key_alt_q       Key_save
#define Key_alt_r       Key_replace
#define Key_alt_s       Key_scopy
#define Key_alt_t       Key_screate
#define Key_alt_u       Key_undo
#define Key_alt_v       Key_sdl
#define Key_alt_w       Key_sexit
#define Key_alt_x       Key_sfind
#define Key_alt_y       Key_shelp
#define Key_alt_z       Key_soptions

#define Key_ctl_enter   Key_enter
#define Key_ctl_tab     Key_ctab

#define Key_alt_tab     Key_ctl_tab         /* map ALT-TAB to CTRL-TAB */
#define Key_alt_enter   Key_ctl_enter       /* map ALT-ENTER to CTRL-ENTER */

#ifdef __USE_QNX_TI
    
/* OS/implementation specific key-define struct */
typedef struct qnx_key_define_s {
    int mc_code;
    int str_idx;
} qnx_key_define_t;

/* define current xtra_key_define_t (enable OS/implementation) */
#define xtra_key_define_t qnx_key_define_t

#endif /* __USE_QNX_TI */

#endif /* __QNX__ */


#ifdef xtra_key_define_t

#ifndef FORCE_BASE_KEY_DEFS
#define FORCE_BASE_KEY_DEFS 0
#endif

/* general key define table */
xtra_key_define_t xtra_key_defines[] = {
#if FORCE_BASE_KEY_DEFS
    {KEY_BACKSPACE,Key_backspace},
    {KEY_LEFT,     Key_left     },
    {KEY_RIGHT,    Key_right    },
    {KEY_UP,       Key_up       },
    {KEY_DOWN,     Key_down     },
    {KEY_NPAGE,    Key_npage    },
    {KEY_PPAGE,    Key_ppage    },
    {KEY_HOME,     Key_home     },
    {KEY_END,      Key_end      },
    {KEY_DC,       Key_dc       },
    {KEY_IC,       Key_ic       },
    {KEY_F(1),     Key_f1       },
    {KEY_F(2),     Key_f2       },
    {KEY_F(3),     Key_f3       },
    {KEY_F(4),     Key_f4       },
    {KEY_F(5),     Key_f5       },
    {KEY_F(6),     Key_f6       },
    {KEY_F(7),     Key_f7       },
    {KEY_F(8),     Key_f8       },
    {KEY_F(9),     Key_f9       },
    {KEY_F(10),    Key_f10      },
    {KEY_F(11),    Key_f11      },
    {KEY_F(12),    Key_f12      },
    {KEY_F(13),    Key_f13      },
    {KEY_F(14),    Key_f14      },
    {KEY_F(15),    Key_f15      },
    {KEY_F(16),    Key_f16      },
    {KEY_F(17),    Key_f17      },
    {KEY_F(18),    Key_f18      },
    {KEY_F(19),    Key_f19      },
    {KEY_F(20),    Key_f20      },
#endif
    {ALT('a'),     Key_alt_a    },
    {ALT('b'),     Key_alt_b    },
    {ALT('c'),     Key_alt_c    },
    {ALT('d'),     Key_alt_d    },
    {ALT('e'),     Key_alt_e    },
    {ALT('f'),     Key_alt_f    },
    {ALT('g'),     Key_alt_g    },
    {ALT('h'),     Key_alt_h    },
    {ALT('i'),     Key_alt_i    },
    {ALT('j'),     Key_alt_j    },
    {ALT('k'),     Key_alt_k    },
    {ALT('l'),     Key_alt_l    },
    {ALT('m'),     Key_alt_m    },
    {ALT('n'),     Key_alt_n    },
    {ALT('o'),     Key_alt_o    },
    {ALT('p'),     Key_alt_p    },
    {ALT('q'),     Key_alt_q    },
    {ALT('r'),     Key_alt_r    },
    {ALT('s'),     Key_alt_s    },
    {ALT('t'),     Key_alt_t    },
    {ALT('u'),     Key_alt_u    },
    {ALT('v'),     Key_alt_v    },
    {ALT('w'),     Key_alt_w    },
    {ALT('x'),     Key_alt_x    },
    {ALT('y'),     Key_alt_y    },
    {ALT('z'),     Key_alt_z    },

    {ALT('\n'),    Key_alt_enter},
    {ALT('\t'),    Key_alt_tab  }
};

#endif  /* xtra_key_define_t */


#ifdef __QNX__

#ifdef __USE_QNX_TI

#define __CT               (__cur_term)
#define __QTISOFFS(_qtisx) (((charoffset*)(&__CT->_strs))[_qtisx])
#define __QTISSTR(_qtisx)  (&__CT->_strtab[0]+__QTISOFFS(_qtisx))

void load_qnx_key_defines (void)
{
    static int _qnx_keys_defined = 0;

    if (!_qnx_keys_defined) {
	int idx, str_idx;
	int term_setup_ok;

        __setupterm(NULL, fileno(stdout), &term_setup_ok);
        if (term_setup_ok != 1)
            return;

        for (idx = 0; 
             idx < sizeof(xtra_key_defines) / sizeof(xtra_key_defines[0]);
             idx++) {
            str_idx = xtra_key_defines[idx].str_idx;
            if (__QTISOFFS(str_idx)) {
                if (*__QTISSTR(str_idx)) {
                    define_sequence(
                        xtra_key_defines[idx].mc_code,
                        __QTISSTR(str_idx),
                        MCKEY_NOACTION);
                }
            }
        }
        _qnx_keys_defined = 1;
    }
}

#endif /* __USE_QNX_TI */

#endif /* __QNX__ */


/* called from key.c/init_key() */
void load_xtra_key_defines (void)
{
#ifdef __QNX__
    load_qnx_key_defines();
#endif
}


