/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/17
 * Time: 17:38
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

using System;
using System.Collections.Generic;
using Lextm.SharpSnmpLib.Mib.Elements;
using Lextm.SharpSnmpLib.Mib.Elements.Entities;
using Lextm.SharpSnmpLib.Mib.Elements.Types;

namespace Lextm.SharpSnmpLib.Mib
{
    /// <summary>
    /// MIB module class.
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Mib")]
    public sealed class MibModule : IModule
    {
        private readonly string _name;
        private readonly Imports _imports;
        private readonly Exports _exports;
        private readonly List<IElement> _tokens = new List<IElement>();
    
        /// <summary>
        /// Creates a <see cref="MibModule"/> with a specific <see cref="Lexer"/>.
        /// </summary>
        /// <param name="name">Module name</param>
        /// <param name="symbols">Lexer</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "lexer")]
        public MibModule(ISymbolEnumerator symbols)
        {
            if (symbols == null)
            {
                throw new ArgumentNullException("lexer");
            }

            Symbol temp = symbols.NextNonEOLSymbol();
            temp.AssertIsValidIdentifier();
            _name = temp.ToString().ToUpperInvariant(); // all module names are uppercase
            
            temp = symbols.NextNonEOLSymbol();
            temp.Expect(Symbol.Definitions);
            
            temp = symbols.NextNonEOLSymbol();
            temp.Expect(Symbol.Assign);
            
            temp = symbols.NextSymbol();
            temp.Expect(Symbol.Begin);
            
            temp = symbols.NextNonEOLSymbol();
            if (temp == Symbol.Imports)
            {
                _imports = ParseDependents(symbols);
            }
            else if (temp == Symbol.Exports)
            {
                _exports = ParseExports(symbols);
            }
            else
            {
                symbols.PutBack(temp);
            }

            ParseEntities(symbols);
        }

        #region Accessors

        /// <summary>
        /// Module name.
        /// </summary>
        public string Name
        {
            get { return _name; }
        }
        
        public Exports Exports
        {
            get { return _exports; }
        }

        public Imports Imports
        {
            get { return _imports; }
        }

        public List<IElement> Tokens
        {
            get { return this._tokens; }
        }

        /// <summary>
        /// Entities + Types + all other elements implementing IDeclaration
        /// </summary>
        public IList<IDeclaration> Declarations
        {
            get
            {
                IList<IDeclaration> result = new List<IDeclaration>();
                foreach (IElement e in _tokens)
                {
                    IDeclaration decl = e as IDeclaration;
                    if (decl != null)
                    {
                        result.Add(decl);
                    }
                }

                return result;
            }
        }

        /// <summary>
        /// OID nodes.
        /// </summary>
        public IList<IEntity> Entities
        {
            get
            {
                IList<IEntity> result = new List<IEntity>();
                foreach (IElement e in _tokens)
                {
                    IEntity entity = e as IEntity;
                    if (entity != null)
                    {
                        result.Add(entity);
                    }
                }

                return result;
            }
        }

        public IList<ITypeAssignment> Types
        {
            get
            {
                IList<ITypeAssignment> result = new List<ITypeAssignment>();
                foreach (IElement e in _tokens)
                {
                    ITypeAssignment type = e as ITypeAssignment;
                    if (type != null)
                    {
                        result.Add(type);
                    }
                }

                return result;
            }
        }

        #endregion

        #region Parsing of Symbols

        private Exports ParseExports(ISymbolEnumerator symbols)
        {
            return new Exports(this, symbols);
        }

        private Imports ParseDependents(ISymbolEnumerator symbols)
        {
            return new Imports(this, symbols);
        }
        
        private void ParseEntities(ISymbolEnumerator symbols)
        {
            Symbol     temp   = symbols.NextNonEOLSymbol();
            SymbolList buffer = new SymbolList();

            while (temp != Symbol.End)
            {
                if (temp == Symbol.Assign)
                {
                    ParseEntity(buffer, symbols);
                    buffer.Clear();
                    // skip linebreaks behind an entity
                    temp = symbols.NextNonEOLSymbol();
                }
                else
                {
                    buffer.Add(temp);
                    temp = symbols.NextSymbol();
                }
            }
        }

        private void ParseEntity(SymbolList preAssignSymbols, ISymbolEnumerator symbols)
        {
            if ((preAssignSymbols == null) || (preAssignSymbols.Count == 0))
            {
                Symbol s = symbols.NextSymbol();
                if (s != null)
                {
                    s.Assert(false, "Invalid Entity declaration");
                }
                else
                {
                    throw new MibException("Invalid Entity declaration");
                }
            }

            // check for a valid identifier
            preAssignSymbols[0].AssertIsValidIdentifier();
            
            if (preAssignSymbols.Count == 1)
            {
                // its a typedef
                _tokens.Add(Lexer.ParseBasicTypeDef(this, preAssignSymbols[0].ToString(), symbols, isMacroSyntax: false));
                return;
            }

            ISymbolEnumerator preAssignSymbolsEnumerator = preAssignSymbols.GetSymbolEnumerator();
            preAssignSymbolsEnumerator.NextNonEOLSymbol(); // returns identifier
            Symbol type = preAssignSymbolsEnumerator.NextNonEOLSymbol();

            // parse declarations
            if (type == Symbol.Object)
            {
                Symbol next = preAssignSymbolsEnumerator.NextNonEOLSymbol();

                if (next == Symbol.Identifier)
                {
                    _tokens.Add(new OidValueAssignment(this, preAssignSymbols, symbols));
                    return;
                }
                else if (next != null)
                {
                    preAssignSymbolsEnumerator.PutBack(next);
                }
            }
            if (type == Symbol.ModuleIdentity)
            {
                _tokens.Add(new ModuleIdentity(this, preAssignSymbols, symbols));
                return;
            }
            if (type == Symbol.ObjectType)
            {
                _tokens.Add(new ObjectType(this, preAssignSymbols, symbols));
                return;
            }
            if (type == Symbol.ObjectGroup)
            {
                _tokens.Add(new ObjectGroup(this, preAssignSymbols, symbols));
                return;
            }
            if (type == Symbol.NotificationGroup)
            {
                _tokens.Add(new NotificationGroup(this, preAssignSymbols, symbols));
                return;
            }
            if (type == Symbol.ModuleCompliance)
            {
                _tokens.Add(new ModuleCompliance(this, preAssignSymbols, symbols));
                return;
            }
            if (type == Symbol.NotificationType)
            {
                _tokens.Add(new NotificationType(this, preAssignSymbols, symbols));
                return;
            }
            if (type == Symbol.ObjectIdentity)
            {
                _tokens.Add(new ObjectIdentity(this, preAssignSymbols, symbols));
                return;
            }
            if (type == Symbol.Macro)
            {
                _tokens.Add(new Macro(this, preAssignSymbols, symbols));
                return;
            }
            if (type == Symbol.TrapType)
            {
                _tokens.Add(new TrapType(this, preAssignSymbols, symbols));
                return;
            }
            if (type == Symbol.AgentCapabilities)
            {
                _tokens.Add(new AgentCapabilities(this, preAssignSymbols, symbols));
                return;
            }

            preAssignSymbols[1].Assert(false, "Unknown/Invalid declaration");
        }

        #endregion

    }
}