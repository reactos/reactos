/*
 * Copyright (C) 2000 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;
#endif

#undef DEFINE_GUID

#ifdef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const GUID name = \
	{ l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    extern const GUID name;
#endif

#ifndef _GUIDDEF_H_
#define _GUIDDEF_H_

typedef GUID *LPGUID;
typedef GUID CLSID,*LPCLSID;
typedef GUID IID,*LPIID;
typedef GUID FMTID,*LPFMTID;

#if 0
#if defined(__cplusplus) && !defined(CINTERFACE)
#define REFGUID             const GUID &
#define REFCLSID            const CLSID &
#define REFIID              const IID &
#define REFFMTID            const FMTID &
#else /* !defined(__cplusplus) && !defined(CINTERFACE) */
#define REFGUID             const GUID* const
#define REFCLSID            const CLSID* const
#define REFIID              const IID* const
#define REFFMTID            const FMTID* const
#endif /* !defined(__cplusplus) && !defined(CINTERFACE) */
#endif

#if defined(__cplusplus) && !defined(CINTERFACE)
#define IsEqualGUID(rguid1, rguid2) (!memcmp(&(rguid1), &(rguid2), sizeof(GUID)))
#else /* defined(__cplusplus) && !defined(CINTERFACE) */
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#endif /* defined(__cplusplus) && !defined(CINTERFACE) */

#if defined(__cplusplus) && !defined(CINTERFACE)
#include <string.h>
inline bool operator==(const GUID& guidOne, const GUID& guidOther)
{
    return !memcmp(&guidOne,&guidOther,sizeof(GUID));
}
inline bool operator!=(const GUID& guidOne, const GUID& guidOther)
{
    return !(guidOne == guidOther);
}
#endif

extern const IID GUID_NULL;
#define IID_NULL            GUID_NULL
#define CLSID_NULL GUID_NULL
#define FMTID_NULL          GUID_NULL

#endif /* _GUIDDEF_H_ */
