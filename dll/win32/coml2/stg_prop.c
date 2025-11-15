/*
 * Compound Storage (32 bit version)
 * Storage implementation
 *
 * This file contains the compound file implementation
 * of the storage interface.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 1999 Thuy Nguyen
 * Copyright 2005 Mike McCormack
 * Copyright 2005 Juan Lang
 * Copyright 2006 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * TODO:
 * - I don't honor the maximum property set size.
 * - Certain bogus files could result in reading past the end of a buffer.
 * - Mac-generated files won't be read correctly, even if they're little
 *   endian, because I disregard whether the generator was a Mac.  This means
 *   strings will probably be munged (as I don't understand Mac scripts.)
 * - Not all PROPVARIANT types are supported.
 * - User defined properties are not supported, see comment in
 *   PropertyStorage_ReadFromStream
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "wine/asm.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "oleauto.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

/***********************************************************************
 * Format ID <-> name conversion
 */
static const WCHAR szSummaryInfo[] = L"\5SummaryInformation";
static const WCHAR szDocSummaryInfo[] = L"\5DocumentSummaryInformation";

#define BITS_PER_BYTE    8
#define CHARMASK         0x1f
#define BITS_IN_CHARMASK 5
#define NUM_ALPHA_CHARS  26

/***********************************************************************
 * FmtIdToPropStgName					[coml2.@]
 */
HRESULT WINAPI FmtIdToPropStgName(const FMTID *rfmtid, LPOLESTR str)
{
    static const char fmtMap[] = "abcdefghijklmnopqrstuvwxyz012345";

    TRACE("%s, %p\n", debugstr_guid(rfmtid), str);

    if (!rfmtid) return E_INVALIDARG;
    if (!str) return E_INVALIDARG;

    if (IsEqualGUID(&FMTID_SummaryInformation, rfmtid))
        lstrcpyW(str, szSummaryInfo);
    else if (IsEqualGUID(&FMTID_DocSummaryInformation, rfmtid))
        lstrcpyW(str, szDocSummaryInfo);
    else if (IsEqualGUID(&FMTID_UserDefinedProperties, rfmtid))
        lstrcpyW(str, szDocSummaryInfo);
    else
    {
        const BYTE *fmtptr;
        WCHAR *pstr = str;
        ULONG bitsRemaining = BITS_PER_BYTE;

        *pstr++ = 5;
        for (fmtptr = (const BYTE *)rfmtid; fmtptr < (const BYTE *)rfmtid + sizeof(FMTID); )
        {
            ULONG i = *fmtptr >> (BITS_PER_BYTE - bitsRemaining);

            if (bitsRemaining >= BITS_IN_CHARMASK)
            {
                *pstr = (WCHAR)(fmtMap[i & CHARMASK]);
                if (bitsRemaining == BITS_PER_BYTE && *pstr >= 'a' &&
                 *pstr <= 'z')
                    *pstr += 'A' - 'a';
                pstr++;
                bitsRemaining -= BITS_IN_CHARMASK;
                if (bitsRemaining == 0)
                {
                    fmtptr++;
                    bitsRemaining = BITS_PER_BYTE;
                }
            }
            else
            {
                if (++fmtptr < (const BYTE *)rfmtid + sizeof(FMTID))
                    i |= *fmtptr << bitsRemaining;
                *pstr++ = (WCHAR)(fmtMap[i & CHARMASK]);
                bitsRemaining += BITS_PER_BYTE - BITS_IN_CHARMASK;
            }
        }
        *pstr = 0;
    }
    TRACE("returning %s\n", debugstr_w(str));
    return S_OK;
}

/***********************************************************************
 * PropStgNameToFmtId                                   [coml2.@]
 */
HRESULT WINAPI PropStgNameToFmtId(const LPOLESTR str, FMTID *rfmtid)
{
    HRESULT hr = STG_E_INVALIDNAME;

    TRACE("%s, %p\n", debugstr_w(str), rfmtid);

    if (!rfmtid) return E_INVALIDARG;
    if (!str) return STG_E_INVALIDNAME;

    if (!lstrcmpiW(str, szDocSummaryInfo))
    {
        *rfmtid = FMTID_DocSummaryInformation;
        hr = S_OK;
    }
    else if (!lstrcmpiW(str, szSummaryInfo))
    {
        *rfmtid = FMTID_SummaryInformation;
        hr = S_OK;
    }
    else
    {
        ULONG bits;
        BYTE *fmtptr = (BYTE *)rfmtid - 1;
        const WCHAR *pstr = str;

        memset(rfmtid, 0, sizeof(*rfmtid));
        for (bits = 0; bits < sizeof(FMTID) * BITS_PER_BYTE;
         bits += BITS_IN_CHARMASK)
        {
            ULONG bitsUsed = bits % BITS_PER_BYTE, bitsStored;
            WCHAR wc;

            if (bitsUsed == 0)
                fmtptr++;
            wc = *++pstr - 'A';
            if (wc > NUM_ALPHA_CHARS)
            {
                wc += 'A' - 'a';
                if (wc > NUM_ALPHA_CHARS)
                {
                    wc += 'a' - '0' + NUM_ALPHA_CHARS;
                    if (wc > CHARMASK)
                    {
                        WARN("invalid character (%d)\n", *pstr);
                        goto end;
                    }
                }
            }
            *fmtptr |= wc << bitsUsed;
            bitsStored = min(BITS_PER_BYTE - bitsUsed, BITS_IN_CHARMASK);
            if (bitsStored < BITS_IN_CHARMASK)
            {
                wc >>= BITS_PER_BYTE - bitsUsed;
                if (bits + bitsStored == sizeof(FMTID) * BITS_PER_BYTE)
                {
                    if (wc != 0)
                    {
                        WARN("extra bits\n");
                        goto end;
                    }
                    break;
                }
                fmtptr++;
                *fmtptr |= (BYTE)wc;
            }
        }
        hr = S_OK;
    }
end:
    return hr;
}
