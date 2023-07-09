#ifndef I_MFLAGS_HXX_
#define I_MFLAGS_HXX_
#pragma INCMSG("--- Beg 'mflags.hxx'")

// Values for uiFlags in MeasureLine()
#define MEASURE_FIRSTINPARA     ((UINT) 0x0001)
#define MEASURE_BREAKATWORD     ((UINT) 0x0002)
#define MEASURE_BREAKATWIDTH    ((UINT) 0x0004)
#define MEASURE_MINWIDTH        ((UINT) 0x0008)
#define MEASURE_MAXWIDTH        ((UINT) 0x0010)
#define MEASURE_BREAKWORDS      ((UINT) 0x0020)
#define MEASURE_EMPTYLASTLINE   ((UINT) 0x0040)
#define MEASURE_FIRSTLINE       ((UINT) 0x0080)
#define MEASURE_BREAKNEARWIDTH  ((UINT) 0x0100)
#define MEASURE_BREAKLONGLINES  ((UINT) 0x0200)
#define MEASURE_NCHARS          ((UINT) 0x0400)
#define MEASURE_AUTOCLEAR       ((UINT) 0x0800)

#pragma INCMSG("--- End 'mflags.hxx'")
#else
#pragma INCMSG("*** Dup 'mflags.hxx'")
#endif
