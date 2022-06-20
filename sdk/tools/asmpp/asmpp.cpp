/*
 * PROJECT:     ReactOS host tools
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ASM preprocessor
 * COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

// Optimize even on debug builds, because otherwise it's ridiculously slow
#ifdef _MSC_VER
#pragma optimize("gst", on)
#pragma auto_inline(on)
#else
#pragma GCC optimize("O3,inline")
#endif

#include "tokenizer.hpp"
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <ctime>

#define PROFILING_ENABLED 0

using namespace std;

time_t search_time;

enum TOKEN_TYPE
{
    Invalid = -1,
    Eof,
    WhiteSpace,
    NewLine,
    Comment,
    DecNumber,
    HexNumber,
    String,

    BraceOpen,
    BraceClose,
    MemRefStart,
    MemRefEnd,
    Colon,
    Operator,
    StringDef,

    KW_include,
    KW_const,
    KW_code,
    KW_endprolog,
    KW_ALIGN,
    KW_EXTERN,
    KW_PUBLIC,
    KW_ENDM,
    KW_END,
    KW_if,
    KW_ifdef,
    KW_ifndef,
    KW_else,
    KW_endif,

    KW_allocstack,
    KW_savereg,
    KW_savexmm128,

    KW_DB,
    KW_DW,
    KW_DD,
    KW_DQ,
    KW_EQU,
    KW_TEXTEQU,
    KW_MACRO,
    KW_PROC,
    KW_FRAME,
    KW_ENDP,
    KW_RECORD,

    KW_MASK,
    KW_ERRDEF,

    Filename,
    Instruction,
    Reg8,
    Reg16,
    Reg32,
    Reg64,
    RegXmm,
    BYTE_PTR,
    WORD_PTR,
    DWORD_PTR,
    QWORD_PTR,
    XMMWORD_PTR,

    LabelName,
    Identifier
};

int fake_printf(const char* format, ...)
{
    return 0;
}

//#define printf fake_printf

// Use a look-ahead for following characters, not included into the match
//#define FOLLOWED_BY(x) R"((?=)" x R"())"
#define FOLLOWED_BY(x) x

#define ANY_CHAR R"((?:.|\n))"
#define WHITESPACE R"((?:[ \t]++))"
#define NEWLINE R"([\n])"
#define WS_OR_NL R"((?:)" WHITESPACE "|" NEWLINE R"()+)"
#define SEPARATOR R"([\s,\=\+\-\*\/\:\~\[\]])"

#define INSTRUCTION \
    "AAA|AAD|AAM|AAS|ADC|ADCX|ADD|ADDPD|ADDPS|ADDSD|ADDSS|ADDSUBPD|ADDSUBPS|" \
    "ADOX|AESDEC|AESDECLAST|AESENC|AESENCLAST|AESIMC|AESKEYGENASSIST|AND|ANDN|" \
    "ANDNPD|ANDNPS|ANDPD|ANDPS|ARPL|BEXTR|BLENDPD|BLENDPS|BLENDVPD|BLENDVPS|" \
    "BLSI|BLSMSK|BLSR|BNDCL|BNDCN|BNDCU|BNDLDX|BNDMK|BNDMOV|BNDSTX|BOUND|BSF|" \
    "BSR|BSWAP|BT|BTC|BTR|BTS|BZHI|CALL|CBW|CDQ|CDQE|CLAC|CLC|CLD|CLDEMOTE|" \
    "CLFLUSH|CLFLUSHOPT|CLI|CLTS|CLWB|CMC|CMOVcc|CMP|CMPPD|CMPPS|CMPS|CMPSB|" \
    "CMPSD|CMPSQ|CMPSS|CMPSW|CMPXCHG|CMPXCHG16B|CMPXCHG8B|COMISD|COMISS|CPUID|" \
    "CQO|CRC32|CVTDQ2PD|CVTDQ2PS|CVTPD2DQ|CVTPD2PI|CVTPD2PS|CVTPI2PD|CVTPI2PS|" \
    "CVTPS2DQ|CVTPS2PD|CVTPS2PI|CVTSD2SI|CVTSD2SS|CVTSI2SD|CVTSI2SS|CVTSS2SD|" \
    "CVTSS2SI|CVTTPD2DQ|CVTTPD2PI|CVTTPS2DQ|CVTTPS2PI|CVTTSD2SI|CVTTSS2SI|CWD|" \
    "CWDE|DAA|DAS|DEC|DIV|DIVPD|DIVPS|DIVSD|DIVSS|DPPD|DPPS|EMMS|ENTER|" \
    "EXTRACTPS|F2XM1|FABS|FADD|FADDP|FBLD|FBSTP|FCHS|FCLEX|FCMOVcc|FCOM|FCOMI|" \
    "FCOMIP|FCOMP|FCOMPP|FCOS|FDECSTP|FDIV|FDIVP|FDIVR|FDIVRP|FFREE|FIADD|" \
    "FICOM|FICOMP|FIDIV|FIDIVR|FILD|FIMUL|FINCSTP|FINIT|FIST|FISTP|FISTTP|" \
    "FISUB|FISUBR|FLD|FLD1|FLDCW|FLDENV|FLDL2E|FLDL2T|FLDLG2|FLDLN2|FLDPI|" \
    "FLDZ|FMUL|FMULP|FNCLEX|FNINIT|FNOP|FNSAVE|FNSTCW|FNSTENV|FNSTSW|FPATAN|" \
    "FPREM|FPREM1|FPTAN|FRNDINT|FRSTOR|FSAVE|FSCALE|FSIN|FSINCOS|FSQRT|FST|" \
    "FSTCW|FSTENV|FSTP|FSTSW|FSUB|FSUBP|FSUBR|FSUBRP|FTST|FUCOM|FUCOMI|" \
    "FUCOMIP|FUCOMP|FUCOMPP|FWAIT|FXAM|FXCH|FXRSTOR|FXSAVE|FXTRACT|FYL2X|" \
    "FYL2XP1|GF2P8AFFINEINVQB|GF2P8AFFINEQB|GF2P8MULB|HADDPD|HADDPS|HLT|" \
    "HSUBPD|HSUBPS|IDIV|IMUL|IN|INC|INS|INSB|INSD|INSERTPS|INSW|INT|INT1|INT3|" \
    "INTO|INVD|INVLPG|INVPCID|IRET|IRETD|JMP|Jcc|KADDB|KADDD|KADDQ|KADDW|" \
    "KANDB|KANDD|KANDNB|KANDND|KANDNQ|KANDNW|KANDQ|KANDW|KMOVB|KMOVD|KMOVQ|" \
    "KMOVW|KNOTB|KNOTD|KNOTQ|KNOTW|KORB|KORD|KORQ|KORTESTB|KORTESTD|KORTESTQ|" \
    "KORTESTW|KORW|KSHIFTLB|KSHIFTLD|KSHIFTLQ|KSHIFTLW|KSHIFTRB|KSHIFTRD|" \
    "KSHIFTRQ|KSHIFTRW|KTESTB|KTESTD|KTESTQ|KTESTW|KUNPCKBW|KUNPCKDQ|KUNPCKWD|" \
    "KXNORB|KXNORD|KXNORQ|KXNORW|KXORB|KXORD|KXORQ|KXORW|LAHF|LAR|LDDQU|" \
    "LDMXCSR|LDS|LEA|LEAVE|LES|LFENCE|LFS|LGDT|LGS|LIDT|LLDT|LMSW|LOCK|LODS|" \
    "LODSB|LODSD|LODSQ|LODSW|LOOP|LOOPcc|LSL|LSS|LTR|LZCNT|MASKMOVDQU|MASKMOVQ|" \
    "MAXPD|MAXPS|MAXSD|MAXSS|MFENCE|MINPD|MINPS|MINSD|MINSS|MONITOR|MOV|MOVAPD|" \
    "MOVAPS|MOVBE|MOVD|MOVDDUP|MOVDIR64B|MOVDIRI|MOVDQ2Q|MOVDQA|MOVDQU|MOVHLPS|" \
    "MOVHPD|MOVHPS|MOVLHPS|MOVLPD|MOVLPS|MOVMSKPD|MOVMSKPS|MOVNTDQ|MOVNTDQA|" \
    "MOVNTI|MOVNTPD|MOVNTPS|MOVNTQ|MOVQ|MOVQ2DQ|MOVS|MOVSB|MOVSD|MOVSHDUP|" \
    "MOVSLDUP|MOVSQ|MOVSS|MOVSW|MOVSX|MOVSXD|MOVUPD|MOVUPS|MOVZX|MPSADBW|MUL|" \
    "MULPD|MULPS|MULSD|MULSS|MULX|MWAIT|NEG|NOP|NOT|OR|ORPD|ORPS|OUT|OUTS|" \
    "OUTSB|OUTSD|OUTSW|PABSB|PABSD|PABSQ|PABSW|PACKSSDW|PACKSSWB|PACKUSDW|" \
    "PACKUSWB|PADDB|PADDD|PADDQ|PADDSB|PADDSW|PADDUSB|PADDUSW|PADDW|PALIGNR|" \
    "PAND|PANDN|PAUSE|PAVGB|PAVGW|PBLENDVB|PBLENDW|PCLMULQDQ|PCMPEQB|PCMPEQD|" \
    "PCMPEQQ|PCMPEQW|PCMPESTRI|PCMPESTRM|PCMPGTB|PCMPGTD|PCMPGTQ|PCMPGTW|" \
    "PCMPISTRI|PCMPISTRM|PDEP|PEXT|PEXTRB|PEXTRD|PEXTRQ|PEXTRW|PHADDD|PHADDSW|" \
    "PHADDW|PHMINPOSUW|PHSUBD|PHSUBSW|PHSUBW|PINSRB|PINSRD|PINSRQ|PINSRW|" \
    "PMADDUBSW|PMADDWD|PMAXSB|PMAXSD|PMAXSQ|PMAXSW|PMAXUB|PMAXUD|PMAXUQ|PMAXUW|" \
    "PMINSB|PMINSD|PMINSQ|PMINSW|PMINUB|PMINUD|PMINUQ|PMINUW|PMOVMSKB|PMOVSX|" \
    "PMOVZX|PMULDQ|PMULHRSW|PMULHUW|PMULHW|PMULLD|PMULLQ|PMULLW|PMULUDQ|POP|" \
    "POPA|POPAD|POPCNT|POPF|POPFD|POPFQ|POR|PREFETCHW|PREFETCHh|PSADBW|PSHUFB|" \
    "PSHUFD|PSHUFHW|PSHUFLW|PSHUFW|PSIGNB|PSIGND|PSIGNW|PSLLD|PSLLDQ|PSLLQ|" \
    "PSLLW|PSRAD|PSRAQ|PSRAW|PSRLD|PSRLDQ|PSRLQ|PSRLW|PSUBB|PSUBD|PSUBQ|PSUBSB|" \
    "PSUBSW|PSUBUSB|PSUBUSW|PSUBW|PTEST|PTWRITE|PUNPCKHBW|PUNPCKHDQ|PUNPCKHQDQ|" \
    "PUNPCKHWD|PUNPCKLBW|PUNPCKLDQ|PUNPCKLQDQ|PUNPCKLWD|PUSH|PUSHA|PUSHAD|" \
    "PUSHF|PUSHFD|PUSHFQ|PXOR|RCL|RCPPS|RCPSS|RCR|RDFSBASE|RDGSBASE|RDMSR|" \
    "RDPID|RDPKRU|RDPMC|RDRAND|RDSEED|RDTSC|RDTSCP|REP|REPE|REPNE|REPNZ|REPZ|" \
    "RET|ROL|ROR|RORX|ROUNDPD|ROUNDPS|ROUNDSD|ROUNDSS|RSM|RSQRTPS|RSQRTSS|SAHF|" \
    "SAL|SAR|SARX|SBB|SCAS|SCASB|SCASD|SCASW|SETcc|SFENCE|SGDT|SHA1MSG1|" \
    "SHA1MSG2|SHA1NEXTE|SHA1RNDS4|SHA256MSG1|SHA256MSG2|SHA256RNDS2|SHL|SHLD|" \
    "SHLX|SHR|SHRD|SHRX|SHUFPD|SHUFPS|SIDT|SLDT|SMSW|SQRTPD|SQRTPS|SQRTSD|" \
    "SQRTSS|STAC|STC|STD|STI|STMXCSR|STOS|STOSB|STOSD|STOSQ|STOSW|STR|SUB|" \
    "SUBPD|SUBPS|SUBSD|SUBSS|SWAPGS|SYSCALL|SYSENTER|SYSEXIT|SYSRET|TEST|" \
    "TPAUSE|TZCNT|UCOMISD|UCOMISS|UD|UMONITOR|UMWAIT|UNPCKHPD|UNPCKHPS|" \
    "UNPCKLPD|UNPCKLPS|VALIGND|VALIGNQ|VBLENDMPD|VBLENDMPS|VBROADCAST|" \
    "VCOMPRESSPD|VCOMPRESSPS|VCVTPD2QQ|VCVTPD2UDQ|VCVTPD2UQQ|VCVTPH2PS|" \
    "VCVTPS2PH|VCVTPS2QQ|VCVTPS2UDQ|VCVTPS2UQQ|VCVTQQ2PD|VCVTQQ2PS|VCVTSD2USI|" \
    "VCVTSS2USI|VCVTTPD2QQ|VCVTTPD2UDQ|VCVTTPD2UQQ|VCVTTPS2QQ|VCVTTPS2UDQ|" \
    "VCVTTPS2UQQ|VCVTTSD2USI|VCVTTSS2USI|VCVTUDQ2PD|VCVTUDQ2PS|VCVTUQQ2PD|" \
    "VCVTUQQ2PS|VCVTUSI2SD|VCVTUSI2SS|VDBPSADBW|VERR|VERW|VEXPANDPD|VEXPANDPS|" \
    "VEXTRACTF128|VEXTRACTF32x4|VEXTRACTF32x8|VEXTRACTF64x2|VEXTRACTF64x4|" \
    "VEXTRACTI128|VEXTRACTI32x4|VEXTRACTI32x8|VEXTRACTI64x2|VEXTRACTI64x4|" \
    "VFIXUPIMMPD|VFIXUPIMMPS|VFIXUPIMMSD|VFIXUPIMMSS|VFMADD132PD|VFMADD132PS|" \
    "VFMADD132SD|VFMADD132SS|VFMADD213PD|VFMADD213PS|VFMADD213SD|VFMADD213SS|" \
    "VFMADD231PD|VFMADD231PS|VFMADD231SD|VFMADD231SS|VFMADDSUB132PD|" \
    "VFMADDSUB132PS|VFMADDSUB213PD|VFMADDSUB213PS|VFMADDSUB231PD|" \
    "VFMADDSUB231PS|VFMSUB132PD|VFMSUB132PS|VFMSUB132SD|VFMSUB132SS|" \
    "VFMSUB213PD|VFMSUB213PS|VFMSUB213SD|VFMSUB213SS|VFMSUB231PD|VFMSUB231PS|" \
    "VFMSUB231SD|VFMSUB231SS|VFMSUBADD132PD|VFMSUBADD132PS|VFMSUBADD213PD|" \
    "VFMSUBADD213PS|VFMSUBADD231PD|VFMSUBADD231PS|VFNMADD132PD|VFNMADD132PS|" \
    "VFNMADD132SD|VFNMADD132SS|VFNMADD213PD|VFNMADD213PS|VFNMADD213SD|" \
    "VFNMADD213SS|VFNMADD231PD|VFNMADD231PS|VFNMADD231SD|VFNMADD231SS|" \
    "VFNMSUB132PD|VFNMSUB132PS|VFNMSUB132SD|VFNMSUB132SS|VFNMSUB213PD|" \
    "VFNMSUB213PS|VFNMSUB213SD|VFNMSUB213SS|VFNMSUB231PD|VFNMSUB231PS|" \
    "VFNMSUB231SD|VFNMSUB231SS|VFPCLASSPD|VFPCLASSPS|VFPCLASSSD|VFPCLASSSS|" \
    "VGATHERDPD|VGATHERDPS|VGATHERQPD|VGATHERQPS|VGETEXPPD|VGETEXPPS|VGETEXPSD|" \
    "VGETEXPSS|VGETMANTPD|VGETMANTPS|VGETMANTSD|VGETMANTSS|VINSERTF128|" \
    "VINSERTF32x4|VINSERTF32x8|VINSERTF64x2|VINSERTF64x4|VINSERTI128|" \
    "VINSERTI32x4|VINSERTI32x8|VINSERTI64x2|VINSERTI64x4|VMASKMOV|VMOVDQA32|" \
    "VMOVDQA64|VMOVDQU16|VMOVDQU32|VMOVDQU64|VMOVDQU8|VPBLENDD|VPBLENDMB|" \
    "VPBLENDMD|VPBLENDMQ|VPBLENDMW|VPBROADCAST|VPBROADCASTB|VPBROADCASTD|" \
    "VPBROADCASTM|VPBROADCASTQ|VPBROADCASTW|VPCMPB|VPCMPD|VPCMPQ|VPCMPUB|" \
    "VPCMPUD|VPCMPUQ|VPCMPUW|VPCMPW|VPCOMPRESSD|VPCOMPRESSQ|VPCONFLICTD|" \
    "VPCONFLICTQ|VPERM2F128|VPERM2I128|VPERMB|VPERMD|VPERMI2B|VPERMI2D|" \
    "VPERMI2PD|VPERMI2PS|VPERMI2Q|VPERMI2W|VPERMILPD|VPERMILPS|VPERMPD|VPERMPS|" \
    "VPERMQ|VPERMT2B|VPERMT2D|VPERMT2PD|VPERMT2PS|VPERMT2Q|VPERMT2W|VPERMW|" \
    "VPEXPANDD|VPEXPANDQ|VPGATHERDD|VPGATHERDQ|VPGATHERQD|VPGATHERQQ|VPLZCNTD|" \
    "VPLZCNTQ|VPMADD52HUQ|VPMADD52LUQ|VPMASKMOV|VPMOVB2M|VPMOVD2M|VPMOVDB|" \
    "VPMOVDW|VPMOVM2B|VPMOVM2D|VPMOVM2Q|VPMOVM2W|VPMOVQ2M|VPMOVQB|VPMOVQD|" \
    "VPMOVQW|VPMOVSDB|VPMOVSDW|VPMOVSQB|VPMOVSQD|VPMOVSQW|VPMOVSWB|VPMOVUSDB|" \
    "VPMOVUSDW|VPMOVUSQB|VPMOVUSQD|VPMOVUSQW|VPMOVUSWB|VPMOVW2M|VPMOVWB|" \
    "VPMULTISHIFTQB|VPROLD|VPROLQ|VPROLVD|VPROLVQ|VPRORD|VPRORQ|VPRORVD|" \
    "VPRORVQ|VPSCATTERDD|VPSCATTERDQ|VPSCATTERQD|VPSCATTERQQ|VPSLLVD|VPSLLVQ|" \
    "VPSLLVW|VPSRAVD|VPSRAVQ|VPSRAVW|VPSRLVD|VPSRLVQ|VPSRLVW|VPTERNLOGD|" \
    "VPTERNLOGQ|VPTESTMB|VPTESTMD|VPTESTMQ|VPTESTMW|VPTESTNMB|VPTESTNMD|" \
    "VPTESTNMQ|VPTESTNMW|VRANGEPD|VRANGEPS|VRANGESD|VRANGESS|VRCP14PD|VRCP14PS|" \
    "VRCP14SD|VRCP14SS|VREDUCEPD|VREDUCEPS|VREDUCESD|VREDUCESS|VRNDSCALEPD|" \
    "VRNDSCALEPS|VRNDSCALESD|VRNDSCALESS|VRSQRT14PD|VRSQRT14PS|VRSQRT14SD|" \
    "VRSQRT14SS|VSCALEFPD|VSCALEFPS|VSCALEFSD|VSCALEFSS|VSCATTERDPD|" \
    "VSCATTERDPS|VSCATTERQPD|VSCATTERQPS|VSHUFF32x4|VSHUFF64x2|VSHUFI32x4|" \
    "VSHUFI64x2|VTESTPD|VTESTPS|VZEROALL|VZEROUPPER|WAIT|WBINVD|WRFSBASE|" \
    "WRGSBASE|WRMSR|WRPKRU|XABORT|XACQUIRE|XADD|XBEGIN|XCHG|XEND|XGETBV|XLAT|" \
    "XLATB|XOR|XORPD|XORPS|XRELEASE|XRSTOR|XRSTORS|XSAVE|XSAVEC|XSAVEOPT|" \
    "XSAVES|XSETBV|XTEST"

vector<TOKEN_DEF> g_TokenList =
{
    //{ TOKEN_TYPE::WhiteSpace, R"((\s+))" },
    { TOKEN_TYPE::WhiteSpace, R"(([ \t]+))" },
    { TOKEN_TYPE::NewLine, R"((\n))" },
    { TOKEN_TYPE::Comment, R"((;.*\n))" },
    { TOKEN_TYPE::HexNumber, R"(([0-9][0-9a-f]*h))" FOLLOWED_BY(R"([\s\n\+\-\*\/,=!\]\(\)])") },
    { TOKEN_TYPE::DecNumber, R"(([0-9]+))" FOLLOWED_BY(R"([\s\n\+\-\*\/,=!\]\(\)])") },
    { TOKEN_TYPE::String, R"((\".*\"))" },

    { TOKEN_TYPE::BraceOpen, R"((\())"},
    { TOKEN_TYPE::BraceClose, R"((\)))"},
    { TOKEN_TYPE::MemRefStart, R"((\[))"},
    { TOKEN_TYPE::MemRefEnd, R"((\]))"},
    { TOKEN_TYPE::Colon, R"((\:))"},
    { TOKEN_TYPE::Operator, R"(([,\+\-\*\/\:]))"},
    { TOKEN_TYPE::StringDef, R"((<.+>))" },

    { TOKEN_TYPE::KW_include, R"((include))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_const, R"((\.const))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_code, R"((\.code))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_endprolog, R"((\.endprolog))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_ALIGN, R"((ALIGN))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_EXTERN, R"((EXTERN))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_EXTERN, R"((EXTRN))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_PUBLIC, R"((PUBLIC))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_ENDM, R"((ENDM))" FOLLOWED_BY(R"([\s\;])") },
    { TOKEN_TYPE::KW_END, R"((END))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_if, R"((if))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_ifdef, R"((ifdef))" FOLLOWED_BY(R"([\s])")},
    { TOKEN_TYPE::KW_ifndef, R"((ifndef))" FOLLOWED_BY(R"([\s])")},
    { TOKEN_TYPE::KW_else, R"((else))" FOLLOWED_BY(R"([\s])")},
    { TOKEN_TYPE::KW_endif, R"((endif))" FOLLOWED_BY(R"([\s])")},

    { TOKEN_TYPE::KW_allocstack, R"((.allocstack))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_savereg, R"((.savereg))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_savexmm128, R"((.savexmm128))" FOLLOWED_BY(R"([\s])") },

    { TOKEN_TYPE::KW_DB, R"((DB))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_DW, R"((DW))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_DD, R"((DD))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_DQ, R"((DQ))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_EQU, R"((EQU))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_TEXTEQU, R"((TEXTEQU))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::KW_MACRO, R"((MACRO))" FOLLOWED_BY(R"([\s\;])") },
    { TOKEN_TYPE::KW_PROC, R"((PROC))" FOLLOWED_BY(R"([\s\;])") },
    { TOKEN_TYPE::KW_FRAME, R"((FRAME))" FOLLOWED_BY(R"([\s\;])") },
    { TOKEN_TYPE::KW_ENDP, R"((ENDP))" FOLLOWED_BY(R"([\s\;])") },
    { TOKEN_TYPE::KW_RECORD, R"((RECORD))" FOLLOWED_BY(R"([\s\;])") },
    { TOKEN_TYPE::KW_MASK, R"((MASK))" FOLLOWED_BY(R"([\s\;])")},
    { TOKEN_TYPE::KW_ERRDEF, R"((\.ERRDEF))" FOLLOWED_BY(R"([\s\;])")},

    { TOKEN_TYPE::Filename, R"(([a-z_][a-z0-9_]*\.inc))" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::Instruction, "(" INSTRUCTION ")" FOLLOWED_BY(R"([\s])") },
    { TOKEN_TYPE::Reg8, R"((al|ah|bl|bh|cl|ch|dl|dh|sil|dil|bpl|spl|r8b|r9b|r10b|r11b|r12b|r13b|r14b|r15b))" FOLLOWED_BY(R"([\s\,])") },
    { TOKEN_TYPE::Reg16, R"((ax|bx|cx|dx|si|di|bp|sp|r8w|r9w|r10w|r11w|r12w|r13w|r14w|r15w))" FOLLOWED_BY(R"([\s\,])") },
    { TOKEN_TYPE::Reg32, R"((eax|ebx|ecx|edx|esi|edi|ebp|esp|r8d|r9d|r10d|r11d|r12d|r13d|r14d|r15d))" FOLLOWED_BY(R"([\s\,])") },
    { TOKEN_TYPE::Reg64, R"((rax|rbx|rcx|rdx|rsi|rdi|rbp|rsp|r8|r9|r10|r11|r12|r13|r14|r15))" FOLLOWED_BY(R"([\s\,])") },
    { TOKEN_TYPE::RegXmm, R"((xmm0|xmm1|xmm2|xmm3|xmm4|xmm5|xmm6|xmm7|xmm8|xmm9|xmm10|xmm11|xmm12|xmm13|xmm14|xmm15))" FOLLOWED_BY(R"([\s\,])") },
    { TOKEN_TYPE::BYTE_PTR, R"((BYTE[\s]+PTR))" FOLLOWED_BY(R"([\s\[])") },
    { TOKEN_TYPE::WORD_PTR, R"((WORD[\s]+PTR))" FOLLOWED_BY(R"([\s\[])") },
    { TOKEN_TYPE::DWORD_PTR, R"((DWORD[\s]+PTR))" FOLLOWED_BY(R"([\s\[])") },
    { TOKEN_TYPE::QWORD_PTR, R"((QWORD[\s]+PTR))" FOLLOWED_BY(R"([\s\[])") },
    { TOKEN_TYPE::XMMWORD_PTR, R"((XMMWORD[\s]+PTR))" FOLLOWED_BY(R"([\s\[])") },

    { TOKEN_TYPE::Identifier, R"((@@))" FOLLOWED_BY(SEPARATOR)},
    { TOKEN_TYPE::Identifier, R"((@[a-z_][a-z0-9_]*))" FOLLOWED_BY(SEPARATOR)},
    { TOKEN_TYPE::Identifier, R"(([a-z_][a-z0-9_]*))" FOLLOWED_BY(SEPARATOR)},

};

// FIXME: use context?
unsigned int g_label_number = 0;

vector<string> g_identifiers;

void
add_mem_id(Token& tok)
{
    g_identifiers.push_back(tok.str());
    //fprintf(stderr, "Added mem id: '%s'\n", tok.str().c_str());
}

bool
is_mem_id(Token& tok)
{
    for (auto id : g_identifiers)
    {
        if (id == tok.str())
        {
            return true;
        }
    }

    return false;
}

bool
iequals(const string &a, const string &b)
{
    size_t sz = a.size();
    if (b.size() != sz)
        return false;
    for (unsigned int i = 0; i < sz; ++i)
        if (tolower(a[i]) != tolower(b[i]))
            return false;
    return true;
}

Token
get_expected_token(Token&& tok, TOKEN_TYPE type)
{
    if (tok.type() != type)
    {
        throw "Not white space after identifier!\n";
    }

    return tok;
}

Token get_ws(Token&& tok)
{
    int type = tok.type();
    if (type != TOKEN_TYPE::WhiteSpace)
    {
        throw "Not white space after identifier!\n";
    }

    return tok;
}

Token get_ws_or_nl(Token&& tok)
{
    int type = tok.type();
    if ((type != TOKEN_TYPE::WhiteSpace) &&
        (type != TOKEN_TYPE::NewLine))
    {
        throw "Not white space after identifier!\n";
    }

    return tok;
}

bool is_string_in_list(vector<string> list, string str)
{
    for (string &s : list)
    {
        if (s == str)
        {
            return true;
        }
    }

    return false;
}

size_t
translate_token(TokenList& tokens, size_t index, const vector<string> &macro_params)
{
    Token tok = tokens[index];
    switch (tok.type())
    {
        case TOKEN_TYPE::Comment:
            printf("//%s", tok.str().c_str() + 1);
            break;

        case TOKEN_TYPE::DecNumber:
        {
            unsigned long long num = stoull(tok.str(), nullptr, 10);
            printf("%llu", num);
            break;
        }

        case TOKEN_TYPE::HexNumber:
        {
            string number = tok.str();
            printf("0x%s", number.substr(0, number.size() - 1).c_str());
            break;
        }

        case TOKEN_TYPE::Identifier:
            if (is_string_in_list(macro_params, tok.str()))
            {
                printf("\\");
            }
            printf("%s", tok.str().c_str());
            break;

        // We migt want to improve these
        case TOKEN_TYPE::BYTE_PTR:
        case TOKEN_TYPE::WORD_PTR:
        case TOKEN_TYPE::DWORD_PTR:
        case TOKEN_TYPE::QWORD_PTR:
        case TOKEN_TYPE::XMMWORD_PTR:

        // Check these. valid only in instructions?
        case TOKEN_TYPE::Reg8:
        case TOKEN_TYPE::Reg16:
        case TOKEN_TYPE::Reg32:
        case TOKEN_TYPE::Reg64:
        case TOKEN_TYPE::RegXmm:
        case TOKEN_TYPE::Instruction:

        case TOKEN_TYPE::WhiteSpace:
        case TOKEN_TYPE::NewLine:
        case TOKEN_TYPE::Operator:
            printf("%s", tok.str().c_str());
            break;

        default:
            printf("%s", tok.str().c_str());
            break;
    }

    return index + 1;
}

size_t complete_line(TokenList &tokens, size_t index, const vector<string> &macro_params)
{
    while (index < tokens.size())
    {
        Token tok = tokens[index];
        index = translate_token(tokens, index, macro_params);
        if ((tok.type() == TOKEN_TYPE::NewLine) ||
            (tok.type() == TOKEN_TYPE::Comment))
        {
            break;
        }
    }

    return index;
}

size_t
translate_expression(TokenList &tokens, size_t index, const vector<string> &macro_params)
{
    while (index < tokens.size())
    {
        Token tok = tokens[index];
        switch (tok.type())
        {
            case TOKEN_TYPE::NewLine:
            case TOKEN_TYPE::Comment:
                return index;

            case TOKEN_TYPE::KW_MASK:
                printf("MASK_");
                index += 2;
                break;

            case TOKEN_TYPE::Instruction:
                if (iequals(tok.str(), "and"))
                {
                    printf("&");
                    index += 1;
                }
                else if (iequals(tok.str(), "or"))
                {
                    printf("|");
                    index += 1;
                }
                else if (iequals(tok.str(), "shl"))
                {
                    printf("<<");
                    index += 1;
                }
                else if (iequals(tok.str(), "not"))
                {
                    printf("!");
                    index += 1;
                }
                else
                {
                    throw "Invalid expression";
                }
                break;

            case TOKEN_TYPE::Operator:
                if (tok.str() == ",")
                {
                    return index;
                }
            case TOKEN_TYPE::WhiteSpace:
            case TOKEN_TYPE::BraceOpen:
            case TOKEN_TYPE::BraceClose:
            case TOKEN_TYPE::DecNumber:
            case TOKEN_TYPE::HexNumber:
            case TOKEN_TYPE::Identifier:
                index = translate_token(tokens, index, macro_params);
                break;

            default:
                index = translate_token(tokens, index, macro_params);
        }
    }

    return index;
}

size_t translate_mem_ref(TokenList& tokens, size_t index, const vector<string>& macro_params)
{
    unsigned int offset = 0;

    Token tok = tokens[index];

    if ((tok.type() == TOKEN_TYPE::DecNumber) ||
        (tok.type() == TOKEN_TYPE::HexNumber))
    {
        offset = stoi(tok.str(), nullptr, 0);
        index += 2;
    }

    index = translate_token(tokens, index, macro_params);

    while (index < tokens.size())
    {
        Token tok = tokens[index];
        index = translate_token(tokens, index, macro_params);
        if (tok.type() == TOKEN_TYPE::MemRefEnd)
        {
            if (offset != 0)
            {
                printf(" + %u", offset);
            }
            return index;
        }
    }

    throw "Failed to translate memory ref";
    return index;
}

size_t translate_instruction_param(TokenList& tokens, size_t index, const vector<string>& macro_params)
{
    switch (tokens[index].type())
    {
        case TOKEN_TYPE::BYTE_PTR:
        case TOKEN_TYPE::WORD_PTR:
        case TOKEN_TYPE::DWORD_PTR:
        case TOKEN_TYPE::QWORD_PTR:
        case TOKEN_TYPE::XMMWORD_PTR:
            index = translate_token(tokens, index, macro_params);

            // Optional whitespace
            if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
            {
                index = translate_token(tokens, index, macro_params);
            }
    }

    while (index < tokens.size())
    {
        Token tok = tokens[index];
        switch (tok.type())
        {
            case TOKEN_TYPE::MemRefStart:
                return translate_mem_ref(tokens, index, macro_params);

            case TOKEN_TYPE::NewLine:
            case TOKEN_TYPE::Comment:
                return index;

            case TOKEN_TYPE::Operator:
                if (tok.str() == ",")
                    return index;

            case TOKEN_TYPE::Identifier:
                index = translate_token(tokens, index, macro_params);
                if (is_mem_id(tok))
                {
                    printf("[rip]");
                }
                break;

            default:
                index = translate_expression(tokens, index, macro_params);
        }
    }

    return index;
}

size_t translate_instruction(TokenList& tokens, size_t index, const vector<string>& macro_params)
{
    // Translate the instruction itself
    index = translate_token(tokens, index, macro_params);

    // Handle instruction parameters
    while (index < tokens.size())
    {
        // Optional whitespace
        if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
        {
            index = translate_token(tokens, index, macro_params);
        }

        // Check for parameters
        Token tok = tokens[index];
        switch (tok.type())
        {
            case TOKEN_TYPE::Comment:
            case TOKEN_TYPE::NewLine:
                return index;

            case TOKEN_TYPE::WhiteSpace:
            case TOKEN_TYPE::Operator:
                index = translate_token(tokens, index, macro_params);
                break;

            default:
                index = translate_instruction_param(tokens, index, macro_params);
                break;
        }
    }

    return index;
}

size_t translate_item(TokenList& tokens, size_t index, const vector<string> &macro_params)
{
    switch (tokens[index].type())
    {
        case TOKEN_TYPE::DecNumber:
        case TOKEN_TYPE::HexNumber:
        case TOKEN_TYPE::String:
        case TOKEN_TYPE::WhiteSpace:
            return translate_token(tokens, index, macro_params);
    }

    throw "Failed to translate item";
    return -1;
}

size_t translate_list(TokenList& tokens, size_t index, const vector<string> &macro_params)
{
    while (index < tokens.size())
    {
        // The item itself
        index = translate_item(tokens, index, macro_params);

        // Optional white space
        if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
        {
            index = translate_token(tokens, index, macro_params);
        }

        // End of list?
        if ((tokens[index].type() == TOKEN_TYPE::Comment) ||
            (tokens[index].type() == TOKEN_TYPE::NewLine))
        {
            return index;
        }

        // We expect a comma here
        if ((tokens[index].type() != TOKEN_TYPE::Operator) ||
            (tokens[index].str() != ","))
        {
            throw "Unexpected end of list";
        }

        index = translate_token(tokens, index, macro_params);
        if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
        {
            index = translate_token(tokens, index, macro_params);
        }
    }

    throw "Failed to translate list";
    return -1;
}

size_t
translate_data_def(TokenList& tokens, size_t index, const vector<string>& macro_params)
{
    Token tok = tokens[index];
    Token tok1 = get_ws(tokens[index + 1]);
    string directive, need, have ="";

    switch (tok.type())
    {
        case TOKEN_TYPE::KW_DB:
            directive = ".byte";
            break;

        case TOKEN_TYPE::KW_DW:
            directive = ".short";
            break;

        case TOKEN_TYPE::KW_DD:
            directive = ".long";
            break;

        case TOKEN_TYPE::KW_DQ:
            directive = ".quad";
            break;
    }

    index += 2;

    while (index < tokens.size())
    {
        // Check if we need '.ascii' for ASCII strings
        if (tokens[index].str()[0] == '\"')
        {
            need = ".ascii";
        }
        else
        {
            need = directive;
        }

        // Output the directive we need (or a comma)
        if (have == "")
        {
            printf("%s ", need.c_str());
        }
        else if (have != need)
        {
            printf("\n%s ", need.c_str());
        }
        else
        {
            printf(", ");
        }

        have = need;

        // The item itself
        index = translate_item(tokens, index, macro_params);

        // Optional white space
        if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
        {
            index = translate_token(tokens, index, macro_params);
        }

        // End of list?
        if ((tokens[index].type() == TOKEN_TYPE::Comment) ||
            (tokens[index].type() == TOKEN_TYPE::NewLine))
        {
            return index;
        }

        // We expect a comma here
        if ((tokens[index].type() != TOKEN_TYPE::Operator) ||
            (tokens[index].str() != ","))
        {
            throw "Unexpected end of list";
        }

        // Skip comma and optional white-space
        index++;
        if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
        {
            index++;
        }
    }

    throw "Failed to translate list";
    return -1;
}

size_t
translate_construct_one_param(string translated, TokenList& tokens, size_t index, const vector<string>& macro_params)
{
    // The next token should be white space
    Token tok1 = get_ws(tokens[index + 1]);

    printf("%s%s", translated.c_str(), tok1.str().c_str());
    return translate_expression(tokens, index + 2, macro_params);
}

size_t
translate_record(TokenList &tokens, size_t index, const vector<string> &macro_params)
{
    unsigned int bits, bitpos = 0;
    unsigned long long oldmask = 0, mask = 0;

    Token tok_name = get_expected_token(tokens[index], TOKEN_TYPE::Identifier);
    index += 4;
    while (index < tokens.size())
    {
        Token tok_member = get_expected_token(tokens[index++], TOKEN_TYPE::Identifier);

        if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
        {
            index++;
        }

        if (tokens[index++].str() != ":")
        {
            throw "Unexpected token";
        }

        if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
        {
            index++;
        }

        Token tok_bits = tokens[index++];
        if ((tok_bits.type() != TOKEN_TYPE::DecNumber) &&
            (tok_bits.type() != TOKEN_TYPE::HexNumber))
        {
            throw "Unexpected token";
        }

        bits = stoi(tok_bits.str(), nullptr, 0);

        printf("%s = %u\n", tok_member.str().c_str(), bitpos);

        oldmask = (1ULL << bitpos) - 1;
        bitpos += bits;
        mask = (1ULL << bitpos) - 1 - oldmask;
        printf("MASK_%s = 0x%llx\n", tok_member.str().c_str(), mask);

        if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
        {
            index++;
        }

        if ((tokens[index].type() == TOKEN_TYPE::NewLine) ||
            (tokens[index].type() == TOKEN_TYPE::Comment))
        {
            break;
        }

        if (tokens[index].str() != ",")
        {
            throw "unexpected token";
        }

        index++;
        if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
        {
            index++;
        }

        if ((tokens[index].type() == TOKEN_TYPE::NewLine) ||
            (tokens[index].type() == TOKEN_TYPE::Comment))
        {
            index++;
        }

        if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
        {
            index++;
        }
    }

    return index;
}

size_t
translate_identifier_construct(TokenList& tokens, size_t index, const vector<string> &macro_params)
{
    Token tok = tokens[index];
    Token tok1 = tokens[index + 1];

    if (tok1.type() == TOKEN_TYPE::Colon)
    {
        if (tok.str() == "@@")
        {
            g_label_number++;
            printf("%u:", g_label_number);
        }
        else
        {
            printf("%s:", tok.str().c_str());
        }
        return index + 2;
    }

    Token tok2 = tokens[index + 2];

    switch (tok2.type())
    {
        case TOKEN_TYPE::KW_MACRO:
            throw "Cannot have a nested macro!";

        case TOKEN_TYPE::KW_DB:
        case TOKEN_TYPE::KW_DW:
        case TOKEN_TYPE::KW_DD:
        case TOKEN_TYPE::KW_DQ:
            printf("%s:%s", tok.str().c_str(), tok1.str().c_str());
            add_mem_id(tok);
            return translate_data_def(tokens, index + 2, macro_params);

        case TOKEN_TYPE::KW_EQU:
            //printf("%s%s", tok.str().c_str(), tok1.str().c_str());
            printf("#define %s ", tok.str().c_str());
            return translate_expression(tokens, index + 3, macro_params);

        case TOKEN_TYPE::KW_TEXTEQU:
        {
            Token tok3 = get_ws(tokens[index + 3]);
            Token tok4 = get_expected_token(tokens[index + 4], TOKEN_TYPE::StringDef);

            string textdef = tok4.str();
            printf("#define %s %s", tok.str().c_str(), textdef.substr(1, textdef.size() - 2).c_str());
            return index + 5;
        }

        case TOKEN_TYPE::KW_PROC:
        {
            printf(".func %s\n", tok.str().c_str());
            printf("%s:", tok.str().c_str());
            index += 3;

            if ((tokens[index].type() == TOKEN_TYPE::WhiteSpace) &&
                (tokens[index + 1].type() == TOKEN_TYPE::KW_FRAME))
            {
#ifdef TARGET_amd64
                printf("\n.seh_proc %s\n", tok.str().c_str());
#else
                printf("\n.cfi_startproc\n");
#endif
                index += 2;
            }
            break;
        }

        case TOKEN_TYPE::KW_ENDP:
        {
            printf(".seh_endproc\n.endfunc");
            index += 3;
            break;
        }

        case TOKEN_TYPE::KW_RECORD:
            index = translate_record(tokens, index, macro_params);
            break;

        default:
            // We don't know what it is, assume it's a macro and treat it like an instruction
            index = translate_instruction(tokens, index, macro_params);
            break;
    }

    return index;
}

size_t
translate_construct(TokenList& tokens, size_t index, const vector<string> &macro_params)
{
    Token tok = tokens[index];

    switch (tok.type())
    {
        case TOKEN_TYPE::WhiteSpace:
        case TOKEN_TYPE::NewLine:
        case TOKEN_TYPE::Comment:
            return translate_token(tokens, index, macro_params);

        case TOKEN_TYPE::Identifier:
            return translate_identifier_construct(tokens, index, macro_params);

        case TOKEN_TYPE::KW_ALIGN:
            index = translate_construct_one_param(".align", tokens, index, macro_params);
            break;

        case TOKEN_TYPE::KW_allocstack:
            index = translate_construct_one_param(".seh_stackalloc", tokens, index, macro_params);
            break;

        case TOKEN_TYPE::KW_code:
#ifdef TARGET_amd64
            printf(".code64");
#else
            printf(".code");
#endif
            printf(" .intel_syntax noprefix");
            index++;
            break;

        case TOKEN_TYPE::KW_const:
            printf(".section .rdata");
            index++;
            break;

        case TOKEN_TYPE::KW_DB:
        case TOKEN_TYPE::KW_DW:
        case TOKEN_TYPE::KW_DD:
        case TOKEN_TYPE::KW_DQ:
            return translate_data_def(tokens, index, macro_params);

        case TOKEN_TYPE::KW_END:
            printf("// END\n");
            return tokens.size();

        case TOKEN_TYPE::KW_endprolog:
            printf(".seh_endprologue");
            index++;
            break;

        case TOKEN_TYPE::KW_EXTERN:
        {
            Token tok1 = get_ws_or_nl(tokens[index + 1]);
            Token tok2 = get_expected_token(tokens[index + 2], TOKEN_TYPE::Identifier);
            add_mem_id(tok2);
            printf("//");
            return complete_line(tokens, index, macro_params);
        }

        case TOKEN_TYPE::KW_if:
        case TOKEN_TYPE::KW_ifdef:
        case TOKEN_TYPE::KW_ifndef:
        case TOKEN_TYPE::KW_else:
        case TOKEN_TYPE::KW_endif:
            // TODO: handle parameter differences between "if" and ".if" etc.
            printf(".");
            return complete_line(tokens, index, macro_params);

        case TOKEN_TYPE::KW_include:
        {
            // The next token should be white space
            Token tok1 = get_ws_or_nl(tokens[index + 1]);
            Token tok2 = get_expected_token(tokens[index + 2], TOKEN_TYPE::Filename);
            printf("#include \"%s.h\"", tok2.str().c_str());
            index += 3;
            break;
        }

        case TOKEN_TYPE::KW_PUBLIC:
            index = translate_construct_one_param(".global", tokens, index, macro_params);
            break;

        case TOKEN_TYPE::KW_savereg:
            printf(".seh_savereg");
            return complete_line(tokens, index + 1, macro_params);

        case TOKEN_TYPE::KW_savexmm128:
            printf(".seh_savexmm");
            return complete_line(tokens, index + 1, macro_params);

        case TOKEN_TYPE::Instruction:
            index = translate_instruction(tokens, index, macro_params);
            break;

        case TOKEN_TYPE::KW_ERRDEF:
            printf("//");
            return complete_line(tokens, index, macro_params);

        default:
            throw "failed to translate construct";
    }

    // Skip optional white-space
    if (tokens[index].type() == TOKEN_TYPE::WhiteSpace)
    {
        index++;
    }

    // Line should end here!
    Token end = tokens[index];
    if ((end.type() != TOKEN_TYPE::Comment) &&
        (end.type() != TOKEN_TYPE::NewLine))
    {
        throw "unexpected tokens";
    }

    return index;
}

size_t
translate_macro(TokenList& tokens, size_t index)
{
    vector<string> macro_params;

    printf(".macro %s", tokens[index].str().c_str());

    // Parse marameters
    index += 3;
    while (index < tokens.size())
    {
        Token tok = tokens[index];
        switch (tok.type())
        {
            case TOKEN_TYPE::NewLine:
            case TOKEN_TYPE::Comment:
                index = translate_token(tokens, index, macro_params);
                break;

            case TOKEN_TYPE::Identifier:
                macro_params.push_back(tok.str());
                printf("%s", tok.str().c_str());
                index++;
                continue;

            case TOKEN_TYPE::WhiteSpace:
            case TOKEN_TYPE::Operator:
                index = translate_token(tokens, index, macro_params);
                continue;
        }

        break;
    }

    // Parse content
    while (index < tokens.size())
    {
        Token tok = tokens[index];
        switch (tok.type())
        {
            case TOKEN_TYPE::KW_ENDM:
                printf(".endm");
                return index + 1;

            default:
                index = translate_construct(tokens, index, macro_params);
        }
    }

    throw "Failed to translate macro";
    return -1;
}

void
translate(TokenList &tokens)
{
    size_t index = 0;
    size_t size = tokens.size();
    vector<string> empty_macro_params;

    while (index < size)
    {
        // Macros are special
        if ((tokens[index].type() == TOKEN_TYPE::Identifier) &&
            (tokens[index + 1].type() == TOKEN_TYPE::WhiteSpace) &&
            (tokens[index + 2].type() == TOKEN_TYPE::KW_MACRO))
        {
            index = translate_macro(tokens, index);
        }
        else
        {
            index = translate_construct(tokens, index, empty_macro_params);
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Invalid parameter!\n");
        return -1;
    }

#if PROFILING_ENABLED
    time_t start_time = time(NULL);
#endif

    try
    {
        // Open and read the input file
        string filename(argv[1]);
        ifstream file(filename);
        stringstream buffer;
        buffer << file.rdbuf();
        string text = buffer.str();

        // Create the tokenizer
        Tokenizer tokenizer(g_TokenList);

        // Get a token list
        TokenList toklist(tokenizer, text);

        // Now translate the tokens
        translate(toklist);
    }
    catch (const char* message)
    {
        fprintf(stderr, "Exception caught: '%s'\n", message);
        return -2;
    }

#if PROFILING_ENABLED
    time_t total_time = time(NULL) + 1 - start_time;
    fprintf(stderr, "total_time = %llu\n", total_time);
    fprintf(stderr, "search_time = %llu\n", search_time);
    fprintf(stderr, "search: %llu %%\n", search_time * 100 / total_time);
#endif

    return 0;
}
