// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Helper methods for code gen
//

namespace MS.Internal.MilCodeGen.Helpers
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;

    public class CodeGenHelpers
    {
        public static bool IsEmpty(string s)
        {
            return (s == null) || (s.Length <= 0);
        }

        internal static string WriteFieldStatementsWithSeparator(McgField[] fields, 
                                                                 string statement,
                                                                 string separator)
        {
            return WriteFieldStatementsFirstLastWithSeparator(fields,
                                                              statement,
                                                              statement,
                                                              statement,
                                                              separator);
        }

        internal static string WriteFieldStatementsFirstLastWithSeparator(McgField[] fields, 
                                                                          string firstStatement,
                                                                          string statement,
                                                                          string lastStatement,
                                                                          string separator)
        {
            StringCodeSink cs = new StringCodeSink();

            for (int i = 0; i < fields.Length; i++)
            {
                if (i == 0)
                {
                    cs.Write(WriteFieldStatement(fields[i], firstStatement));
                    cs.Write(separator);                    
                }
                else if (i < fields.Length - 1)
                {
                    cs.Write(WriteFieldStatement(fields[i], statement));
                    cs.Write(separator);
                }
                else
                {
                    cs.Write(WriteFieldStatement(fields[i], lastStatement));
                }
            }

            return cs.ToString();
        }

        internal static string WriteFieldStatements(McgField[] fields, string statement)
        {
            StringCodeSink cs = new StringCodeSink();

            foreach(McgField field in fields)
            {
                cs.WriteBlock(WriteFieldStatement(field, statement));
            }

            return cs.ToString();
        }

        internal static string WriteFieldStatement(McgField field, string statement)
        {
            string fieldName = field.PropertyName;
            string animationContainerName = field.PropertyName + "Animations";
            string animationContainerType = field.Type.ManagedName + "AnimationCollection";

            StringBuilder output = new StringBuilder(statement);
            output.Replace("{type}", field.Type.Name);
            output.Replace("{managedType}", field.Type.ManagedName);
            output.Replace("{localName}", field.Name);
            output.Replace("{propertyName}", field.PropertyName);
            output.Replace("{propertyFlag}", field.PropertyFlag);
            output.Replace("{dpPropertyName}", field.DPPropertyName);
            output.Replace("{fieldName}", fieldName);
            output.Replace("{parseMethod}", field.Type.ParseMethod);
            output.Replace("{animationContainerName}", animationContainerName);
            output.Replace("{animationContainerType}", animationContainerType);
            output.Replace("{marshalValue}", field.IsUnmanagedOnly ? field.Default : field.PropertyName);            
            return output.ToString();
        }

        internal static void ThrowValidationException(XmlNode node, string message)
        {
            throw new ApplicationException(String.Format(
                "Validation failed at node {0}: {1}",
                node.Name,
                message));
        }

        internal static void ThrowValidationException(string message)
        {
            throw new ApplicationException(String.Format(
                "Validation failed: {0}",
                message));
        }

        internal enum ParameterType
        {
            None                                 = 0x0000,
            ManagedParamList                     = 0x0001,
            ManagedCallParamList                 = 0x0002,
            ManagedImportsParamList              = 0x0004,
            UnmanagedParamList                   = 0x0008,
            UnmanagedCallParamList               = 0x0010,
            RenderDataCallParamList              = 0x0020,
            SkipAnimations                       = 0x0040,
            AssignToMemberVariables              = 0x0080
        }

        /// <summary>
        /// AppendParameters - Helper routine to append parameters to a param list.
        /// </summary>
        internal static void AppendParameters(
            DelimitedList parameterList,             
            ICollection fields, 
            ParameterType parameterType )
        {
            foreach(McgField field in fields)
            {
                AppendParameter(parameterList, field, parameterType);
            }
        }

        internal static void AppendParameter(
            DelimitedList parameterList,             
            McgField field, 
            ParameterType parameterType )
        {
            AppendParameter(parameterList,
                                    field.Type,
                                    field.Name,
                                    field.Name,
                                    field.PropertyName,
                                    field.PropertyName,
                                    field.IsAnimated, 
                                    parameterType);
        }

        /// <summary>
        /// AppendStructParameters - Helper routine to append parameters to a param list.
        /// </summary>
        internal static int AppendStructParameters(
            DelimitedList parameterList,             
            PaddedStructData paddedFields, 
            int initialPosition,
            bool unmanagedStruct)
        {
            return AppendStructParameters(
                parameterList,             
                paddedFields, 
                initialPosition,
                unmanagedStruct,
                false
                );
        }

        internal static int AppendStructParameters(
            DelimitedList parameterList,             
            PaddedStructData paddedFields, 
            int initialPosition,
            bool unmanagedStruct,
            bool kernelAccessibleStruct
            )
        {
            int padSuffix = 0;
            
            Helpers.CodeGenHelpers.AlignedFieldOffsetHelper alignedFieldHelper = new Helpers.CodeGenHelpers.AlignedFieldOffsetHelper(4, 4);
            
            foreach (AlignmentEntry alignmentEntry in paddedFields.AlignmentEntries)
            {
                McgField field = alignmentEntry.Field;
                int position = initialPosition+alignmentEntry.Offset;
                
                alignedFieldHelper.MoveToNextEntry(position, alignmentEntry.Size);
                int alignedFieldOffset = alignedFieldHelper.AlignedFieldOffset;
                
                if (unmanagedStruct)
                {
                    // Insert enough padding to ensure the field is properly aligned
                    foreach(string alignmentField in alignedFieldHelper.AlignmentFields)
                    {
                        parameterList.Append(alignmentField);
                    }
                }
                
                // This primary if else else block switches between the tree types of entries in the
                // paddedFields alignment list: padding, animation and normal field entries.
                // The offset of each field is stored in "position", which is the padding offset of the
                // entry + the initial position.
                if (field == null)
                {
                    Debug.Assert(alignmentEntry.IsPad);

                    string padIdentifier = "padQuadAlignment" + padSuffix;

                    if (unmanagedStruct)
                    {
                        parameterList.Append("UINT32 " + padIdentifier + ";");
                    }
                    else
                    {
                        parameterList.Append("[FieldOffset(" + alignedFieldOffset + ")] private UInt32 " + padIdentifier);
                    }
                    ++padSuffix;
                }
                else if (alignmentEntry.IsAnimation)
                {
                    if (unmanagedStruct)
                    {
                        parameterList.Append("HMIL_RESOURCE " + "h" + field.PropertyName + "Animations;");
                    }
                    else
                    {
                        parameterList.Append("[FieldOffset(" + alignedFieldOffset + ")] internal DUCE.ResourceHandle h" + field.PropertyName + "Animations");
                    }
                }
                else
                {
                    // This case is a boring, non animate field.

                    bool constructedParam = false;
                    McgType type = field.Type;
                    McgResource resourceType = type as McgResource;

                    // If it's an McgResource, we have to handle reference types and collections
                    if(resourceType != null)
                    {
                        // Currently, collections are accounted for by storing just their size inline
                        if (resourceType.IsCollection) 
                        {
                            if (unmanagedStruct)
                            {
                                parameterList.Append("UINT32 " + field.Name + "Size;");
                            }
                            else
                            {
                                parameterList.Append("[FieldOffset(" + alignedFieldOffset + ")] internal UInt32 " + field.Name + "Size");
                            }
                            constructedParam = true;
                        }
                        // If it's not a collection, is it a reference type?  If so, it's passed by handle.
                        else if (!resourceType.IsValueType)
                        {
                            if (unmanagedStruct)
                            {
                                parameterList.Append("HMIL_RESOURCE " + "h" + field.Name + ";");
                            }
                            else
                            {
                                parameterList.Append("[FieldOffset(" + alignedFieldOffset + ")] internal DUCE.ResourceHandle h" + field.Name);
                            }
                            
                            constructedParam = true;
                        }
                    }

                    // If we haven't yet handled this parameter, we're left with a primitive type, 
                    // like a struct or an enum.  At this point, the value will be stored inline.
                    // The only real question left is whether the unmanaged struct matches the managed struct.
                    // If so, the struct is simply stored.  If not, then .NeedsConvert is true, and
                    // the record is of the unmanaged type, even on the managed side of things.
                    if (!constructedParam) 
                    {
                        if (type.NeedsConvert) 
                        {
                            if (unmanagedStruct)
                            {
                                parameterList.Append(type.BaseUnmanagedType + " " + field.Name + ";");
                            }
                            else
                            {
                                parameterList.Append("[FieldOffset(" + alignedFieldOffset + ")] internal " + type.MarshalUnmanagedType + " " + field.Name);
                            }
                        }
                        else
                        {
                            if (unmanagedStruct)
                            {
                                if (kernelAccessibleStruct)
                                {
                                    McgEnum enumType = type as McgEnum;

                                    if (enumType != null)
                                    {
                                        parameterList.Append(enumType.KernelAccessibleType + " " + field.Name + ";");
                                        constructedParam = true;
                                    }
                                }

                                if (!constructedParam)
                                {
                                    parameterList.Append(type.BaseUnmanagedType + " " + field.Name + ";");
                                }
                            }
                            else
                            {
                                parameterList.Append("[FieldOffset(" + alignedFieldOffset + ")] internal " + type.ManagedName + " " + field.Name);
                            }
                        }
                    }
                }
            }
            
            if (unmanagedStruct)
            {
                // Insert enough padding to ensure the struct's size falls on a packing boundary
                foreach(string packingField in alignedFieldHelper.NativePackingFields)
                {
                    parameterList.Append(packingField);
                }
            }
            else
            {
                // Insert enough padding to ensure the struct's size falls on a packing boundary
                foreach(string packingField in alignedFieldHelper.ManagedPackingFields)
                {
                    parameterList.Append(packingField);
                }
            }
            
            return initialPosition+paddedFields.PaddedSize;
        }

        /// <summary>
        /// AppendParameters - Helper routine to append parameters to a param list.
        /// </summary>
        internal static void AppendParameter(
            DelimitedList parameterList,
            McgType type, 
            string name,
            string fieldName, 
            string propertyName,
            string internalReadName,
            bool isAnimated, 
            ParameterType parameterType )
        {
            isAnimated = isAnimated && ((parameterType & ParameterType.SkipAnimations) == 0);

            McgResource resourceType = type as McgResource;

            // We accumulate the parameter(s) in these strings.
            string paramString = String.Empty;
            string animateParamString = String.Empty;

            if ((parameterType & ParameterType.ManagedParamList) != 0)
            {
                paramString = type.Name + " " + name;

                // Field is animated -- also pass resource by handle.
                if (((resourceType == null) || resourceType.IsValueType) && isAnimated)
                {
                    // Field is animated -- also pass resource by handle.
                    animateParamString = "AnimationClock " + name + "Animations";
                }
            }
            else if ((parameterType & ParameterType.ManagedCallParamList) != 0)
            {
                paramString = name;

                // Field is animated -- also pass resource by handle.
                if (((resourceType == null) || resourceType.IsValueType) && isAnimated)
                {
                    // Field is animated -- also pass resource by handle.
                    animateParamString =  name + "Animations";
                }
            }
            else if ((parameterType & ParameterType.RenderDataCallParamList) != 0)
            {            
                // Field is a resource that can not be passed by value.
                if(resourceType != null && !resourceType.IsValueType)
                {                   
                    paramString = "_renderData.AddDependentResource(" + fieldName + ")";
                }
                else
                {
                    paramString = ConvertToValueType(type, name);

                    if (isAnimated)
                    {
                        // Field is animated -- also pass resource by handle.
                        animateParamString = "h" + propertyName + "Animations";
                    }
                }
            }
            else if ((parameterType & ParameterType.ManagedImportsParamList) != 0)
            {
                // If it's not a value type, then we can pass by handle
                if ((resourceType != null) && !resourceType.IsValueType)
                {
                    paramString = DuceHandle.ManagedTypeName + " h" + propertyName;
                }
                else
                {
                    // If it is a value type, we need to know whether we convert it or not
                    if (type.NeedsConvert)
                    {
                        // If it does need a convert, then the import is typed to the
                        // unmanaged value type, because that's what the Convert* method
                        // returns.
                        paramString = type.UnmanagedDataType + " " + name;
                    }
                    else
                    {
                        // Otherwise, we know that this value type does not need a conversion,
                        // and thus should be passed directly as its managed type.
                        paramString = type.ManagedName + " " + name;
                    }

                    // Field is animated -- also pass resource by handle.
                    if (isAnimated)
                    {
                        animateParamString = DuceHandle.ManagedTypeName + " h" + GeneratorMethods.FirstCap(name) + "Animations";
                    }                    
                }
            }            
            else if ((parameterType & ParameterType.UnmanagedParamList) != 0)
            {
                if ((resourceType != null) && !resourceType.IsValueType)
                {
                    paramString = DuceHandle.UnmanagedTypeName + " h" + GeneratorMethods.FirstCap(name);
                }
                else
                {
                    paramString = type.UnmanagedDataType + " " + name;

                    // Field is animated -- also pass resource by handle.
                    if (isAnimated)
                    {
                        animateParamString = DuceHandle.UnmanagedTypeName + " h" + GeneratorMethods.FirstCap(name) + "Animations";
                    }                    
                }
            }
            else
            {
                Debug.Assert((parameterType & ParameterType.UnmanagedCallParamList) != 0);

                if ((resourceType != null) && !resourceType.IsValueType)
                {
                    paramString = "h" + GeneratorMethods.FirstCap(name);
                }
                else
                {
                    paramString = name;

                    // Field is animated -- also pass resource by handle.
                    if (isAnimated)
                    {
                        animateParamString = "h" + GeneratorMethods.FirstCap(name) + "Animations";
                    }                    
                }
            }

            // animateParamString should either be Empty or have content - it shouldn't be null
            Debug.Assert(null != animateParamString);


            // Now that we have the parameter(s), we have to decide what to do with them
            if ((parameterType & ParameterType.AssignToMemberVariables) != 0)
            {
                parameterList.Append("this." + paramString + " = " + paramString);

                if (animateParamString.Length != 0)
                {
                    parameterList.Append("this." + animateParamString + " = " + animateParamString);
                }
            }
            else
            {
                parameterList.Append(paramString);

                if (animateParamString.Length != 0)
                {
                    parameterList.Append(animateParamString);
                }
            }
        }

        internal static string ConvertToValueType(McgType type, string fieldName)
        {
            switch (type.Name)
            {
                case "Boolean":
                    return "CompositionResourceManager.BooleanToUInt32(" + fieldName + ")";
                case "Color":
                    return "CompositionResourceManager.ColorToMilColorF(" + fieldName + ")";
                case "Matrix3D":
                    return "CompositionResourceManager.Matrix3DToD3DMATRIX(" + fieldName + ")";
                case "Matrix":
                    return "CompositionResourceManager.MatrixToMilMatrix3x2D(" + fieldName + ")";
                case "Vector3D":
                    return "CompositionResourceManager.Vector3DToMilPoint3F(" + fieldName + ")";
                case "Point3D":
                    return "CompositionResourceManager.Point3DToMilPoint3F(" + fieldName + ")";
                case "Quaternion":
                    return "CompositionResourceManager.QuaternionToMilQuaternionF(" + fieldName + ")";
                default:
                    return fieldName;
            }
        }

        /// <summary>
        ///     FormatComment - formats a comment at a given line break length.
        /// </summary>
        /// <returns>
        ///     string - The string which contains the comment, or String.Empty if there's nothing to do.
        /// </returns>
        /// <param name="comment"> string - The comment itself. </param>
        /// <param name="lineBreakLength"> int - the line break length </param>
        /// <param name="leadingString"> string - the leading string to prepend to the output if the 
        /// comment contains any text. </param>
        /// <param name="alternateText"> string - The string to return if the comment is empty. </param>
        /// <param name="managed"> If true, emit a managed-style comment, else unmanaged. </param>
        internal static string FormatComment(string comment,
                                             int lineBreakLength,
                                             string leadingString,
                                             string alternateText,
                                             bool managed)
        {
            if (comment == null)
            {
                return alternateText;
            }

            comment = comment.Replace("\r\n", "\n");
            comment = comment.Trim();

            if (comment.Length <= 0)
            {
                return alternateText;
            }

            StringCodeSink cs = new StringCodeSink();

            cs.Write(leadingString);

            cs.Write(FormatComment(comment, lineBreakLength, managed ? "///     " : "//                  "));

            return cs.ToString();
        }

        /// <summary>
        ///     FormatComment - formats a comment at a given line break length.
        /// </summary>
        /// <returns>
        ///     string - The string which contains the comment, or String.Empty if there's nothing to do.
        /// </returns>
        /// <param name="comment"> string - The comment itself. </param>
        /// <param name="lineBreakLength"> int - the line break length </param>
        /// <param name="lineLeader"> string - the leading string to prepend to each line</param>
        internal static string FormatComment(string comment,
                                             int lineBreakLength,
                                             string lineLeader)
        {
            if (comment == null)
            {
                return String.Empty;
            }

            comment = comment.Replace("\r\n", "\n");
            comment = comment.Trim();

            StringCodeSink cs = new StringCodeSink();

            int curChar = 0;
            bool firstLine = true;

            while (curChar < comment.Length)
            {
                //
                // Find the appropriate line-break point
                //

                int firstNonSpace = curChar;

                while ((firstNonSpace < comment.Length) && Char.IsWhiteSpace(comment[firstNonSpace]))
                {
                    firstNonSpace++;
                }

                if (firstNonSpace == comment.Length)
                {
                    break;
                }

                int lastSpace = firstNonSpace + lineBreakLength - 1;

                //
                // The line-break point is the first occurrence of
                // a newline character, if one occurs within lineBreakLength
                // of the firstNonSpace.
                // We have to take a minimum because IndexOf requires that the
                // count doesn't go beyond the end of the string.
                //
                int firstNewline = comment.IndexOf(
                                    '\n',
                                    firstNonSpace,
                                    Math.Min(lastSpace, comment.Length - 1) - firstNonSpace + 1);

                if (firstNewline != -1)
                {
                    lastSpace = firstNewline - 1;
                }
                else
                {
                    //
                    // If no newline characters appear within lineBreakLength of the
                    // firstNonSpace, the line-break point is the end of the comment,
                    // if the comment ends within lineBreakLength of the firstNonSpace
                    //
                    if (lastSpace >= comment.Length)
                    {
                        lastSpace = comment.Length - 1;
                    }
                    else
                    {
                        //
                        // Otherwise the line-break point is the last space
                        // character within lineBreakLength of the firstNonSpace.
                        //
                        while ((lastSpace > firstNonSpace) && !Char.IsWhiteSpace(comment[lastSpace]))
                        {
                            lastSpace--;
                        }

                        //
                        // If no space characters exist, then the line is unbreakable
                        // so we just break at lineBreakLength from firstNonSpace
                        //
                        if (lastSpace == firstNonSpace)
                        {
                            lastSpace = firstNonSpace + lineBreakLength - 1;
                        }
                    }
                }

                //
                // For all lines but the first we write a newline character to
                // terminate the previous line.
                //
                if (firstLine)
                {
                    firstLine = false;
                }
                else
                {
                    cs.Write("\n");
                }

                cs.Write(lineLeader);

                cs.Write(comment.Substring(firstNonSpace, lastSpace - firstNonSpace + 1));

                //
                // Set curChar to the beginning of the next line
                //
                curChar = lastSpace + 1;
            }

            return cs.ToString();
        }

        /// <summary>
        /// FormatParam - returns a string representation of the comment for a given parameter
        /// </summary>
        /// <returns>
        /// The string representation of the comment for a given parameter
        /// </returns>
        /// <param name="fieldName"> The name of the parameter. </param>
        /// <param name="comment"> The comment associated with the parameter. </param>
        /// <param name="fieldTypeName"> The name of the field's type. </param>
        /// <param name="lineBreakLength"> The max length of each line of the comment. </param>
        internal static string FormatParam(string fieldName,
                                           string comment,
                                           string fieldTypeName,
                                           int lineBreakLength)
        {
            string prefix = "/// <param name=\"" + fieldName + "\">";
            string postfix = "</param>";

            // Does this fit onto one line? (add two for spaces)
            if (prefix.Length + postfix.Length + comment.Length + 2 <= lineBreakLength)
            {
                return prefix + " " + comment + " " + postfix;
            }

            StringCodeSink cs = new StringCodeSink();

            string formattedComment = FormatComment(comment, lineBreakLength, "\n", null, true /* managed */);

            cs.Write(prefix);

            if (null == formattedComment)
            {
                cs.Write(fieldTypeName);
            }
            else
            {
                cs.Write(formattedComment + "\n/// ");
            }

            cs.Write(postfix);

            return cs.ToString();
        }

        /// <summary>
        /// FormatParam - returns a string representation of the comment for a given parameter for unmanged code.
        /// </summary>
        /// <returns>
        /// The string representation of the comment for a given parameter
        /// </returns>
        /// <param name="fieldName"> The name of the parameter. </param>
        /// <param name="comment"> The comment associated with the parameter. </param>
        /// <param name="fieldTypeName"> The name of the field's type. </param>
        /// <param name="lineBreakLength"> The max length of each line of the comment. </param>
        internal static string FormatParamUnmanaged(string fieldName,
                                                    string comment,
                                                    string fieldTypeName,
                                                    int lineBreakLength)
        {
            string prefix = "//              " + fieldName + ":";

            // Does this fit onto one line? (add one for the space)
            if (prefix.Length + comment.Length + 1 <= lineBreakLength)
            {
                return prefix + " " + comment;
            }

            StringCodeSink cs = new StringCodeSink();

            string formattedComment = FormatComment(comment, lineBreakLength, "\n", null, false /* unmanaged */);

            cs.Write("//              " + fieldName + ":");

            if ((null == formattedComment) || (formattedComment.Length < 1))
            {
                cs.Write(fieldTypeName);
            }
            else
            {
                cs.Write(formattedComment);
            }

            return cs.ToString();
        }

        /// <summary>
        /// PaddedStructData - this class contains an array of AlignmentEntry instances which 
        /// describe how to lay out a struct for correct alignment as well as the new resultant
        /// struct size.
        /// </summary>
        public class PaddedStructData
        {
            public PaddedStructData(
                AlignmentEntry[] alignmentEntries,
                int paddedSize)
            {
                AlignmentEntries = alignmentEntries;
                PaddedSize = paddedSize;
            }

            /// <summary>
            /// Identity constructor -- converts an array of fields to a padded struct data,
            /// with no padding or alignment sorting being performed.
            /// </summary>
            /// <param name="fields">The fields to convert</param>
            public PaddedStructData(
                McgField[] fields)
            {
                AlignmentEntries = new AlignmentEntry[fields.Length];
                int i = 0;

                foreach (McgField field in fields)
                {
                    AlignmentEntry entry = 
                        new AlignmentEntry(field, false, false);

                    entry.Offset = PaddedSize;

                    PaddedSize += field.Type.UnpaddedSize;

                    AlignmentEntries[i++] = entry;
                }
            }


            /// <summary>
            /// Builds the real field names and decides their type.
            /// </summary>
            internal void BuildMarshaledFieldNames()
            {
                int padSuffix = 0;

                foreach (AlignmentEntry entry in AlignmentEntries) 
                {
                    McgField field = entry.Field;

                    //
                    // This primary if else else block switches between the tree types of entries in the
                    // paddedFields alignment list: padding, animation and normal field entries.
                    //
                    if (field == null)
                    {
                        entry.Name = "padQuadAlignment" + padSuffix++;
                        entry.IsPad = true;
                    }
                    else if (entry.IsAnimation)
                    {
                        entry.Name = "h" + field.PropertyName + "Animations";
                        entry.IsHandle = true;
                    }
                    else
                    {
                        // This case is a boring, non animate field.

                        McgType type = field.Type;
                        McgResource resourceType = type as McgResource;

                        // If it's an McgResource, we have to handle reference types and collections
                        if (resourceType != null && !resourceType.IsValueType)
                        {
                            entry.Name = "h" + field.Name;
                            entry.IsHandle = true;
                        }
                        else
                        {
                            entry.Name = field.Name;

                            if (type.Name == "ResourceHandle") 
                            {
                                entry.IsHandle = true;
                            }
                        }
                    }
                }
            }

            public AlignmentEntry[] AlignmentEntries;
            public int PaddedSize;
        }

        /// <summary>
        /// AlignmentEntry - class
        /// This is an entry in the alignment array.
        /// It specifies the field being referred to, whether it's the field itself
        /// or the associated animations, or if it is a DWORD padding entry.
        /// </summary>
        public class AlignmentEntry
        {
            public AlignmentEntry(
                McgField field,
                bool isAnimation,
                bool isPad)
            {
                Field = field;
                IsAnimation = isAnimation;
                IsPad = isPad;
            }

            public int Size
            {
                get
                {
                    if (IsPad) return 4;
                    if (IsAnimation) return DuceHandle.Size;

                    return Field.Type.UnpaddedSize;
                }
            }

            public string Name = String.Empty;

            public bool IsAnimation = false;
            public bool IsPad = false;
            public bool IsHandle = false;

            public McgField Field = null;
            public int Offset = 0;
        }

        /// <summary>
        /// SortStructForAlignment - returns an array of AlignmentEntry's which describe
        /// the correct, padded/aligned layout for this struct.
        /// </summary>
        /// <returns>
        /// PaddedStructData - this contains an array of AlignmentEntry's which dictate the fields 
        /// in the sorted struct and the resultant struct size.
        /// </returns>
        /// <param name="fields"> The fields to sort. </param>
        /// <param name="doAnimations"> Whether or not to add extra handles for animations. </param>
        /// <param name="padFullStructure"> Whether or not to pad the entire structure to a multiple of 8 bytes. </param>
        internal static PaddedStructData SortStructForAlignment(
            McgField[] fields,
            bool doAnimations,
            bool padFullStructure)
        {
            Debug.Assert(DuceHandle.Size == 4 || DuceHandle.Size == 8,
                "We assume UCE handles are 4 or 8 bytes.");
            
            bool alignHandles = DuceHandle.Size > 4;

            //
            //  The algorithm used to align the fields is to sort the fields into three buckets:
            //
            //    1) QWord aligned fields whose length happens to be evenly divisible by 8.
            //    2) QWord aligned fields whose length is evenly divisible by 4, but not 8.
            //    3) DWords
            //
            //  Fields in bucket 1 can be immediately emited because the following offset will
            //  always be QWord aligned.  Example:
            //
            //    +----------+
            //    |     8    |
            //    +----------+  <- Offset = 8   (QWord aligned)
            //    |          |
            //    |    24    |
            //    |          |
            //    +----------+  <- Offset = 32  (QWord aligned)
            //
            //  At the end of the sorting we will then use fields in bucket 3 as padding for fields
            //  in bucket 2.

            List<AlignmentEntry> sorted = new List<AlignmentEntry>();
            List<AlignmentEntry> needsDwordPadding = new List<AlignmentEntry>();
            List<AlignmentEntry> dwords = new List<AlignmentEntry>();

            foreach (McgField field in fields)
            {
                if (field.Type.ShouldBeQuadWordAligned)
                {
                    AlignmentEntry qwordEntry = new AlignmentEntry(field, /* isAnimation = */ false, /* isPad = */ false);
                    int paddingNeeded = field.Type.UnpaddedSize % 8;

                    if (paddingNeeded == 0)
                    {
                        sorted.Add(qwordEntry);
                    }
                    else if (paddingNeeded == 4)
                    {
                        needsDwordPadding.Add(qwordEntry);
                    }
                    else
                    {
                        Debug.Fail("We currently only support QWord/DWords aligment.");
                    }
                }
                else
                {
                    if (field.Type.UnpaddedSize == 4)
                    {
                        dwords.Add(new AlignmentEntry(field, /* isAnimation = */ false, /* isPad = */ false));
                    }
                    else
                    {
                        Debug.Fail("We currently do not support fields smaller than 4 bytes.");
                    }
                }

                if (doAnimations && field.IsAnimated)
                {
                    AlignmentEntry animationEntry = new AlignmentEntry(field, /* isAnimation = */true, /* isPadding = */ false);

                    if (alignHandles)
                    {
                        sorted.Add(animationEntry);
                    }
                    else
                    {
                        dwords.Add(animationEntry);
                    }
                }
            }

            //
            //  Use our available DWord sized fields to pad any QWord aligned fields which
            //  have lengths not evenly divisible 8.  Example:
            //
            //    +----------+ 
            //    |          |
            //    |    12    |
            //    |          |
            //    +----------+  <- Offset = 12  (DWord aligned)
            //    |     4    |
            //    +----------+  <- Offset = 16  (QWord aligned)
            //
            //  If we have no DWords available we create padding.
            //

            foreach (AlignmentEntry qwordEntry in needsDwordPadding)
            {
                sorted.Add(qwordEntry);

                if (dwords.Count > 0)
                {
                    sorted.Add(dwords[0]);
                    dwords.RemoveAt(0);
                }
                else
                {
                    sorted.Add(new AlignmentEntry(/* field = */ null, /* isAnimation = */ false, /* isPad = */ true));
                }
            }

            //
            //  Emit any remaining DWords
            //
            
            foreach (AlignmentEntry dwordEntry in dwords)
            {
                sorted.Add(dwordEntry);
            }

            //
            //  Loop through the list of sorted entries and compute the size of the struct.
            //

            int structSize = 0;
            
            foreach(AlignmentEntry entry in sorted)
            {
                structSize += entry.Size;
            }

            //
            //  Pad the struct itself if necessary.
            //
            
            if (padFullStructure)
            {               
                // Finally, the struct itself must be aligned
                if ((structSize % 8) != 0)
                {
                    // We're only handling DWORD -> QWORD padding at this point
                    Debug.Assert((structSize % 4) == 0);
                
                    sorted.Add(new AlignmentEntry(/* field = */ null, /* isAnimation = */ false, /* isPad = */ true));
                    structSize += 4;
                }
            }

            AlignmentEntry[] asArray = (AlignmentEntry[])sorted.ToArray();

            int offset = 0;

            // Now walk the list and add calculated offsets to each entry
            foreach (AlignmentEntry entry in asArray)
            {
                entry.Offset = offset;
                offset += entry.Size;
            }

            return new PaddedStructData(asArray, structSize);
        }

        // This method adds the string name of each value set in flags enum T to
        // the delimitedlist.
        //
        // (e.g., MyEnum.Value1 | MyEnum.Value1 -> {"Value1", "Value2"})
        //
        public static void AddFlagsToList<T>(DelimitedList list, T value) // Would be "where T:Enum" if it weren't for CS0702  (danlehen, 05/10/04)
        {
            T[] flags = SplitEnumFlags<T>(value);

            foreach(T flag in flags)
            {
                list.Append(Enum.GetName(typeof(T), value));
            }
        }

        // This method returns an array of enum T where each entry is a flag
        // of T which was set in value.
        //
        // (e.g., MyEnum.Value1 | MyEnum.Value2 -> {MyEnum.Value1, MyEnum.Value2})
        //
        public static T[] SplitEnumFlags<T>(T value) // Would be "where T:Enum" if it weren't for CS0702 (danlehen, 05/10/04)
        {
            Array flags = Enum.GetValues(typeof(T));
            List<T> values = new List<T>();

            for(int i = 0; i < flags.Length; i++)
            {
                T flag = (T) flags.GetValue(i);

                // Because we could not constrain T to our enum type we cannot
                // cast T directly to int (we get CS0030).  So instead we cast it to an
                // object from where we can cast to an int.
                //
                int v = (int)((object)value);
                int f = (int)((object)flag);

                if ((v & f) != 0)
                {
                    values.Add(value);
                }
            }

            return values.ToArray();
        }
        
        /// <summary>
        /// AlignedFieldOffsetHelper - class
        /// This is a helper used to adjust field offsets so that 
        /// they fall on alignment boundaries.
        /// This is needed for generating code for arm which requires memory access
        /// be on word (4 byte) boundaries
        /// </summary>
        public class AlignedFieldOffsetHelper
        {
            public AlignedFieldOffsetHelper(int alignment, int packing)
            {
                Debug.Assert(alignment > 0);
                Debug.Assert(packing > 0);
                
                this.alignment = alignment;
                this.packing = packing;
            }
            
            // get the aligned offset of the current field
            public int AlignedFieldOffset
            {
                get { return this.alignedFieldOffset; }
            }
            
            // get unmanaged fields required to align the current field
            public IEnumerable<string> AlignmentFields
            {
                get 
                {
                    return CreatePaddingFields(this.alignmentPadding);
                }
            }

            // get unmanaged fields required to make the struct end on a packing boundary
            public IEnumerable<string> NativePackingFields
            {
                get 
                {
                    return CreatePaddingFields(this.packPadding);
                }
            }
            
            // get unmanaged fields required to make the struct end on a packing boundary
            public IEnumerable<string> ManagedPackingFields
            {
                get 
                {
                    // Insert a field to ensure the struct's size falls on a packing boundary
                    if (this.packPadding > 0)
                    {
                        yield return "[FieldOffset(" + (this.packedStructSize - 1) + ")] private byte " + GetNextName("BYTEPacking");
                    }                
                }
            }
            
            public void MoveToNextEntry(int fieldOffset, int fieldSize)
            {
                int paddedFieldOffset = fieldOffset + this.accumulatedPadding;
                this.alignmentPadding = GetPadding(paddedFieldOffset, this.alignment);
                this.alignedFieldOffset = paddedFieldOffset + this.alignmentPadding;
                this.accumulatedPadding += this.alignmentPadding;
                
                int nextFieldOffset = this.alignedFieldOffset + fieldSize;
                this.packPadding = GetPadding(nextFieldOffset, this.packing);
                this.packedStructSize = nextFieldOffset + packPadding;
            }
            
            // Get padding required to align offset an an (alignment) byte boundary
            private static int GetPadding(int offset, int alignment)
            {
                int padding = offset % alignment;
                
                if (padding != 0)
                {
                    padding = alignment - padding;
                    Debug.Assert(padding > 0);
                }
                
                return padding;
            }
            
            private IEnumerable<string> CreatePaddingFields(int padding)
            {
                for (; padding >= 4; padding -= 4)
                {
                    yield return "UINT32 " + GetNextName("UINT32Padding") + ";";
                }
                
                for (; padding >= 2; padding -= 2)
                {
                    yield return "UINT16 " + GetNextName("UINT16Padding") + ";";
                }
                
                for (; padding >= 1; padding -= 1)
                {
                    yield return "BYTE " + GetNextName("BYTEPadding") + ";";
                }
            }
            
            private string GetNextName(string baseName)
            {
                return baseName + (this.paddingSuffix++);
            }
            
            private readonly int alignment;
            private readonly int packing;

            private int alignedFieldOffset;
            private int alignmentPadding;
            
            private int packedStructSize;
            private int packPadding;
            
            private int accumulatedPadding;
            private int paddingSuffix;
        }
    }
}




