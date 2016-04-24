/*
 * ntintsafe.h
 *
 * Windows NT helper functions for integer overflow prevention
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
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
#pragma once

#ifndef _NTINTSAFE_H_INCLUDED_
#define _NTINTSAFE_H_INCLUDED_

/* Include the sdk version */
#include <intsafe.h>

/* We don't want this one */
#undef _INTSAFE_H_INCLUDED_

#endif // !_NTINTSAFE_H_INCLUDED_
