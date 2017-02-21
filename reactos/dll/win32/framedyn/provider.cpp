/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/framedyn/provider.cpp
 * PURPOSE:         Provider class implementation
 * PROGRAMMERS:     Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <provider.h>
#include <wbemcli.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
void Provider::Flush()
{
    return;
}

/*
 * @implemented
 */
HRESULT Provider::ValidateDeletionFlags(long lFlags)
{
    if (lFlags == 0)
        return WBEM_S_NO_ERROR;

    return WBEM_E_UNSUPPORTED_PARAMETER;
}

/*
 * @implemented
 */
HRESULT Provider::ValidateMethodFlags(long lFlags)
{
    if (lFlags == 0)
        return WBEM_S_NO_ERROR;

    return WBEM_E_UNSUPPORTED_PARAMETER;
}

/*
 * @implemented
 */
HRESULT Provider::ValidateQueryFlags(long lFlags)
{
    if (lFlags == 0)
        return WBEM_S_NO_ERROR;

    return WBEM_E_UNSUPPORTED_PARAMETER;
}
