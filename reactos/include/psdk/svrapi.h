/*
* Copyright 2006 Konstantin Petrov
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

#ifndef SVRAPI_INCLUDED
#define SVRAPI_INCLUDED

#include <lmcons.h>
#include <lmerr.h>

#include <pshpack1.h>

typedef struct _share_info_1 {
       char shi1_netname[LM20_NNLEN+1];
       char shi1_pad1;
       unsigned short shi1_type;
       char* shi1_remark;
} share_info_1;

typedef struct _share_info_50 {
       char shi50_netname[LM20_NNLEN+1];
       unsigned char shi50_type;
       unsigned short shi50_flags;
       char* shi50_remark;
       char* shi50_path;
       char shi50_rw_password[SHPWLEN+1];
       char shi50_ro_password[SHPWLEN+1];
} share_info_50;

#include <poppack.h>

#endif /* SVRAPI_INCLUDED */
