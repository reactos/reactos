using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Lextm.SharpSnmpLib.Mib.Elements
{
    public interface IDeclaration: IElement
    {
        /// <summary>
        /// Name.
        /// </summary>
        string Name
        {
            get;
        }
    }
}
