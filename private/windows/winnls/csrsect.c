/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    csrpro.c

Abstract:

    This module implements functions that are used by the functions in section.c 
    to communicate with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/
#include "nls.h"
#include "ntwow64n.h"

NTSTATUS
CsrBasepNlsPreserveSection(
    IN UINT  uiType,
    IN UINT  CodePage
    )
{

#if defined(BUILD_WOW6432)
   return NtWow64CsrBasepNlsPreserveSection(uiType, CodePage);
#else

    BASE_API_MSG m;
    PBASE_NLS_PRESERVE_SECTION_MSG a = &m.u.NlsPreserveSection;

    a->uiType   = uiType;
    a->CodePage = CodePage;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
                                             BasepNlsPreserveSection),
                         sizeof(*a) );    

    return m.ReturnValue;

#endif

}
