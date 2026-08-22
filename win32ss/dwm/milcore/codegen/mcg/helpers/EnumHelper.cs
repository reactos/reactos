// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Generate enums
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

    public class EnumHelper : GeneratorMethods
    {
        /// <summary>
        /// Write a native enum definition
        /// </summary>
        public static string FormatNativeEnum(McgEnum e)
        {
            string comment = String.Empty;
            if (!CodeGenHelpers.IsEmpty(e.Comment))
            {
                comment =
                        [[inline]]
                            //
                            [[Helpers.CodeGenHelpers.FormatComment(e.Comment, 80, "// ")]]
                            //
                        [[/inline]];
            }

            //
            // Some enums are declared using a namespace and some are not. Since
            // the syntax for each differs, we need to preserve the difference.
            //
            // We use a macro to define the namespace so it can be turned off
            // for kernel components.
            //
            if (!e.UseFlatEnum)
            {
                if (e.Flags)
                {
                    return comment +
                        [[inline]]
                            BEGIN_MILFLAGENUM( [[e.UnmanagedName]] )
                                [[GetEnumMembers(e)]]
                            END_MILFLAGENUM
                        [[/inline]]
                        ;
                }
                else
                {
                    return comment +
                        [[inline]]
                            BEGIN_MILENUM( [[e.UnmanagedName]] )
                                [[GetEnumMembers(e)]]
                            END_MILENUM
                        [[/inline]]
                        ;
                }
            }
            else
            {
                if (e.Flags)
                {
                    throw new ApplicationException(String.Format(
                        "Flat enums can't be flag type: {0}",
                        e.UnmanagedName));
                }

                return comment +
                    [[inline]]
                        typedef enum
                        {
                            [[GetEnumMembers(e)]]

                            [[e.UnmanagedName]]_[[GetNativeEnumForceSize(e)]]
                        } [[e.UnmanagedName]];
                    [[/inline]]
                    ;
            }
        }

        /// <summary>
        /// Write a managed enum definition
        /// </summary>
        public static string FormatManagedEnum(McgEnum e)
        {
            string comment = String.Empty;
            if (!CodeGenHelpers.IsEmpty(e.Comment))
            {
                comment =
                    [[inline]]
                        //
                        [[Helpers.CodeGenHelpers.FormatComment(e.Comment, 80, "// ")]]
                        //
                    [[/inline]]
                    ;
            }

            string maybeFlagsDeclaration = String.Empty;
            if (e.Flags)
            {
                maybeFlagsDeclaration =
                    [[inline]]
                        [System.Flags]
                    [[/inline]]
                    ;
            }

            //
            // Need to refactor McgType so that we can create a new copy
            // with a different ManagedName so that this code can
            // use ManagedName instead of UnmanagedName.
            //

            return comment + maybeFlagsDeclaration +
                [[inline]]
                    internal enum [[e.UnmanagedName]][[GetManagedEnumInheritance(e)]]
                    {
                        [[GetEnumMembers(e)]]

                        [[GetManagedEnumForceSize(e)]]
                    }
                [[/inline]]
                ;
        }

        /// <summary>
        /// Get the members of the enum. This is identical for both managed and
        /// native code.
        /// </summary>
        private static string GetEnumMembers(McgEnum e)
        {
            StringCodeSink cs = new StringCodeSink();

            foreach (IMcgEnumChild enumChild in e.AllChildren)
            {
                McgEnumValue enumValue = enumChild as McgEnumValue;
                McgBlockCommentedEnumValues block = enumChild as McgBlockCommentedEnumValues;

                if (enumValue != null)
                {
                    if (enumValue.Comment != null)
                    {
                        cs.BeginBlock(1);
                        cs.Write(FormatEnumValue(enumValue));
                        cs.EndBlock(1);
                    }
                    else
                    {
                        cs.Write(FormatEnumValue(enumValue));
                    }
                }
                else
                {
                    Debug.Assert(block != null);

                    cs.BeginBlock(2);
                    cs.Write(
                        [[inline]]
                            //
                            [[Helpers.CodeGenHelpers.FormatComment(block.Comment, 80, "// ")]]
                            //

                        [[/inline]]
                        );

                    foreach (McgEnumValue ev in block.Values)
                    {
                        if (ev.Comment != null)
                        {
                            cs.BeginBlock(1);
                            cs.Write(FormatEnumValue(ev));
                            cs.EndBlock(1);
                        }
                        else
                        {
                            cs.Write(FormatEnumValue(ev));
                        }
                    }

                    cs.EndBlock(2);
                }
            }

            return cs.ToString();
        }

        /// <summary>
        /// Format an enum value, possibly with a trailing comment. The
        /// formatting is identical, regardless of whether it's native/managed.
        /// </summary>
        private static string FormatEnumValue(McgEnumValue enumValue)
        {
            string comment = String.Empty;
            if (enumValue.Comment != null)
            {
                comment =
                        [[inline]]
                            //
                            [[Helpers.CodeGenHelpers.FormatComment(enumValue.Comment, 80, "// ")]]
                            //
                        [[/inline]];
            }

            if (enumValue.Value != null)
            {
                return comment +
                    [[inline]]
                        [[enumValue.Name]] = [[enumValue.Value]],
                    [[/inline]]
                    ;
            }
            else
            {
                return comment +
                    [[inline]]
                        [[enumValue.Name]],
                    [[/inline]]
                    ;
            }
        }

        /// <summary>
        /// Some managed enums are declared as
        /// enum Foo : ushort
        /// This function returns the correct string to append to
        /// enum Foo
        /// </summary>
        private static string GetManagedEnumInheritance(McgEnum e)
        {
            if (e.MarshaledSize == 0 || e.MarshaledSize == 4)
            {
                return "";
            }
            else if (e.MarshaledSize == 2)
            {
                return " : ushort";
            }
            else
            {
                Debug.Assert(false, "Unexpected MarshaledSize");
                return null;
            }
        }

        /// <summary>
        /// At the end of each enum declaration we include a value of maximum
        /// size to make sure the enum is the size we expect it to be. This
        /// returns the definition for managed code.
        /// </summary>
        private static string GetManagedEnumForceSize(McgEnum e)
        {
            if (e.MarshaledSize == 0 || e.MarshaledSize == 4)
            {
                return
                    [[inline]]
                        FORCE_DWORD = unchecked((int)0xffffffff)
                    [[/inline]]
                    ;
            }
            else if (e.MarshaledSize == 2)
            {
                return
                    [[inline]]
                        FORCE_WORD = unchecked((int)0xffff)
                    [[/inline]]
                    ;
            }
            else
            {
                Debug.Assert(false, "Unexpected MarshaledSize");
                return null;
            }
        }


        /// <summary>
        /// At the end of each enum declaration we include a value of maximum
        /// size to make sure the enum is the size we expect it to be. This
        /// returns the definition for native code.
        /// </summary>
        private static string GetNativeEnumForceSize(McgEnum e)
        {
            if (e.MarshaledSize == 0 || e.MarshaledSize == 4)
            {
                return
                    [[inline]]
                        FORCE_DWORD = 0xffffffff
                    [[/inline]]
                    ;
            }
            else if (e.MarshaledSize == 2)
            {
                return
                    [[inline]]
                        FORCE_WORD = 0xffff
                    [[/inline]]
                    ;
            }
            else
            {
                Debug.Assert(false, "Unexpected MarshaledSize");
                return null;
            }
        }
    }
}



