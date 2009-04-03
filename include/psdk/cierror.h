/*
 * Copyright 2006 Mike McCormack
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

#ifndef __WINE_CIERROR_H__
#define __WINE_CIERROR_H__

#define CI_E_ALREADY_INITIALIZED       0x8004180a
#define CI_E_NOT_INITIALIZED           0x8004180b
#define CI_E_BUFFERTOOSMALL            0x8004180c
#define CI_E_PROPERTY_NOT_CACHED       0x8004180d
#define CI_E_INVALID_STATE             0x8004180f
#define CI_E_FILTERING_DISABLED        0x80041810
#define CI_E_DISK_FULL                 0x80041811
#define CI_E_SHUTDOWN                  0x80041812
#define CI_E_WORKID_NOTVALID           0x80041813
#define CI_E_NOT_FOUND                 0x80041815
#define CI_E_USE_DEFAULT_PID           0x80041816
#define CI_E_DUPLICATE_NOTIFICATION    0x80041817
#define CI_E_UPDATES_DISABLED          0x80041818
#define CI_E_INVALID_FLAGS_COMBINATION 0x80041819
#define CI_E_OUTOFSEQ_INCREMENT_DATA   0x8004181a
#define CI_E_SHARING_VIOLATION         0x8004181b
#define CI_E_LOGON_FAILURE             0x8004181c
#define CI_E_NO_CATALOG                0x8004181d
#define CI_E_STRANGE_PAGEORSECTOR_SIZE 0x8004181e
#define CI_E_TIMEOUT                   0x8004181f
#define CI_E_NOT_RUNNING               0x80041820

#endif
