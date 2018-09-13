/*** 
*clsid.h
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file defines the CLSIDs
*
*Implementation Notes:
*
*****************************************************************************/

#if defined(WIN32) && 0
DEFINE_OLEGUID(CLSID_PSDispatch, 	0x00020400, 0, 0);
DEFINE_OLEGUID(CLSID_PSEnumVARIANT,	0x00020404, 0, 0);
DEFINE_OLEGUID(CLSID_PSTypeInfo,	0x00020401, 0, 0);
DEFINE_OLEGUID(CLSID_PSTypeLib,		0x00020402, 0, 0);
#else
DEFINE_OLEGUID(CLSID_PSDispatch, 	0x00020420, 0, 0);
DEFINE_OLEGUID(CLSID_PSEnumVARIANT,	0x00020421, 0, 0);
DEFINE_OLEGUID(CLSID_PSTypeInfo,	0x00020422, 0, 0);
DEFINE_OLEGUID(CLSID_PSTypeLib,		0x00020423, 0, 0);
#endif

DEFINE_OLEGUID(CLSID_PSAutomation,	0x00020424, 0, 0);
DEFINE_OLEGUID(CLSID_PSTypeComp,	0x00020425, 0, 0);

DEFINE_OLEGUID(CLSID_InProcFreeMarshaler, 0x0000001c, 0, 0);
