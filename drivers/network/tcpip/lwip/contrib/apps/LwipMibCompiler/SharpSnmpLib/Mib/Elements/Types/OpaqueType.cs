
namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    public class OpaqueType : OctetStringType
    {
        public OpaqueType(IModule module, string name, ISymbolEnumerator symbols)
            : base(module, name, symbols)
        {
        }
    }
}
