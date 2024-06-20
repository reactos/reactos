namespace Lextm.SharpSnmpLib.Mib.Elements.Entities
{
    public sealed class ModuleIdentity : EntityBase
    {
        public ModuleIdentity(IModule module, SymbolList preAssignSymbols, ISymbolEnumerator symbols)
            : base(module, preAssignSymbols, symbols)
        {
        }
    }
}