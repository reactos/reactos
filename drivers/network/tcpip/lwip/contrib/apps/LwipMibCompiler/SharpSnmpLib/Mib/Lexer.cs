/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/17
 * Time: 16:50
 *
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Lextm.SharpSnmpLib.Mib.Elements.Types;

namespace Lextm.SharpSnmpLib.Mib
{
    /// <summary>
    /// Lexer class that parses MIB files into symbol list.
    /// </summary>
    public sealed class Lexer
    {
        private readonly SymbolList _symbols = new SymbolList();

        public Lexer(string file)
            : this(file, new StreamReader(file))
        {
        }

        public Lexer(string file, TextReader stream)
        {
            this.Parse(file, stream);
        }


        public ISymbolEnumerator GetEnumerator()
        {
            return _symbols.GetSymbolEnumerator();
        }


        #region Parsing of MIB File

        private class ParseParams
        {
            public string File;
            public StringBuilder Temp = new StringBuilder();
            public bool StringSection = false;
            public bool AssignSection = false;
            public bool AssignAhead = false;
            public bool DotSection = false;

        }

        /// <summary>
        /// Parses MIB file to symbol list.
        /// </summary>
        /// <param name="file">File</param>
        /// <param name="stream">File stream</param>
        private void Parse(string file, TextReader stream)
        {
            if (stream == null)
            {
                throw new ArgumentNullException("stream");
            }

            ParseParams pp = new ParseParams();
            pp.File = file;
            
            string line;
            int row = 0;
            while ((line = stream.ReadLine()) != null)
            {
                if (!pp.StringSection && line.TrimStart().StartsWith("--", StringComparison.Ordinal))
                {
                    row++;
                    continue; // commented line
                }

                ParseLine(pp, line, row);
                row++;
            }
        }

        private void ParseLine(ParseParams pp, string line, int row)
        {
            line = line + "\n";
            int count = line.Length;
            for (int i = 0; i < count; i++)
            {
                char current = line[i];
                bool moveNext = Parse(pp, current, row, i);
                if (moveNext)
                {
                    break;
                }
            }
        }

        private bool Parse(ParseParams pp, char current, int row, int column)
        {
            switch (current)
            {
                case '\n':
                case '{':
                case '}':
                case '(':
                case ')':
                case '[':
                case ']':
                case ';':
                case ',':
                case '|':
                    if (!pp.StringSection)
                    {
                        bool moveNext = ParseLastSymbol(pp, row, column);
                        if (moveNext)
                        {
                            _symbols.Add(CreateSpecialSymbol(pp.File, '\n', row, column));
                            return true;
                        }

                        _symbols.Add(CreateSpecialSymbol(pp.File, current, row, column));
                        return false;
                    }

                    break;
                case '"':
                    pp.StringSection = !pp.StringSection;
                    break;
                case '\r':
                    return false;
                default:
                    if ((int)current == 0x1A)
                    {
                        // IMPORTANT: ignore invisible characters such as SUB.
                        return false;
                    }
                    
                    if (Char.IsWhiteSpace(current) && !pp.AssignSection && !pp.StringSection)
                    {                     
                        bool moveNext = ParseLastSymbol(pp, row, column);
                        if (moveNext)
                        {
                            _symbols.Add(CreateSpecialSymbol(pp.File, '\n', row, column));
                            return true;
                        }

                        return false;
                    }
                    
                    if (pp.AssignAhead)
                    {
                        pp.AssignAhead = false;
                        ParseLastSymbol(pp, row, column);
                        break;
                    }

                    if (pp.DotSection && current != '.')
                    {
                        ParseLastSymbol(pp, row, column);
                        pp.DotSection = false;
                    }

                    if (current == '.' && !pp.StringSection)
                    {
                        if (!pp.DotSection)
                        {
                            ParseLastSymbol(pp, row, column);
                            pp.DotSection = true;
                        }
                    }
                    
                    if (current == ':' && !pp.StringSection)
                    {    
                        if (!pp.AssignSection)
                        {
                            ParseLastSymbol(pp, row, column);
                        }
                        
                        pp.AssignSection = true;
                    }
                    
                    if (current == '=' && !pp.StringSection)
                    {
                        pp.AssignSection = false; 
                        pp.AssignAhead = true;
                    }

                    break;
            }

            pp.Temp.Append(current);
            return false;
        }

        private bool ParseLastSymbol(ParseParams pp, int row, int column)
        {
            if (pp.Temp.Length > 0)
            {
                Symbol s = new Symbol(pp.File, pp.Temp.ToString(), row, column);

                pp.Temp.Length = 0;

                if (s.ToString().StartsWith(Symbol.Comment.ToString()))
                {
                    // ignore the rest symbols on this line because they are in comment.
                    return true;
                }

                _symbols.Add(s);
            }

            return false;
        }

        private static Symbol CreateSpecialSymbol(string file, char value, int row, int column)
        {
            string str;
            switch (value)
            {
                case '\n':
                    str = Environment.NewLine;
                    break;
                case '{':
                    str = "{";
                    break;
                case '}':
                    str = "}";
                    break;
                case '(':
                    str = "(";
                    break;
                case ')':
                    str = ")";
                    break;
                case '[':
                    str = "[";
                    break;
                case ']':
                    str = "]";
                    break;
                case ';':
                    str = ";";
                    break;
                case ',':
                    str = ",";
                    break;
                case '|':
                    str = "|";
                    break;
                default:
                    throw new ArgumentException("value is not a special character");
            }

            return new Symbol(file, str, row, column);
        }

        #endregion

        #region Static Parse Helper

        public static ITypeAssignment ParseBasicTypeDef(IModule module, string name, ISymbolEnumerator symbols, bool isMacroSyntax = false)
        {
            Symbol current = symbols.NextNonEOLSymbol();

            if (current == Symbol.Bits)
            {
                return new BitsType(module, name, symbols);
            }
            if (IntegerType.IsIntegerType(current))
            {
                return new IntegerType(module, name, current, symbols);
            }
            if (UnsignedType.IsUnsignedType(current))
            {
                return new UnsignedType(module, name, current, symbols);
            }
            if (current == Symbol.Opaque)
            {
                return new OpaqueType(module, name, symbols);
            }
            if (current == Symbol.IpAddress)
            {
                return new IpAddressType(module, name, symbols);
            }
            if (current == Symbol.TextualConvention)
            {
                return new TextualConvention(module, name, symbols);
            }
            if (current == Symbol.Octet)
            {
                Symbol next = symbols.NextNonEOLSymbol();

                if (next == Symbol.String)
                {
                    return new OctetStringType(module, name, symbols);
                }

                symbols.PutBack(next);
            }
            if (current == Symbol.Object)
            {
                Symbol next = symbols.NextNonEOLSymbol();

                if (next == Symbol.Identifier)
                {
                    return new ObjectIdentifierType(module, name, symbols);
                }

                symbols.PutBack(next);
            }
            if (current == Symbol.Sequence)
            {
                Symbol next = symbols.NextNonEOLSymbol();

                if (next == Symbol.Of)
                {
                    return new SequenceOf(module, name, symbols);
                }
                else
                {
                    symbols.PutBack(next);
                    return new Sequence(module, name, symbols);
                }
            }
            if (current == Symbol.Choice)
            {
                return new Choice(module, name, symbols);
            }


            return new TypeAssignment(module, name, current, symbols, isMacroSyntax);
        }

        public static void ParseOidValue(ISymbolEnumerator symbols, out string parent, out uint value)
        {
            parent = null;
            value = 0;
            
            Symbol current  = symbols.NextNonEOLSymbol();
            current.Expect(Symbol.OpenBracket);

            Symbol previous = null;
            StringBuilder longParent = new StringBuilder();

            current = symbols.NextNonEOLSymbol();
            longParent.Append(current);

            while ((current = symbols.NextNonEOLSymbol()) != null)
            {
                bool succeeded;

                if (current == Symbol.OpenParentheses)
                {
                    longParent.Append(current);
                    
                    current = symbols.NextNonEOLSymbol();
                    succeeded = UInt32.TryParse(current.ToString(), out value);
                    current.Assert(succeeded, "not a decimal");
                    longParent.Append(current);
                    current = symbols.NextNonEOLSymbol();
                    current.Expect(Symbol.CloseParentheses);
                    longParent.Append(current);
                    continue;
                }

                if (current == Symbol.CloseBracket)
                {
                    parent = longParent.ToString();
                    return;
                }

                succeeded = UInt32.TryParse(current.ToString(), out value);
                if (succeeded)
                {
                    // numerical way
                    while ((current = symbols.NextNonEOLSymbol()) != Symbol.CloseBracket)
                    {
                        longParent.Append(".").Append(value);
                        succeeded = UInt32.TryParse(current.ToString(), out value);
                        current.Assert(succeeded, "not a decimal");
                    }

                    current.Expect(Symbol.CloseBracket);
                    parent = longParent.ToString();
                    return;
                }

                longParent.Append(".");
                longParent.Append(current);
                current = symbols.NextNonEOLSymbol();
                current.Expect(Symbol.OpenParentheses);
                longParent.Append(current);
                current = symbols.NextNonEOLSymbol();
                succeeded = UInt32.TryParse(current.ToString(), out value);
                current.Assert(succeeded, "not a decimal");
                longParent.Append(current);
                current = symbols.NextNonEOLSymbol();
                current.Expect(Symbol.CloseParentheses);
                longParent.Append(current);
                previous = current;
            }

            throw MibException.Create("end of file reached", previous);
        }


        public static ValueRanges DecodeRanges(ISymbolEnumerator symbols)
        {
            ValueRanges result = new ValueRanges();

            Symbol startSymbol = symbols.NextNonEOLSymbol();
            Symbol current = startSymbol;
            current.Expect(Symbol.OpenParentheses);

            while (current != Symbol.CloseParentheses)
            {
                Symbol value1Symbol = symbols.NextNonEOLSymbol();

                if ((value1Symbol == Symbol.Size) && !result.IsSizeDeclaration)
                {
                    result.IsSizeDeclaration = true;
                    symbols.NextNonEOLSymbol().Expect(Symbol.OpenParentheses);
                    continue;
                }

                // check for valid number
                Int64? value1 = DecodeNumber(value1Symbol);
                if (!value1.HasValue)
                {
                    value1Symbol.Assert(false, "Invalid range declaration!");                    
                }

                // process next symbol
                ValueRange range;
                current = symbols.NextNonEOLSymbol();

                if (current == Symbol.DoubleDot)
                {
                    // its a continuous range
                    Symbol value2Symbol = symbols.NextNonEOLSymbol();
                    Int64? value2 = DecodeNumber(value2Symbol);
                    value2Symbol.Assert(value2.HasValue && (value2.Value >= value1.Value), "Invalid range declaration!");

                    if (value2.Value == value1.Value)
                    {
                        range = new ValueRange(value1.Value, null);
                    }
                    else
                    {
                        range = new ValueRange(value1.Value, value2.Value);
                    }

                    current = symbols.NextNonEOLSymbol();
                }
                else
                {
                    // its a single number
                    range = new ValueRange(value1.Value, null);
                }

                // validate range
                if (result.IsSizeDeclaration)
                {
                    value1Symbol.Assert(range.Start >= 0, "Invalid range declaration! Size must be greater than 0");
                }

                result.Add(range);
                
                // check next symbol
                current.Expect(Symbol.Pipe, Symbol.CloseParentheses);
            }

            if (result.IsSizeDeclaration)
            {
                current = symbols.NextNonEOLSymbol();
                current.Expect(Symbol.CloseParentheses);
            }

            // validate ranges in between
            for (int i=0; i<result.Count; i++)
            {
                for (int k=i+1; k<result.Count; k++)
                {
                    startSymbol.Assert(!result[i].IntersectsWith(result[k]), "Invalid range declaration! Overlapping of ranges!");
                }
            }

            return result;
        }

        public static Int64? DecodeNumber(Symbol number)
        {
            Int64  result;
            string numString = (number != null) ? number.ToString() : null;

            if (!String.IsNullOrEmpty(numString))
            {
                if (numString.StartsWith("'") && (numString.Length > 3))
                {
                    // search second apostrophe
                    int end = numString.IndexOf('\'', 1);
                    if (end == (numString.Length - 2))
                    {
                        try
                        {
                            switch (numString[numString.Length - 1])
                            {
                                case 'b':
                                case 'B':
                                    result = Convert.ToInt64(numString.Substring(1, numString.Length - 3), 2);
                                    return result;
                                case 'h':
                                case 'H':
                                    result = Convert.ToInt64(numString.Substring(1, numString.Length - 3), 16);
                                    return result;
                            }
                        }
                        catch
                        {
                        }
                    }
                }
                else if (Int64.TryParse(numString, out result))
                {
                    return result;
                }
            }

            return null;
        }

        public static ValueMap DecodeEnumerations(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();
            current.Expect(Symbol.OpenBracket);

            ValueMap map = new ValueMap();
            do
            {
                current = symbols.NextNonEOLSymbol();
                string identifier = current.ToString();

                current = symbols.NextNonEOLSymbol();
                current.Expect(Symbol.OpenParentheses);

                current = symbols.NextNonEOLSymbol();
                Int64 enumValue;
                if (Int64.TryParse(current.ToString(), out enumValue))
                {
                    try
                    {
                        // Have to include the number as it seems repeated identifiers are allowed ??
                        map.Add(enumValue, String.Format("{0}({1})", identifier, enumValue));
                    }
                    catch (ArgumentException ex)
                    {
                        current.Assert(false, ex.Message);
                    }
                }
                else
                {
                    // Need to get "DefinedValue".
                }

                current = symbols.NextNonEOLSymbol();
                current.Expect(Symbol.CloseParentheses);

                current = symbols.NextNonEOLSymbol();
            } while (current == Symbol.Comma);

            current.Expect(Symbol.CloseBracket);

            return map;
        }

        #endregion

    }
}
