using System.Collections.Generic;
using System.Text;

namespace Lextm.SharpSnmpLib.Mib
{
    public class ObjectIdentifier: List<KeyValuePair<string, uint>>
    {
        public void Add(string name, uint oid)
        {
            this.Add(new KeyValuePair<string, uint>(name, oid));
        }

        public void Prepend(string name, uint oid)
        {
            this.Insert(0, new KeyValuePair<string, uint>(name, oid));
        }

        public void Insert(int index, string name, uint oid)
        {
            this.Insert(index, new KeyValuePair<string, uint>(name, oid));
        }

        public string GetOidString()
        {
            StringBuilder result = new StringBuilder();

            foreach (KeyValuePair<string, uint> level in this)
            {
                result.Append(level.Value);
                result.Append('.');
            }

            if (result.Length > 0)
            {
                result.Length--;
            }

            return result.ToString();
        }

        public uint[] GetOidValues()
        {
            List<uint> result = new List<uint>();
            
            foreach (KeyValuePair<string, uint> level in this)
            {
                result.Add(level.Value);
            }

            return result.ToArray();
        }

    }
}
