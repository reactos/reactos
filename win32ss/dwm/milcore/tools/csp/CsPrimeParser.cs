// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Parses "C# prime". Converts a C#' file to an in-memory C#
//              program.
//
//

namespace MS.Internal.Csp
{
    using System;
    using System.IO;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Collections;

    internal sealed class CsPrimeParser
    {
        private struct Position
        {
            public int LineNumber;
            public int Column;
        }

        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        private CsPrimeParser(string filename, bool sourceDebuggingSupport)
        {
            _sourceDebuggingSupport = sourceDebuggingSupport;
            _stringBuilder = new StringBuilder();

            _reInlineStart = new Regex(
                @"^[\s\n]*inline[\s\n]*$",
                RegexOptions.IgnoreCase);

            _reWhitespace = new Regex(@"^[\s\n]*$");

            _reInlineEnd = new Regex(
                @"^[\s\n]*/inline[\s\n]*$",
                RegexOptions.IgnoreCase);

            _reConditionalStart = new Regex(
                @"^[\s\n]*conditional[\s\n]*\((?<condition>.*)\)[\s\n]*$",
                RegexOptions.IgnoreCase);

            _reConditionalEnd = new Regex(
                @"^[\s\n]*/conditional[\s\n]*$",
                RegexOptions.IgnoreCase);
            
            ProcessFile(filename);
        }
        #endregion Constructors

        
        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------

        #region Internal Methods
        internal static string[] Parse(string [] filenames, bool sourceDebuggingSupport)
        {
            string[] results = new string[filenames.Length];
            int i=0;

            foreach (string filename in filenames) 
            {
                CsPrimeParser parser = new CsPrimeParser(Path.GetFullPath(filename), sourceDebuggingSupport);
                results[i++] = parser.Result;
            }

            return results;
        }

        internal static string[] Parse(string[] filenames)
        {
            return Parse(filenames, true);
        }
        #endregion Internal Methods


        //------------------------------------------------------
        //
        //  Internal Properties
        //
        //------------------------------------------------------

        #region Internal Properties
        string Result
        {
            get
            {
                return _stringBuilder.ToString().Replace("\n", "\r\n");
            }
        }
        #endregion Internal Properties

        //------------------------------------------------------
        //
        //  Internal Events
        //
        //------------------------------------------------------


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods
        private void ProcessFile(string filename)
        {
            using (StreamReader sr = File.OpenText(filename))
            {
                BeginFile(filename);
                String inputLine;

                while ((inputLine = sr.ReadLine()) != null)
                {
                    _current.LineNumber++;

                    ProcessLine(inputLine);
                }

                EndFile();
            }
        }

        private string ReportingString(Position pos, string type, string description)
        {
            return String.Format("{0}({1}) : {2}: {3}", _filename, pos.LineNumber, type, description);
        }

        private void ReportError(string description)
        {
            ReportError(description, _current);
        }

        private void ReportError(string description, Position pos)
        {
            // 

            throw new Exception(ReportingString(pos, "error", description));
        }

        private void ReportWarning(string description)
        {
            Console.WriteLine(ReportingString(_current, "warning", description));
        }
        
        private void BeginFile(string filename)
        {
            _filename = filename;
            _current.LineNumber = 0;
            _stringBuilder.Length = 0;
            _inMiddleOfOutputLine = false;
            _inInline = false;
            _sbTag = null;
        }

        private void EndFile()
        {
            if (InTag)
            {
                ReportError("Missing ]]");
            }

            if (_inInline) 
            {
                ReportError("Missing [[/inline]]");
            }
        }

        private void ProcessLine(string line)
        {
            line = line + "\n";
            _current.Column = 0;

            if (_sourceDebuggingSupport) 
            {
                if (_inMiddleOfOutputLine)
                {
                    _stringBuilder.Append("\n");
                    _inMiddleOfOutputLine = false;
                }

                // Append a #line directive for each source line. This enables source debugging.
                _stringBuilder.Append("#line " + _current.LineNumber + " \"" + _filename + "\"\n");
            }

            do
            {
                // Detect "[[" or "]]", whichever we're expecting
                int newIndex;
                newIndex = line.IndexOf(InTag?"]]":"[[", _current.Column);

                // 

                if (newIndex == -1)
                {
                    ProcessText(line.Substring(_current.Column));
                    break;
                }

                ProcessText(line.Substring(_current.Column, newIndex-_current.Column));

                if (!InTag)
                {
                    _tagStart.LineNumber = _current.LineNumber;
                    _tagStart.Column = newIndex;
                    _sbTag = new StringBuilder();
                }
                else
                {
                    ProcessTag(_sbTag.ToString());
                    _tagStart.LineNumber = -1;
                    _sbTag = null;
                }

                _current.Column = newIndex + 2;
            } while (_current.Column < line.Length);
        }

        private void ProcessTag(string tag)
        {
            if (!_inInline)
            {
                Match m = _reInlineStart.Match(tag);

                if (!m.Success)
                {
                    ReportError(String.Format("[[inline]] expected, '[[{0}]]' found", tag), _tagStart);
                }

                // Begin a new [[inline]]

                _inInline = true;
                _justStartedInline = true;
                _onFirstTerm = true;
                _inline = _tagStart;  // Record the position of the [[inline]]
            }
            else if (_reInlineEnd.Match(tag).Success) 
            {
                if (_inConditional)
                {
                    ReportError(String.Format("[[/conditional]] expected, '[[{0}]]' found", tag), _tagStart);
                }

                _inInline = false;
            }
            else if (!_inConditional && _reConditionalStart.Match(tag).Success)
            {
                Match m = _reConditionalStart.Match(tag);

                _condition = m.Groups["condition"].Value;
                _inConditional = true;
                _isStartOfConditional = true;                
            }
            else if (_inConditional && _reConditionalEnd.Match(tag).Success)
            {
                _inConditional = false;
                if (!_isStartOfConditional)
                {
                    _stringBuilder.Append(" : String.Empty)");
                }
            }
            else // tag encountered inline - embedded source.
            {
                ProcessEmbeddedCSharp(tag);
            }
        }
        
        /// <summary>
        /// Remove the given number of leading spaces from the given text.
        /// </summary>
        private string TrimForIndent(string text, int level)
        {
            int iIndex = 0;
            int iStop = Math.Min(text.Length, level);
            
            if (   (iStop > 0)
                && (text[iStop-1] == '\n'))
            {
                iStop--;
            }

            // Find the first non-space
            for (; iIndex < iStop; iIndex++)
            {
                if (text[iIndex] != ' ')
                {
                    ReportWarning(String.Format(
                        "Unexpected text in indent region, '{0}'",
                        text.Substring(iIndex, iStop - iIndex)));
                    break;
                }
            }

            return text.Substring(iIndex);
        }

        private void ProcessText(string text)
        {
            if (InTag)
            {
                _sbTag.Append(text);
            }
            else if (_inInline)
            {
                ProcessInlinedText(text);
            }
            else
            {
                // Outside an [[inline]] ... [[/inline]] - just emit the text verbatim
                _stringBuilder.Append(text);

                if (text.Length > 0) {
                    _inMiddleOfOutputLine = text[text.Length-1] != '\n';
                }
            }
        }

        private void ProcessInlinedText(string text)
        {
            // Remove indenting whitespace

            if (_current.Column == 0)
            {
                text = TrimForIndent(text, _inline.Column + 4);
            }


            // Ignore the whitespace at the start of an [[inline]]
            
            if (_justStartedInline) 
            {
                _justStartedInline = false;

                if (_reWhitespace.Match(text).Success) 
                {
                    return;
                }
            }


            // Skip empty text

            if (text == "")
            {
                return;
            }


            // Handle \n

            bool fNewline = false;

            if (text[text.Length-1] == '\n')
            {
                text = text.Remove(text.Length-1, 1);
                fNewline = true;
            }

            if (_isStartOfConditional)
            {
                _isStartOfConditional = false;
                _stringBuilder.Append(TermSeparator());
                _stringBuilder.Append("(" + _condition + " ? ");
                _onFirstTerm = true;
            }

            // Convert the text to code for a C# string

            if (text != "")
            {
                // Indent

                IndentIfNecessary();

                // 'Escape' the quote characters (replace " with "")
    
                text = text.Replace("\"", "\"\"");
    
                // Emit the string as an @-literal (i.e. verbatim literal)
    
                _stringBuilder.Append(TermSeparator());

                _stringBuilder.Append("@\"" + text + "\"");
                _inMiddleOfOutputLine = true;
            }
            
            if (fNewline) {
                IndentIfNecessary();
                _stringBuilder.Append(TermSeparator());
                _stringBuilder.Append("\"\\n\"\n");
                _inMiddleOfOutputLine = false;
            }
        }

        private void IndentIfNecessary()
        {
            if (!_inMiddleOfOutputLine)
            {
                _stringBuilder.Append(' ', _inline.Column + 4);
            }
        }

        private void ProcessEmbeddedCSharp(string text)
        {
            IndentIfNecessary();
            
            _stringBuilder.Append(TermSeparator());
            _stringBuilder.Append("MS.Internal.Csp.CsPrimeRuntime.ConvertAndIndent((");
            _stringBuilder.Append(text);
            _stringBuilder.Append("),");
            _stringBuilder.Append(_tagStart.Column - _inline.Column - 4);
            _stringBuilder.Append(")");
            _inMiddleOfOutputLine = true;
        }
        
        private string TermSeparator()
        {
            string result;

            if (_inMiddleOfOutputLine) 
            {
                result = _onFirstTerm ? "" : " + ";
            }
            else
            {
                result = _onFirstTerm ? "  " : "+ ";
            }
            _onFirstTerm = false;
            return result;
        }
        #endregion Private Methods


        //------------------------------------------------------
        //
        //  Private Properties
        //
        //------------------------------------------------------

        #region Private Properties
        private bool InTag
        {
            get
            {
                return _sbTag != null;
            }
        }
        #endregion Private Properties
        

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields
        private Regex _reInlineStart;
        private Regex _reInlineEnd;
        private Regex _reWhitespace;

        private Regex _reConditionalStart;
        private Regex _reConditionalEnd;

        private bool _sourceDebuggingSupport;

        // Current file
        private string _filename;

        // Current position in file
        private Position _current;

        // Position of the starting [[ of the current tag.
        private Position _tagStart;

        // Position of the current [[inline]] tag
        private Position _inline;
        
        private bool _inInline;          // Inside [[inline]] ... [[/inline]]
        private bool _justStartedInline;
        private bool _onFirstTerm;         // We're on the first term of the string
                                        // expression (a series of
                                        // concatenations)
        private bool _inConditional;
        private string _condition;      // The condition which, if true, will result in a 
                                        // a sub-portion of an inline block being emitted.
        private bool _isStartOfConditional;


        // The text inside the current [[...]] tag
        private StringBuilder _sbTag;


        // The output C# code
        private StringBuilder _stringBuilder;
        bool _inMiddleOfOutputLine;
        #endregion Private Fields
    }
}



