using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib
{
    public interface ISymbolEnumerator: IEnumerator<Symbol>
    {
        bool PutBack(Symbol item);
    }
}
