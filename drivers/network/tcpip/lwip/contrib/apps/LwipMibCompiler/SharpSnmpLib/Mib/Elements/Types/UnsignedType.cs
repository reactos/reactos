using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    /**
     * As this type is used for Counter32 and TimeTicks as well as Unsigned32
     * and Gauge32 it incorrectly allows range restrictions of Counter32 and
     * TimeTicks.  This is ok as currently we do not care about detecting
     * incorrect MIBs and this doesn't block the decoding of correct MIBs.
     */
    public class UnsignedType : BaseType
    {
        public enum Types
        {
            Unsigned32,
            Gauge32,
            Counter32,
            TimeTicks,
            Counter64,
        }

        private Types _type;
        private ValueRanges _ranges;

        public UnsignedType(IModule module, string name, Symbol type, ISymbolEnumerator symbols)
            : base(module, name)
        {
            Types? t = GetExactType(type);
            type.Assert(t.HasValue, "Unknown symbol for unsigned type!");
            _type = t.Value;

            Symbol current = symbols.NextNonEOLSymbol();
            if (current == Symbol.OpenParentheses)
            {
                current.Assert((_type != Types.Counter64), "Ranges are not supported for Counter64 type!"); // our internal struct can only hold int64 values

                symbols.PutBack(current);
                _ranges = Lexer.DecodeRanges(symbols);
                current.Assert(!_ranges.IsSizeDeclaration, "SIZE keyword is not allowed for ranges of unsigned types!");
            }
            else
            {
                symbols.PutBack(current);
            }
        }

        public Types Type
        {
            get { return _type; }
        }

        public ValueRanges Ranges
        {
            get { return _ranges; }
        }

        internal static Types? GetExactType(Symbol symbol)
        {
            if (symbol == Symbol.Unsigned32)
            {
                // [APPLICATION 2] IMPLICIT INTEGER (0..4294967295) // from SNMPv2-SMI
                return Types.Unsigned32;
            }
            else if (symbol == Symbol.Gauge32)
            {
                // [APPLICATION 2] IMPLICIT INTEGER (0..4294967295) // from SNMPv2-SMI
                return Types.Gauge32;
            }
            else if (symbol == Symbol.Counter32)
            {
                // [APPLICATION 1] IMPLICIT INTEGER (0..4294967295) // from SNMPv2-SMI
                return Types.Counter32;
            }
            else if (symbol == Symbol.TimeTicks)
            {
                // [APPLICATION 3] IMPLICIT INTEGER (0..4294967295) // from SNMPv2-SMI + RFC1155-SMI
                return Types.TimeTicks;
            }
            else if (symbol == Symbol.Gauge)
            {
                // [APPLICATION 2] IMPLICIT INTEGER (0..4294967295) // from RFC1155-SMI
                return Types.Gauge32;
            }
            else if (symbol == Symbol.Counter)
            {
                // [APPLICATION 1] IMPLICIT INTEGER (0..4294967295) // from RFC1155-SMI
                return Types.Counter32;
            }
            else if (symbol == Symbol.Counter64)
            {
               // [APPLICATION 6] IMPLICIT INTEGER (0..18446744073709551615) // from SNMPv2-SMI
               return Types.Counter64;
            }

            return null;
        }

        internal static bool IsUnsignedType(Symbol symbol)
        {
            return GetExactType(symbol).HasValue;
        }
    }
}
