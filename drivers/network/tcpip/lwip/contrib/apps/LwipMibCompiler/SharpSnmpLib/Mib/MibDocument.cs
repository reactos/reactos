/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/17
 * Time: 17:38
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib
{
    /// <summary>
    /// MIB document.
    /// </summary>
    public sealed class MibDocument
    {
        private readonly List<IModule> _modules = new List<IModule>();

        /// <summary>
        /// Initializes a new instance of the <see cref="MibDocument" /> class.
        /// </summary>
        /// <param name="file">The file.</param>
        public MibDocument(string file)
            : this(new Lexer(file))
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MibDocument"/> class.
        /// </summary>
        /// <param name="lexer">The lexer.</param>
        public MibDocument(Lexer lexer)
        {
            ISymbolEnumerator symbols = lexer.GetEnumerator();

            Symbol current;
            while ((current = symbols.NextNonEOLSymbol()) != null)
            {
                symbols.PutBack(current);
                _modules.Add(new MibModule(symbols));                
            }
        }
        
        /// <summary>
        /// <see cref="MibModule"/> containing in this document.
        /// </summary>
        public IList<IModule> Modules
        {
            get
            {
                return _modules;
            }
        }
    }
}