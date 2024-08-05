using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    public class OctetStringType : BaseType
    {
        private ValueRanges _size;

        public OctetStringType(IModule module, string name, ISymbolEnumerator symbols)
            : base(module, name)
        {
            Symbol current = symbols.NextNonEOLSymbol();
            if (current == Symbol.OpenParentheses)
            {
                symbols.PutBack(current);
                _size = Lexer.DecodeRanges(symbols);
                current.Assert(_size.IsSizeDeclaration, "SIZE keyword is required for ranges of octet string!");
            }
            else
            {
                symbols.PutBack(current);
                _size   = new ValueRanges(isSizeDecl: true);
            }
        }

        public ValueRanges Size
        {
            get { return _size; }
        }
    }
}
