#ifndef _INC_REACTOS_RESOURCE_H
#define _INC_REACTOS_RESOURCE_H

/* Global File Version UINTs */

#define RES_UINT_FV_MAJOR	0
#define RES_UINT_FV_MINOR	0
#define RES_UINT_FV_REVISION	14
/* Build number as YYYYMMDD */
#define RES_UINT_FV_BUILD	19990608

/* ReactOS Product Version UINTs */

#define RES_UINT_PV_MAJOR	0
#define RES_UINT_PV_MINOR	0
#define RES_UINT_PV_REVISION	14
/* Build number as YYYYMMDD */
#define RES_UINT_PV_BUILD	19990608

/* Common version strings for rc scripts */

#define RES_STR_COMPANY_NAME	"ReactOS Development Team\0"
#define RES_STR_LEGAL_COPYRIGHT	"Copyright (c) 1998, 1999 ReactOS Team\0"
#define RES_STR_PRODUCT_NAME	"ReactOS Operating System\0"
#define RES_STR_PRODUCT_VERSION	"post 0.0.14\0"

/* FILE_VERSION defaults to PRODUCT_VERSION */
#define RES_STR_FILE_VERSION	RES_STR_PRODUCT_VERSION

#endif /* ndef _INC_REACTOS_RESOURCE_H */
