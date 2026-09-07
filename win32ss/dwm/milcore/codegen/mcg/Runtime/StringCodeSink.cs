// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: CodeSink which builds a string.
//

namespace MS.Internal.MilCodeGen.Runtime
{
    using System;
    using System.IO;
    using System.Text;
    using System.Collections;

    public class StringCodeSink : CodeSink
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        public StringCodeSink()
        {
            _stringBuilder = new StringBuilder();
        }
        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods
        public void Clear()
        {
            _stringBuilder.Length = 0;
        }

        public override string ToString()
        {
            return _stringBuilder.ToString();
        }
        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------

        #region Public Properties
        public bool IsEmpty
        {
            get
            {
                return _stringBuilder.Length == 0;
            }
        }
        #endregion Public Properties
        
        //------------------------------------------------------
        //
        //  Public Events
        //
        //------------------------------------------------------


        //------------------------------------------------------
        //
        //  Protected Internal Methods
        //
        //------------------------------------------------------
        
        #region Protected Internal Methods
        protected internal override void InternalWrite(string output)
        {
            _stringBuilder.Append(output);
        }
        #endregion
        
        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields
        StringBuilder _stringBuilder;
        #endregion Private Fields
    }

}




