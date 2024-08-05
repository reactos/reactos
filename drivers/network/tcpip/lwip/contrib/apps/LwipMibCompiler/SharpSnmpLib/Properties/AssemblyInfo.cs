// <summary>#SNMP Library. An open source SNMP implementation for .NET.</summary>
// <copyright company="Lex Y. Li" file="AssemblyInfo.cs">Copyright (C) 2008  Lex Y. Li</copyright>
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

#region Using directives

using System;
using System.Reflection;
using System.Resources;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

#endregion

// General Information about an assembly is controlled through the following
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
[assembly: AssemblyTitle("SharpSnmpLib")]
[assembly: AssemblyDescription("#SNMP Library for .NET")]
[assembly: AssemblyConfiguration("Lesser GPL 2.1+")]
[assembly: AssemblyCompany("LeXtudio")]
[assembly: AssemblyProduct("#SNMPLib")]
[assembly: AssemblyCopyright("(C) 2008-2010 Malcolm Crowe, Lex Li, Steve Santacroce, and other contributors.")]
[assembly: AssemblyTrademark("")]
[assembly: AssemblyCulture("")]

// This sets the default COM visibility of types in the assembly to invisible.
// If you need to expose index type to COM, use [ComVisible(true)] on that type.
[assembly: ComVisible(false)]

// The assembly version has following format :
//
// Major.Minor.Build.Revision
//
// You can specify all the values or you can use the default the Revision and
// Build Numbers by using the '*' as shown below:
[assembly: AssemblyVersion("7.0.011207.31")]
#if (!CF)
[assembly: AssemblyFileVersion("7.0.011207.31")]
#endif
[assembly: NeutralResourcesLanguage("en-US")]
[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Lextm")]

[assembly: InternalsVisibleTo("SharpSnmpLib.Tests, PublicKey=0024000004800000940000000602000000240000525341310004000001000100f7030532c52524"
+ "993841a0d09420340f3814e1b65473851bdcd18815510b035a2ae9ecee69c4cd2d9e4d6e6d5fbf"
+ "a564e86c4a4cddc9597619a31c060846ebb2e99511a0323ff82b1ebd95d6a4912502945f0e769f"
+ "190a69a439dbfb969ebad72a6f7e2e047907da4a7b9c08c6e98d5f1be8b8cafaf3eb978914059a"
+ "245d4bc1")]
