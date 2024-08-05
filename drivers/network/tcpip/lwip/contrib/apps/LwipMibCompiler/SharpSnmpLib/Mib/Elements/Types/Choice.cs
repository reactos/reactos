/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/31
 * Time: 11:39
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    /// <summary>
    /// The CHOICE type represents a list of alternatives..
    /// </summary>
    public sealed class Choice : BaseType
    {
        /// <summary>
        /// Creates a <see cref="Choice"/> instance.
        /// </summary>
        /// <param name="module"></param>
        /// <param name="name"></param>
        /// <param name="lexer"></param>
        public Choice(IModule module, string name, ISymbolEnumerator symbols)
            : base(module, name)
        {
            while (symbols.NextNonEOLSymbol() != Symbol.OpenBracket)
            {
            }

            while (symbols.NextNonEOLSymbol() != Symbol.CloseBracket)
            {
            }
        }
    }
}