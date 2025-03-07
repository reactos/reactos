//
// common_utf8.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Common UTF-8 utilities
//

#include <corecrt_internal.h>
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>

namespace __crt_mbstring
{
    size_t return_illegal_sequence(mbstate_t* ps, __crt_cached_ptd_host& ptd)
    {
        *ps = {};
        ptd.get_errno().set(EILSEQ);
        return INVALID;
    }

    size_t reset_and_return(size_t retval, mbstate_t* ps)
    {
        *ps = {};
        return retval;
    }
}
