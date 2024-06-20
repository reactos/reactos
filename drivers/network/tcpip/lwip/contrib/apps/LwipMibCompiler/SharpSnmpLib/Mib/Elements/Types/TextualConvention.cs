using System;

namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    public sealed class TextualConvention : ITypeAssignment, ITypeReferrer
    {
        private IModule _module;
        private string _name;
        private DisplayHint _displayHint;
        private Status _status;
        private string _description;
        private string _reference;
        private ITypeAssignment _syntax;

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "module")]
        public TextualConvention(IModule module, string name, ISymbolEnumerator symbols)
        {
            _module = module;
            _name = name;

            _displayHint = ParseDisplayHint(symbols);
            _status      = ParseStatus(symbols);
            _description = ParseDescription(symbols);
            _reference   = ParseReference(symbols);
            _syntax      = ParseSyntax(module, symbols);
        }

        private static DisplayHint ParseDisplayHint(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();

            if (current == Symbol.DisplayHint)
            {
                return new DisplayHint(symbols.NextNonEOLSymbol().ToString().Trim(new char[] { '"' }));
            }

            symbols.PutBack(current);
            return null;
        }

        private static Status ParseStatus(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();
            current.Expect(Symbol.Status);

            try
            {
                return (Status)Enum.Parse(typeof(Status), symbols.NextNonEOLSymbol().ToString());
            }
            catch (ArgumentException)
            {
                current.Assert(false, "Invalid/Unknown status");
            }

            return Status.current;
        }

        private static string ParseDescription(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();
            current.Expect(Symbol.Description);

            return symbols.NextNonEOLSymbol().ToString().Trim(new char[] { '"' });
        }

        private static string ParseReference(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();

            if (current == Symbol.Reference)
            {
                string reference = symbols.NextNonEOLSymbol().ToString();
                if ((reference.Length >= 2) && reference.StartsWith("\"") && reference.EndsWith("\""))
                {
                    return reference.Substring(1, reference.Length-2);
                }

                return reference;
            }

            symbols.PutBack(current);
            return null;
        }

        private static ITypeAssignment ParseSyntax(IModule module, ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();
            current.Expect(Symbol.Syntax);

            /* 
             * RFC2579 definition:
             *       Syntax ::=   -- Must be one of the following:
             *                    -- a base type (or its refinement), or
             *                    -- a BITS pseudo-type
             *               type
             *             | "BITS" "{" NamedBits "}"
             *
             * From section 3.5:
             *      The data structure must be one of the alternatives defined
             *      in the ObjectSyntax CHOICE or the BITS construct.  Note
             *      that this means that the SYNTAX clause of a Textual
             *      Convention can not refer to a previously defined Textual
             *      Convention.
             *      
             *      The SYNTAX clause of a TEXTUAL CONVENTION macro may be
             *      sub-typed in the same way as the SYNTAX clause of an
             *      OBJECT-TYPE macro.
             * 
             * Therefore the possible values are (grouped by underlying type):
             *      INTEGER, Integer32
             *      OCTET STRING, Opaque
             *      OBJECT IDENTIFIER
             *      IpAddress
             *      Counter64
             *      Unsigned32, Counter32, Gauge32, TimeTicks
             *      BITS
             * With appropriate sub-typing.
             */

            return Lexer.ParseBasicTypeDef(module, String.Empty, symbols, isMacroSyntax: true);
        }

        public IModule Module
        {
            get { return _module; }
        }

        public string Name
        {
            get { return _name; }
        }

        public string DisplayHint
        {
            get { return _displayHint == null ? null : _displayHint.ToString(); }
        }

        public Status Status
        {
            get { return _status; }
        }

        public string Description
        {
            get { return _description; }
        }

        public string Reference
        {
            get { return _reference; }
        }

        public ITypeAssignment Syntax
        {
            get { return _syntax; }
        }

        //internal object Decode(Variable v)
        //{
        //    if (_syntax is IntegerType)
        //    {
        //        Integer32 i = v.Data as Integer32;
        //        if (i == null || (_syntax as IntegerType).IsEnumeration)
        //        {
        //            return null;
        //        }
        //        else if (_displayHint != null)
        //        {
        //            return _displayHint.Decode(i.ToInt32());
        //        }
        //        else
        //        {
        //            return i.ToInt32();
        //        }
        //    }
        //    else if (_syntax is UnsignedType)
        //    {
        //        Integer32 i = v.Data as Integer32;
        //        if (i == null)
        //        {
        //            return null;
        //        }
        //        else if (_displayHint != null)
        //        {
        //            return _displayHint.Decode(i.ToInt32());
        //        }
        //        else
        //        {
        //            return i.ToInt32();
        //        }
        //    }
        //    else if (_syntax is OctetStringType)
        //    {
        //        OctetString o = v.Data as OctetString;
        //        if (o == null)
        //        {
        //            return null;
        //        }
        //        else
        //        {
        //            // TODO: Follow the format specifier for octet strings.
        //            return null;
        //        }
        //    }
        //    else
        //    {
        //        return null;
        //    }
        //}

        #region ITypeReferrer Member

        public ITypeAssignment ReferredType
        {
            get { return _syntax; }
            set { _syntax = value; }
        }

        public ITypeAssignment BaseType
        {
            get
            {
                ITypeReferrer   tr     = this;
                ITypeAssignment result = this;

                while ((tr != null) && (tr.ReferredType != null))
                {
                    result = tr.ReferredType;
                    tr = tr.ReferredType as ITypeReferrer;
                }

                return result;
            }
        }

        #endregion
    }
}