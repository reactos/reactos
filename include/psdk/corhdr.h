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

typedef enum CorElementType
{
    ELEMENT_TYPE_END            = 0x00,
    ELEMENT_TYPE_VOID           = 0x01,
    ELEMENT_TYPE_BOOLEAN        = 0x02,
    ELEMENT_TYPE_CHAR           = 0x03,
    ELEMENT_TYPE_I1             = 0x04,
    ELEMENT_TYPE_U1             = 0x05,
    ELEMENT_TYPE_I2             = 0x06,
    ELEMENT_TYPE_U2             = 0x07,
    ELEMENT_TYPE_I4             = 0x08,
    ELEMENT_TYPE_U4             = 0x09,
    ELEMENT_TYPE_I8             = 0x0a,
    ELEMENT_TYPE_U8             = 0x0b,
    ELEMENT_TYPE_R4             = 0x0c,
    ELEMENT_TYPE_R8             = 0x0d,
    ELEMENT_TYPE_STRING         = 0x0e,
    ELEMENT_TYPE_PTR            = 0x0f,
    ELEMENT_TYPE_BYREF          = 0x10,
    ELEMENT_TYPE_VALUETYPE      = 0x11,
    ELEMENT_TYPE_CLASS          = 0x12,
    ELEMENT_TYPE_VAR            = 0x13,
    ELEMENT_TYPE_ARRAY          = 0x14,
    ELEMENT_TYPE_GENERICINST    = 0x15,
    ELEMENT_TYPE_TYPEDBYREF     = 0x16,
    ELEMENT_TYPE_I              = 0x18,
    ELEMENT_TYPE_U              = 0x19,
    ELEMENT_TYPE_FNPTR          = 0x1b,
    ELEMENT_TYPE_OBJECT         = 0x1c,
    ELEMENT_TYPE_SZARRAY        = 0x1d,
    ELEMENT_TYPE_MVAR           = 0x1e,
    ELEMENT_TYPE_CMOD_REQD      = 0x1f,
    ELEMENT_TYPE_CMOD_OPT       = 0x20,
    ELEMENT_TYPE_INTERNAL       = 0x21,
    ELEMENT_TYPE_MAX            = 0x22,
    ELEMENT_TYPE_MODIFIER       = 0x40,
    ELEMENT_TYPE_SENTINEL       = 0x01 | ELEMENT_TYPE_MODIFIER,
    ELEMENT_TYPE_PINNED         = 0x05 | ELEMENT_TYPE_MODIFIER,
    ELEMENT_TYPE_R4_HFA         = 0x06 | ELEMENT_TYPE_MODIFIER,
    ELEMENT_TYPE_R8_HFA         = 0x07 | ELEMENT_TYPE_MODIFIER,

} CorElementType;

#define RidToToken(rid,tktype) ((rid) |= (tktype))
#define TokenFromRid(rid,tktype) ((rid) | (tktype))
#define RidFromToken(tk) ((RID)((tk) & 0x00ffffff))
#define TypeFromToken(tk) ((ULONG32)((tk) & 0xff000000))
#define IsNilToken(tk) ((RidFromToken(tk)) == 0)

typedef LPVOID  mdScope;
typedef ULONG32 mdToken;

typedef mdToken mdModule;
typedef mdToken mdTypeRef;
typedef mdToken mdTypeDef;
typedef mdToken mdFieldDef;
typedef mdToken mdMethodDef;
typedef mdToken mdParamDef;
typedef mdToken mdInterfaceImpl;
typedef mdToken mdMemberRef;
typedef mdToken mdCustomAttribute;
typedef mdToken mdPermission;
typedef mdToken mdSignature;
typedef mdToken mdEvent;
typedef mdToken mdProperty;
typedef mdToken mdModuleRef;
typedef mdToken mdAssembly;
typedef mdToken mdAssemblyRef;
typedef mdToken mdFile;
typedef mdToken mdExportedType;
typedef mdToken mdManifestResource;
typedef mdToken mdTypeSpec;
typedef mdToken mdGenericParam;
typedef mdToken mdMethodSpec;
typedef mdToken mdGenericParamConstraint;
typedef mdToken mdString;
typedef mdToken mdCPToken;

#endif /* __WINE_CORHDR_H */
