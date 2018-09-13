/***                                                                  ***/
/***   INTEL CORPORATION PROPRIETARY INFORMATION                      ***/
/***                                                                  ***/
/***   This software is supplied under the terms of a license         ***/
/***   agreement or nondisclosure agreement with Intel Corporation    ***/
/***   and may not be copied or disclosed except in accordance with   ***/
/***   the terms of that agreement.                                   ***/
/***   Copyright (c) 1992,1993,1994,1995,1996,1997 Intel Corporation. ***/
/***                                                                  ***/

#ifndef _EM_TOOLS_H
#define _EM_TOOLS_H

typedef struct EM_version_s
{
	int      major;
	int      minor;
} EM_version_t;

typedef struct EM_library_version_s
{
	EM_version_t   xversion;
	EM_version_t   api;
	EM_version_t   emdb;
    char           date[12];
    char           time[9];
} EM_library_version_t;

#endif /* _EM_TOOLS_H */

