/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Martin Hentschel <info@cl-soft.de>
 *
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text.RegularExpressions;
using CCodeGeneration;
using Lextm.SharpSnmpLib.Mib;
using Lextm.SharpSnmpLib.Mib.Elements.Entities;
using Lextm.SharpSnmpLib.Mib.Elements.Types;
using LwipSnmpCodeGeneration;

namespace LwipMibCompiler
{
	class Program
	{
		private static readonly Regex _alphaNumericRegex = new Regex("[^a-zA-Z0-9]");

		static void Main(string[] args)
		{
			Console.WriteLine("lwIP MIB Compiler");
			Console.WriteLine("");

			// check args
			if ((args.Length < 2) || String.IsNullOrWhiteSpace(args[0]) || String.IsNullOrWhiteSpace(args[1]))
			{
				PrintUsage();
				return;
			}

			string mibFile = args[0];
			if (!File.Exists(mibFile))
			{
				Console.WriteLine(String.Format("Unable to find file '{0}'!", mibFile));
			}

			string destFile = args[1];
			string destHeaderFile;

			if (Directory.Exists(destFile))
			{
				// only directory passed -> create dest filename from mib filename
				string mibFileName = Path.GetFileNameWithoutExtension(mibFile).ToLowerInvariant();
				destFile = Path.Combine(destFile, mibFileName + ".c");
			}
			
			string destFileExt = Path.GetExtension(destFile);
			if (!String.IsNullOrEmpty(destFileExt))
			{
				destHeaderFile = destFile.Substring(0, destFile.Length - destFileExt.Length);
			}
			else
			{
				destHeaderFile = destFile;
			}
			destHeaderFile += ".h";

			for (int i=2; i<args.Length; i++)
			{
				if (!String.IsNullOrWhiteSpace(args[i]) && Directory.Exists(args[i]))
				{
					MibTypesResolver.RegisterResolver(new FileSystemMibResolver(args[i], true));
				}
			}

			
			// read and resolve MIB
			Console.WriteLine(" Reading MIB file...");
			
			MibDocument md = new MibDocument(mibFile);
			MibTypesResolver.ResolveTypes(md.Modules[0]);
			MibTree mt = new MibTree(md.Modules[0] as MibModule);

			if (mt.Root.Count == 0)
			{
				Console.WriteLine("No root element found inside MIB!");
				return;
			}

			MibCFile generatedFile = new MibCFile();
			MibHeaderFile generatedHeaderFile = new MibHeaderFile();

			foreach (MibTreeNode mibTreeNode in mt.Root)
			{
				// create LWIP object tree from MIB structure
				Console.WriteLine(" Creating lwIP object tree " + mibTreeNode.Entity.Name);

				SnmpMib snmpMib = new SnmpMib();
				snmpMib.Oid = mibTreeNode.Entity.Value;
				snmpMib.BaseOid = MibTypesResolver.ResolveOid(mibTreeNode.Entity).GetOidValues();
				snmpMib.Name = mibTreeNode.Entity.Name;

				ProcessMibTreeNode(mibTreeNode, snmpMib);

				// let the tree transform itself depending on node structure
				snmpMib.Analyze();

				if (snmpMib.ChildNodes.Count != 0)
				{
					// generate code from LWIP object tree
					Console.WriteLine(" Generating code " + snmpMib.Name);
					snmpMib.Generate(generatedFile, generatedHeaderFile);
				}
			}

			string preservedCode = MibCFile.GetPreservedCode(destFile);
			if (!string.IsNullOrEmpty(preservedCode))
			{
				generatedFile.PreservedCode.Add(new PlainText(preservedCode));
			}
			else
			{
				generatedFile.PreservedCode.AddRange(generatedFile.Implementation);
			}
			generatedFile.Implementation.Clear();


			using (StreamWriter fileWriter = new StreamWriter(destHeaderFile))
			{
				CGenerator cGenerator = new CGenerator(fileWriter, destHeaderFile, 3, " ", Environment.NewLine);
				generatedHeaderFile.Save(cGenerator);
			}
			using (StreamWriter fileWriter = new StreamWriter(destFile))
			{
				CGenerator cGenerator = new CGenerator(fileWriter, destFile, 3, " ", Environment.NewLine);
				generatedFile.Save(cGenerator);
			}

			Console.WriteLine(" Done");
		}

		private static void PrintUsage()
		{
			string codeBase = Assembly.GetExecutingAssembly().CodeBase;
			string appName = Path.GetFileName(codeBase);

			Console.WriteLine("Usage:");
			Console.WriteLine(String.Format("  {0} <source MIB file> <dest C file> [<search path 1 for referred MIB's> <search path 2 for referred MIB's> ...]", appName));
			Console.WriteLine("");
			Console.WriteLine("    <source MIB file>");
			Console.WriteLine("      Path and filename of MIB file to convert.");
			Console.WriteLine("");
			Console.WriteLine("    <dest C file>");
			Console.WriteLine("      Destination path and file. If a path is passed only, filename is auto");
			Console.WriteLine("      generated from MIB file name.");
			Console.WriteLine("");
			Console.WriteLine("    <search path X for referred MIB's>");
			Console.WriteLine("      It's important to provide all referred MIB's in order to correctly ");
			Console.WriteLine("      resolve all used types.");
			Console.WriteLine("");
		}


		#region Generation of LWIP Object Tree

		private static void ProcessMibTreeNode(MibTreeNode mibTreeNode, SnmpTreeNode assignedSnmpNode)
		{
			foreach (MibTreeNode mtn in mibTreeNode.ChildNodes)
			{
				// in theory container nodes may also be scalars or tables at the same time (for now only process real containers)
				if (mtn.NodeType == MibTreeNodeType.Container)
				{
					SnmpTreeNode snmpTreeNode = GenerateSnmpTreeNode(mtn, assignedSnmpNode);
					assignedSnmpNode.ChildNodes.Add(snmpTreeNode);

					ProcessMibTreeNode(mtn, snmpTreeNode);
				}
				else if ((mtn.NodeType & MibTreeNodeType.Scalar) != 0)
				{
					SnmpScalarNode snmpScalarNode = GenerateSnmpScalarNode(mtn, assignedSnmpNode);
					if (snmpScalarNode != null)
					{
						assignedSnmpNode.ChildNodes.Add(snmpScalarNode);
					}
				}
				else if ((mtn.NodeType & MibTreeNodeType.Table) != 0)
				{
					SnmpTableNode snmpTableNode = GenerateSnmpTableNode(mtn, assignedSnmpNode);
					if (snmpTableNode != null)
					{
						assignedSnmpNode.ChildNodes.Add(snmpTableNode);
					}
				}
			}
		}

		private static SnmpTreeNode GenerateSnmpTreeNode(MibTreeNode mibTreeNode, SnmpTreeNode parentNode)
		{
			SnmpTreeNode result = new SnmpTreeNode(parentNode);
			result.Name    = _alphaNumericRegex.Replace (mibTreeNode.Entity.Name, "");
			result.Oid     = mibTreeNode.Entity.Value;
			result.FullOid = MibTypesResolver.ResolveOid(mibTreeNode.Entity).GetOidString();

			return result;
		}

		private static SnmpScalarNode GenerateSnmpScalarNode(MibTreeNode mibTreeNode, SnmpTreeNode parentNode, bool ignoreAccessibleFlag = false)
		{
			ObjectType ote = mibTreeNode.Entity as ObjectType;
			if (ote != null)
			{
				return GenerateSnmpScalarNode(ote, parentNode, ignoreAccessibleFlag);
			}

			return null;
		}

		private static SnmpScalarNode GenerateSnmpScalarNode(ObjectType ote, SnmpTreeNode parentNode, bool ignoreAccessibleFlag = false)
		{
			SnmpScalarNode result;

			ITypeAssignment mibType = ote.BaseType;
			IntegerType it = (mibType as IntegerType);
			if (it != null)
			{
				if (ote.ReferredType.Name == Symbol.TruthValue.ToString())
				{
					result = new SnmpScalarNodeTruthValue(parentNode);
				}
				else if ((it.Type == IntegerType.Types.Integer) || (it.Type == IntegerType.Types.Integer32))
				{
					result = new SnmpScalarNodeInt(parentNode);
				}
				else
				{
					Console.WriteLine(String.Format("Unsupported IntegerType '{0}'!", it.Type));
					return null;
				}
				if (it.IsEnumeration)
				{
					result.Restrictions.AddRange(CreateRestrictions(it.Enumeration));
				}
				else
				{
					result.Restrictions.AddRange(CreateRestrictions(it.Ranges));
				}
			}
			else
			{
				UnsignedType ut = (mibType as UnsignedType);
				if (ut != null)
				{
					if ((ut.Type == UnsignedType.Types.Unsigned32) ||
							  (ut.Type == UnsignedType.Types.Gauge32))
					{
						result = new SnmpScalarNodeUint(SnmpDataType.Gauge, parentNode);
					}
					else if (ut.Type == UnsignedType.Types.Counter32)
					{
						result = new SnmpScalarNodeUint(SnmpDataType.Counter, parentNode);
					}
					else if (ut.Type == UnsignedType.Types.TimeTicks)
					{
						result = new SnmpScalarNodeUint(SnmpDataType.TimeTicks, parentNode);
					}
					else if (ut.Type == UnsignedType.Types.Counter64)
					{
						result = new SnmpScalarNodeCounter64(parentNode);
						if ((ut.Ranges != null) && (ut.Ranges.Count > 0))
						{
							Console.WriteLine(String.Format("Generation of ranges is not supported for Counter64 type!"));
							return null;
						}
					}
					else
					{
						Console.WriteLine(String.Format("Unsupported UnsignedType '{0}'!", ut.Type));
						return null;
					}
					result.Restrictions.AddRange(CreateRestrictions(ut.Ranges));
				}
				else if (mibType is IpAddressType)
				{
					result = new SnmpScalarNodeOctetString(SnmpDataType.IpAddress, parentNode);
					result.Restrictions.AddRange(CreateRestrictions((mibType as OctetStringType).Size));
				}
				else if (mibType is OpaqueType)
				{
					result = new SnmpScalarNodeOctetString(SnmpDataType.Opaque, parentNode);
					result.Restrictions.AddRange(CreateRestrictions((mibType as OctetStringType).Size));
				}
				else if (mibType is OctetStringType)
				{
					result = new SnmpScalarNodeOctetString(SnmpDataType.OctetString, parentNode);
					result.Restrictions.AddRange(CreateRestrictions((mibType as OctetStringType).Size));
				}
				else if (mibType is ObjectIdentifierType)
				{
					result = new SnmpScalarNodeObjectIdentifier(parentNode);
				}
				else if (mibType is BitsType)
				{
					result = new SnmpScalarNodeBits(parentNode, (uint)((mibType as BitsType).Map.GetHighestValue() + 1));
					result.Restrictions.AddRange(CreateRestrictions(mibType as BitsType));
				}
				else
				{
					TypeAssignment ta = mibType as TypeAssignment;
					if (ta != null)
					{
						Console.WriteLine(String.Format("Unsupported BaseType: Module='{0}', Name='{1}', Type='{2}'!", ta.Module.Name, ta.Name, ta.Type));
					}
					else
					{
						Console.WriteLine(String.Format("Unsupported BaseType: Module='{0}', Name='{1}'!", mibType.Module, mibType.Name));
					}
					
					return null;
				}
			}

			result.Name = _alphaNumericRegex.Replace(ote.Name, "");
			result.Oid  = ote.Value;

			if (ote.Access == MaxAccess.readWrite)
			{
				result.AccessMode = SnmpAccessMode.ReadWrite;
			}
			else if (ote.Access == MaxAccess.readOnly)
			{
				result.AccessMode = SnmpAccessMode.ReadOnly;
			}
			else if (ote.Access == MaxAccess.readCreate)
			{
				result.AccessMode = SnmpAccessMode.ReadOnly;
			}
			else if (ignoreAccessibleFlag && (ote.Access == MaxAccess.notAccessible))
			{
				result.AccessMode = SnmpAccessMode.NotAccessible;
			}
			else
			{
				// not accessible or unsupported access type
				return null;
			}

			return result;
		}

		private static IEnumerable<IRestriction> CreateRestrictions(ValueRanges ranges)
		{
			List<IRestriction> result = new List<IRestriction>();

			if (ranges != null)
			{
				foreach (ValueRange range in ranges)
				{
					if (!range.End.HasValue)
					{
						result.Add(new IsEqualRestriction(range.Start));
					}
					else
					{
						result.Add(new IsInRangeRestriction(range.Start, range.End.Value));
					}
				}
			}

			return result;
		}

		private static IEnumerable<IRestriction> CreateRestrictions(ValueMap map)
		{
			if ((map != null) && (map.Count > 0))
			{
				return CreateRestrictions(map.GetContinousRanges());
			}

			return new List<IRestriction>();
		}

		private static IEnumerable<IRestriction> CreateRestrictions(BitsType bt)
		{
			List<IRestriction> result = new List<IRestriction>();

			if ((bt != null) && (bt.Map != null))
			{
				result.Add(new BitMaskRestriction(bt.Map.GetBitMask()));
			}

			return result;
		}

		private static SnmpTableNode GenerateSnmpTableNode(MibTreeNode mibTreeNode, SnmpTreeNode parentNode)
		{
			SnmpTableNode result = new SnmpTableNode(parentNode);
			result.Name = mibTreeNode.Entity.Name;
			result.Oid  = mibTreeNode.Entity.Value;

			// expect exactly one row entry
			if ((mibTreeNode.ChildNodes.Count != 1) || ((mibTreeNode.ChildNodes[0].NodeType & MibTreeNodeType.TableRow) == 0) || (mibTreeNode.ChildNodes[0].Entity.Value != 1))
			{
				Console.WriteLine("Found table with unsupported properties! Table needs exactly one (fixed) TableRow with OID=1 ! (" + mibTreeNode.Entity.Name + ")");
				return null;
			}

			MibTreeNode rowNode = mibTreeNode.ChildNodes[0];
			
			ObjectType rot = rowNode.Entity as ObjectType;
			if (rot != null)
			{
				if (!String.IsNullOrWhiteSpace(rot.Augments))
				{
					result.AugmentedTableRow = rot.Augments;

					// the indices from another table shall be used because this table is only an extension of it
					rot = MibTypesResolver.ResolveDeclaration(rot.Module, rot.Augments) as ObjectType;
				}

				if (rot.Indices != null)
				{
					foreach (string index in rot.Indices)
					{
						ObjectType indexEntity = MibTypesResolver.ResolveDeclaration(rot.Module, index) as ObjectType;
						if (indexEntity == null)
						{
							Console.WriteLine(String.Format("Could not resolve index '{0}' for table '{1}'! Table omitted!", index, result.Name));
							return null;
						}

						result.IndexNodes.Add(GenerateSnmpScalarNode(indexEntity, parentNode, ignoreAccessibleFlag: true));
					}
				}
			}

			if (result.IndexNodes.Count == 0)
			{
				// a table cannot be used without index
				Console.WriteLine("Found table without any index column ! (" + mibTreeNode.Entity.Name + ")");
				return null;
			}

			// add child nodes
			foreach (MibTreeNode cellNode in rowNode.ChildNodes)
			{
				SnmpScalarNode ssn = GenerateSnmpScalarNode(cellNode, parentNode);
				if (ssn != null)
				{
					result.CellNodes.Add(ssn);
				}
			}

			return result;
		}

		#endregion

	}
}
