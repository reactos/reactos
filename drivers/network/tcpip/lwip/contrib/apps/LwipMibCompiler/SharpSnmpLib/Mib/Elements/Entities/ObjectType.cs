using System;
using System.Collections.Generic;
using Lextm.SharpSnmpLib.Mib.Elements.Types;

namespace Lextm.SharpSnmpLib.Mib.Elements.Entities
{
    public sealed class ObjectType : EntityBase, ITypeReferrer
    {
        private ITypeAssignment _syntax;
        private string _units;
        private MaxAccess _access;
        private Status _status;
        private string _description;
        private string _reference;
        private IList<string> _indices;
        private string _augments;
        private string _defVal;

        public ObjectType(IModule module, SymbolList preAssignSymbols, ISymbolEnumerator symbols)
            : base(module, preAssignSymbols, symbols)
        {
            ParseProperties(preAssignSymbols);
        }

        private void ParseProperties(SymbolList header)
        {
            ISymbolEnumerator headerSymbols = header.GetSymbolEnumerator();
            Symbol temp = headerSymbols.NextNonEOLSymbol();

            // Skip name
            temp = headerSymbols.NextNonEOLSymbol();
            temp.Expect(Symbol.ObjectType);

            _syntax         = ParseSyntax       (Module, headerSymbols);
            _units          = ParseUnits        (headerSymbols);
            _access         = ParseAccess       (headerSymbols);
            _status         = ParseStatus       (headerSymbols);
            _description    = ParseDescription  (headerSymbols);
            _reference      = ParseReference    (headerSymbols);
            _indices        = ParseIndices      (headerSymbols);
            _augments        = ParseAugments     (headerSymbols);
            _defVal         = ParseDefVal       (headerSymbols);
        }

        private static string ParseAugments(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();

            if (current == Symbol.Augments)
            {
                string augment = null;

                current = symbols.NextNonEOLSymbol();
                current.Expect(Symbol.OpenBracket);

                current = symbols.NextNonEOLSymbol();
                augment = current.ToString();

                current = symbols.NextNonEOLSymbol();
                current.Expect(Symbol.CloseBracket);

                return augment;
            }
            else if (current != null)
            {
                symbols.PutBack(current);
            }
            
            return null;
        }

        private static string ParseDefVal(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();

            if (current == Symbol.DefVal)
            {
                current = symbols.NextNonEOLSymbol();
                current.Expect(Symbol.OpenBracket);

                string defVal = null;
                current = symbols.NextNonEOLSymbol();

                if (current == Symbol.OpenBracket)
                {
                    int depth = 1;
                    // TODO: decode this.
                    while (depth > 0)
                    {
                        current = symbols.NextNonEOLSymbol();
                        if (current == Symbol.OpenBracket)
                        {
                            depth++;
                        }
                        else if (current == Symbol.CloseBracket)
                        {
                            depth--;
                        }
                    }
                }
                else
                {
                    defVal = current.ToString();
                    current = symbols.NextNonEOLSymbol();
                    current.Expect(Symbol.CloseBracket);
                }

                return defVal;
            }
            else if (current != null)
            {
                symbols.PutBack(current);
            }

            return null;
        }

        private static IList<string> ParseIndices(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();

            if (current == Symbol.Index)
            {
                current = symbols.NextNonEOLSymbol();
                current.Expect(Symbol.OpenBracket);

                List<string> indices = new List<string>();

                while (current != Symbol.CloseBracket)
                {
                   current = symbols.NextNonEOLSymbol();
                   
                   bool lastIndex = false;
                    if (current == Symbol.Implied)
                    {
                        current = symbols.NextNonEOLSymbol();
                        lastIndex = true; // 'IMPLIED' may only be used for last index 
                    }

                    current.Assert((current != Symbol.Comma) && (current != Symbol.CloseBracket), "Expected index name but found symbol!");
                    indices.Add(current.ToString());

                    current = symbols.NextNonEOLSymbol();
                    if (lastIndex)
                    {
                       current.Expect(Symbol.CloseBracket);
                    }
                    else
                    {
                       current.Expect(Symbol.Comma, Symbol.CloseBracket);
                    }
                }

                return indices;
            }
            else if (current != null)
            {
                symbols.PutBack(current);
            }

            return null;
        }

        private static string ParseReference(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();

            if (current == Symbol.Reference)
            {
                return symbols.NextNonEOLSymbol().ToString();
            }
            else if (current != null)
            {
                symbols.PutBack(current);
            }

            return null;
        }

        private static string ParseDescription(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();

            if (current == Symbol.Description)
            {
                return symbols.NextNonEOLSymbol().ToString().Trim(new char[] { '"' });
            }
            else if (current != null)
            {
                symbols.PutBack(current);
            }

            return null;
        }

        private static Status ParseStatus(ISymbolEnumerator symbols)
        {
            Status status = Status.obsolete;

            Symbol current = symbols.NextNonEOLSymbol();
            current.Expect(Symbol.Status);
            
            current = symbols.NextNonEOLSymbol();
            try
            {
                status = (Status)Enum.Parse(typeof(Status), current.ToString());
            }
            catch (ArgumentException)
            {
                current.Assert(false, "Invalid/Unknown status");
            }
            
            return status;
        }

        private static MaxAccess ParseAccess(ISymbolEnumerator symbols)
        {
            MaxAccess access = MaxAccess.notAccessible;

            Symbol current = symbols.NextNonEOLSymbol();
            current.Expect(Symbol.MaxAccess, Symbol.Access);

            current = symbols.NextNonEOLSymbol();
            switch (current.ToString())
            {
                case "not-accessible":
                    access = MaxAccess.notAccessible;
                    break;
                case "accessible-for-notify":
                    access = MaxAccess.accessibleForNotify;
                    break;
                case "read-only":
                    access = MaxAccess.readOnly;
                    break;
                case "read-write":
                    access = MaxAccess.readWrite;
                    break;
                case "read-create":
                    access = MaxAccess.readCreate;
                    break;
                case "write-only":
                    access = MaxAccess.readWrite;
                    break;
                default:
                    current.Assert(false, "Invalid/Unknown access");
                    break;
            }

            return access;
        }

        private static string ParseUnits(ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();

            if (current == Symbol.Units)
            {
                return symbols.NextNonEOLSymbol().ToString();
            }
            else if (current != null)
            {
                symbols.PutBack(current);
            }

            return null;
        }

        private static ITypeAssignment ParseSyntax(IModule module, ISymbolEnumerator symbols)
        {
            Symbol current = symbols.NextNonEOLSymbol();
            current.Expect(Symbol.Syntax);

            return Lexer.ParseBasicTypeDef(module, String.Empty, symbols, isMacroSyntax: true);
        }

        private static bool IsProperty(Symbol sym)
        {
            string s = sym.ToString();
            return s == "SYNTAX" || s == "MAX-ACCESS" || s == "STATUS" || s == "DESCRIPTION";
        }

        public ITypeAssignment Syntax
        {
            get { return _syntax; }
            internal set { _syntax = value; }
        }

        public override string Description
        {
            get { return _description; }
        }

        public MaxAccess Access
        {
            get { return _access; }
        }

        public IList<string> Indices
        {
            get { return _indices; }
        }

        public string Augments
        {
            get { return _augments; }
        }

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
                ITypeAssignment result = null;

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