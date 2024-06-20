using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Lextm.SharpSnmpLib.Mib
{
    public class SymbolList : List<Symbol>
    {
        public class SymbolEnumerator : ISymbolEnumerator
        {
            private SymbolList _list  = null;
            private int        _index = -1;

            internal SymbolEnumerator(SymbolList list)
            {
                if (list == null)
                {
                    throw new ArgumentNullException("lexer");
                }

                _list = list;
            }

            #region ISymbolEnumerator Member

            public bool PutBack(Symbol item)
            {
                if ((_index < 0) || (_index >= _list.Count) || (item != _list[_index]))
                {
                    throw new ArgumentException(@"wrong last symbol", "last");
                    //return false;
                }

                _index--;
                return true;
            }

            #endregion

            #region IEnumerator<Symbol> Member

            public Symbol Current
            {
                get
                {
                    if ((_index >= 0) && (_index <= _list.Count))
                    {
                        return _list[_index];
                    }

                    return null;
                }
            }

            #endregion

            #region IDisposable Member

            public void Dispose()
            {
            }

            #endregion

            #region IEnumerator Member

            object System.Collections.IEnumerator.Current
            {
                get { return this.Current; }
            }

            public bool MoveNext()
            {
                _index++;
                return (_index >= 0) && (_index < _list.Count);
            }

            public void Reset()
            {
                _index = -1;
            }

            #endregion
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SymbolList"/> class.
        /// </summary>
        public SymbolList()
        {
        }

        public ISymbolEnumerator GetSymbolEnumerator()
        {
            return new SymbolEnumerator(this);
        }

        public string Join(string separator)
        {
            if (separator == null)
                separator = "";

            StringBuilder result = new StringBuilder();

            foreach (Symbol s in this)
            {
                result.Append(s);
                result.Append(separator);
            }

            if (result.Length > 0)
            {
                result.Length -= separator.Length;
            }

            return result.ToString();
        }
    }

    public static class SymbolEnumeratorExtension
    {
        public static Symbol NextSymbol(this IEnumerator<Symbol> enumerator)
        {
            if (enumerator.MoveNext())
            {
                return enumerator.Current;
            }

            return null;
        }

        public static Symbol NextNonEOLSymbol(this IEnumerator<Symbol> enumerator)
        {
            while (enumerator.MoveNext())
            {
                if (enumerator.Current != Symbol.EOL)
                {
                    return enumerator.Current;
                }
            }

            return null;
        }
    }
}
