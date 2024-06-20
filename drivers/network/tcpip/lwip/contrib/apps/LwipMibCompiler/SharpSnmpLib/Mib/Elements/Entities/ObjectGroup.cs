/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/21
 * Time: 19:27
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

namespace Lextm.SharpSnmpLib.Mib.Elements.Entities
{
    /// <summary>
    /// Description of ObjectGroupNode.
    /// </summary>
    public sealed class ObjectGroup : EntityBase
    {
        public ObjectGroup(IModule module, SymbolList preAssignSymbols, ISymbolEnumerator symbols)
            : base(module, preAssignSymbols, symbols)
        {
        }
   }
}