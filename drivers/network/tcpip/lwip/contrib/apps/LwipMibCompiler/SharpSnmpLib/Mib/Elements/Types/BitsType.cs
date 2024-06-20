using System;
using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    public class BitsType : BaseType
    {
        private ValueMap _map;

        public BitsType(IModule module, string name, ISymbolEnumerator symbols)
            : base(module, name)
        {
            _map = Lexer.DecodeEnumerations(symbols);
        }

        public ValueMap Map
        {
            get { return _map; }
        }

        public string this[int value]
        {
            get { return _map[value]; }
        }
    }
}
