#define IMSG_%%FOR_ALL%%            %%INDEX%%

#ifndef PROTOS_ONLY

CONST FNSCSENDMESSAGE gapfnScSendMessage[] = {
    MSGFN(%%FOR_ALL_BUT_LAST%%),
    MSGFN(%%FOR_LAST%%)
};

CONST INT gcapfnScSendMessage = sizeof(gapfnScSendMessage) / sizeof(FNSCSENDMESSAGE);

#endif

#ifdef _USERK_

#define MSGCALLFN(func) NtUserfn ## func

typedef LRESULT (APIENTRY *FNMESSAGECALL)(PWND, UINT, WPARAM, LPARAM,
        ULONG_PTR, DWORD, BOOL);

#define MESSAGECALLPROTO(func)                              \
     LRESULT CALLBACK MSGCALLFN(func)(                      \
        PWND pwnd, UINT msg, WPARAM wParam, LPARAM lParam,  \
        ULONG_PTR xParam, DWORD xpfnWndProc, BOOL bAnsi)

MESSAGECALLPROTO(%%FOR_ALL%%);

/*
 * Some function names don't exist in the kernel so override them.
 */
#define NtUserfnINDESTROYCLIPBRD    NtUserfnDWORD
#define NtUserfnEMGETSEL            NtUserfnOPTOUTLPDWORDOPTOUTLPDWORD
#define NtUserfnEMSETSEL            NtUserfnDWORD
#define NtUserfnINWPARAMDBCSCHAR    NtUserfnDWORD
#define NtUserfnCBGETEDITSEL        NtUserfnOPTOUTLPDWORDOPTOUTLPDWORD
#define NtUserfnPOWERBROADCAST      NtUserfnDWORD
#define NtUserfnLOGONNOTIFY         NtUserfnKERNELONLY
#define NtUserfnINLPKDRAWSWITCHWND  NtUserfnKERNELONLY

#ifndef PROTOS_ONLY

CONST FNMESSAGECALL gapfnMessageCall[] = {
    MSGCALLFN(%%FOR_ALL_BUT_LAST%%),
    MSGCALLFN(%%FOR_LAST%%)
};

#else

extern CONST FNMESSAGECALL gapfnMessageCall[];

#endif
#endif
