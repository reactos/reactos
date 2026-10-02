// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Build a list or expressions
//

namespace MS.Internal.MilCodeGen.Runtime
{
    using System;

    public sealed class OrList : DelimitedList
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public OrList() : base ("|| ", DelimiterPosition.BeforeItem) {}

        #endregion Constructors
    }
}




