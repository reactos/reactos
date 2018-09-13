/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    d3dm.c

Abstract:

    This module contains the disassembler code that is specific to the
    DM

Author:

    Ramon J. San Andres (ramonsa)  22-August-1993

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop



typedef LPCH FAR *LPLPCH;

extern XOSD Disassemble( HPID, HTID, LPSDI, PVOID, INT, BOOL );
extern void OutputHexString(LPLPCH, int *, LPCH, int);


XOSD
disasm (
    LPSDI   lpsdi,
    void   *Memory,
    int     Size
    )
{
    return Disassemble( (HPID)0, (HTID)0, lpsdi, Memory, Size, FALSE );
}


XOSD
GetRegisterValue (
        HPID hpid,
        HTID htid,
        UINT wValue,
        LONG lValue
        )
{
    UNREFERENCED_PARAMETER( hpid    );
    UNREFERENCED_PARAMETER( htid    );
    UNREFERENCED_PARAMETER( wValue  );
    UNREFERENCED_PARAMETER( lValue  );

    return xosdNone;
}

XOSD
SetAddress (
        HPID hpid,
        HTID htid,
        UINT wValue,
        LONG lValue
        )
{
    UNREFERENCED_PARAMETER( hpid    );
    UNREFERENCED_PARAMETER( htid    );
    UNREFERENCED_PARAMETER( wValue  );
    UNREFERENCED_PARAMETER( lValue  );

    return xosdNone;
}

XOSD
ReadMemBuffer (
        HPID hpid,
        HTID htid,
        UINT wValue,
        LONG lValue
        )
{
    UNREFERENCED_PARAMETER( hpid    );
    UNREFERENCED_PARAMETER( htid    );
    UNREFERENCED_PARAMETER( wValue  );
    UNREFERENCED_PARAMETER( lValue  );

    return xosdNone;
}


LSZ
ObtainSymbol (
    PADDR   Addr1,
    SOP     Sop,
    PADDR   Addr2,
    LSZ     Lsz,
    LONG   *Lpl
    )
{
    UNREFERENCED_PARAMETER( Addr1   );
    UNREFERENCED_PARAMETER( Sop     );
    UNREFERENCED_PARAMETER( Addr2   );
    UNREFERENCED_PARAMETER( Lsz     );
    UNREFERENCED_PARAMETER( Lpl     );

    return NULL;
}






void OutputSymbol (
    HPID    hpidLocal,
    HTID    htidLocal,
    BOOL    fSymbols,
    BOOL    fSegOvr,
    LPADDR  lpaddrOp,
    int     ireg,
    int     length,
    LPADDR  lpaddrLoc,
    LPLPCH  ppBuf,
    int *   pcch
)
{
    UNREFERENCED_PARAMETER( hpidLocal );
    UNREFERENCED_PARAMETER( htidLocal );
    UNREFERENCED_PARAMETER( fSymbols  );
    UNREFERENCED_PARAMETER( fSegOvr   );
    UNREFERENCED_PARAMETER( ireg      );
    UNREFERENCED_PARAMETER( length    );
    UNREFERENCED_PARAMETER( lpaddrLoc );

    OutputHexString ( ppBuf, pcch, (LPCH) &offAddr( *lpaddrOp ), 4 );
}

#define D3DM 1
#undef GetSymbol
#include "..\..\em\p_i386\d3.c"

