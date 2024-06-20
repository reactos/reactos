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
using System.Text;
using CCodeGeneration;

namespace LwipSnmpCodeGeneration
{
	public class SnmpTreeNode: SnmpScalarAggregationNode
	{
		private readonly List<SnmpNode> childNodes       = new List<SnmpNode>();
		private readonly List<SnmpScalarNode> childScalarNodes = new List<SnmpScalarNode>();
		private string fullOid = "";

		public SnmpTreeNode(SnmpTreeNode parentNode)
			: base(parentNode)
		{
		}

		public override string FullNodeName
		{
			get { return this.Name.ToLowerInvariant() + "_treenode"; }
		}

		public string FullOid
		{
			get { return this.fullOid; }
			set { this.fullOid = value; }
		}

		public List<SnmpNode> ChildNodes
		{
			get { return this.childNodes; }
		}

		protected override IEnumerable<SnmpScalarNode> AggregatedScalarNodes
		{
			get { return this.childScalarNodes; }
		}

		private void GenerateAggregatedCode(MibCFile mibFile, bool generateDeclarations, bool generateImplementations)
		{
			VariableType instanceType = new VariableType("instance", LwipDefs.Vt_StNodeInstance, "*");
			base.GenerateAggregatedCode(
				mibFile,
				instanceType,
				String.Format("{0}->node->oid", instanceType.Name),
				generateDeclarations,
				generateImplementations);
		}

		private void GenerateAggregateMethodDeclarations(MibCFile mibFile)
		{
			if (LwipOpts.GenerateSingleAccessMethodsForTreeNodeScalars && (this.childScalarNodes.Count > 1))
			{
				GenerateAggregatedCode(mibFile, true, false);
			}
		}

		public override void GenerateCode(MibCFile mibFile)
		{
			string nodeInitialization;

			if (LwipOpts.GenerateSingleAccessMethodsForTreeNodeScalars && (this.childScalarNodes.Count > 1))
			{
				GenerateAggregatedCode(mibFile, false, true);
			}

			// create and add node declaration
			if (this.childNodes.Count > 0)
			{
				StringBuilder subnodeArrayInitialization = new StringBuilder();

				for (int i=0; i<this.childNodes.Count; i++)
				{
					subnodeArrayInitialization.Append("  &");
					subnodeArrayInitialization.Append(this.childNodes[i].FullNodeName);
					subnodeArrayInitialization.Append(".node");
					if (!(this.childNodes[i] is SnmpTreeNode))
					{
						subnodeArrayInitialization.Append(".node");
					}

					if (i < (this.childNodes.Count - 1))
					{
						subnodeArrayInitialization.Append(",\n");
					}
				}

				VariableDeclaration subnodeArray = new VariableDeclaration(
					new VariableType(this.Name.ToLowerInvariant() + "_subnodes", LwipDefs.Vt_StNode, "*", ConstType.Both, String.Empty),
					"{\n" + subnodeArrayInitialization + "\n}",
					isStatic: true);

				mibFile.Declarations.Add(subnodeArray);

				nodeInitialization = String.Format("SNMP_CREATE_TREE_NODE({0}, {1})", this.Oid, subnodeArray.Type.Name);
			}
			else
			{
				nodeInitialization = String.Format("SNMP_CREATE_EMPTY_TREE_NODE({0})", this.Oid);
			}

			mibFile.Declarations.Add(new VariableDeclaration(
				new VariableType(this.FullNodeName, LwipDefs.Vt_StTreeNode, null, ConstType.Value),
				nodeInitialization,
				isStatic: true));
		}

		public override void Analyze()
		{
			this.childScalarNodes.Clear();

			// delegate analyze (don't use enumerator because the child node may change our child collection by e.g. removing or replacing itself)
			for (int i=this.ChildNodes.Count-1; i>=0; i--)
			{
				this.ChildNodes[i].Analyze();
			}

			// collect scalar nodes
			foreach (SnmpNode childNode in this.childNodes)
			{
				SnmpScalarNode scalarNode = childNode as SnmpScalarNode;
				if (scalarNode != null)
				{
					this.childScalarNodes.Add(scalarNode);
				}
			}

			base.Analyze();

			// check if we can merge this node to a scalar array node (all childs need to be scalars)
			if (this.childNodes.Count > 0)
			{
				if (LwipOpts.GenerateScalarArrays && (this.childScalarNodes.Count == this.childNodes.Count) && (this.ParentNode != null))
				{
					SnmpScalarArrayNode scalarArrayNode = new SnmpScalarArrayNode(this.childScalarNodes, this.ParentNode);
					scalarArrayNode.Oid  = this.Oid;
					scalarArrayNode.Name = this.Name;
					scalarArrayNode.Analyze();

					for (int i=0; i<this.ParentNode.ChildNodes.Count; i++)
					{
						if (this.ParentNode.ChildNodes[i] == this)
						{
							this.ParentNode.ChildNodes.RemoveAt(i);
							this.ParentNode.ChildNodes.Insert(i, scalarArrayNode);
							break;
						}
					}
				}
				else if (LwipOpts.GenerateSingleAccessMethodsForTreeNodeScalars && (this.childScalarNodes.Count > 1))
				{
					foreach (SnmpScalarNode scalarNode in this.childScalarNodes)
					{
						scalarNode.UseExternalMethods = true;
						scalarNode.ExternalGetMethod  = this.GetMethodName;
						scalarNode.ExternalTestMethod = this.TestMethodName;
						scalarNode.ExternalSetMethod  = this.SetMethodName;
					}
				}
			}
			else // if (this.childNodes.Count == 0)
			{
				if (!LwipOpts.GenerateEmptyFolders && (this.ParentNode != null))
				{
					// do not generate this empty folder because it only waste (static) memory
					for (int i=0; i<this.ParentNode.ChildNodes.Count; i++)
					{
						if (this.ParentNode.ChildNodes[i] == this)
						{
							this.ParentNode.ChildNodes.RemoveAt(i);
							break;
						}
					}
				}
			}
		}

		public override void Generate(MibCFile generatedFile, MibHeaderFile generatedHeaderFile)
		{
			// generate code of child nodes
			foreach (SnmpNode childNode in this.childNodes)
			{
				if (childNode is SnmpTreeNode)
				{
					childNode.Generate(generatedFile, generatedHeaderFile);
				}
			}

			Comment dividerComment = new Comment(
				String.Format("--- {0} {1} -----------------------------------------------------", this.Name, this.fullOid),
				singleLine: true);

			generatedFile.Declarations.Add(dividerComment);
			generatedFile.Implementation.Add(dividerComment);

			this.GenerateAggregateMethodDeclarations(generatedFile);

			foreach (SnmpNode childNode in this.childNodes)
			{
				if (!(childNode is SnmpTreeNode))
				{
					childNode.Generate(generatedFile, generatedHeaderFile);
				}
			}

			base.Generate(generatedFile, generatedHeaderFile);
		}
	}
}
