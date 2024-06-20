// Module interface.
// Copyright (C) 2008-2010 Malcolm Crowe, Lex Li, and other contributors.
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 5/1/2009
 * Time: 10:40 AM
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */
using System.Collections.Generic;
using Lextm.SharpSnmpLib.Mib.Elements.Entities;
using Lextm.SharpSnmpLib.Mib.Elements.Types;
using Lextm.SharpSnmpLib.Mib.Elements;

namespace Lextm.SharpSnmpLib.Mib
{
    /// <summary>
    /// MIB Module interface.
    /// </summary>
    public interface IModule
    {
        /// <summary>
        /// Module name.
        /// </summary>
        string Name
        {
            get;
        }

        Exports Exports
        {
            get;
        }

        Imports Imports
        {
            get;
        }

        /// <summary>
        /// Entities + Types + all other elements implementing IDeclaration
        /// </summary>
        IList<IDeclaration> Declarations
        {
            get;
        }

        /// <summary>
        /// Entities.
        /// </summary>
        IList<IEntity> Entities
        {
            get;
        }

        /// <summary>
        /// Known types.
        /// </summary>
        IList<ITypeAssignment> Types
        {
            get;
        }
       
    }
}
