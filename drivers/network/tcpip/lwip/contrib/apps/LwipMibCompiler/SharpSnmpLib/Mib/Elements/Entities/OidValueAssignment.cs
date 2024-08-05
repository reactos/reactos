/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/17
 * Time: 20:49
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

using System;

namespace Lextm.SharpSnmpLib.Mib.Elements.Entities
{
    /// <summary>
    /// Object identifier node.
    /// </summary>
    public sealed class OidValueAssignment : EntityBase
    {
        /// <summary>
        /// Creates a <see cref="OidValueAssignment"/>.
        /// </summary>
        /// <param name="module">Module</param>
        /// <param name="name">Name</param>
        /// <param name="lexer">Lexer</param>
        public OidValueAssignment(IModule module, SymbolList preAssignSymbols, ISymbolEnumerator symbols)
            : base(module, preAssignSymbols, symbols)
        {
        }
    }
}