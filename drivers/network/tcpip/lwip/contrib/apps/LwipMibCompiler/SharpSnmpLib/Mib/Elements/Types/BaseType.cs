
namespace Lextm.SharpSnmpLib.Mib.Elements.Types
{
    public abstract class BaseType : ITypeAssignment
    {
        private IModule _module;
        private string _name;

        protected BaseType(IModule module, string name)
        {
            _module = module;
            _name   = name;
        }

        public virtual IModule Module
        {
            // differentiate between:
            //    FddiTimeNano ::= INTEGER (0..2147483647) 
            //    which is an IntegerType which appears under Types in a MibModule and therefore has a name and module
            // and
            //    SYNTAX INTEGER (0..2147483647) 
            //    which is also an IntegerType but not defined as a separate type and therefore has NO name and NO module
            get
            {
                if (!string.IsNullOrEmpty(_name))
                {
                    return _module;
                }
                else
                {
                    return null;
                }                
            }
            protected set { _module = value; }
        }

        public virtual string Name
        {
            get
            {
                if (!string.IsNullOrEmpty(_name))
                {
                    return _name;
                }
                else
                {
                    return "{ Implicit Base Type }";
                }
            }
            protected set { _name = value; }
        }

    }
}
