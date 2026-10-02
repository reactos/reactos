// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


using System;
using System.Runtime.InteropServices;

namespace System.Windows.Media
{
    
    //
    // AV Events
    //
    internal enum AVEvent    {
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
    
}

