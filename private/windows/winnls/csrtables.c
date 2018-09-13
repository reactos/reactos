/*++

Copyright (c) 1998-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    csrpro.c

Abstract:

    This module implements functions that are used by the functions in tables.c
    to communicate with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/


#include "nls.h"
#include "ntwow64n.h"


NTSTATUS
CsrBasepNlsCreateSection(
    IN UINT uiType,
    IN LCID Locale,
    OUT PHANDLE phSection)
{

#if defined(BUILD_WOW6432)
    return NtWow64CsrBasepNlsCreateSection( uiType,
                                            Locale,
                                            phSection );
#else

    BASE_API_MSG m;
    PBASE_NLS_CREATE_SECTION_MSG a = &m.u.NlsCreateSection;

    a->Locale = Locale;
    a->uiType = uiType;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
                                             BasepNlsCreateSection),
                         sizeof(*a) );

    //
    //  Save the handle to the new section.
    //
    *phSection = a->hNewSection;

    return (m.ReturnValue);
#endif

}
