/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/18
 * Time: 13:24
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */
using System;
using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    /* Please be aware of the following possible constructs:
     * 
     *    isnsRegEntityIndex          OBJECT-TYPE
     *    SYNTAX                      IsnsEntityIndexIdOrZero
     *                                ( 1 .. 4294967295 )
     *    MAX-ACCESS              not-accessible
     * 
     * 
     */

    /// <summary/>
    /// </summary>
    public sealed class TypeAssignment : ITypeAssignment
    {
        private IModule _module;
        private string _name;
        private string _type;
        private ValueRanges _ranges;
        private ValueMap _map;

        /// <summary>
        /// Creates an <see cref="TypeAssignment" />.
        /// </summary>
        /// <param name="module">The module.</param>
        /// <param name="name">The name.</param>
        /// <param name="type">The type.</param>
        /// <param name="symbols">The symbols.</param>
        /// <param name="isMacroSyntax">if set to <c>true</c> indicates that the syntax clause of a macro is parsed (e.g. OBJECT-TYPE, TEXTUAL-CONVENTION).</param>
        public TypeAssignment(IModule module, string name, Symbol type, ISymbolEnumerator symbols, bool isMacroSyntax)
        {
            _module = module;
            _name   = name;

            SymbolList typeSymbols = new SymbolList();
            typeSymbols.Add(type);

            Symbol current = symbols.NextSymbol();
            while (current != Symbol.EOL)
            {
                if (current == Symbol.OpenParentheses)
                {
                    // parse range of unknown type
                    symbols.PutBack(current);
                    _ranges = Lexer.DecodeRanges(symbols);
                    break;
                }
                else if (current == Symbol.OpenBracket)
                {
                    symbols.PutBack(current);
                    _map = Lexer.DecodeEnumerations(symbols);
                    break;
                }

                typeSymbols.Add(current);
                current = symbols.NextSymbol();
            }

            _type = typeSymbols.Join(" ");

            if ((_ranges == null) && (_map == null))
            {
                current = symbols.NextNonEOLSymbol();
                if (current == Symbol.OpenParentheses)
                {
                    // parse range of unknown type
                    symbols.PutBack(current);
                    _ranges = Lexer.DecodeRanges(symbols);
                }
                else if (current == Symbol.OpenBracket)
                {
                    symbols.PutBack(current);
                    _map = Lexer.DecodeEnumerations(symbols);
                }
                else if (current != null)
                {
                    symbols.PutBack(current);
                }
            }

            if (isMacroSyntax)
            {
                // inside a macro the syntax is limited to one line, except there are brackets used for ranges/enums
                return;
            }

            // outside macro Syntax clause we  wait for two consecutive linebreaks with a following valid identifier as end condition         
            Symbol previous = current;
            Symbol veryPrevious = null;

            while ((current = symbols.NextSymbol()) != null)
            {
                if ((veryPrevious == Symbol.EOL) && (previous == Symbol.EOL) && current.IsValidIdentifier())
                {
                    symbols.PutBack(current);
                    return;
                }

                veryPrevious = previous;
                previous = current;
            }
            
            previous.Assert(false, "end of file reached");
        }

        public string Type
        {
            get { return _type; }
        }

        public ValueRanges Ranges
        {
            get { return _ranges; }
        }

        public IDictionary<long, string> Map
        {
            get { return _map; }
        }

        #region ITypeAssignment Member

        public IModule Module
        {
            get { return _module; }
        }

        public string Name
        {
            get { return _name; }
        }

        #endregion
    }
}