#ifdef __cplusplus
extern "C" {
#endif


//
//  Disassembler handling
//
#define CC                    0xcc
#define ISCC                  2

enum {
    iregAL    = CV_REG_AL,
    iregCL    = CV_REG_CL,
    iregDL    = CV_REG_DL,
    iregBL    = CV_REG_BL,
    iregAH    = CV_REG_AH,
    iregCH    = CV_REG_CH,
    iregDH    = CV_REG_DH,
    iregBH    = CV_REG_BH,
    iregAX    = CV_REG_AX,
    iregCX    = CV_REG_CX,
    iregDX    = CV_REG_DX,
    iregBX    = CV_REG_BX,
    iregSP    = CV_REG_SP,
    iregBP    = CV_REG_BP,
    iregSI    = CV_REG_SI,
    iregDI    = CV_REG_DI,
    iregEAX   = CV_REG_EAX,
    iregECX   = CV_REG_ECX,
    iregEDX   = CV_REG_EDX,
    iregEBX   = CV_REG_EBX,
    iregESP   = CV_REG_ESP,
    iregEBP   = CV_REG_EBP,
    iregESI   = CV_REG_ESI,
    iregEDI   = CV_REG_EDI,
    iregES    = CV_REG_ES,
    iregCS    = CV_REG_CS,
    iregSS    = CV_REG_SS,
    iregDS    = CV_REG_DS,
    iregFS    = CV_REG_FS,
    iregGS    = CV_REG_GS,
#ifdef CV_REG_ST
    iregST    = CV_REG_ST,
    iregNR    = CV_REG_NR,
    iregDXAX  = CV_REG_DXAX,
    iregESBX  = CV_REG_ESBX,
#endif
    iregIP    = CV_REG_IP,
    iregFLAGS = CV_REG_FLAGS,
    iregEFLAGS = CV_REG_EFLAGS,
    iregEIP   = CV_REG_EIP,
    iregST0   = CV_REG_ST0,
    iregST1   = CV_REG_ST1,
    iregST2   = CV_REG_ST2,
    iregST3   = CV_REG_ST3,
    iregST4   = CV_REG_ST4,
    iregST5   = CV_REG_ST5,
    iregST6   = CV_REG_ST6,
    iregST7   = CV_REG_ST7,
    iregCTRL  = 136,
    iregSTAT  = 137,
    iregTAG   = 138,
    iregFPIP  = 139,
    iregFPCS  = 140,
    iregFPDO  = 141,
    iregFPDS  = 142,
    iregISEM  = 143,
    iregCFPR  = 144,
    iregMax   = 145
    };

typedef short IREG;

#define iregNone iregMax

#ifdef __cplusplus
} // extern "C" {
#endif
