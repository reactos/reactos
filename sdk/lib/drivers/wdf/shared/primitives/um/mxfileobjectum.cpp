/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxFileObjectUm.cpp

Abstract:

    Implementation of MxFileObjectUm functions.

Author:

Revision History:



--*/

#include "Mx.h"

#include <strsafe.h>

#include "fxmin.hpp"

PUNICODE_STRING
MxFileObject::GetFileName(
    _Inout_opt_ PUNICODE_STRING Filename
   )
{
    LPCWSTR name;

    ASSERTMSG("Filename parameter cannot be NULL\n", Filename != NULL);

    name = m_FileObject->GetName();
    RtlInitUnicodeString(Filename, name);

    return Filename;
}

