/*
 * Copyright 2004 Jon Griffiths
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
 */

#ifndef MAPIVAL_H
#define MAPIVAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mapiutil.h>
#include <stddef.h>
#include <stdarg.h>

BOOL  WINAPI FBadRglpszW(LPWSTR*,ULONG);
BOOL  WINAPI FBadRowSet(LPSRowSet);
BOOL  WINAPI FBadRglpNameID(LPMAPINAMEID*,ULONG);
BOOL  WINAPI FBadEntryList(LPENTRYLIST);
ULONG WINAPI FBadRestriction(LPSRestriction);
ULONG WINAPI FBadPropTag(ULONG);
ULONG WINAPI FBadRow(LPSRow);
ULONG WINAPI FBadProp(LPSPropValue);
ULONG WINAPI FBadSortOrderSet(LPSSortOrderSet);
ULONG WINAPI FBadColumnSet(LPSPropTagArray);

#define FBadRgPropVal(p,n) FAILED(ScCountProps((n),(p),NULL))
#define FBadPropVal(p)     FBadRgPropVal(1,(p))
#define FBadAdrList(p)     FBadRowSet((LPSRowSet)(p))

#define BAD_STANDARD_OBJ(a,b,c,d)         FALSE
#define FBadUnknown(a)                    FALSE
#define FBadQueryInterface(a,b,c)         FALSE
#define FBadAddRef(a)                     FALSE
#define FBadRelease(a)                    FALSE
#define FBadGetLastError(a,b,c,d)         FALSE
#define FBadSaveChanges(a,b)              FALSE
#define FBadGetProps(a,b,c,d)             FALSE
#define FBadGetPropList(a,b)              FALSE
#define FBadOpenProperty(a,b,c,d,e,f)     FALSE
#define FBadSetProps(a,b,c,d)             FALSE
#define FBadDeleteProps(a,b,c)            FALSE
#define FBadCopyTo(a,b,c,d,e,f,g,h,i,j)   FALSE
#define FBadCopyProps(a,b,c,d,e,f,g,h)    FALSE
#define FBadGetNamesFromIDs(a,b,c,d,e,f)  FALSE
#define FBadGetIDsFromNames(a,b,c,d,e)    FALSE

#define ValidateParms(x)   do { } while(0)
#define UlValidateParms(x) do { } while(0)
#define CheckParms(x)      do { } while(0)

#define ValidateParameters1(a,b) do { } while(0)
#define ValidateParameters2(a,b,c) do { } while(0)
#define ValidateParameters3(a,b,c,d) do { } while(0)
#define ValidateParameters4(a,b,c,d,e) do { } while(0)
#define ValidateParameters5(a,b,c,d,e,f) do { } while(0)
#define ValidateParameters6(a,b,c,d,e,f,g) do { } while(0)
#define ValidateParameters7(a,b,c,d,e,f,g,h) do { } while(0)
#define ValidateParameters8(a,b,c,d,e,f,g,h,i) do { } while(0)
#define ValidateParameters9(a,b,c,d,e,f,g,h,i,j) do { } while(0)
#define ValidateParameters10(a,b,c,d,e,f,g,h,i,j,k) do { } while(0)
#define ValidateParameters11(a,b,c,d,e,f,g,h,i,j,k,l) do { } while(0)
#define ValidateParameters12(a,b,c,d,e,f,g,h,i,j,k,l,m) do { } while(0)
#define ValidateParameters13(a,b,c,d,e,f,g,h,i,j,k,l,m,n) do { } while(0)
#define ValidateParameters14(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) do { } while(0)
#define ValidateParameters15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) do { } while(0)
#define ValidateParameters16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q) do { } while(0)

#define UlValidateParameters1(a,b) do { } while(0)
#define UlValidateParameters2(a,b,c) do { } while(0)
#define UlValidateParameters3(a,b,c,d) do { } while(0)
#define UlValidateParameters4(a,b,c,d,e) do { } while(0)
#define UlValidateParameters5(a,b,c,d,e,f) do { } while(0)
#define UlValidateParameters6(a,b,c,d,e,f,g) do { } while(0)
#define UlValidateParameters7(a,b,c,d,e,f,g,h) do { } while(0)
#define UlValidateParameters8(a,b,c,d,e,f,g,h,i) do { } while(0)
#define UlValidateParameters9(a,b,c,d,e,f,g,h,i,j) do { } while(0)
#define UlValidateParameters10(a,b,c,d,e,f,g,h,i,j,k) do { } while(0)
#define UlValidateParameters11(a,b,c,d,e,f,g,h,i,j,k,l) do { } while(0)
#define UlValidateParameters12(a,b,c,d,e,f,g,h,i,j,k,l,m) do { } while(0)
#define UlValidateParameters13(a,b,c,d,e,f,g,h,i,j,k,l,m,n) do { } while(0)
#define UlValidateParameters14(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) do { } while(0)
#define UlValidateParameters15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) do { } while(0)
#define UlValidateParameters16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q) do { } while(0)

#define CheckParameters1(a,b) do { } while(0)
#define CheckParameters2(a,b,c) do { } while(0)
#define CheckParameters3(a,b,c,d) do { } while(0)
#define CheckParameters4(a,b,c,d,e) do { } while(0)
#define CheckParameters5(a,b,c,d,e,f) do { } while(0)
#define CheckParameters6(a,b,c,d,e,f,g) do { } while(0)
#define CheckParameters7(a,b,c,d,e,f,g,h) do { } while(0)
#define CheckParameters8(a,b,c,d,e,f,g,h,i) do { } while(0)
#define CheckParameters9(a,b,c,d,e,f,g,h,i,j) do { } while(0)
#define CheckParameters10(a,b,c,d,e,f,g,h,i,j,k) do { } while(0)
#define CheckParameters11(a,b,c,d,e,f,g,h,i,j,k,l) do { } while(0)
#define CheckParameters12(a,b,c,d,e,f,g,h,i,j,k,l,m) do { } while(0)
#define CheckParameters13(a,b,c,d,e,f,g,h,i,j,k,l,m,n) do { } while(0)
#define CheckParameters14(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) do { } while(0)
#define CheckParameters15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) do { } while(0)
#define CheckParameters16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q) do { } while(0)

#ifdef __cplusplus
}
#endif

#endif /* MAPIVAL_H */
