/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/31
 * Time: 12:20
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

namespace Lextm.SharpSnmpLib.Mib.Elements
{
    public sealed class TrapType : IDeclaration
    {
        private readonly IModule _module;
        private readonly string _name;
        private readonly int _value;

        public TrapType(IModule module, SymbolList preAssignSymbols, ISymbolEnumerator symbols)
        {
            _module = module;
            _name = preAssignSymbols[0].ToString();

            Symbol valueSymbol = symbols.NextNonEOLSymbol();

            bool succeeded = int.TryParse(valueSymbol.ToString(), out _value);
            valueSymbol.Assert(succeeded, "not a decimal");
        }

        public int Value
        {
            get { return _value; }
        }

        #region IDeclaration Member

        public IModule Module
        {
            get { return _module; }
        }

        public string Name
        {
            get { return _name; }
        }

        #endregion
    }
}