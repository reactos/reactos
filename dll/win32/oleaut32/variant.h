/*
 * Variant Inlines
 *
 * Copyright 2003 Jon Griffiths
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

#include "windef.h"
#include "winerror.h"
#include "objbase.h"
#include "oleauto.h"
#include <math.h>

/* Get just the type from a variant pointer */
#define V_TYPE(v)  (V_VT((v)) & VT_TYPEMASK)

/* Flags set in V_VT, other than the actual type value */
#define VT_EXTRA_TYPE (VT_VECTOR|VT_ARRAY|VT_BYREF|VT_RESERVED)

/* Get the extra flags from a variant pointer */
#define V_EXTRA_TYPE(v) (V_VT((v)) & VT_EXTRA_TYPE)

/* Missing in Windows but useful VTBIT_* defines */
#define VTBIT_BOOL      (1 << VT_BSTR)
#define VTBIT_BSTR      (1 << VT_BSTR)
#define VTBIT_DATE      (1 << VT_DATE)
#define VTBIT_DISPATCH  (1 << VT_DISPATCH)
#define VTBIT_EMPTY     (1 << VT_EMPTY)
#define VTBIT_ERROR     (1 << VT_ERROR)
#define VTBIT_INT       (1 << VT_INT)
#define VTBIT_NULL      (1 << VT_NULL)
#define VTBIT_UINT      (1 << VT_UINT)
#define VTBIT_UNKNOWN   (1 << VT_UNKNOWN)
#define VTBIT_VARIANT   (1 << VT_VARIANT)
#define VTBIT_15        (1 << 15)        /* no variant type with this number */

/* Size constraints */
#define I1_MAX   0x7f
#define I1_MIN   ((-I1_MAX)-1)
#define UI1_MAX  0xff
#define UI1_MIN  0
#define I2_MAX   0x7fff
#define I2_MIN   ((-I2_MAX)-1)
#define UI2_MAX  0xffff
#define UI2_MIN  0
#define I4_MAX   0x7fffffff
#define I4_MIN   ((-I4_MAX)-1)
#define UI4_MAX  0xffffffff
#define UI4_MIN  0
#define I8_MAX   (((LONGLONG)I4_MAX << 32) | UI4_MAX)
#define I8_MIN   ((-I8_MAX)-1)
#define UI8_MAX  (((ULONGLONG)UI4_MAX << 32) | UI4_MAX)
#define UI8_MIN  0
#define DATE_MAX 2958465
#define DATE_MIN -657434
#define R4_MAX 3.402823567797336e38
#define R4_MIN 1.40129846432481707e-45
#define R8_MAX 1.79769313486231470e+308
#define R8_MIN 4.94065645841246544e-324

/* Value of sign for a positive decimal number */
#define DECIMAL_POS 0

#define DEC_MAX_SCALE    28 /* Maximum scale for a decimal */

/* Internal flags for low level conversion functions */
#define  VAR_BOOLONOFF 0x0400 /* Convert bool to "On"/"Off" */
#define  VAR_BOOLYESNO 0x0800 /* Convert bool to "Yes"/"No" */
#define  VAR_NEGATIVE  0x1000 /* Number is negative */

unsigned int get_type_size(ULONG*, VARTYPE);
HRESULT VARIANT_ClearInd(VARIANTARG *);
BOOL get_date_format(LCID, DWORD, const SYSTEMTIME *,
        const WCHAR *, WCHAR *, int);
