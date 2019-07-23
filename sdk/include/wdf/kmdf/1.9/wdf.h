/*
 * wdf.h
 *
 * Windows Driver Framework - Primary Header File
 *
 * This file is part of the ReactOS WDF package.
 *
 * Contributors:
 *   Created by Benjamin Aerni <admin@bennottelling.com>
 *
 * Intended Usecase:
 *   Kernel mode drivers
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef _WDF_H_
#define _WDF_H_


typedef VOID (NTAPI *WDFFUNC) (VOID);
extern const WDFFUNC WdfFunctions[];

#ifndef _Dispatch_type_
#include <driverspecs.h>
#endif

/* Basic Defintions */
#include "wdfassert.h"
#include "wdffuncenum.h"
#include "wdfglobals.h"
#include "wdfpool.h"
#include "wdfstatus.h"
#include "wdftypes.h"
#include "wdfverifier.h"

/* Generic Objects */
/* #include "wdfobject.h" */
#endif /* _WDF_H_ */
