using System;
using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib
{
    public class ValueRanges: List<ValueRange>
    {
        public bool IsSizeDeclaration { get; internal set; }

        public ValueRanges(bool isSizeDecl = false)
        {
            IsSizeDeclaration = isSizeDecl;
        }

        public bool Contains(Int64 value)
        {
            foreach (ValueRange range in this)
            {
                if (range.Contains(value))
                {
                    return true;
                }
            }

            return false;
        }
    }

    public class ValueRange
    {
        private readonly Int64 _start;
        private readonly Int64? _end;

        public ValueRange(Int64 first, Int64? second)
        {
            _start = first;
            _end   = second;
        }

        public Int64 Start
        {
            get { return _start; }
        }

        public Int64? End
        {
            get { return _end; }
        }

        public bool IntersectsWith(ValueRange other)
        {
            if (this._end == null)
            {
                return other.Contains(this._start);
            }
            else if (other._end == null)
            {
                return this.Contains(other._start);
            }

            return (this._start <= other.End) && (this._end >= other._start);
        }

        public bool Contains(Int64 value)
        {
            if (_end == null)
            {
                return value == _start;
            }
            else
            {
                return (_start <= value) && (value <= _end);
            }
        }
    }
}
