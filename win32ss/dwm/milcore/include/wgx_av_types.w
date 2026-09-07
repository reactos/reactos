/*
 Licensed to the .NET Foundation under one or more agreements.
 The .NET Foundation licenses this file to you under the MIT license.
 See the LICENSE file in the project root for more information.
*/


/*=========================================================================*\



    File: avtypes.w

    Module Name: Milcore

    Description: Audio/Video types.

\*=========================================================================*/

#################################
#
#    C# Header
#
#################################
;begin CSHARP_ONLY
using System;
using System.Runtime.InteropServices;

namespace System.Windows.Media
{
;end CSHARP_ONLY

#################################
#
#    C++ Header
#
#################################
;begin CPP_ONLY
#pragma once
;end CPP_ONLY

#################################
#
#   Common Enumerations
#
#################################
;begin COMMON

//
// AV Events
//
enum AVEvent
{
    AVMediaNone              = 0,
    AVMediaOpened            = 1,
    AVMediaClosed            = 2,
    AVMediaStarted           = 3,
    AVMediaStopped           = 4,
    AVMediaPaused            = 5,
    AVMediaRateChanged       = 6,
    AVMediaEnded             = 7,
    AVMediaFailed            = 8,
    AVMediaBufferingStarted  = 9,
    AVMediaBufferingEnded    = 10,
    AVMediaPrerolled         = 11,
    AVMediaScriptCommand     = 12,
    AVMediaNewFrame          = 13,
};

;end COMMON

;begin CPP_ONLY
//
// Align(4) guarantees that this will be preserved as two sequential double words.
//
struct __declspec(align(4)) AVEventData
{
    DWORD       avEvent;
    HRESULT     errorHResult;
    ULONG       typeLength;
    ULONG       paramLength;
    WCHAR       typeAndParamStrings[1];
};
;end CPP_ONLY

;end PUBLIC

#############################################
#
#    C# footer
#
#############################################
;begin CSHARP_ONLY
}
;end CSHARP_ONLY



