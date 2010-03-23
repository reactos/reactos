/*
 * internal include file of the Local Printmonitor User Interface
 *
 * Copyright 2007 Detlef Riekenberg
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

#ifndef __WINE_LOCALUI__
#define __WINE_LOCALUI__

#include <windef.h>
#include <winuser.h>

/* ## Resource-ID ## */
#define ADDPORT_DIALOG  100
#define ADDPORT_EDIT    101

#define LPTCONFIG_DIALOG 200
#define LPTCONFIG_GROUP  201
#define LPTCONFIG_EDIT   202

#define IDS_LOCALPORT       300
#define IDS_INVALIDNAME     301
#define IDS_PORTEXISTS      302
#define IDS_NOTHINGTOCONFIG 303

/* ## Reserved memorysize for the strings (in WCHAR) ## */
#define IDS_LOCALPORT_MAXLEN 32
#define IDS_INVALIDNAME_MAXLEN 48
#define IDS_PORTEXISTS_MAXLEN  48
#define IDS_NOTHINGTOCONFIG_MAXLEN 80

/* ## Type of Ports ## */
/* windows types */
#define PORT_IS_UNKNOWN  0
#define PORT_IS_LPT      1
#define PORT_IS_COM      2
#define PORT_IS_FILE     3
#define PORT_IS_FILENAME 4

/* wine extensions */
#define PORT_IS_WINE     5
#define PORT_IS_UNIXNAME 5
#define PORT_IS_PIPE     6
#define PORT_IS_CUPS     7
#define PORT_IS_LPR      8


#endif /* __WINE_LOCALUI__ */
