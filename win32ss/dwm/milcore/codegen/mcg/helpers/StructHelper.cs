// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Generate structs
//
//---------------------------------------------------------------------------

namespace MS.Internal.MilCodeGen.Helpers
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;

    public class StructHelper : GeneratorMethods
    {
        /// <summary>
        /// Write a native struct definition
        /// </summary>
        public static string FormatNativeStructure(McgResource resource)
        {
            string decl =
                [[inline]]
                    typedef struct[[(resource.Extends == null)? "" : " : " + resource.Extends.UnmanagedDataType]]
                    {
                        [[GetNativeStructMembers(resource)]]
                    } [[resource.UnmanagedDataType]];
                [[/inline]]
                ;

            if (resource.PragmaPack != null)
            {
                return
                    [[inline]]
                        [[GetNativeClassBlockComment(resource)]]
                        #pragma pack(push, [[resource.PragmaPack]])
                        [[decl]]
                        #pragma pack(pop)
                    [[/inline]]
                    ;
            }
            else
            {
                return
                    [[inline]]
                        [[GetNativeClassBlockComment(resource)]]
                        [[decl]]
                    [[/inline]]
                    ;
            }
        }

        /// <summary>
        /// Write a managed struct definition
        /// </summary>
        public static string FormatManagedStructure(McgResource resource)
        {
            Debug.Assert(resource.PragmaPack == null, "Pack pragma unsupported for managed types");

            //
            // Need to refactor McgType so that we can create a new copy
            // with a different ManagedName so that this code can
            // use ManagedName instead of UnmanagedDataType.
            //

            return
                [[inline]]
                    [[GetManagedClassBlockComment(resource)]]
                    [StructLayout(LayoutKind.Sequential, Pack=1)]
                    internal struct [[resource.UnmanagedDataType]][[(resource.Extends == null)? "" : " : " + resource.Extends.UnmanagedDataType]]
                    {
                        [[GetManagedStructMembers(resource)]]
                    };
                [[/inline]]
                ;
        }

        /// <summary>
        /// Get the members of the struct in string form appropriate for a
        /// native definition
        /// </summary>
        private static string GetNativeStructMembers(McgResource resource)
        {
            StringCodeSink cs = new StringCodeSink();

            foreach (IMcgStructChild child in resource.LocalChildren)
            {
                WriteChild(cs, child);
            }

            return cs.ToString();
        }

        private static void WriteChild(CodeSink cs, IMcgStructChild child)
        {
            McgField field = child as McgField;
            McgBlockCommentedFields block = child as McgBlockCommentedFields;
            McgUnion union = child as McgUnion;
            McgConstant constant = child as McgConstant;

            if (field != null)
            {
                WriteField(cs, field);
            }
            else if (block != null)
            {
                WriteBlockCommentedFields(cs, block);
            }
            else if (union != null)
            {
                WriteUnion(cs, union);
            }
            else
            {
                Debug.Assert(constant != null);
                WriteConstant(cs, constant);
            }
        }

        private static void WriteBlockCommentedFields(CodeSink cs, McgBlockCommentedFields block)
        {
            cs.BeginBlock(2);

            cs.Write(
                [[inline]]
                    //
                    [[Helpers.CodeGenHelpers.FormatComment(block.Comment, 80, "// ")]]
                    //

                [[/inline]]
                );

            foreach (IMcgStructChild child in block.Children)
            {
                WriteChild(cs, child);
            }

            cs.EndBlock(2);
        }

        private static void WriteUnion(CodeSink cs, McgUnion union)
        {
            cs.BeginBlock(1);

            cs.Write(
                [[inline]]
                    union
                    {
                [[/inline]]
                );

            cs.EndBlock(-1);

            cs.Indent(4);

            foreach (IMcgStructChild child in union.Children)
            {
                WriteChild(cs, child);
            }

            cs.EndBlock(-1);

            cs.Unindent();

            cs.Write(
                [[inline]]
                    };
                [[/inline]]
                );

            cs.EndBlock(1);
        }

        private static void WriteField(CodeSink cs, McgField field)
        {
            McgArray array = field.Type as McgArray;
            StringBuilder sb = new StringBuilder();

            if (field.Comment != null)
            {
                cs.BeginBlock(1);
                cs.Write(
                    [[inline]]
                        //
                        [[Helpers.CodeGenHelpers.FormatComment(field.Comment, 80, "// ")]]
                        //
                    [[/inline]]
                    );
            }

            if (array != null)
            {
                foreach (McgArrayDimension dim in array.Dimensions)
                {
                    sb.Append("[");
                    sb.Append(dim.Size);
                    sb.Append("]");
                }
            }

            cs.Write(
                [[inline]]
                    [[field.Type.UnmanagedDataType]] [[field.UnmanagedName]][[sb.ToString()]];
                [[/inline]]
                );

            if (field.Comment != null)
            {
                cs.EndBlock(1);
            }
        }

        private static void WriteConstant(CodeSink cs, McgConstant constant)
        {
            cs.BeginBlock(1);

            cs.Write(
                [[inline]]
                    enum {[[constant.Name]] = [[constant.Value]]};
                [[/inline]]
                );

            cs.EndBlock(1);
        }

        /// <summary>
        /// Get the members of the struct in string form appropriate for a
        /// managed definition
        /// </summary>
        private static StringBuilder GetManagedStructMembers(McgResource resource)
        {
            //
            // As with FormatManagedStructure, it would be nice if we used
            // the managed field names instead of unmanaged. We could do it
            // here, but for consistency with FormatManagedStructure, we use
            // unmanaged.
            //

            StringBuilder structMembers = new StringBuilder();

            //
            // Disallow unions
            //
            foreach (IMcgStructChild child in resource.LocalChildren)
            {
                if (child is McgUnion)
                {
                    Helpers.CodeGenHelpers.ThrowValidationException(
                        String.Format(
                            "'{0}' contains a union - not supported for managed code",
                            resource.UnmanagedDataType));
                }
            }

            foreach (McgField field in resource.LocalFields)
            {
                //
                // Disallow arrays (we can add this later if there is need for it)
                //
                if (field.Type is McgArray)
                {
                    Helpers.CodeGenHelpers.ThrowValidationException(
                        String.Format(
                            "'{0}.{1}' is an array - not yet supported for managed code",
                            resource.UnmanagedDataType,
                            field.UnmanagedName));
                }

                structMembers.Append(
                    [[inline]]
                        internal [[field.Type.ManagedName]] [[field.UnmanagedName]];
                    [[/inline]]
                    );
            }

            return structMembers;
        }

        /// <summary>
        /// Returns a block comment describing a class for native code.
        /// </summary>
        private static StringBuilder GetNativeClassBlockComment(McgResource resource)
        {
            StringBuilder comment = new StringBuilder();

            comment.Append(
                [[inline]]
                    //+-----------------------------------------------------------------------------
                    //
                    //  Class:
                    [[Helpers.CodeGenHelpers.FormatComment(resource.UnmanagedDataType, 80, "//      ")]]
                [[/inline]]
                );

            if (!CodeGenHelpers.IsEmpty(resource.Comment))
            {
                comment.Append(
                    [[inline]]
                        //
                        //  Synopsis:
                        [[Helpers.CodeGenHelpers.FormatComment(resource.Comment, 80, "//      ")]]
                    [[/inline]]
                    );
            }

            comment.Append(
                [[inline]]
                    //
                    //------------------------------------------------------------------------------
                [[/inline]]
                );

            return comment;
        }

        /// <summary>
        /// Returns a block comment describing a class for managed code.
        /// </summary>
        private static StringBuilder GetManagedClassBlockComment(McgResource resource)
        {
            StringBuilder comment = new StringBuilder();

            comment.Append(
                [[inline]]
                    /// <summary>
                    [[Helpers.CodeGenHelpers.FormatComment(resource.UnmanagedDataType, 80, "///     ")]]
                [[/inline]]
                );

            if (!CodeGenHelpers.IsEmpty(resource.Comment))
            {
                comment.Append(
                    [[inline]]
                        [[Helpers.CodeGenHelpers.FormatComment(resource.Comment, 80, "///     ")]]
                    [[/inline]]
                    );
            }

            comment.Append(
                [[inline]]
                    /// </summary>
                [[/inline]]
                );

            return comment;
        }
    }
}



