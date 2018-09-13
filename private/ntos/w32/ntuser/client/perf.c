/**************************************************************************\
* Module Name: perf.c
*
* performance timing calls
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 12-Jun-1991 mikeke
*
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "usercli.h"
#include <string.h>

#include "ntcsrdll.h"
#include "csuser.h"

/**************************************************************************\
\**************************************************************************/

typedef CSR_QLPC_API_MSG *PCSRMSG;

typedef PDWORD (* PINSTUBFUNC)(PDWORD, PDWORD, PDWORD, PDWORD);
typedef void (* POUTSTUBFUNC)(PDWORD, PDWORD);

PDWORD Copy4(PDWORD psrc,PDWORD pdst,PDWORD pparam,PDWORD pmax);
PDWORD Copy5(PDWORD psrc,PDWORD pdst,PDWORD pparam,PDWORD pmax);
PDWORD CopySTR(PDWORD psrc,PDWORD pdst,PDWORD pparam,PDWORD pmax);

#if !defined(_MIPS_) && !defined(_PPC_)
void OutCopy4(
    PDWORD psrc,
    PDWORD pdst)
{
    pdst[0] = psrc[0];
    pdst[1] = psrc[1];
    pdst[2] = psrc[2];
    pdst[3] = psrc[3];
}

PDWORD Copy4(
    PDWORD psrc,
    PDWORD pdst,
    PDWORD pparam,
    PDWORD pmax)
{
    if (pdst + 5 <= pmax) {
        pdst[0] = psrc[0];
        pdst[1] = psrc[1];
        pdst[2] = psrc[2];
        pdst[3] = psrc[3];
        return pdst + 4;
    }
    return 0;
    pparam;
}

PDWORD Copy5(
    PDWORD psrc,
    PDWORD pdst,
    PDWORD pparam,
    PDWORD pmax)
{
    if (pdst + 5 <= pmax) {
        pdst[0] = psrc[0];
        pdst[1] = psrc[1];
        pdst[2] = psrc[2];
        pdst[3] = psrc[3];
        pdst[4] = psrc[4];
        return pdst + 5;
    }
    return 0;
    pparam;
}

PDWORD Copy6(
    PDWORD psrc,
    PDWORD pdst,
    PDWORD pparam,
    PDWORD pmax)
{
    if (pdst + 6 <= pmax) {
        pdst[0] = psrc[0];
        pdst[1] = psrc[1];
        pdst[2] = psrc[2];
        pdst[3] = psrc[3];
        pdst[4] = psrc[4];
        pdst[5] = psrc[5];
        return pdst + 6;
    }
    return 0;
    pparam;
}

PDWORD CopySTR(
    PDWORD psrc,
    PDWORD pdst,
    PDWORD pparam,
    PDWORD pmax)
{
    PBYTE length = (PBYTE)psrc;

    while (*length)
        length++;

    length = (length - (PBYTE)psrc + (PBYTE)pdst);
    length = (PBYTE)(((DWORD)length +3 ) & ~3);

    if (length <= (PBYTE)pmax) {
        while ((PBYTE)pdst != (PBYTE)length) {
            *pdst = *psrc;
            pdst++;
            psrc++;
        }
        return (PDWORD)length;
    }
    return 0;
    pparam;
}
#endif

/**************************************************************************\
\**************************************************************************/

PDWORD AIDoClientInStuff(
    PDWORD psrc,
    PDWORD pbase,
    PDWORD ptemplate,
    PDWORD pmax);

VOID AIDoClientOutStuff(
    PDWORD psrc,
    PDWORD pbase,
    PDWORD ptemplate);

PDWORD DoClientInStuff(
    PDWORD psrc,
    PDWORD pbase,
    PDWORD ptemplate,
    PDWORD pmax)
{
    PDWORD pparam;
    PDWORD param;
    PDWORD pdst;

    if (ptemplate[0]) {
        pdst = ((PINSTUBFUNC)ptemplate[0])(psrc, pbase, NULL, pmax);
        if (!pdst) {
            return 0;
        }
        ptemplate++;

        while (ptemplate[0]) {
            pparam = (PDWORD)((PBYTE)pbase + ptemplate[1]);
            param = (PDWORD)*pparam;
            *pparam = (PBYTE)pdst - (PBYTE)pbase;
            pdst = ((PINSTUBFUNC)ptemplate[0])(param, pdst, pparam, pmax);
            if (!pdst) {
                return 0;
            }
            ptemplate += 2;
        }
    }
}

VOID DoClientOutStuff(
    PDWORD psrc,
    PDWORD pdst,
    PDWORD ptemplate)
{
    PDWORD pparam;

    while (ptemplate[0]) {
        pparam = (PDWORD)((PBYTE)psrc + ptemplate[1]);
        ((POUTSTUBFUNC)ptemplate[0])(
            (PDWORD)((PBYTE)psrc + *pparam),
            *(PDWORD *)((PBYTE)pdst + ptemplate[1])
        );
        ptemplate += 2;
    }
}

/**************************************************************************\
\**************************************************************************/

DWORD MakeCSCall(
    DWORD findex,
    PDWORD psrc,
    PDWORD pInTemplate,
    PDWORD pOutTemplate
    )
{
    PCSR_QLPC_TEB pteb = (PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb;
    PCSR_QLPC_STACK pstack;
    PCSRMSG pmsg;
    ULONG retval;
    PDWORD pbase;
    PDWORD pdst;
    PDWORD plast;
    PDWORD pmax;

    //
    // connect to the server
    //

    if (pteb == NULL) {
        pteb = CsrClientThreadConnect();
        if (pteb == NULL) {
            return 0;
        }
    }

    pstack = pteb->MessageStack;
    if (pstack->BatchCount) {
        CsrClientSendMessage();
        pbase = (PDWORD)((PBYTE)pstack + pstack->Base);
    } else {
        pbase = (PDWORD)((PBYTE)pstack + pstack->Current);
    }
    pmax = (PDWORD)((PBYTE)pstack + pstack->Limit);
    pmsg = (PCSRMSG)(pbase+1);

    //
    // is there enough space left on the stack?
    //

    pdst = (PDWORD)(pmsg + 1);
    if (pdst <= pmax) {

        //
        // copy the data to shared memory
        //

        if (pInTemplate) {
            plast = DoClientInStuff(psrc, pdst, pInTemplate, pmax);
        } else {
            plast = pdst;
        }

        if (plast) {

            //
            // Make the call
            //

            pmsg->Length = (PBYTE)plast - (PBYTE)pmsg;
            pmsg->ApiNumber = findex;
            *pbase = pstack->Base;
            pstack->Base = pstack->Current+4;
            pstack->Current = (PBYTE)pdst - (PBYTE)pstack;
            pstack->BatchCount = 1;

            retval = CsrClientSendMessage();

            pstack->Current = pstack->Base - 4;
            pstack->Base = *pbase;

            //
            // Do any post call copies
            //

            if (pOutTemplate) {
                DoClientOutStuff(pdst, psrc, pOutTemplate);
            }

            return retval;
        }
    }

    //
    // an error occured
    //

    return 0;
}

/**************************************************************************\
\**************************************************************************/

#if defined(_MIPS_) || defined(_PPC_)
DWORD AIMakeCSCall(
    DWORD findex,
    PDWORD psrc,
    PDWORD pInTemplate,
    PDWORD pOutTemplate
    )
{
    PCSR_QLPC_TEB pteb = (PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb;
    PCSR_QLPC_STACK pstack;
    PCSRMSG pmsg;
    ULONG retval;
    PDWORD pbase;
    PDWORD pdst;
    PDWORD plast;
    PDWORD pmax;

    //
    // connect to the server
    //

    if (pteb == NULL) {
        pteb = CsrClientThreadConnect();
        if (pteb == NULL) {
            return 0;
        }
    }

    pstack = pteb->MessageStack;
    if (pstack->BatchCount) {
        CsrClientSendMessage();
        pbase = (PDWORD)((PBYTE)pstack + pstack->Base);
    } else {
        pbase = (PDWORD)((PBYTE)pstack + pstack->Current);
    }
    pmax = (PDWORD)((PBYTE)pstack + pstack->Limit);
    pmsg = (PCSRMSG)(pbase+1);

    //
    // is there enough space left on the stack?
    //

    if ((PDWORD)((PBYTE)pmsg + sizeof(PCSRMSG)) <= pmax) {

        //
        // copy the data to shared memory
        //

        pdst = (PDWORD)(pmsg + 1);
        if (pInTemplate) {
            plast = AIDoClientInStuff(psrc, pdst, pInTemplate, pmax);
        } else {
            plast = pdst;
        }

        if (plast) {

            //
            // Make the call
            //

            pmsg->Length = (PBYTE)plast - (PBYTE)pmsg;
            pmsg->ApiNumber = findex;
            *pbase = pstack->Base;
            pstack->Base = pstack->Current+4;
            pstack->Current = (PBYTE)pdst - (PBYTE)pstack;
            pstack->BatchCount = 1;

            retval = CsrClientSendMessage();

            pstack->Current = pstack->Base - 4;
            pstack->Base = *pbase;

            //
            // Do any post call copies
            //

            if (pOutTemplate) {
                AIDoClientOutStuff(pdst, psrc, pOutTemplate);
            }

            return retval;
        }
    }

    //
    // an error occured
    //

    return 0;
}
#endif

/**************************************************************************\
\**************************************************************************/

#ifdef LATER
typedef struct _TESTMSG {
    int a;
    int b;
    RECT *lprc;
    int c;
    LPSTR lpstr;
    RECT rc;
} TESTMSG;

PDWORD CSInTestCall(
    PDWORD psrc,
    PDWORD pdst,
    PDWORD pmax)
{
    PDWORD pvar;
    TESTMSG *pmsg;
    TESTMSG *pparam;

    if (pmax > pdst + sizeof(TESTMSG)) {
        pmsg = (TESTMSG *)pdst;
        pparam = (TESTMSG *)psrc;
        pvar = (PDWORD)(pmsg + 1);

        pmsg->a = pparam->a;
        pmsg->b = pparam->b;
        pmsg->c = pparam->c;

        pmsg->lprc = (RECT *)(((PBYTE)&(pmsg->rc)) - (PBYTE)pmsg);
        pmsg->rc = *(pparam->lprc);

        pmsg->lpstr = (LPSTR)((PBYTE)pvar - (PBYTE)pmsg);
        pvar = CopySTR((PDWORD)pparam->lpstr, pvar, NULL, pmax);

        if (pvar) {
            return (PDWORD)pvar;
        }
    }
    return 0;
}

typedef struct _TESTMSG {
    HWND hwnd;
    int a;
    int b;
    int c;
    int d;
    int e;
} TESTMSG;

PDWORD CSInTestCall(
    PDWORD psrc,
    PDWORD pdst,
    PDWORD pmax)
{
    TESTMSG *pmsg;
    TESTMSG *pparam;

    if (pmax > pdst + sizeof(TESTMSG)) {
        pmsg = (TESTMSG *)pdst;
        pparam = (TESTMSG *)psrc;

        pmsg->hwnd = pparam->hwnd;
        pmsg->a = pparam->a;
        pmsg->b = pparam->b;
        pmsg->c = pparam->c;
        pmsg->d = pparam->d;
        pmsg->e = pparam->e;
        return (PDWORD)(pmsg + 1);
    }
    return 0;
}
#endif

/**************************************************************************\
\**************************************************************************/

typedef PDWORD (* PCSINFUNC)(PDWORD, PDWORD, PDWORD);
typedef PDWORD (* PCSOUTFUNC)(PDWORD, PDWORD);


DWORD CSMakeCall(
    DWORD findex,
    PDWORD psrc,
    PCSINFUNC pInFunc,
    PCSOUTFUNC pOutFunc
    )
{
    PCSR_QLPC_TEB pteb = (PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb;
    PCSR_QLPC_STACK pstack;
    PCSRMSG pmsg;
    ULONG retval;
    PDWORD pbase;
    PDWORD pdst;
    PDWORD plast;
    PDWORD pmax;

    //
    // connect to the server
    //

    if (pteb == NULL) {
        pteb = CsrClientThreadConnect();
        if (pteb == NULL) {
            return 0;
        }
    }

    pstack = pteb->MessageStack;
    if (pstack->BatchCount) {
        CsrClientSendMessage();
        pbase = (PDWORD)((PBYTE)pstack + pstack->Base);
    } else {
        pbase = (PDWORD)((PBYTE)pstack + pstack->Current);
    }
    pmax = (PDWORD)((PBYTE)pstack + pstack->Limit);
    pmsg = (PCSRMSG)(pbase+1);

    //
    // is there enough space left on the stack?
    //

    pdst = (PDWORD)(pmsg + 1);
    if (pdst <= pmax) {

        //
        // copy the data to shared memory
        //

        if (pInFunc) {
            plast = pInFunc(psrc, pdst, pmax);
        } else {
            plast = pdst;
        }

        if (plast) {

            //
            // Make the call
            //

            pmsg->Length = (PBYTE)plast - (PBYTE)pmsg;
            pmsg->ApiNumber = findex;
            *pbase = pstack->Base;
            pstack->Base = pstack->Current+4;
            pstack->Current = (PBYTE)pdst - (PBYTE)pstack;
            pstack->BatchCount = 1;

            retval = CsrClientSendMessage();

            pstack->Current = pstack->Base - 4;
            pstack->Base = *pbase;

            //
            // Do any post call copies
            //

            if (pOutFunc) {
                pOutFunc(pdst, psrc);
            }

            return retval;
        }
    }

    //
    // an error occured
    //

    return 0;
}

/**************************************************************************\
\**************************************************************************/

// DWORD CTestCall(int a, int b, LPRECT lprc, int c, LPSTR lpstr)

#if 0
DWORD CTestCall(HWND hwnd, int a, int b, int c, int d, int e)
{
    return CSMakeCall(
        CSR_MAKE_API_NUMBER(4,FI_CTESTCALL),
        (PDWORD)&hwnd,
        CSInTestCall,
        NULL);
}
#endif

/**************************************************************************\
\**************************************************************************/

/**************************************************************************\
\**************************************************************************/

/**************************************************************************\
\**************************************************************************/

#ifdef LATER
DWORD InTestCall[] = {
    (DWORD)Copy5,
    (DWORD)Copy4,
    8,
    (DWORD)CopySTR,
    16,
    0
};
#endif

DWORD InTestCall[] = {
    (DWORD)Copy6,
    0
};

DWORD ITestCall(HWND hwnd, int a, int b, int c, int d, int e)
{
   return MakeCSCall(
       CSR_MAKE_API_NUMBER(4,FI_CTESTCALL),
       (PDWORD)&hwnd,
       InTestCall,
       NULL
   );
}

DWORD AITestCall(HWND hwnd, int a, int b, int c, int d, int e)
{
#if defined(_MIPS_) || defined(_PPC_)
   return AIMakeCSCall(
#else
   return MakeCSCall(
#endif
       CSR_MAKE_API_NUMBER(4,FI_CTESTCALL),
       (PDWORD)&hwnd,
       InTestCall,
       NULL
   );
}

/**************************************************************************\
\**************************************************************************/
