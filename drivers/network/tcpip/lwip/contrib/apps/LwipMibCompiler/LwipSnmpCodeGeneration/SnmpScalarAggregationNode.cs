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

using System.Collections.Generic;
using System.Globalization;
using CCodeGeneration;

namespace LwipSnmpCodeGeneration
{
	public abstract class SnmpScalarAggregationNode: SnmpNode
	{
		private bool getMethodRequired  = false;
		private bool testMethodRequired = false;
		private bool setMethodRequired  = false;

		protected SnmpScalarAggregationNode(SnmpTreeNode parentNode)
			: base(parentNode)
		{
		}

		protected virtual string GetMethodName
		{
			get { return this.FullNodeName + LwipDefs.FnctSuffix_GetValue; }
		}

		protected bool GetMethodRequired
		{
			get { return this.getMethodRequired; }
		}

		protected virtual string TestMethodName
		{
			get { return this.FullNodeName + LwipDefs.FnctSuffix_SetTest; }
		}

		protected bool TestMethodRequired
		{
			get { return this.testMethodRequired; }
		}

		protected virtual string SetMethodName
		{
			get { return this.FullNodeName + LwipDefs.FnctSuffix_SetValue; }
		}

		protected bool SetMethodRequired
		{
			get { return this.setMethodRequired; }
		}

		protected abstract IEnumerable<SnmpScalarNode> AggregatedScalarNodes
		{
			get;
		}

		public override void Analyze()
		{
			base.Analyze();

			this.getMethodRequired  = false;
			this.testMethodRequired = false;
			this.setMethodRequired  = false;

			foreach (SnmpScalarNode scalarNode in this.AggregatedScalarNodes)
			{
				if ((scalarNode.AccessMode == SnmpAccessMode.ReadOnly) || (scalarNode.AccessMode == SnmpAccessMode.ReadWrite))
				{
					this.getMethodRequired = true;
				}
				if ((scalarNode.AccessMode == SnmpAccessMode.WriteOnly) || (scalarNode.AccessMode == SnmpAccessMode.ReadWrite))
				{
					this.testMethodRequired = true;
					this.setMethodRequired = true;
				}

				if (this.getMethodRequired && this.setMethodRequired)
				{
					break;
				}
			}
		}

		protected void GenerateAggregatedCode(MibCFile mibFile, VariableType instanceType, string switchSelector, bool generateDeclarations = true, bool generateImplementations = true)
		{
			if (this.getMethodRequired)
			{
				FunctionDeclaration getMethodDecl = new FunctionDeclaration(this.GetMethodName, isStatic: true);
				getMethodDecl.Parameter.Add(instanceType);
				getMethodDecl.Parameter.Add(new VariableType("value", VariableType.VoidString, "*"));
				getMethodDecl.ReturnType = new VariableType(null, LwipDefs.Vt_S16);

				if (generateDeclarations)
				{
					mibFile.Declarations.Add(getMethodDecl);
				}
				if (generateImplementations)
				{
					Function getMethod = Function.FromDeclaration(getMethodDecl);
					GenerateGetMethodCode(getMethod, switchSelector);
					mibFile.Implementation.Add(getMethod);
				}
			}
	
			if (this.testMethodRequired)
			{
				FunctionDeclaration testMethodDecl = new FunctionDeclaration(this.TestMethodName, isStatic: true);
				testMethodDecl.Parameter.Add(instanceType);
				testMethodDecl.Parameter.Add(new VariableType("len", LwipDefs.Vt_U16));
				testMethodDecl.Parameter.Add(new VariableType("value", VariableType.VoidString, "*"));
				testMethodDecl.ReturnType = new VariableType(null, LwipDefs.Vt_Snmp_err);

				if (generateDeclarations)
				{
					mibFile.Declarations.Add(testMethodDecl);
				}
				if (generateImplementations)
				{
					Function testMethod = Function.FromDeclaration(testMethodDecl);
					GenerateTestMethodCode(testMethod, switchSelector);
					mibFile.Implementation.Add(testMethod);
				}
			}

			if (this.setMethodRequired)
			{
				FunctionDeclaration setMethodDecl = new FunctionDeclaration(this.SetMethodName, isStatic: true);
				setMethodDecl.Parameter.Add(instanceType);
				setMethodDecl.Parameter.Add(new VariableType("len", LwipDefs.Vt_U16));
				setMethodDecl.Parameter.Add(new VariableType("value", VariableType.VoidString, "*"));
				setMethodDecl.ReturnType = new VariableType(null, LwipDefs.Vt_Snmp_err);
				
				if (generateDeclarations)
				{
					mibFile.Declarations.Add(setMethodDecl);
				}
				if (generateImplementations)
				{
					Function setMethod = Function.FromDeclaration(setMethodDecl);
					GenerateSetMethodCode(setMethod, switchSelector);
					mibFile.Implementation.Add(setMethod);
				}
			}
		}

		protected virtual void GenerateGetMethodCode(Function getMethod, string switchSelector)
		{
			VariableDeclaration returnValue = new VariableDeclaration((VariableType)getMethod.ReturnType.Clone());
			returnValue.Type.Name = "value_len";
			getMethod.Declarations.Add(returnValue);
			Switch sw = new Switch(switchSelector);

			bool valueVarUsed = false;

			foreach (SnmpScalarNode scalarNode in this.AggregatedScalarNodes)
			{
				if ((scalarNode.AccessMode == SnmpAccessMode.ReadOnly) || (scalarNode.AccessMode == SnmpAccessMode.ReadWrite))
				{
					SwitchCase sc = new SwitchCase(scalarNode.Oid.ToString(CultureInfo.InvariantCulture));
					sc.Declarations.Add(new Comment(scalarNode.Name, singleLine: true));

					scalarNode.GenerateGetMethodCode(sc, getMethod.Parameter[1].Name, ref valueVarUsed, returnValue.Type.Name);

					sw.Switches.Add(sc);
				}
			}

			SwitchCase scd = SwitchCase.GenerateDefault();
			scd.AddCodeFormat("LWIP_DEBUGF(SNMP_MIB_DEBUG,(\"{0}(): unknown id: %\"S32_F\"\\n\", {1}));", getMethod.Name, switchSelector);
			scd.AddCodeFormat("{0} = 0;", returnValue.Type.Name);
			sw.Switches.Add(scd);

			if (!valueVarUsed)
			{
				getMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", getMethod.Parameter[1].Name);
			}

			getMethod.AddElement(sw);

			getMethod.AddCodeFormat("return {0};", returnValue.Type.Name);
		}

		protected virtual void GenerateTestMethodCode(Function testMethod, string switchSelector)
		{
			VariableDeclaration returnValue = new VariableDeclaration((VariableType)testMethod.ReturnType.Clone(), LwipDefs.Def_ErrorCode_WrongValue);
			returnValue.Type.Name = "err";
			testMethod.Declarations.Add(returnValue);
			Switch sw = new Switch(switchSelector);

			bool valueVarUsed = false;
			bool lenVarUsed = false;

			foreach (SnmpScalarNode scalarNode in this.AggregatedScalarNodes)
			{
				if ((scalarNode.AccessMode == SnmpAccessMode.WriteOnly) || (scalarNode.AccessMode == SnmpAccessMode.ReadWrite))
				{
					SwitchCase sc = new SwitchCase(scalarNode.Oid.ToString(CultureInfo.InvariantCulture));
					sc.Declarations.Add(new Comment(scalarNode.Name, singleLine: true));

					scalarNode.GenerateTestMethodCode(sc, testMethod.Parameter[2].Name, ref valueVarUsed, testMethod.Parameter[1].Name, ref lenVarUsed, returnValue.Type.Name);

					sw.Switches.Add(sc);
				}
			}

			SwitchCase scd = SwitchCase.GenerateDefault();
			scd.AddCodeFormat("LWIP_DEBUGF(SNMP_MIB_DEBUG,(\"{0}(): unknown id: %\"S32_F\"\\n\", {1}));", testMethod.Name, switchSelector);
			sw.Switches.Add(scd);

			if (!valueVarUsed)
			{
				testMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", testMethod.Parameter[2].Name);
			}
			if (!lenVarUsed)
			{
				testMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", testMethod.Parameter[1].Name);
			}

			testMethod.AddElement(sw);

			testMethod.AddCodeFormat("return {0};", returnValue.Type.Name);
		}

		protected virtual void GenerateSetMethodCode(Function setMethod, string switchSelector)
		{
			VariableDeclaration returnValue = new VariableDeclaration((VariableType)setMethod.ReturnType.Clone(), LwipDefs.Def_ErrorCode_Ok);
			returnValue.Type.Name = "err";
			setMethod.Declarations.Add(returnValue);
			Switch sw = new Switch(switchSelector);

			bool valueVarUsed = false;
			bool lenVarUsed = false;
			
			foreach (SnmpScalarNode scalarNode in this.AggregatedScalarNodes)
			{
				if ((scalarNode.AccessMode == SnmpAccessMode.WriteOnly) || (scalarNode.AccessMode == SnmpAccessMode.ReadWrite))
				{
					SwitchCase sc = new SwitchCase(scalarNode.Oid.ToString(CultureInfo.InvariantCulture));
					sc.Declarations.Add(new Comment(scalarNode.Name, singleLine: true));

					scalarNode.GenerateSetMethodCode(sc, setMethod.Parameter[2].Name, ref valueVarUsed, setMethod.Parameter[1].Name, ref lenVarUsed, returnValue.Type.Name);

					sw.Switches.Add(sc);
				}
			}

			SwitchCase scd = SwitchCase.GenerateDefault();
			scd.AddCodeFormat("LWIP_DEBUGF(SNMP_MIB_DEBUG,(\"{0}(): unknown id: %\"S32_F\"\\n\", {1}));", setMethod.Name, switchSelector);
			sw.Switches.Add(scd);

			if (!valueVarUsed)
			{
				setMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", setMethod.Parameter[2].Name);
			}
			if (!lenVarUsed)
			{
				setMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", setMethod.Parameter[1].Name);
			}

			setMethod.AddElement(sw);

			setMethod.AddCodeFormat("return {0};", returnValue.Type.Name);
		}
	}
}
