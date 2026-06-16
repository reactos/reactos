/*
 * Copyright (C) 2019 Dmitry Timoshkov
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

#ifndef __ADSERR_H
#define __ADSERR_H

#ifdef RC_INVOKED
#define _HRESULT_TYPEDEF_(x) (x)
#else
#define _HRESULT_TYPEDEF_(x) ((HRESULT)x)
#endif

#define E_ADS_BAD_PATHNAME                                 _HRESULT_TYPEDEF_(0x80005000)
#define E_ADS_INVALID_DOMAIN_OBJECT                        _HRESULT_TYPEDEF_(0x80005001)
#define E_ADS_INVALID_USER_OBJECT                          _HRESULT_TYPEDEF_(0x80005002)
#define E_ADS_INVALID_COMPUTER_OBJECT                      _HRESULT_TYPEDEF_(0x80005003)
#define E_ADS_UNKNOWN_OBJECT                               _HRESULT_TYPEDEF_(0x80005004)
#define E_ADS_PROPERTY_NOT_SET                             _HRESULT_TYPEDEF_(0x80005005)
#define E_ADS_PROPERTY_NOT_SUPPORTED                       _HRESULT_TYPEDEF_(0x80005006)
#define E_ADS_PROPERTY_INVALID                             _HRESULT_TYPEDEF_(0x80005007)
#define E_ADS_BAD_PARAMETER                                _HRESULT_TYPEDEF_(0x80005008)
#define E_ADS_OBJECT_UNBOUND                               _HRESULT_TYPEDEF_(0x80005009)
#define E_ADS_PROPERTY_NOT_MODIFIED                        _HRESULT_TYPEDEF_(0x8000500A)
#define E_ADS_PROPERTY_MODIFIED                            _HRESULT_TYPEDEF_(0x8000500B)
#define E_ADS_CANT_CONVERT_DATATYPE                        _HRESULT_TYPEDEF_(0x8000500C)
#define E_ADS_PROPERTY_NOT_FOUND                           _HRESULT_TYPEDEF_(0x8000500D)
#define E_ADS_OBJECT_EXISTS                                _HRESULT_TYPEDEF_(0x8000500E)
#define E_ADS_SCHEMA_VIOLATION                             _HRESULT_TYPEDEF_(0x8000500F)
#define E_ADS_COLUMN_NOT_SET                               _HRESULT_TYPEDEF_(0x80005010)
#define S_ADS_ERRORSOCCURRED                               _HRESULT_TYPEDEF_(0x00005011)
#define S_ADS_NOMORE_ROWS                                  _HRESULT_TYPEDEF_(0x00005012)
#define S_ADS_NOMORE_COLUMNS                               _HRESULT_TYPEDEF_(0x00005013)
#define E_ADS_INVALID_FILTER                               _HRESULT_TYPEDEF_(0x80005014)

#endif /* __ADSERR_H */
