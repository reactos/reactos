using System;

namespace Lextm.SharpSnmpLib.Mib.Elements.Entities
{
    public abstract class EntityBase: IEntity
    {
        private readonly IModule _module;
        private string _parent;
        private readonly uint   _value;
        private readonly string _name;

        public EntityBase(IModule module, SymbolList preAssignSymbols, ISymbolEnumerator symbols)
        {
            _module = module;
            _name   = preAssignSymbols[0].ToString();

            Lexer.ParseOidValue(symbols, out _parent, out _value);
        }

        public IModule Module
        {
            get { return _module; }
        }

        public string Parent
        {
            get { return _parent; }
            set { _parent = value; }
        }

        public uint Value
        {
            get { return _value; }
        }

        public string Name
        {
            get { return _name; }
        }
        
        public virtual string Description
        {
            get { return string.Empty; }
        }
    }
}
