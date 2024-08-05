/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/6/7
 * Time: 17:34
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib.Elements
{
    /// <summary>
    /// Description of Exports.
    /// </summary>
    public sealed class Exports: IElement
    {
        private IModule _module;
        private readonly IList<string> _types = new List<string>();

        public Exports(IModule module, ISymbolEnumerator s)
        {
            _module = module;

            Symbol previous = null;
            Symbol current;
            do
            {
                current = s.NextSymbol();

                if (current == Symbol.EOL)
                {
                    continue;
                }
                else if (((current == Symbol.Comma) || (current == Symbol.Semicolon)) && (previous != null))
                {
                    previous.AssertIsValidIdentifier();
                    _types.Add(previous.ToString());
                }

                previous = current;
            }
            while (current != Symbol.Semicolon);
        }

        #region IElement Member

        public IModule Module
        {
            get { return _module; }
        }

        #endregion
    }
}