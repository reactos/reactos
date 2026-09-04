// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*------------------------------------------------------------------------------
//

//
//  File:       W4Warn.h
//  Contents:   #pragmas to adjust warning levels.
//  Note:       we don't want to use a single line comment before the warning is
//              disabled.
//----------------------------------------------------------------------------*/

#pragma once
#ifdef _MANAGED
#pragma unmanaged
#endif

/*
 * This helps to track down "Illegal attempt to instantiate abstract class" messages
 */
#pragma warning(error:4259) /* pure virtual function not defined                                       */

#pragma warning(error:4102) /* 'label' : unreferenced label                                            */

#pragma warning(disable:4041) /* compiler limit reached: terminating browser output                    */

#ifdef _M_IA64
#pragma warning(disable:4268) /* 'variable' : 'const' static/global data initialized with compiler generated default constructor fills the object with zeros */
#endif

/*
 * Const members are encouraged and more beneficial than auto-generated assignment operators
 */
#pragma warning(disable:4512) /* 'class': assignment operator could not be generated                   */

