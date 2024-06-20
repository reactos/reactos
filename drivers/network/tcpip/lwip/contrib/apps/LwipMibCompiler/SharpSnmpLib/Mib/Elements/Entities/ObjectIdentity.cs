
namespace Lextm.SharpSnmpLib.Mib.Elements.Entities
{
    /// <summary>
    /// Object identifier node.
    /// </summary>
    public sealed class ObjectIdentity : EntityBase
    {
       
        /// <summary>
        /// Creates a <see cref="ObjectIdentity"/>.
        /// </summary>
        /// <param name="module">Module name</param>
        /// <param name="header">Header</param>
        /// <param name="lexer">Lexer</param>
        public ObjectIdentity(IModule module, SymbolList preAssignSymbols, ISymbolEnumerator symbols)
            : base(module, preAssignSymbols, symbols)
        {
        }
    }
}