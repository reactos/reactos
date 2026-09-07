// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Base class to which source code can be written.
//

namespace MS.Internal.MilCodeGen.Runtime
{
    using System;
    using System.IO;
    using System.Text;
    using System.Collections;
    using System.Diagnostics;

    public abstract class CodeSink
    {
        public const int BlockNoBlankLines = -1;

        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        public CodeSink()
        {
            _numFollowingLines = BlockNoBlankLines;
            _indentAmount = 0;
            _indentStack = new Stack(2);
        }
        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods
        /// <summary>
        /// Write the given text, verbatim.
        /// </summary>
        public void Write(string output)
        {
            BeginBlock(0);
            WriteWithIndentation(output);
            EndBlock(0);
        }

        /// <summary>
        /// Write the given text. Assumes this is a block of text
        /// (blocks are separated by a blank line).
        /// </summary>
        public void WriteBlock(string output)
        {
            BeginBlock(0);
            WriteWithIndentation(output);
            EndBlock(1);
        }

        /// <summary>
        /// Begin a block of text.
        /// This method will write out the correct number of blank lines.
        ///
        /// You probably don't want any blank lines if this is the first thing
        /// written, so this method doesn't write out anything if
        /// _numFollowingLines is BlockNoBlankLines (indicating the beginning of
        /// the file, method, code block, etc.)
        ///
        /// Otherwise, the number of blank lines printed out is the maximum of
        /// 1) the number of blank lines following the last block and
        /// 2) the number of blank lines preceding the next block.
        ///
        /// </summary>
        public void BeginBlock(int numPrecedingNewlines)
        {
            if (_numFollowingLines != BlockNoBlankLines)
            {
                for (int i = 0; i < Math.Max(_numFollowingLines, numPrecedingNewlines); i++)
                {
                    WriteWithIndentation("\n");
                }
            }

            _numFollowingLines = 0;
        }

        /// <summary>
        /// End a block of text.
        ///
        /// Since you probably don't want any blank lines unless there are more
        /// things written, this method stores the number of lines, but doesn't
        /// write them out until a new block is started.
        ///
        /// If you wish to mark the "beginning", meaning no blank lines will be
        /// printed out for the next block of text, you can call
        /// EndBlock(BlockNoBlankLines)
        /// </summary>
        public void EndBlock(int numFollowingNewlines)
        {
            Debug.Assert(_numFollowingLines == BlockNoBlankLines || _numFollowingLines >= 0);

            _numFollowingLines = numFollowingNewlines;
        }

        /// <summary>
        /// Marks the end of a block (written with a series of Write() calls).
        /// </summary>
        public void EndBlock()
        {
            EndBlock(1);
        }

        /// <summary>
        /// Indent - Increase the indent amount by the specified amount. 
        /// </summary>
        public void Indent(int indentAmount)
        {
            Debug.Assert(_indentAmount < Int32.MaxValue - indentAmount);

            _indentStack.Push(_indentAmount);
            _indentAmount += indentAmount;
        }

        /// <summary>
        /// Unindent - reduce the indent level one increment
        /// </summary>
        public void Unindent()
        {
            Debug.Assert(_indentStack.Count > 0);
            _indentAmount = (int)_indentStack.Pop();
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
        //  Protected Internal Methods
        //
        //------------------------------------------------------
        
        #region Protected Internal Methods
        protected internal abstract void InternalWrite(string output);
        #endregion


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        private void WriteWithIndentation(string output)
        {
            StringBuilder spacesBuilder = new StringBuilder(_indentAmount);

            for (int i = 0; i < _indentAmount; i++)
            {
                spacesBuilder.Append(' ');
            }

            string spaces = spacesBuilder.ToString();

            int startIndex = 0;

            while (startIndex < output.Length)
            {
                int endIndex = output.IndexOf('\n', startIndex);

                if (endIndex == -1)
                {
                    endIndex = output.Length;
                }

                if (_doIndent)
                {
                    InternalWrite(spaces);
                }

                InternalWrite(output.Substring(startIndex, endIndex - startIndex));

                if (endIndex != output.Length)
                {
                    InternalWrite("\n");
                    _doIndent = true;
                }
                else
                {
                    _doIndent = false;
                }

                startIndex = endIndex + 1;
            }
        }

        #endregion Private Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields
        int _numFollowingLines;
        int _indentAmount;
        bool _doIndent = true;
        Stack _indentStack;
        #endregion Private Fields
    }
}




