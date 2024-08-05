/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/31
 * Time: 12:07
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib.Elements
{
    public sealed class ImportsFrom
    {
        private readonly string _module;
        private readonly List<string> _types = new List<string>();

        public ImportsFrom(Symbol last, ISymbolEnumerator symbols)
        {
            Symbol previous = last;
            Symbol current;
            while ((current = symbols.NextSymbol()) != Symbol.From)
            {
                if (current == Symbol.EOL) 
                {
                    continue;
                }
                
                if (current == Symbol.Comma)
                {
                    previous.AssertIsValidIdentifier();
                    _types.Add(previous.ToString());
                }
                
                previous = current;
            }

            previous.AssertIsValidIdentifier();
            _types.Add(previous.ToString());

            _module = symbols.NextSymbol().ToString().ToUpperInvariant(); // module names are uppercase
        }
        
        public string Module
        {
            get { return _module; }
        }

        public IList<string> Types
        {
            get { return _types; }
        }

        public override string ToString()
        {
            return string.Join(", ", _types.ToArray()) + " FROM " + _module;
        }
    }
}