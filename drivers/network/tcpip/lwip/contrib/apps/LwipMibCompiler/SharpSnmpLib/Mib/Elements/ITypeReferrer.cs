using Lextm.SharpSnmpLib.Mib.Elements.Types;

namespace Lextm.SharpSnmpLib.Mib.Elements
{
    public interface ITypeReferrer
    {
        ITypeAssignment ReferredType { get; set; }
        ITypeAssignment BaseType { get; }
    }
}
