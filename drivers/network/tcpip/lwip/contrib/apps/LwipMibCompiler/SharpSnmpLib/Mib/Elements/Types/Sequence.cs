/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/21
 * Time: 19:43
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */


namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    /// <summary>
    /// The SEQUENCE type represents a set of specified types. This is roughly analogous to a <code>struct</code> in C.
    /// </summary>
    public sealed class Sequence : BaseType
    {
        /// <summary>
        /// Creates a <see cref="Sequence" /> instance.
        /// </summary>
        /// <param name="module">The module.</param>
        /// <param name="name">The name.</param>
        /// <param name="symbols">The enumerator.</param>
        public Sequence(IModule module, string name, ISymbolEnumerator symbols)
            : base(module, name)
        {
            // parse between ( )
            Symbol temp = symbols.NextNonEOLSymbol();
            int bracketSection = 0;
            temp.Expect(Symbol.OpenBracket);
            bracketSection++;
            while (bracketSection > 0)
            {
                temp = symbols.NextNonEOLSymbol();
                if (temp == Symbol.OpenBracket)
                {
                    bracketSection++;
                }
                else if (temp == Symbol.CloseBracket)
                {
                    bracketSection--;
                }
            }
        }
    }
}