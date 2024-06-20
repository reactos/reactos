/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/7/25
 * Time: 20:41
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

using System;
using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    /// <summary>
    /// The INTEGER type represents a list of alternatives, or a range of numbers..
    /// Includes Integer32 as it's indistinguishable from INTEGER.
    /// </summary>
    /**
     * As this type is used for Integer32 as well as INTEGER it incorrectly
     * allows enumeration sub-typing of Integer32.  This is ok as currently we
     * do not care about detecting incorrect MIBs and this doesn't block the
     * decoding of correct MIBs.
     */
    public sealed class IntegerType : BaseType
    {
        public enum Types
        {
            Integer,
            Integer32
        }

        private Types _type;
        private bool _isEnumeration;
        private ValueMap _map;
        private ValueRanges _ranges;

        /// <summary>
        /// Creates an <see cref="IntegerType"/> instance.
        /// </summary>
        /// <param name="module"></param>
        /// <param name="name"></param>
        /// <param name="enumerator"></param>
        public IntegerType(IModule module, string name, Symbol type, ISymbolEnumerator symbols)
            : base (module, name)
        {
            Types? t = GetExactType(type);
            type.Assert(t.HasValue, "Unknown symbol for unsigned type!");
            _type = t.Value;

            _isEnumeration = false;

            Symbol current = symbols.NextNonEOLSymbol();
            if (current == Symbol.OpenBracket)
            {
                _isEnumeration = true;
                symbols.PutBack(current);
                _map = Lexer.DecodeEnumerations(symbols);
            }
            else if (current == Symbol.OpenParentheses)
            {
                symbols.PutBack(current);
                _ranges = Lexer.DecodeRanges(symbols);
                current.Assert(!_ranges.IsSizeDeclaration, "SIZE keyword is not allowed for ranges of integer types!");
            }
            else
            {
                symbols.PutBack(current);
            }
        }

        public Types Type
        {
            get { return _type; }
        }

        public ValueRanges Ranges
        {
            get { return _ranges; }
        }

        public bool IsEnumeration
        {
            get
            {
                return _isEnumeration;
            }
        }

        public ValueMap Enumeration
        {
            get { return _isEnumeration ? _map : null; }
        }

        internal static Types? GetExactType(Symbol symbol)
        {
            if (symbol == Symbol.Integer)
            {
                // represents the ASN.1 builtin INTEGER type:
                //  may be represent any arbitrary (signed/unsigned) integer (in theory may have any size)
                return Types.Integer;
            }
            else if (symbol == Symbol.Integer32)
            {
                // Integer32 ::= INTEGER (-2147483648..2147483647) // from SNMPv2-SMI
                return Types.Integer32;
            }

            return null;
        }

        internal static bool IsIntegerType(Symbol symbol)
        {
            return GetExactType(symbol).HasValue;
        }
    }
}