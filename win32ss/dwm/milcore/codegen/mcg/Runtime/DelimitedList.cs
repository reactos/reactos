// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Build a list of delimited items, similar to String.Join but each
//              item exists on a seperate line.
//

namespace MS.Internal.MilCodeGen.Runtime
{
    using System;
    using System.IO;
    using System.Text;
    using System.Collections;

    public enum DelimiterPosition
    {
        BeforeItem,
        AfterItem
    }

    public class DelimitedList
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        public DelimitedList(string delimiter, DelimiterPosition position): this(delimiter, position, true)
        {
        }

        public DelimitedList(string delimiter, DelimiterPosition position, bool insertNewline)
        {
            _parameters = new StringBuilder();
            _lastComment = "";
            _delimiter = delimiter;
            _delimiterPosition = position;
            _insertNewline = insertNewline;
        }
        #endregion Constructors


        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods
        public override string ToString()
        {
            ApplyLastComment();
            return _parameters.ToString();
        }

        public void Clear()
        {
            _parameters.Length = 0;
        }

        public void Append(string[] parameters)
        {
            if (parameters != null)
            {
                for (int i = 0; i < parameters.Length; i++)
                {
                    Append(parameters[i], "");
                }
            }
        }

        public void Append(string sParam)
        {
            Append(sParam, "");
        }

        /// <summary>
        /// Append a parameter, with an optional end-of-line "//" comment.
        /// </summary>
        public void Append(string sParam, string comment)
        {
            bool firstParameter = _parameters.Length == 0;

            if (!firstParameter && _delimiterPosition == DelimiterPosition.AfterItem) 
            {
                _parameters.Append(_delimiter);
            }

            ApplyLastComment();

            if (!firstParameter && _insertNewline) 
            {
                _parameters.Append("\n");
            }

            if (comment != "") 
            {
                comment = " // " + comment;
            }

            if (!firstParameter && _delimiterPosition == DelimiterPosition.BeforeItem) 
            {
                _parameters.Append(_delimiter);
            }

            _lastComment = comment;
            _parameters.Append(sParam);
        }
        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------


        //------------------------------------------------------
        //
        //  Public Events
        //
        //------------------------------------------------------


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods
        /// <summary>
        /// Apply the last comment given to us, if any.
        /// Note: Must be idempotent.
        /// </summary>
        private void ApplyLastComment()
        {
            _parameters.Append(_lastComment);
            _lastComment = "";
        }
        #endregion Private Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields
        StringBuilder _parameters;
        string _lastComment;
        string _delimiter;
        DelimiterPosition _delimiterPosition;
        bool _insertNewline;
        #endregion Private Fields
    }
}




