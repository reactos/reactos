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
    /// <summary>
    /// The IMPORTS construct is used to specify items used in the current MIB module which are defined in another MIB module or ASN.1 module.
    /// </summary>
    public sealed class Imports : List<ImportsFrom>, IElement
    {
        private IModule _module;

        /// <summary>
        /// Creates an <see cref="Imports"/> instance.
        /// </summary>
        /// <param name="lexer"></param>
        public Imports(IModule module, ISymbolEnumerator symbols)
        {
            _module = module;

            Symbol current;
            while ((current = symbols.NextSymbol()) != Symbol.Semicolon)
            {
                if (current == Symbol.EOL)
                {
                    continue;
                }

                ImportsFrom imports = new ImportsFrom(current, symbols);

                this.Add(imports);
            }
        }

        public IList<string> Dependents
        {
            get
            {
                List<string> result = new List<string>();

                foreach (ImportsFrom import in this)
                {
                    result.Add(import.Module);
                }

                return result;
            }
        }

        public ImportsFrom GetImportFromType(string type)
        {
            foreach (ImportsFrom import in this)
            {
                if (import.Types.Contains(type))
                {
                    return import;
                }
            }

            return null;
        }
       
        #region IElement Member

        public IModule Module
        {
            get { return _module; }
        }

        #endregion

    }
}