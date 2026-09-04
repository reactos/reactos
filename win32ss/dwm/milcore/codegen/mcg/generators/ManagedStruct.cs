// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Generate files for UCE master resources.
//

namespace MS.Internal.MilCodeGen.Generators
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    using MS.Internal.MilCodeGen;
    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;

    public partial class ManagedResources : Main.GeneratorBase
    {
        private void WriteGetHashCode(McgResource resource,
                                      StringCodeSink cs)
        {
            // At this time, we only know how to emit GetHashCode() for value types
            if (!resource.IsValueType) return;

            string getHashCodeBody = String.Empty;

            if (resource.LocalFields.Length < 1)
            {
                throw new System.NotSupportedException("Do not attempt to generate local fields for resources without local fields.");
            }
            else
            {
                StringCodeSink returnStmt = new StringCodeSink();

                // The inline block intentionally lacks a newline so that the first field
                // will be on the same line as the return.
                returnStmt.Write(
                    [[inline]]
                        // Perform field-by-field XOR of HashCodes
                        return [[/inline]]);

                returnStmt.Write(Helpers.CodeGenHelpers.WriteFieldStatementsWithSeparator(
                    resource.LocalFields,
                    "{fieldName}.GetHashCode()",
                    " ^\n       "                // The spaces trailing the newline indent the fields under the return.
                    ));

                // Terminate the last field with a semi-colon.
                returnStmt.Write(";\n");

                getHashCodeBody = returnStmt.ToString();

                // If this resource exposes a special empty state we need to wrap
                // the getHashCodyBody in a special clause to return a known
                // constant.
                if (resource.EmptyField != null)
                {
                    string emptyHashCode;
                    string emptyFlag;

                    if (resource.HasDistinguishedEmpty)
                    {
                        emptyHashCode = "c_identityHashCode";
                        emptyFlag = "IsDistinguishedIdentity";
                    }
                    else
                    {
                        emptyHashCode = "0";
                        emptyFlag = "Is" + resource.EmptyField.Name;
                    }

                    getHashCodeBody =
                        [[inline]]
                            if ([[emptyFlag]])
                            {
                                return [[emptyHashCode]];
                            }
                            else
                            {
                                [[getHashCodeBody]]
                            }
                        [[/inline]];
                }

            }

            cs.Write(
                [[inline]]
                    /// <summary>
                    /// Returns the HashCode for this [[resource.Name]]
                    /// </summary>
                    /// <returns>
                    /// int - the HashCode for this [[resource.Name]]
                    /// </returns>
                    public override int GetHashCode()
                    {
                        [[getHashCodeBody]]
                    }

                [[/inline]]
                );

        }

        private void WriteEqualsObject(McgResource resource,
                                       StringCodeSink cs)
        {
            if (!_resourceModel.ShouldGenerate(CodeSections.ManagedValueMethods, resource))
                return;

            string equalsBody = String.Empty;
            string lowerName = GeneratorMethods.FirstLower(resource.Name);

            StripDimensionSuffix(ref lowerName);

            if (resource.LocalFields.Length < 1)
            {
                throw new System.NotSupportedException("Do not attempt to generate local fields for resources without local fields.");
            }
            else
            {
                StringCodeSink returnStmt = new StringCodeSink();

                returnStmt.Write("return ");

                returnStmt.Write(Helpers.CodeGenHelpers.WriteFieldStatementsWithSeparator(
                    resource.LocalFields,
                    lowerName + "1.{fieldName}.Equals(" + lowerName + "2.{fieldName})",
                    " &&\n       "                // The spaces trailing the newline indent the fields under the return.
                    ));

                // Terminate the last field with a semi-colon.
                returnStmt.Write(";\n");

                equalsBody = returnStmt.ToString();

                if (resource.HasDistinguishedEmpty)
                {
                    equalsBody =
                        [[inline]]
                            if ([[lowerName]]1.IsDistinguishedIdentity || [[lowerName]]2.IsDistinguishedIdentity)
                            {
                                return [[lowerName]]1.Is[[resource.EmptyField.Name]] == [[lowerName]]2.Is[[resource.EmptyField.Name]];
                            }
                            else
                            {
                                [[equalsBody]]
                            }
                        [[/inline]];
                }
                else if (resource.EmptyField != null)
                {
                    equalsBody =
                        [[inline]]
                            if ([[lowerName]]1.Is[[resource.EmptyField.Name]])
                            {
                                return [[lowerName]]2.Is[[resource.EmptyField.Name]];
                            }
                            else
                            {
                                [[equalsBody]]
                            }
                        [[/inline]];
                }

            }

            cs.Write(
                [[inline]]
                    /// <summary>
                    /// Compares two [[resource.Name]] instances for object equality.  In this equality
                    /// Double.NaN is equal to itself, unlike in numeric equality.
                    /// Note that double values can acquire error when operated upon, such that
                    /// an exact comparison between two values which
                    /// are logically equal may fail.
                    /// </summary>
                    /// <returns>
                    /// bool - true if the two [[resource.Name]] instances are exactly equal, false otherwise
                    /// </returns>
                    /// <param name='[[lowerName]]1'>The first [[resource.Name]] to compare</param>
                    /// <param name='[[lowerName]]2'>The second [[resource.Name]] to compare</param>
                    public static bool Equals ([[resource.Name]] [[lowerName]]1, [[resource.Name]] [[lowerName]]2)
                    {
                        [[equalsBody]]
                    }

                    /// <summary>
                    /// Equals - compares this [[resource.Name]] with the passed in object.  In this equality
                    /// Double.NaN is equal to itself, unlike in numeric equality.
                    /// Note that double values can acquire error when operated upon, such that
                    /// an exact comparison between two values which
                    /// are logically equal may fail.
                    /// </summary>
                    /// <returns>
                    /// bool - true if the object is an instance of [[resource.Name]] and if it's equal to "this".
                    /// </returns>
                    /// <param name='o'>The object to compare to "this"</param>
                    public override bool Equals(object o)
                    {
                        if ((null == o) || !(o is [[resource.Name]]))
                        {
                            return false;
                        }

                        [[resource.Name]] value = ([[resource.Name]])o;
                        return [[resource.Name]].Equals(this,value);
                    }

                    /// <summary>
                    /// Equals - compares this [[resource.Name]] with the passed in object.  In this equality
                    /// Double.NaN is equal to itself, unlike in numeric equality.
                    /// Note that double values can acquire error when operated upon, such that
                    /// an exact comparison between two values which
                    /// are logically equal may fail.
                    /// </summary>
                    /// <returns>
                    /// bool - true if "value" is equal to "this".
                    /// </returns>
                    /// <param name='value'>The [[resource.Name]] to compare to "this"</param>
                    public bool Equals([[resource.Name]] value)
                    {
                        return [[resource.Name]].Equals(this, value);
                    }
                [[/inline]]
                );

        }

        private static string WriteToString(McgResource resource)
        {
            if (resource.SkipToString) return String.Empty;

            StringCodeSink cs = new StringCodeSink();

            if (resource.Extends == null)
            {
                string readPreamble = String.Empty;

                if (resource.IsFreezable)
                {
                    readPreamble = "ReadPreamble();";
                }

                cs.Write(
                    [[inline]]
                        /// <summary>
                        /// Creates a string representation of this object based on the current culture.
                        /// </summary>
                        /// <returns>
                        /// A string representation of this object.
                        /// </returns>
                        public override string ToString()
                        {
                            [[readPreamble]]
                            // Delegate to the internal method which implements all ToString calls.
                            return ConvertToString(null /* format string */, null /* format provider */);
                        }

                    [[/inline]]
                        );

                cs.Write(
                    [[inline]]
                        /// <summary>
                        /// Creates a string representation of this object based on the IFormatProvider
                        /// passed in.  If the provider is null, the CurrentCulture is used.
                        /// </summary>
                        /// <returns>
                        /// A string representation of this object.
                        /// </returns>
                        public string ToString(IFormatProvider provider)
                        {
                            [[readPreamble]]
                            // Delegate to the internal method which implements all ToString calls.
                            return ConvertToString(null /* format string */, provider);
                        }

                    [[/inline]]
                        );

                cs.Write(
                    [[inline]]
                        /// <summary>
                        /// Creates a string representation of this object based on the format string
                        /// and IFormatProvider passed in.
                        /// If the provider is null, the CurrentCulture is used.
                        /// See the documentation for IFormattable for more information.
                        /// </summary>
                        /// <returns>
                        /// A string representation of this object.
                        /// </returns>
                        string IFormattable.ToString(string format, IFormatProvider provider)
                        {
                            [[readPreamble]]
                            // Delegate to the internal method which implements all ToString calls.
                            return ConvertToString(format, provider);
                        }

                    [[/inline]]
                        );

                String modifiers = String.Empty;

                if (!resource.IsAbstract)
                {
                    if (resource.Extends != null)
                    {
                        modifiers = " override";
                    }
                                }
                else
                {
                    modifiers = " virtual";
                }

                cs.Write(
                    [[inline]]
                        /// <summary>
                        /// Creates a string representation of this object based on the format string
                        /// and IFormatProvider passed in.
                        /// If the provider is null, the CurrentCulture is used.
                        /// See the documentation for IFormattable for more information.
                        /// </summary>
                        /// <returns>
                        /// A string representation of this object.
                        /// </returns>
                        internal[[modifiers]] string ConvertToString(string format, IFormatProvider provider)
                        {
                    [[/inline]]
                        );

                if (resource.IsAbstract && (resource.Extends == null))
                {
                    cs.Write(
                        [[inline]]
                                return base.ToString();
                            }
                        [[/inline]]
                        );
                }
                else if (resource.IsCollection)
                {
                    cs.Write(
                        [[inline]]

                                if (_collection.Count == 0)
                                {
                                    return String.Empty;
                                }

                                StringBuilder str = new StringBuilder();

                                // Consider using this separator
                                // Helper to get the numeric list separator for a given culture.
                                // char separator = MS.Internal.TokenizerHelper.GetNumericListSeparator(provider);

                                for (int i=0; i<_collection.Count; i++)
                                {
                                    str.AppendFormat(
                                        provider,
                                        "{0:" + format + "}",
                                        _collection[i]);

                                    if (i != _collection.Count-1)
                                    {
                                        str.Append(" ");
                                    }
                                }

                                return str.ToString();
                            }
                        [[/inline]]
                            );
                }
                else // not a collection
                {
                    // If this resource exposes a special empty state we need to return
                    // the special empty string (since the empty state is sometimes
                    // signified by configuring the fields to an otherwise invalid state.)
                    if (resource.EmptyField != null)
                    {
                        cs.WriteBlock(
                            [[inline]]
                                    if (Is[[resource.EmptyField.Name]])
                                    {
                                        return "[[resource.EmptyField.Name]]";
                                    }
                            [[/inline]]
                            );
                    }

                    cs.Write(
                        [[inline]]
                                // Helper to get the numeric list separator for a given culture.
                                char separator = MS.Internal.TokenizerHelper.GetNumericListSeparator(provider);
                        [[/inline]]
                            );

                    if (resource.LocalFields.Length < 1)
                    {
                        cs.Write(
                            [[inline]]
                                    return base.ToString();
                            [[/inline]]
                                );
                    }
                    else
                    {
                        cs.Write(
                            [[inline]]
                                    return String.Format(provider,
                                                         "[[/inline]]
                                );

                        for (int i = 0; i < resource.LocalFields.Length; i++)
                        {
                            cs.Write(
                                [[inline]]
                                    {[[i+1]]:" + format + "}[[/inline]]
                                    );

                            if (i < resource.LocalFields.Length - 1)
                            {
                                cs.Write(
                                    [[inline]]
                                        {0}[[/inline]]
                                        );
                            }
                        }

                        cs.Write(
                            [[inline]]
                                ",
                                                         separator,
                            [[/inline]]
                            );

                        for (int i = 0; i < resource.LocalFields.Length; i++)
                        {
                            McgField field = resource.LocalFields[i];

                            cs.Write(
                                [[inline]]
                                                             [[resource.IsValueType ? field.InternalName : field.PropertyName]][[/inline]]
                                );

                            if (i < resource.LocalFields.Length - 1)
                            {
                                cs.Write(",\n");
                            }
                        }

                        cs.Write(
                            [[inline]]
                                );
                                }

                            [[/inline]]
                                );
                    }
                }
            }

            return cs.ToString();
        }

        /// <summary>
        /// WriteParseBody
        /// This method returns a string which will parse an instance of the type referred to
        /// by McgType.  It assumes that there is a variable called "value" to which it can
        /// assign the value it produces, as well as assuming that there's a TokenizerHelper
        /// called "th" available and in the correct state for parsing.  If there's ever a need
        /// to alter these assumptions, we can add parameters to WriteParseBody to control these.
        /// It assumes that the first token is present in the string referred to by firstToken,
        /// (typically a local string var or "th.GetCurrentToken()", and that all subsequent
        /// tokens are required.  It is left to the caller to call .LastTokenRequired() if it
        /// wished to ensure that there are no trailing tokens.
        /// </summary>
        private static string WriteParseBody(McgType type,
                                             string firstToken)
        {
            McgResource resource = type as McgResource;

            string parseBody = String.Empty;

            // If the type has a specific ParseMethod, we'll use it.
            if ((type.ParseMethod != null) && (type.ParseMethod.Length > 0))
            {
                parseBody =
                    [[inline]]
                        value = [[type.ParseMethod]]([[firstToken]], formatProvider);
                    [[/inline]];
            }
            else
            {
                // We don't know how to handle non-McgResource types which don't specify a ParseMethod.
                Debug.Assert(resource != null);

                parseBody =
                    [[inline]]
                        value = new [[resource.Name]](
                            [[Helpers.CodeGenHelpers.WriteFieldStatementsFirstLastWithSeparator(
                                resource.LocalFields,
                                "{parseMethod}(" + firstToken + ", formatProvider)",
                                "{parseMethod}(th.NextTokenRequired(), formatProvider)",
                                "{parseMethod}(th.NextTokenRequired(), formatProvider)",
                                ",\n")]]);
                    [[/inline]];

                // If this resource exposes a special empty state we need to wrap
                // the parseBody in a special clause to handle this string.
                if (resource.EmptyField != null)
                {
                    parseBody =
                        [[inline]]
                            // The token will already have had whitespace trimmed so we can do a
                            // simple string compare.
                            if ([[firstToken]] == "[[resource.EmptyField.Name]]")
                            {
                                value = [[resource.EmptyField.Name]];
                            }
                            else
                            {
                                [[parseBody]]
                            }
                       [[/inline]];
                }
            }

            return parseBody;
        }

        private void WriteParse(McgResource resource,
                                StringCodeSink cs)
        {
            // At this time, we only know how to emit Parse for value types
            if (!resource.IsValueType) return;
            if (resource.SkipToString) return;

            cs.Write(
                [[inline]]
                    /// <summary>
                    /// Parse - returns an instance converted from the provided string using
                    /// the culture "en-US"
                    /// <param name="source"> string with [[resource.Name]] data </param>
                    /// </summary>
                    public static [[resource.Name]] Parse(string source)
                    {
                        IFormatProvider formatProvider = System.Windows.Markup.TypeConverterHelper.InvariantEnglishUS;

                        TokenizerHelper th = new TokenizerHelper(source, formatProvider);

                        [[resource.Name]] value;

                        String firstToken = th.NextTokenRequired();

                        [[WriteParseBody(resource, "firstToken")]]

                        // There should be no more tokens in this string.
                        th.LastTokenRequired();

                        return value;
                    }
                [[/inline]]
                );
        }


        private string WriteObjectMethods(McgResource resource)
        {
            StringCodeSink cs = new StringCodeSink();

            WriteValueMethods(resource, cs);

            WriteEqualsObject(resource, cs);
            WriteGetHashCode(resource, cs);
            WriteParse(resource, cs);

            return cs.ToString();
        }

        private void WriteValueMethods(McgResource resource, StringCodeSink cs)
        {
            if (!_resourceModel.ShouldGenerate(CodeSections.ManagedValueMethods, resource))
                return;

            string equalsBody = String.Empty;
            StringCodeSink returnStmt = new StringCodeSink();

            string lowerName = GeneratorMethods.FirstLower(resource.Name);

            StripDimensionSuffix(ref lowerName);

            // The inline block intentionally lacks a newline so that the first field
            // will be on the same line as the return.
            returnStmt.Write(
                [[inline]]
                    return [[/inline]]);

            returnStmt.Write(Helpers.CodeGenHelpers.WriteFieldStatementsWithSeparator(
                                 resource.LocalFields,
                                 lowerName + "1.{fieldName} == " + lowerName + "2.{fieldName}",
                                 " &&\n       "                // The spaces trailing the newline indent the fields under the return.
                                 ));

            // Terminate the last field with a semi-colon.
            returnStmt.Write(
                [[inline]]
                    ;
                [[/inline]]
                );

            equalsBody = returnStmt.ToString();

            if (resource.HasDistinguishedEmpty)
            {
                equalsBody =
                    [[inline]]
                        if ([[lowerName]]1.IsDistinguishedIdentity || [[lowerName]]2.IsDistinguishedIdentity)
                        {
                            return [[lowerName]]1.Is[[resource.EmptyField.Name]] == [[lowerName]]2.Is[[resource.EmptyField.Name]];
                        }
                        else
                        {
                            [[equalsBody]]
                        }
                    [[/inline]];
            }

            cs.Write(
                [[inline]]
                    /// <summary>
                    /// Compares two [[resource.Name]] instances for exact equality.
                    /// Note that double values can acquire error when operated upon, such that
                    /// an exact comparison between two values which are logically equal may fail.
                    /// Furthermore, using this equality operator, Double.NaN is not equal to itself.
                    /// </summary>
                    /// <returns>
                    /// bool - true if the two [[resource.Name]] instances are exactly equal, false otherwise
                    /// </returns>
                    /// <param name='[[lowerName]]1'>The first [[resource.Name]] to compare</param>
                    /// <param name='[[lowerName]]2'>The second [[resource.Name]] to compare</param>
                    public static bool operator == ([[resource.Name]] [[lowerName]]1, [[resource.Name]] [[lowerName]]2)
                    {
                        [[equalsBody]]
                    }

                    /// <summary>
                    /// Compares two [[resource.Name]] instances for exact inequality.
                    /// Note that double values can acquire error when operated upon, such that
                    /// an exact comparison between two values which are logically equal may fail.
                    /// Furthermore, using this equality operator, Double.NaN is not equal to itself.
                    /// </summary>
                    /// <returns>
                    /// bool - true if the two [[resource.Name]] instances are exactly unequal, false otherwise
                    /// </returns>
                    /// <param name='[[lowerName]]1'>The first [[resource.Name]] to compare</param>
                    /// <param name='[[lowerName]]2'>The second [[resource.Name]] to compare</param>
                    public static bool operator != ([[resource.Name]] [[lowerName]]1, [[resource.Name]] [[lowerName]]2)
                    {
                        return !([[lowerName]]1 == [[lowerName]]2);
                    }
                [[/inline]]
            );
        }

        private void WriteTypeConverter(McgResource resource)
        {
            string contextParam = "";

            if (_resourceModel.ShouldGenerate(CodeSections.ManagedTypeConverterContext, resource))
            {
                contextParam = ", context";
            }

            if (_resourceModel.ShouldGenerate(CodeSections.ManagedTypeConverter, resource))
            {
                string converterName = resource.Name+"Converter";
                string valueSerializerName = resource.Name+"ValueSerializer";


                // ---------------------------
                // Write out the TypeConverter
                // ---------------------------

                using (FileCodeSink csFile = new FileCodeSink(
                    Path.Combine(_resourceModel.OutputDirectory, resource.ManagedDestinationDir),
                    converterName+".cs",
                    true /* Create dir if necessary */))
                {
                    string serializationContextCanConvertTo = String.Empty;
                    string serializationContextConvertTo = String.Empty;

                    csFile.WriteBlock(
                        [[inline]]
                            [[Helpers.ManagedStyle.WriteFileHeader(converterName+".cs")]]
                        [[/inline]]
                        );

                    foreach (string s in resource.Namespaces)
                    {
                        csFile.Write(
                            [[inline]]
                                using [[s]];
                            [[/inline]]
                                );
                    }

                    csFile.Write(
                        [[inline]]
                            #pragma warning disable 1634, 1691  // suppressing PreSharp warnings
                        [[/inline]]
                        );

                    // If the resource may not always be serializable as a string, we need to check if this instance can.
                    if (!resource.IsAlwaysSerializableAsString)
                    {
                        serializationContextCanConvertTo =
                            [[inline]]

                                // When invoked by the serialization engine we can convert to string only for some instances
                                if (context != null && context.Instance != null)
                                {
                                    if (!(context.Instance is [[resource.Name]]))
                                    {
                                        throw new ArgumentException(SR.Get(SRID.General_Expected_Type, "[[resource.Name]]"), "context");
                                    }

                                    [[resource.Name]] value = ([[resource.Name]])context.Instance;

                                    #pragma warning suppress 6506 // value is obviously not null
                                    return value.CanSerializeToString();
                                }


                            [[/inline]]
                                ;

                        serializationContextConvertTo =
                            [[inline]]

                                // When invoked by the serialization engine we can convert to string only for some instances
                                if (context != null && context.Instance != null)
                                {
                                    #pragma warning suppress 6506 // instance is obviously not null
                                    if (!instance.CanSerializeToString())
                                    {
                                        throw new NotSupportedException(SR.Get(SRID.Converter_ConvertToNotSupported));
                                    }
                                }


                            [[/inline]]
                                ;

                    }

                    csFile.WriteBlock(
                        [[inline]]

                            namespace [[resource.ManagedNamespace]]
                            {
                                /// <summary>
                                /// [[resource.Name]]Converter - Converter class for converting instances of other types to and from [[resource.Name]] instances
                                /// </summary>
                                public sealed class [[converterName]] : TypeConverter
                                {
                                    /// <summary>
                                    /// Returns true if this type converter can convert from a given type.
                                    /// </summary>
                                    /// <returns>
                                    /// bool - True if this converter can convert from the provided type, false if not.
                                    /// </returns>
                                    /// <param name="context"> The ITypeDescriptorContext for this call. </param>
                                    /// <param name="sourceType"> The Type being queried for support. </param>
                                    public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
                                    {
                                        if (sourceType == typeof(string))
                                        {
                                            return true;
                                        }

                                        return base.CanConvertFrom(context, sourceType);
                                    }

                                    /// <summary>
                                    /// Returns true if this type converter can convert to the given type.
                                    /// </summary>
                                    /// <returns>
                                    /// bool - True if this converter can convert to the provided type, false if not.
                                    /// </returns>
                                    /// <param name="context"> The ITypeDescriptorContext for this call. </param>
                                    /// <param name="destinationType"> The Type being queried for support. </param>
                                    public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
                                    {
                                        if (destinationType == typeof(string))
                                        {
                                            [[serializationContextCanConvertTo]]return true;
                                        }

                                        return base.CanConvertTo(context, destinationType);
                                    }

                                    /// <summary>
                                    /// Attempts to convert to a [[resource.Name]] from the given object.
                                    /// </summary>
                                    /// <returns>
                                    /// The [[resource.Name]] which was constructed.
                                    /// </returns>
                                    /// <exception cref="NotSupportedException">
                                    /// A NotSupportedException is thrown if the example object is null or is not a valid type
                                    /// which can be converted to a [[resource.Name]].
                                    /// </exception>
                                    /// <param name="context"> The ITypeDescriptorContext for this call. </param>
                                    /// <param name="culture"> The requested CultureInfo.  Note that conversion uses "en-US" rather than this parameter. </param>
                                    /// <param name="value"> The object to convert to an instance of [[resource.Name]]. </param>
                                    public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
                                    {
                                        if (value == null)
                                        {
                                            throw GetConvertFromException(value);
                                        }

                                        String source = value as string;

                                        if (source != null)
                                        {
                                            return [[resource.Name]].Parse(source[[contextParam]]);
                                        }

                                        return base.ConvertFrom(context, culture, value);
                                    }

                                    /// <summary>
                                    /// ConvertTo - Attempt to convert an instance of [[resource.Name]] to the given type
                                    /// </summary>
                                    /// <returns>
                                    /// The object which was constructoed.
                                    /// </returns>
                                    /// <exception cref="NotSupportedException">
                                    /// A NotSupportedException is thrown if "value" is null or not an instance of [[resource.Name]],
                                    /// or if the destinationType isn't one of the valid destination types.
                                    /// </exception>
                                    /// <param name="context"> The ITypeDescriptorContext for this call. </param>
                                    /// <param name="culture"> The CultureInfo which is respected when converting. </param>
                                    /// <param name="value"> The object to convert to an instance of "destinationType". </param>
                                    /// <param name="destinationType"> The type to which this will convert the [[resource.Name]] instance. </param>
                                    public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
                                    {
                                        if (destinationType != null && value is [[resource.Name]])
                                        {
                                            [[resource.Name]] instance = ([[resource.Name]])value;

                                            if (destinationType == typeof(string))
                                            {
                                                [[serializationContextConvertTo]]// Delegate to the formatting/culture-aware ConvertToString method.

                                                #pragma warning suppress 6506 // instance is obviously not null
                                                return instance.ConvertToString(null, culture);
                                            }
                                        }

                                        // Pass unhandled cases to base class (which will throw exceptions for null value or destinationType.)
                                        return base.ConvertTo(context, culture, value, destinationType);
                                    }
                                }

                            }
                        [[/inline]]
                            );
                }


                // -----------------------------
                // Write out the ValueSerializer
                // -----------------------------

                using (FileCodeSink csFile = new FileCodeSink(
                    Path.Combine(_resourceModel.OutputDirectory, resource.ConverterDestinationDir ),
                    valueSerializerName+".cs",
                    true /* Create dir if necessary */))
                {
                    string valueSerializerCanConvertTo;
                    string valueSerializerConvertTo = String.Empty;

                    string valueSerializerNamespace = resource.ManagedNamespace + ".Converters";

                    csFile.WriteBlock(
                        [[inline]]
                            [[Helpers.ManagedStyle.WriteFileHeader(valueSerializerName+".cs")]]
                        [[/inline]]
                        );

                    foreach (string s in resource.Namespaces)
                    {
                        csFile.Write(
                            [[inline]]
                                using [[s]];
                            [[/inline]]
                                );
                    }

                    csFile.Write(
                        [[inline]]
                            #pragma warning disable 1634, 1691  // suppressing PreSharp warnings
                        [[/inline]]
                        );

                    // If always serializable to string, ensure that it's still the right type.
                    if (resource.IsAlwaysSerializableAsString)
                    {
                        valueSerializerCanConvertTo =
                            [[inline]]

                                // Validate the input type
                                if (!(value is [[resource.Name]]))
                                {
                                    return false;
                                }

                                return true;

                            [[/inline]]
                                ;
                    }

                    // If the resource may not always be serializable as a string, we need to check if this instance can.
                    else
                    {
                        valueSerializerCanConvertTo =
                            [[inline]]

                                // When invoked by the serialization engine we can convert to string only for some instances
                                if (!(value is [[resource.Name]]))
                                {
                                    return false;
                                }

                                [[resource.Name]] instance  = ([[resource.Name]]) value;

                                #pragma warning suppress 6506 // instance is obviously not null
                                return instance.CanSerializeToString();


                            [[/inline]]
                                ;

                        valueSerializerConvertTo =
                            [[inline]]
                                // When invoked by the serialization engine we can convert to string only for some instances
                                #pragma warning suppress 6506 // instance is obviously not null
                                if (!instance.CanSerializeToString())
                                {
                                    // Let base throw an exception.
                                    return base.ConvertToString(value, context);
                                }

                            [[/inline]]
                                ;


                    }

                    csFile.WriteBlock(
                        [[inline]]

                            namespace [[valueSerializerNamespace]]
                            {
                                /// <summary>
                                /// [[resource.Name]]ValueSerializer - ValueSerializer class for converting instances of strings to and from [[resource.Name]] instances
                                /// This is used by the MarkupWriter class.
                                /// </summary>
                                public class [[valueSerializerName]] : ValueSerializer 
                                {
                                    /// <summary>
                                    /// Returns true.
                                    /// </summary>
                                    public override bool CanConvertFromString(string value, IValueSerializerContext context)
                                    {
                                        return true;
                                    }

                                    /// <summary>
                                    /// Returns true if the given value can be converted into a string
                                    /// </summary>
                                    public override bool CanConvertToString(object value, IValueSerializerContext context)
                                    {
                                        [[valueSerializerCanConvertTo]]
                                    }

                                    /// <summary>
                                    /// Converts a string into a [[resource.Name]].
                                    /// </summary>
                                    public override object ConvertFromString(string value, IValueSerializerContext context)
                                    {
                                        if (value != null)
                                        {
                                            return [[resource.Name]].Parse(value[[contextParam]] );
                                        }
                                        else
                                        {
                                            return base.ConvertFromString( value, context );
                                        }

                                    }

                                    /// <summary>
                                    /// Converts the value into a string.
                                    /// </summary>
                                    public override string ConvertToString(object value, IValueSerializerContext context)
                                    {
                                        if (value is [[resource.Name]])
                                        {
                                            [[resource.Name]] instance = ([[resource.Name]]) value;
                                            [[valueSerializerConvertTo]]

                                            #pragma warning suppress 6506 // instance is obviously not null
                                            return instance.ConvertToString(null, System.Windows.Markup.TypeConverterHelper.InvariantEnglishUS);
                                        }

                                        return base.ConvertToString(value, context);
                                    }
                                }




                            }
                        [[/inline]]
                            );
                }





           }
        }

        // Turns "foo[34]D --> foo"
        private void StripDimensionSuffix(ref string name)
        {
            if (name.EndsWith("3D") || name.EndsWith("4D"))
            {
                name = name.Remove(name.Length - 2, 2);
                Debug.Assert(name.Length != 0);
            }
        }
    }
}



