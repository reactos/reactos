/*
 * Copyright 2008 James Hawkins
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

#ifndef __WINE_CORHDR_H
#define __WINE_CORHDR_H

typedef enum CorTokenType
{
    mdtModule                   = 0x00000000,
    mdtTypeRef                  = 0x01000000,
    mdtTypeDef                  = 0x02000000,
    mdtFieldDef                 = 0x04000000,
    mdtMethodDef                = 0x06000000,
    mdtParamDef                 = 0x08000000,
    mdtInterfaceImpl            = 0x09000000,
    mdtMemberRef                = 0x0a000000,
    mdtCustomAttribute          = 0x0c000000,
    mdtPermission               = 0x0e000000,
    mdtSignature                = 0x11000000,
    mdtEvent                    = 0x14000000,
    mdtProperty                 = 0x17000000,
    mdtModuleRef                = 0x1a000000,
    mdtTypeSpec                 = 0x1b000000,
    mdtAssembly                 = 0x20000000,
    mdtAssemblyRef              = 0x23000000,
    mdtFile                     = 0x26000000,
    mdtExportedType             = 0x27000000,
    mdtManifestResource         = 0x28000000,
    mdtGenericParam             = 0x2a000000,
    mdtMethodSpec               = 0x2b000000,
    mdtGenericParamConstraint   = 0x2c000000,
    mdtString                   = 0x70000000,
    mdtName                     = 0x71000000,
    mdtBaseType                 = 0x72000000,
} CorTokenType;

#define RidToToken(rid,tktype) ((rid) |= (tktype))
#define TokenFromRid(rid,tktype) ((rid) | (tktype))
#define RidFromToken(tk) ((RID)((tk) & 0x00ffffff))
#define TypeFromToken(tk) ((ULONG32)((tk) & 0xff000000))
#define IsNilToken(tk) ((RidFromToken(tk)) == 0)

#endif /* __WINE_CORHDR_H */
