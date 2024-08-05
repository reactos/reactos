using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    /// <summary>
    /// The SEQUENCE OF type represents a list of data sets..
    /// </summary>
    public sealed class SequenceOf : BaseType
    {
        private string _type;

        public SequenceOf(IModule module, string name, ISymbolEnumerator sym)
            : base(module, name)
        {
            _type = sym.NextNonEOLSymbol().ToString();
        }

        public string Type
        {
            get { return _type; }
        }
    }
}