// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************\
* 

*
* Module Name:
*
*   sRGB/sRGB64 conversion.
*
* Abstract:
*
*   Conversion between the sRGB and sRGB64 color spaces.
*
* Created:
*
*   6/9/1999 agodfrey
*
\**************************************************************************/

// We cast ARGB and ARGB64 variables to these unions when necessary.
// We could instead define ARGB and ARGB64 to be unions themselves, but the
// compiler is bad at enregistering unions.

#define SRGB_FRACTIONBITS 13U
#define SRGB_INTEGERBITS (16U - SRGB_FRACTIONBITS)
#define SRGB_ONE (1U << (SRGB_FRACTIONBITS))
#define SRGB_HALF (1U << (SRGB_FRACTIONBITS - 1U))
#define SRGB_MAX 32767U
#define SRGB_MIN -32768



