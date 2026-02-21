/*
 * Copyright 2018 Fabian Maurer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Compile with
    csc /target:library /out:dll.dll comtest.cs
*/

using System.Runtime.InteropServices;

namespace DLL
{
    [Guid("1dbc4491-080d-45c5-a15d-1e3c4610bdd9"), ComVisible(true), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ITest
    {
        void Func(ref int i);
    }

    [ComVisible(true), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ITest2
    {
        void Func2(ref int i);
    }

    [Guid("2e106e50-e7a4-4489-8538-83643f100fdc"), ComVisible(true), ClassInterface(ClassInterfaceType.None)]
    public class Test : ITest, ITest2
    {
        public void Func(ref int i)
        {
            i = 42;
        }
        public void Func2(ref int i)
        {
            i = 43;
        }
    }
}
