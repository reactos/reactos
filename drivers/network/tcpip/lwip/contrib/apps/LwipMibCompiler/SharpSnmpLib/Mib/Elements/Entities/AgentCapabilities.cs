/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/31
 * Time: 13:18
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

namespace Lextm.SharpSnmpLib.Mib.Elements.Entities
{
    /// <summary>
    /// The AGENT-CAPABILITIES construct is used to specify implementation characteristics of an SNMP agent sub-system with respect to object types and events.
    /// </summary>
    public sealed class AgentCapabilities : EntityBase
    {       
        /// <summary>
        /// Creates an <see cref="AgentCapabilities"/> instance.
        /// </summary>
        /// <param name="module"></param>
        /// <param name="header"></param>
        /// <param name="lexer"></param>
        public AgentCapabilities(IModule module, SymbolList preAssignSymbols, ISymbolEnumerator symbols)
            : base(module, preAssignSymbols, symbols)
        {
        }
       
    }
}