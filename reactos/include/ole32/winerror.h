/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include\ole32\winerror.h
 * PURPOSE:         Defines windows error codes
 * PROGRAMMER:      jurgen van gael [jurgen.vangael@student.kuleuven.ac.be]
 * UPDATE HISTORY:
 *                  Created 01/05/2001
 */
/********************************************************************


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.


********************************************************************/
#ifndef _WINERROR_H
#define _WINERROR_H

//
//	Return Code macros
//
#define	SUCCEEDED(Status)	((HRESULT)(Status) >= 0)
#define	FAILED(Status)		((HRESULT)(Status)<0)

//
//	Success Codes
//
#define S_OK						0x00000000L
#define S_FALSE						0x00000001L

//
//	Error Codes
//
#define E_NOINTERFACE				0x80000004L
#define E_POINTER					0x80004003L
#define	E_FAIL						0x80004005L
#define E_UNEXPECTED				0x8000FFFFL
#define	CLASS_E_NOAGGREGATION		0x80040110L
#define	CLASS_E_CLASSNOTAVAILABLE	0x80040111L
#define	E_OUTOFMEMORY				0x8007000EL
#define E_INVALIDARG				0x80070057L


#endif