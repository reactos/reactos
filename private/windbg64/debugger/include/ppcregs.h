
//
//      this is copied from emp.hmd
//

//      This is all for PPC

#ifdef __cplusplus
extern "C" {
#endif


#pragma pack ( push, 1 )

typedef double FLOAT64;

typedef FLOAT64 FAR * LPFLOAT64;
typedef ULONG FAR * LPUL;


typedef struct _FPRCXT {
    ULONG mq;
    FLOAT64 fpr0;
    FLOAT64 fpr1;
    FLOAT64 fpr2;
    FLOAT64 fpr3;
    FLOAT64 fpr4;
    FLOAT64 fpr5;
    FLOAT64 fpr6;
    FLOAT64 fpr7;
    FLOAT64 fpr8;
    FLOAT64 fpr9;
    FLOAT64 fpr10;
    FLOAT64 fpr11;
    FLOAT64 fpr12;
    FLOAT64 fpr13;
    FLOAT64 fpr14;
    FLOAT64 fpr15;
    FLOAT64 fpr16;
    FLOAT64 fpr17;
    FLOAT64 fpr18;
    FLOAT64 fpr19;
    FLOAT64 fpr20;
    FLOAT64 fpr21;
    FLOAT64 fpr22;
    FLOAT64 fpr23;
    FLOAT64 fpr24;
    FLOAT64 fpr25;
    FLOAT64 fpr26;
    FLOAT64 fpr27;
    FLOAT64 fpr28;
    FLOAT64 fpr29;
    FLOAT64 fpr30;
    FLOAT64 fpr31;
    ULONG   fpscr;
} FPRCXT;

typedef FPRCXT FAR * LPFPRCXT;

typedef struct _REGCXT {
    ULONG gpr0;
    ULONG gpr1;
    ULONG gpr2;
    ULONG gpr3;
    ULONG gpr4;
    ULONG gpr5;
    ULONG gpr6;
    ULONG gpr7;
    ULONG gpr8;
    ULONG gpr9;
    ULONG gpr10;
    ULONG gpr11;
    ULONG gpr12;
    ULONG gpr13;
    ULONG gpr14;
    ULONG gpr15;
    ULONG gpr16;
    ULONG gpr17;
    ULONG gpr18;
    ULONG gpr19;
    ULONG gpr20;
    ULONG gpr21;
    ULONG gpr22;
    ULONG gpr23;
    ULONG gpr24;
    ULONG gpr25;
    ULONG gpr26;
    ULONG gpr27;
    ULONG gpr28;
    ULONG gpr29;
    ULONG gpr30;
    ULONG gpr31;

    ULONG pc;
    ULONG lr;
    ULONG cr;
    ULONG ctr;
    ULONG xer;

    FPRCXT fpregs;

} REGCXT;
#pragma pack ( pop )

typedef REGCXT FAR * LPREGCXT;


#ifdef __cplusplus
} // extern "C" {
#endif
