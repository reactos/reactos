// NAnt - A .NET build tool
// Copyright (C) 2001 Gerry Shaw
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Ian MacLean (ian_maclean@another.com)

namespace SysGen.BuildEngine.Attributes {

    using System;
    using System.Reflection;

    /// <summary>Indicates that class should be treated as a NAnt element.</summary>
    /// <remarks>
    /// Attach this attribute to a subclass of Element to have NAnt be able
    /// to recognize it.  The name should be short but must not confict
    /// with any other element already in use.
    /// </remarks>
    [AttributeUsage(AttributeTargets.Class, Inherited=false, AllowMultiple=false)]
    public class ElementNameAttribute : Attribute {

        string _name;

        public ElementNameAttribute(string name) {
            _name = name;
        }

        public string Name {
            get { return _name; }
            set { _name = value; }
        }
    }
}
