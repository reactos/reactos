// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Build a list of parameters. Handles the comma between parameters.
//

namespace MS.Internal.MilCodeGen.Runtime
{
    using System;

    public sealed class ParameterList : DelimitedList
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public ParameterList() : base (",", DelimiterPosition.AfterItem) {}

        #endregion Constructors
    }
}




