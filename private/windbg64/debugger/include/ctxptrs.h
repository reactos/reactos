#if defined( _NTMIPS_ )  ||  defined( _NTALPHA_ ) || defined( _NTPPC_ ) || defined( _NTIA64H_ )
#define _CTXPTRS_H_
#endif

#ifndef _CTXPTRS_H_
#define _CTXPTRS_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined( _ALPHA_ ) && !defined (TARGET_ALPHA) && !defined(_IA64_) && !defined(TARGET_IA64)

typedef  ULONG_PTR  KNONVOLATILE_CONTEXT_POINTERS;
typedef PULONG_PTR PKNONVOLATILE_CONTEXT_POINTERS;

#elif defined(_IA64_) || defined(TARGET_IA64)

//
// from ntia64.h
//

//
// Nonvolatile context pointer record.for IA64
//

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {
    PFLOAT128  LowFloatingContext[2];       // Intel-IA64-Filler
    PFLOAT128  FltS0;                       // Intel-IA64-Filler
    PFLOAT128  FltS1;                       // Intel-IA64-Filler
    PFLOAT128  FltS2;                       // Intel-IA64-Filler
    PFLOAT128  FltS3;                       // Intel-IA64-Filler
    PFLOAT128  HighFloatingContext[10];     // Intel-IA64-Filler
    PFLOAT128  FltS4;                       // Intel-IA64-Filler
    PFLOAT128  FltS5;                       // Intel-IA64-Filler
    PFLOAT128  FltS6;                       // Intel-IA64-Filler
    PFLOAT128  FltS7;                       // Intel-IA64-Filler
    PFLOAT128  FltS8;                       // Intel-IA64-Filler
    PFLOAT128  FltS9;                       // Intel-IA64-Filler
    PFLOAT128  FltS10;                      // Intel-IA64-Filler
    PFLOAT128  FltS11;                      // Intel-IA64-Filler
    PFLOAT128  FltS12;                      // Intel-IA64-Filler
    PFLOAT128  FltS13;                      // Intel-IA64-Filler
    PFLOAT128  FltS14;                      // Intel-IA64-Filler
    PFLOAT128  FltS15;                      // Intel-IA64-Filler
    PFLOAT128  FltS16;                      // Intel-IA64-Filler
    PFLOAT128  FltS17;                      // Intel-IA64-Filler
    PFLOAT128  FltS18;                      // Intel-IA64-Filler
    PFLOAT128  FltS19;                      // Intel-IA64-Filler

    PULONGLONG IntegerContext[3];           // Intel-IA64-Filler
    PULONGLONG IntS0;                       // Intel-IA64-Filler
    PULONGLONG IntS1;                       // Intel-IA64-Filler
    PULONGLONG IntS2;                       // Intel-IA64-Filler
    PULONGLONG IntS3;                       // Intel-IA64-Filler
    PULONGLONG IntV0;                       // Intel-IA64-Filler
    PULONGLONG IntAp;                       // Intel-IA64-Filler
    PULONGLONG IntT0;                       // Intel-IA64-Filler
    PULONGLONG IntT1;                       // Intel-IA64-Filler
    PULONGLONG IntSp;                       // Intel-IA64-Filler
    PULONGLONG IntNats;                     // Intel-IA64-Filler

    PULONGLONG Preds;                       // Intel-IA64-Filler

    PULONGLONG BrRp;                        // Intel-IA64-Filler
    PULONGLONG BrS0;                        // Intel-IA64-Filler
    PULONGLONG BrS1;                        // Intel-IA64-Filler
    PULONGLONG BrS2;                        // Intel-IA64-Filler
    PULONGLONG BrS3;                        // Intel-IA64-Filler
    PULONGLONG BrS4;                        // Intel-IA64-Filler

    PULONGLONG ApUNAT;                      // Intel-IA64-Filler
    PULONGLONG ApLC;                        // Intel-IA64-Filler
    PULONGLONG ApEC;                        // Intel-IA64-Filler
    PULONGLONG RsPFS;                       // Intel-IA64-Filler
    PULONGLONG RsRNAT;                      // Intel-IA64-Filler

    PULONGLONG StFSR;                       // Intel-IA64-Filler
    PULONGLONG StFIR;                       // Intel-IA64-Filler
    PULONGLONG StFDR;                       // Intel-IA64-Filler
    PULONGLONG Cflag;                       // Intel-IA64-Filler

} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

#else

//
// modified from ntalpha.h   June 7, 1993.
//

//
// Nonvolatile context pointer record.
//

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {

    PLARGE_INTEGER FloatingContext[1];
    PLARGE_INTEGER FltF1;
    // Nonvolatile floating point registers start here.
    PLARGE_INTEGER FltF2;
    PLARGE_INTEGER FltF3;
    PLARGE_INTEGER FltF4;
    PLARGE_INTEGER FltF5;
    PLARGE_INTEGER FltF6;
    PLARGE_INTEGER FltF7;
    PLARGE_INTEGER FltF8;
    PLARGE_INTEGER FltF9;
    PLARGE_INTEGER FltF10;
    PLARGE_INTEGER FltF11;
    PLARGE_INTEGER FltF12;
    PLARGE_INTEGER FltF13;
    PLARGE_INTEGER FltF14;
    PLARGE_INTEGER FltF15;
    PLARGE_INTEGER FltF16;
    PLARGE_INTEGER FltF17;
    PLARGE_INTEGER FltF18;
    PLARGE_INTEGER FltF19;
    PLARGE_INTEGER FltF20;
    PLARGE_INTEGER FltF21;
    PLARGE_INTEGER FltF22;
    PLARGE_INTEGER FltF23;
    PLARGE_INTEGER FltF24;
    PLARGE_INTEGER FltF25;
    PLARGE_INTEGER FltF26;
    PLARGE_INTEGER FltF27;
    PLARGE_INTEGER FltF28;
    PLARGE_INTEGER FltF29;
    PLARGE_INTEGER FltF30;
    PLARGE_INTEGER FltF31;

    PLARGE_INTEGER IntegerContext[1];
    PLARGE_INTEGER IntT0;
    PLARGE_INTEGER IntT1;
    PLARGE_INTEGER IntT2;
    PLARGE_INTEGER IntT3;
    PLARGE_INTEGER IntT4;
    PLARGE_INTEGER IntT5;
    PLARGE_INTEGER IntT6;
    PLARGE_INTEGER IntT7;
    // Nonvolatile integer registers start here.
    PLARGE_INTEGER IntS0;
    PLARGE_INTEGER IntS1;
    PLARGE_INTEGER IntS2;
    PLARGE_INTEGER IntS3;
    PLARGE_INTEGER IntS4;
    PLARGE_INTEGER IntS5;
    PLARGE_INTEGER IntFp;
    PLARGE_INTEGER IntA0;
    PLARGE_INTEGER IntA1;
    PLARGE_INTEGER IntA2;
    PLARGE_INTEGER IntA3;
    PLARGE_INTEGER IntA4;
    PLARGE_INTEGER IntA5;
    PLARGE_INTEGER IntT8;
    PLARGE_INTEGER IntT9;
    PLARGE_INTEGER IntT10;
    PLARGE_INTEGER IntT11;
    PLARGE_INTEGER IntRa;
    PLARGE_INTEGER IntT12;
    PLARGE_INTEGER IntAt;
    PLARGE_INTEGER IntGp;
    PLARGE_INTEGER IntSp;
    PLARGE_INTEGER IntZero;

} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

#endif  // !_ALPHA_ && !TARGET_ALPHA

#ifdef __cplusplus
} // extern "C" {
#endif

#endif  //  _CTXPTRS_H_
