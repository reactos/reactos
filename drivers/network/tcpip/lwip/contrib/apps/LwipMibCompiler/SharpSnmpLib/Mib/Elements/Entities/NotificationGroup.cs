/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/21
 * Time: 19:34
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

namespace Lextm.SharpSnmpLib.Mib.Elements.Entities
{
    /// <summary>
    /// Description of NotificationGroupNode.
    /// </summary>
    public sealed class NotificationGroup : EntityBase
    {
        public NotificationGroup(IModule module, SymbolList preAssignSymbols, ISymbolEnumerator symbols)
            : base(module, preAssignSymbols, symbols)
        {
        }
   }
}