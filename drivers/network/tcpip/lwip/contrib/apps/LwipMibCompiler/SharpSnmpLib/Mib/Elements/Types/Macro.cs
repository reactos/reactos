
namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    public sealed class Macro : ITypeAssignment
    {
        private IModule _module;
        private string _name;

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1804:RemoveUnusedLocals", MessageId = "temp")]
        public Macro(IModule module, SymbolList preAssignSymbols, ISymbolEnumerator symbols)
        {
            _module = module;
            _name = preAssignSymbols[0].ToString();
            
            while (symbols.NextNonEOLSymbol() != Symbol.Begin)
            {                
            }

            while (symbols.NextNonEOLSymbol() != Symbol.End)
            {
            }
        }

        public IModule Module
        {
            get { return _module; }
        }

        public string Name
        {
            get { return _name; }
        }
    }
}