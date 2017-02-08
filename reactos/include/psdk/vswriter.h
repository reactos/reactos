/*
 * Copyright 2014 Hans Leidekker for CodeWeavers
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

#ifndef __WINE_VSWRITER_H
#define __WINE_VSWRITER_H

typedef enum
{
    VSS_UT_UNDEFINED,
    VSS_UT_BOOTABLESYSTEMSTATE,
    VSS_UT_SYSTEMSERVICE,
    VSS_UT_USERDATA,
    VSS_UT_OTHER
} VSS_USAGE_TYPE;

typedef enum
{
    VSS_ST_UNDEFINED,
    VSS_ST_TRANSACTEDDB,
    VSS_ST_NONTRANSACTEDDB,
    VSS_ST_OTHER
} VSS_SOURCE_TYPE;

typedef enum
{
    VSS_AWS_UNDEFINED,
    VSS_AWS_NO_ALTERNATE_WRITER,
    VSS_AWS_ALTERNATE_WRITER_EXISTS,
    VSS_AWS_THIS_IS_ALTERNATE_WRITER
} VSS_ALTERNATE_WRITER_STATE;

#endif /* ___WINE_VSWRITER_H */
