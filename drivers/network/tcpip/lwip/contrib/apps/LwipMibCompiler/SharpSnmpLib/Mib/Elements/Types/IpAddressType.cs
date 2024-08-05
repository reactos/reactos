
namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    public class IpAddressType : OctetStringType
    {
        public IpAddressType(IModule module, string name, ISymbolEnumerator symbols)
            : base(module, name, symbols)
        {
            if (this.Size.Count != 0)
            {
                throw new MibException("Size definition not allowed for IpAddress type!");
            }

            // IpAddress type is defined as:
            // IpAddress ::=
            //    [APPLICATION 0]
            //        IMPLICIT OCTET STRING (SIZE (4))
            this.Size.Add(new ValueRange(4, null));
        }
    }
}
