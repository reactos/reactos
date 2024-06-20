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
using CCodeGeneration;

namespace LwipSnmpCodeGeneration
{
	public class SnmpScalarNode: SnmpNode
	{
		protected const string LocalValueName = "v"; // name of (casted) local value variable

		private SnmpDataType dataType;
		private SnmpAccessMode accessMode;
		private readonly List<IRestriction> restrictions = new List<IRestriction>();

		private bool   useExternalMethods = false;
		private string externalGetMethod;
		private string externalTestMethod;
		private string externalSetMethod;


		public SnmpScalarNode(SnmpTreeNode parentNode)
			: base(parentNode)
		{
		}

		public override string FullNodeName
		{
			get { return this.Name.ToLowerInvariant() + "_scalar"; }
		}

		public SnmpDataType DataType
		{
			get { return this.dataType; }
			set { this.dataType = value; }
		}

		public List<IRestriction> Restrictions
		{
			get { return this.restrictions; }
		}

		public SnmpAccessMode AccessMode
		{
			get { return this.accessMode; }
			set { this.accessMode = value; }
		}

		public virtual string FixedValueLength
		{
			get { return null; }
		}

		/// <summary>
		/// If scalar is used as a table index its value becomes part of the OID. This value returns how many OID parts are required to represent this value.
		/// </summary>
		public virtual int OidRepresentationLen
		{
			get { return -1; }
		}

		public bool UseExternalMethods
		{
			get { return this.useExternalMethods; }
			set { this.useExternalMethods = value; }
		}

		public string ExternalGetMethod
		{
			get { return this.externalGetMethod; }
			set { this.externalGetMethod = value; }
		}
		public string ExternalTestMethod
		{
			get { return this.externalTestMethod; }
			set { this.externalTestMethod = value; }
		}
		public string ExternalSetMethod
		{
			get { return this.externalSetMethod; }
			set { this.externalSetMethod = value; }
		}

		public override void GenerateCode(MibCFile mibFile)
		{
			string getMethodName;
			string testMethodName;
			string setMethodName;

			if (this.useExternalMethods)
			{
				getMethodName  = this.externalGetMethod;
				testMethodName = this.externalTestMethod;
				setMethodName  = this.externalSetMethod;
			}
			else
			{
				getMethodName  = LwipDefs.Null;
				testMethodName = LwipDefs.Null;
				setMethodName  = LwipDefs.Null;
				
				if ((this.accessMode == SnmpAccessMode.ReadWrite) || (this.accessMode == SnmpAccessMode.ReadOnly))
				{
					FunctionDeclaration  getMethodDecl = new FunctionDeclaration(this.Name + LwipDefs.FnctSuffix_GetValue, isStatic: true);
					getMethodDecl.Parameter.Add(new VariableType("instance", LwipDefs.Vt_StNodeInstance, "*"));
					getMethodDecl.Parameter.Add(new VariableType("value", VariableType.VoidString, "*"));
					getMethodDecl.ReturnType = new VariableType(null, LwipDefs.Vt_S16);
					mibFile.Declarations.Add(getMethodDecl);

					Function getMethod = Function.FromDeclaration(getMethodDecl);
					getMethodName = getMethod.Name;

					VariableDeclaration returnValue = new VariableDeclaration((VariableType)getMethod.ReturnType.Clone());
					returnValue.Type.Name = "value_len";
					getMethod.Declarations.Add(returnValue);
					getMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", getMethod.Parameter[0].Name);

					bool valueVarUsed = false;
					GenerateGetMethodCode(getMethod, getMethod.Parameter[1].Name, ref valueVarUsed, returnValue.Type.Name);
					if (!valueVarUsed)
					{
						getMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", getMethod.Parameter[1].Name);
					}

					getMethod.AddCodeFormat("return {0};", returnValue.Type.Name);

					mibFile.Implementation.Add(getMethod);
				}

				if ((this.accessMode == SnmpAccessMode.ReadWrite) || (this.accessMode == SnmpAccessMode.WriteOnly))
				{
					bool valueVarUsed;
					bool lenVarUsed;
					VariableDeclaration returnValue;

					if (this.restrictions.Count > 0)
					{
						FunctionDeclaration testMethodDecl = new FunctionDeclaration(this.Name + LwipDefs.FnctSuffix_SetTest, isStatic: true);
						testMethodDecl.Parameter.Add(new VariableType("instance", LwipDefs.Vt_StNodeInstance, "*"));
						testMethodDecl.Parameter.Add(new VariableType("len", LwipDefs.Vt_U16));
						testMethodDecl.Parameter.Add(new VariableType("value", VariableType.VoidString, "*"));
						testMethodDecl.ReturnType = new VariableType(null, LwipDefs.Vt_Snmp_err);
						mibFile.Declarations.Add(testMethodDecl);

						Function testMethod = Function.FromDeclaration(testMethodDecl);
						testMethodName = testMethod.Name;

						returnValue = new VariableDeclaration((VariableType)testMethod.ReturnType.Clone(), LwipDefs.Def_ErrorCode_WrongValue);
						returnValue.Type.Name = "err";
						testMethod.Declarations.Add(returnValue);
						testMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", testMethod.Parameter[0].Name);

						valueVarUsed = false;
						lenVarUsed = false;

						GenerateTestMethodCode(testMethod, testMethod.Parameter[2].Name, ref valueVarUsed, testMethod.Parameter[1].Name, ref lenVarUsed, returnValue.Type.Name);

						if (!valueVarUsed)
						{
							testMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", testMethod.Parameter[2].Name);
						}
						if (!lenVarUsed)
						{
							testMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", testMethod.Parameter[1].Name);
						}

						testMethod.AddCodeFormat("return {0};", returnValue.Type.Name);

						mibFile.Implementation.Add(testMethod);

					}
					else
					{
						testMethodName = LwipDefs.FnctName_SetTest_Ok;
					}

					FunctionDeclaration setMethodDecl  = null;
					setMethodDecl = new FunctionDeclaration(this.Name + LwipDefs.FnctSuffix_SetValue, isStatic: true);
					setMethodDecl.Parameter.Add(new VariableType("instance", LwipDefs.Vt_StNodeInstance, "*"));
					setMethodDecl.Parameter.Add(new VariableType("len", LwipDefs.Vt_U16));
					setMethodDecl.Parameter.Add(new VariableType("value", VariableType.VoidString, "*"));
					setMethodDecl.ReturnType = new VariableType(null, LwipDefs.Vt_Snmp_err);
					mibFile.Declarations.Add(setMethodDecl);

					Function setMethod = Function.FromDeclaration(setMethodDecl);
					setMethodName = setMethod.Name;

					returnValue = new VariableDeclaration((VariableType)setMethod.ReturnType.Clone(), LwipDefs.Def_ErrorCode_Ok);
					returnValue.Type.Name = "err";
					setMethod.Declarations.Add(returnValue);
					setMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", setMethod.Parameter[0].Name);

					valueVarUsed = false;
					lenVarUsed = false;

					GenerateSetMethodCode(setMethod, setMethod.Parameter[2].Name, ref valueVarUsed, setMethod.Parameter[1].Name, ref lenVarUsed, returnValue.Type.Name);

					if (!valueVarUsed)
					{
						setMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", setMethod.Parameter[2].Name);
					}
					if (!lenVarUsed)
					{
						setMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", setMethod.Parameter[1].Name);
					}

					setMethod.AddCodeFormat("return {0};", returnValue.Type.Name);

					mibFile.Implementation.Add(setMethod);
				}
			}

			// create and add node declaration
			string nodeInitialization;
			if (this.accessMode == SnmpAccessMode.ReadOnly)
			{
				nodeInitialization = String.Format("SNMP_SCALAR_CREATE_NODE_READONLY({0}, {1}, {2})",
					this.Oid,
					LwipDefs.GetAsn1DefForSnmpDataType(this.dataType),
					getMethodName);
			}
			else
			{
				nodeInitialization = String.Format("SNMP_SCALAR_CREATE_NODE({0}, {1}, {2}, {3}, {4}, {5})",
					this.Oid,
					LwipDefs.GetLwipDefForSnmpAccessMode(this.accessMode),
					LwipDefs.GetAsn1DefForSnmpDataType(this.dataType),
					getMethodName,
					testMethodName,
					setMethodName);
			}

			mibFile.Declarations.Add(new VariableDeclaration(
				new VariableType(this.FullNodeName, LwipDefs.Vt_StScalarNode, null, ConstType.Value),
				nodeInitialization, isStatic: true));
		}

		public virtual void GenerateGetMethodCode(CodeContainerBase container, string valueVarName, ref bool valueVarUsed, string retLenVarName)
		{
			bool localValueVarUsed;
			if (GenerateValueDeclaration(container, LocalValueName, valueVarName))
			{
				valueVarUsed = true;
				localValueVarUsed = false;
			}
			else
			{
				localValueVarUsed = true;  // do not generate UNUSED_ARG code
			}

			if (this.FixedValueLength == null)
			{
				// check that value with variable length fits into buffer
				container.AddElement(new Comment(String.Format("TODO: take care that value with variable length fits into buffer: ({0} <= SNMP_MAX_VALUE_SIZE)", retLenVarName), singleLine: true));
			}

			GenerateGetMethodCodeCore(container, LocalValueName, ref localValueVarUsed, retLenVarName);
			if (!localValueVarUsed)
			{
				container.AddCode(String.Format("LWIP_UNUSED_ARG({0});", LocalValueName));
			}
		}

		protected virtual void GenerateGetMethodCodeCore(CodeContainerBase container, string localValueVarName, ref bool localValueVarUsed, string retLenVarName)
		{
			container.AddElement(new Comment(String.Format("TODO: put requested value to '*{0}' here", localValueVarName), singleLine: true));
			container.AddCodeFormat("{0} = {1};", 
				retLenVarName,
				(!String.IsNullOrWhiteSpace(this.FixedValueLength)) ? this.FixedValueLength : "0");
		}

		public virtual void GenerateTestMethodCode(CodeContainerBase container, string valueVarName, ref bool valueVarUsed, string lenVarName, ref bool lenVarUsed, string retErrVarName)
		{
			if (this.Restrictions.Count > 0)
			{
				bool localVarUsed;
				if (GenerateValueDeclaration(container, LocalValueName, valueVarName))
				{
					valueVarUsed = true;
					localVarUsed = false;
				}
				else
				{
					localVarUsed = true;  // do not generate UNUSED_ARG code
				}

				if (!String.IsNullOrWhiteSpace(this.FixedValueLength))
				{
					// check for fixed value
					container.AddCodeFormat("LWIP_ASSERT(\"Invalid length for datatype\", ({0} == {1}));", lenVarName, this.FixedValueLength);
					lenVarUsed = true;
				}

				GenerateTestMethodCodeCore(container, LocalValueName, ref localVarUsed, lenVarName, ref lenVarUsed, retErrVarName);

				if (!localVarUsed)
				{
					container.AddCode(String.Format("LWIP_UNUSED_ARG({0});", LocalValueName));
				}
			}
			else
			{
				container.AddCodeFormat("{0} = {1};", retErrVarName, LwipDefs.Def_ErrorCode_Ok);
			}
		}

		protected virtual void GenerateTestMethodCodeCore(CodeContainerBase container, string localValueVarName, ref bool localValueVarUsed, string lenVarName, ref bool lenVarUsed, string retErrVarName)
		{
			container.AddElement(new Comment(String.Format("TODO: test new value here:\nif (*{0} == ) {1} = {2};", localValueVarName, retErrVarName, LwipDefs.Def_ErrorCode_Ok)));
		}

		public virtual void GenerateSetMethodCode(CodeContainerBase container, string valueVarName, ref bool valueVarUsed, string lenVarName, ref bool lenVarUsed, string retErrVarName)
		{
			bool localVarUsed;
			if (GenerateValueDeclaration(container, LocalValueName, valueVarName))
			{
				valueVarUsed = true;
				localVarUsed = false;
			}
			else
			{
				localVarUsed = true; // do not generate UNUSED_ARG code
			}

			GenerateSetMethodCodeCore(container, LocalValueName, ref localVarUsed, lenVarName, ref lenVarUsed, retErrVarName);

			if (!localVarUsed)
			{
				container.AddCode(String.Format("LWIP_UNUSED_ARG({0});", LocalValueName));
			}
		}

		protected virtual void GenerateSetMethodCodeCore(CodeContainerBase container, string localValueVarName, ref bool localValueVarUsed, string lenVarName, ref bool lenVarUsed, string retErrVarName)
		{
			container.AddElement(new Comment(String.Format("TODO: store new value contained in '*{0}' here", localValueVarName), singleLine: true)); 
		}


		protected virtual bool GenerateValueDeclaration(CodeContainerBase container, string variableName, string sourceName)
		{
			container.AddDeclaration(new VariableDeclaration(
				new VariableType(variableName, LwipDefs.Vt_U8, "*"),
				"(" + new VariableType(null, LwipDefs.Vt_U8, "*") + ")" + sourceName));

			return true;
		}

		public static SnmpScalarNode CreateFromDatatype(SnmpDataType dataType, SnmpTreeNode parentNode)
		{
			switch (dataType)
			{
				case SnmpDataType.Integer:
					return new SnmpScalarNodeInt(parentNode);

				case SnmpDataType.Gauge:
				case SnmpDataType.Counter:
				case SnmpDataType.TimeTicks:
					return new SnmpScalarNodeUint(dataType, parentNode);
			}

			return new SnmpScalarNode(parentNode);
		}
	}
}
