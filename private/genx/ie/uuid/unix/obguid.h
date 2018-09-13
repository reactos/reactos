/* processed by mwprepro */
/***
*obguid.h
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  Definitions of private OB owned GUIDs.
*
* [00]	26-Jan-93 gustavj: Created.
*
*Implementation Notes:
*  OLE has given OB a range of GUIDs to use for its classes.  This range
*  consists of 256 possible GUIDs.  The GUIDs for public classes
*  (such as ItypeInfo) take GUIDs starting at the low end of the range,
*  private classes(such as GEN_DTINFO), take GUIDs starting at the high
*  end.  New GUIDs that are added should maintain this convention.
*
*  GUIDs for OB public classes are defined in switches.hxx.
*
*****************************************************************************/

#ifndef obguid_HXX_INCLUDED
#define obguid_HXX_INCLUDED

#define DEFINE_OBOLEGUID(name, b) DEFINE_OLEGUID(name,(0x00020400+b), 0, 0);

DEFINE_OBOLEGUID(CLSID_GenericTypeLibOLE, 0xff)

DEFINE_OBOLEGUID(IID_TYPEINFO	      , 0xfc)
DEFINE_OBOLEGUID(IID_DYNTYPEINFO      , 0xfb)

DEFINE_OBOLEGUID(IID_CDefnTypeComp    , 0xf5)

DEFINE_OBOLEGUID(IID_TYPELIB_GEN_DTINFO  , 0xf2)

// {DD23B040-296F-101B-99A1-08002B2BD119}
DEFINE_GUID(CLSID_TypeLibCF,
    0xDD23B040L,0x296F,0x101B,0x99,0xA1,0x08,0x00,0x2B,0x2B,0xD1,0x19);
//{F5AA2660-BA14-1069-8AEE-00DD010F7D13}
DEFINE_GUID(IID_IGenericTypeLibOLE,
    0xF5AA2660L,0xBA14,0x1069,0x8A,0xEE,0x00,0xDD,0x01,0x0F,0x7D,0x13);


#endif  // !obguid_HXX_INCLUDED

