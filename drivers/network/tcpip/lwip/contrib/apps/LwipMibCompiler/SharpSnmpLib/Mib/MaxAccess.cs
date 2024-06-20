using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Lextm.SharpSnmpLib.Mib
{
    public enum MaxAccess
    {
        notAccessible,
        accessibleForNotify,
        readOnly,
        readWrite,
        readCreate
    }

}
