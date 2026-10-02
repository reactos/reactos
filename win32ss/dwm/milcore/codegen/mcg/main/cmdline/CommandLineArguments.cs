// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


namespace Utilities
{
    using System;
    using System.Diagnostics;
    using System.Reflection;
    using System.Collections;
    using System.IO;
    using System.Text;

    /// <summary>
    /// Used to control parsing of command line arguments.
    /// </summary>
    [Flags]    
    public enum CommandLineArgumentType
    {
        /// <summary>
        /// Indicates that this field is required. An error will be displayed
        /// if it is not present when parsing arguments.
        /// </summary>
        Required    = 0x01,
        /// <summary>
        /// Only valid in conjunction with Multiple.
        /// Duplicate values will result in an error.
        /// </summary>
        Unique      = 0x02,
        /// <summary>
        /// Inidicates that the argument may be specified more than once.
        /// Only valid if the argument is a collection
        /// </summary>
        Multiple    = 0x04,

        /// <summary>
        /// The default type for non-collection arguments.
        /// The argument is not required, but an error will be reported if it is specified more than once.
        /// </summary>
        AtMostOnce  = 0x00,
        
        /// <summary>
        /// For non-collection arguments, when the argument is specified more than
        /// once no error is reported and the value of the argument is the last
        /// value which occurs in the argument list.
        /// </summary>
        LastOccurenceWins = Multiple,

        /// <summary>
        /// The default type for collection arguments.
        /// The argument is permitted to occur multiple times, but duplicate 
        /// values will cause an error to be reported.
        /// </summary>
        MultipleUnique  = Multiple | Unique,
    }
    
    /// <summary>
    /// Allows control of command line parsing.
    /// Attach this attribute to instance fields of types used
    /// as the destination of command line argument parsing.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field)]
    public class CommandLineArgumentAttribute : Attribute
    {
        /// <summary>
        /// Allows control of command line parsing.
        /// </summary>
        /// <param name="type"> Specifies the error checking to be done on the argument. </param>
        public CommandLineArgumentAttribute(CommandLineArgumentType type)
        {
            this.type = type;
        }
        
        /// <summary>
        /// The error checking to be done on the argument.
        /// </summary>
        public CommandLineArgumentType Type
        {
            get { return this.type; }
        }
        /// <summary>
        /// Returns true if the argument did not have an explicit short name specified.
        /// </summary>
        public bool DefaultShortName    { get { return null == this.shortName; } }
        
        /// <summary>
        /// The short name of the argument.
        /// </summary>
        public string ShortName
        {
            get { return this.shortName; }
            set { this.shortName = value; }
        }

        /// <summary>
        /// Returns true if the argument did not have an explicit long name specified.
        /// </summary>
        public bool DefaultLongName     { get { return null == this.longName; } }
        
        /// <summary>
        /// The long name of the argument.
        /// </summary>
        public string LongName
        {
            get { Debug.Assert(!this.DefaultLongName); return this.longName; }
            set { this.longName = value; }
        }
        
        private string shortName;
        private string longName;
        private CommandLineArgumentType type;
    }

    /// <summary>
    /// Indicates that this argument is the default argument.
    /// '/' or '-' prefix only the argument value is specified.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field)]
    public class DefaultCommandLineArgumentAttribute : CommandLineArgumentAttribute
    {
        /// <summary>
        /// Indicates that this argument is the default argument.
        /// </summary>
        /// <param name="type"> Specifies the error checking to be done on the argument. </param>
        public DefaultCommandLineArgumentAttribute(CommandLineArgumentType type)
            : base (type)
        {
        }
    }

    /// <summary>
    /// Parser for command line arguments.
    ///
    /// The parser specification is infered from the instance fields of the object
    /// specified as the destination of the parse.
    /// Valid argument types are: int, uint, string, bool, enums
    /// Also argument types of Array of the above types are also valid.
    /// 
    /// Error checking options can be controlled by adding a CommandLineArgumentAttribute
    /// to the instance fields of the destination object.
    ///
    /// At most one field may be marked with the DefaultCommandLineArgumentAttribute
    /// indicating that arguments without a '-' or '/' prefix will be parsed as that argument.
    ///
    /// If not specified then the parser will infer default options for parsing each
    /// instance field. The default long name of the argument is the field name. The
    /// default short name is the first character of the long name. Long names and explicitly
    /// specified short names must be unique. Default short names will be used provided that
    /// the default short name does not conflict with a long name or an explicitly
    /// specified short name.
    ///
    /// Arguments which are array types are collection arguments. Collection
    /// arguments can be specified multiple times.
    /// </summary>
    public class CommandLineArgumentParser
    {
        /// <summary>
        /// Creates a new command line argument parser.
        /// </summary>
        /// <param name="argumentSpecification"> The type of object to  parse. </param>
        /// <param name="reporter"> The destination for parse errors. </param>
        public CommandLineArgumentParser(Type argumentSpecification, ErrorReporter reporter)
        {
            this.reporter = reporter;
            this.arguments = new ArrayList();
            this.argumentMap = new Hashtable();
            
            foreach (FieldInfo field in argumentSpecification.GetFields())
            {
                if (!field.IsStatic && !field.IsInitOnly && !field.IsLiteral)
                {
                    CommandLineArgumentAttribute attribute = GetAttribute(field);
                    if (attribute is DefaultCommandLineArgumentAttribute)
                    {
                        Debug.Assert(this.defaultArgument == null);
                        this.defaultArgument = new Argument(attribute, field, reporter);
                    }
                    else
                    {
                        this.arguments.Add(new Argument(attribute, field, reporter));
                    }
                }
            }
            
            // add explicit names to map
            foreach (Argument argument in this.arguments)
            {
                Debug.Assert(!argumentMap.ContainsKey(argument.LongName));
                this.argumentMap[argument.LongName] = argument;
                if (argument.ExplicitShortName && argument.ShortName != null && argument.ShortName.Length > 0)
                {
                    Debug.Assert(!argumentMap.ContainsKey(argument.ShortName));
                    this.argumentMap[argument.ShortName] = argument;
                }
            }
            
            // add implicit names which don't collide to map
            foreach (Argument argument in this.arguments)
            {
                if (!argument.ExplicitShortName && argument.ShortName != null && argument.ShortName.Length > 0)
                {
                    if (!argumentMap.ContainsKey(argument.ShortName))
                        this.argumentMap[argument.ShortName] = argument;
                }
            }
        }
        
        private static CommandLineArgumentAttribute GetAttribute(FieldInfo field)
        {
            object[] attributes = field.GetCustomAttributes(typeof(CommandLineArgumentAttribute), false);
            if (attributes.Length == 1)
                return (CommandLineArgumentAttribute) attributes[0];

            Debug.Assert(attributes.Length == 0);
            return null;
        }
        
        private void ReportUnrecognizedArgument(string argument)
        {
            this.reporter(string.Format("Unrecognized command line argument '{0}'", argument));
        }
        
        /// <summary>
        /// Parses an argument list into an object
        /// </summary>
        /// <param name="args"></param>
        /// <param name="destination"></param>
        /// <returns> true if an error occurred </returns>
        private bool ParseArgumentList(string[] args, object destination)
        {
            bool hadError = false;
            if (args != null)
            {
                foreach (string argument in args)
                {
                    if (argument.Length > 0)
                    {
                        switch (argument[0])
                        {
                        case '-':
                        case '/':
                            int endIndex = argument.IndexOfAny(new char[] {':', '+', '-'}, 1);
                            string option = argument.Substring(1, endIndex == -1 ? argument.Length - 1 : endIndex - 1);
                            string optionArgument;
                            if (endIndex == -1)
                            {
                                optionArgument = null;
                            }
                            else if (argument.Length > 1 + option.Length && argument[1 + option.Length] == ':')
                            {
                                optionArgument = argument.Substring(option.Length + 2);
                            }
                            else
                            {
                                optionArgument = argument.Substring(option.Length + 1);
                            }
                                
                            Argument arg = (Argument) this.argumentMap[option];
                            if (arg == null)
                            {
                                ReportUnrecognizedArgument(argument);
                                hadError = true;
                            }
                            else
                            {
                                hadError |= !arg.SetValue(optionArgument, destination);
                            }
                            break;
                        case '@':
                            string[] nestedArguments;
                            hadError |= LexFileArguments(argument.Substring(1), out nestedArguments);
                            hadError |= ParseArgumentList(nestedArguments, destination);
                            break;
                        default:
                            if (this.defaultArgument != null)
                            {
                                hadError |= !this.defaultArgument.SetValue(argument, destination);
                            }
                            else
                            {
                                ReportUnrecognizedArgument(argument);
                                hadError = true;
                            }
                            break;
                        }
                    }
                }
            }
            
            return hadError;
        }
        
        /// <summary>
        /// Parses an argument list.
        /// </summary>
        /// <param name="args"> The arguments to parse. </param>
        /// <param name="destination"> The destination of the parsed arguments. </param>
        /// <returns> true if no parse errors were encountered. </returns>
        public bool Parse(string[] args, object destination)
        {
            bool hadError = ParseArgumentList(args, destination);

            // check for missing required arguments
            foreach (Argument arg in this.arguments)
            {
                hadError |= arg.Finish(destination);
            }
            if (this.defaultArgument != null)
            {
                hadError |= this.defaultArgument.Finish(destination);
            }
            
            return !hadError;
        }
        
        
        /// <summary>
        /// A user firendly usage string describing the command line argument syntax.
        /// </summary>
        public string Usage
        {
            get
            {
                StringBuilder builder = new StringBuilder();
                
                int oldLength;
                foreach (Argument arg in this.arguments)
                {
                    oldLength = builder.Length;
                    
                    builder.Append("    /");
                    builder.Append(arg.LongName);
                    Type valueType = arg.ValueType;
                    if (valueType == typeof(int))
                    {
                        builder.Append(":<int>");
                    }
                    else if (valueType == typeof(uint))
                    {
                        builder.Append(":<uint>");
                    }
                    else if (valueType == typeof(bool))
                    {
                        builder.Append("[+|-]");
                    }
                    else if (valueType == typeof(string))
                    {
                        builder.Append(":<string>");
                    }
                    else
                    {
                        Debug.Assert(valueType.IsEnum);
                        
                        builder.Append(":{");
                        bool first = true;
                        foreach (FieldInfo field in valueType.GetFields())
                        {
                            if (field.IsStatic)
                            {
                                if (first)
                                    first = false;
                                else
                                    builder.Append('|');
                                builder.Append(field.Name);
                            }
                        }
                        builder.Append('}');
                    }
                    
                    if (arg.ShortName != arg.LongName && this.argumentMap[arg.ShortName] == arg)
                    {
                        builder.Append(' ', IndentLength(builder.Length - oldLength));
                        builder.Append("short form /");
                        builder.Append(arg.ShortName);
                    }
                    
                    builder.Append(Utility.NewLine);
                }
                
                oldLength = builder.Length;
                builder.Append("    @<file>");
                builder.Append(' ', IndentLength(builder.Length - oldLength));
                builder.Append("Read response file for more options");
                builder.Append(Utility.NewLine);
                
                if (this.defaultArgument != null)
                {
                    oldLength = builder.Length;
                    builder.Append("    <");
                    builder.Append(this.defaultArgument.LongName);
                    builder.Append(">");
                    builder.Append(Utility.NewLine);
                }
                
                return builder.ToString();
            }
        }
            
        private static int IndentLength(int lineLength)
        {
            return Math.Max(4, 40 - lineLength);
        }
        
        private bool LexFileArguments(string fileName, out string[] arguments)
        {
            string args  = null;
                    
            try
            {
                using (FileStream file = new FileStream(fileName, FileMode.Open, FileAccess.Read))
                {
                    args = (new StreamReader(file)).ReadToEnd();
                }
            }
            catch (Exception e)
            {
                this.reporter(string.Format("Error: Can't open command line argument file '{0}' : '{1}'", fileName, e.Message));
                arguments = null;
                return false;
            }

            bool hadError = false;                    
            ArrayList argArray = new ArrayList();
            StringBuilder currentArg = new StringBuilder();
            bool inQuotes = false;
            int index = 0;
            
            // while (index < args.Length)
            try
            {
                while (true)
                {
                    // skip whitespace
                    while (char.IsWhiteSpace(args[index]))
                    {
                        index += 1;
                    }
                    
                    // # - comment to end of line
                    if (args[index] == '#')
                    {
                        index += 1;
                        while (args[index] != '\n')
                        {
                            index += 1;
                        }
                        continue;
                    }
                    
                    // do one argument
                    do
                    {
                        if (args[index] == '\\')
                        {
                            int cSlashes = 1;
                            index += 1;
                            while (index == args.Length && args[index] == '\\')
                            {
                                cSlashes += 1;
                            }

                            if (index == args.Length || args[index] != '"')
                            {
                                currentArg.Append('\\', cSlashes);
                            }
                            else
                            {
                                currentArg.Append('\\', (cSlashes >> 1));
                                if (0 != (cSlashes & 1))
                                {
                                    currentArg.Append('"');
                                }
                                else
                                {
                                    inQuotes = !inQuotes;
                                }
                            }
                        }
                        else if (args[index] == '"')
                        {
                            inQuotes = !inQuotes;
                            index += 1;
                        }
                        else
                        {
                            currentArg.Append(args[index]);
                            index += 1;
                        }
                    } while (!char.IsWhiteSpace(args[index]) || inQuotes);
                    argArray.Add(currentArg.ToString());
                    currentArg.Length = 0;
                }
            }
            catch (System.IndexOutOfRangeException)
            {
                // got EOF 
                if (inQuotes)
                {
                    this.reporter(string.Format("Error: Unbalanced '\"' in command line argument file '{0}'", fileName));
                    hadError = true;
                }
                else if (currentArg.Length > 0)
                {
                    // valid argument can be terminated by EOF
                    argArray.Add(currentArg.ToString());
                }
            }
            
            arguments = (string[]) argArray.ToArray(typeof (string));
            return hadError;
        }
        
        private static string LongName(CommandLineArgumentAttribute attribute, FieldInfo field)
        {
            return (attribute == null || attribute.DefaultLongName) ? field.Name : attribute.LongName;
        }
        
        private static string ShortName(CommandLineArgumentAttribute attribute, FieldInfo field)
        {
            return !ExplicitShortName(attribute) ? LongName(attribute, field).Substring(0,1) : attribute.ShortName;
        }
        
        private static bool ExplicitShortName(CommandLineArgumentAttribute attribute)
        {
            return (attribute != null && !attribute.DefaultShortName);
        }
        
        private static Type ElementType(FieldInfo field)
        {
            if (IsCollectionType(field.FieldType))
                return field.FieldType.GetElementType();
            else
                return null;
        }
        
        private static CommandLineArgumentType Flags(CommandLineArgumentAttribute attribute, FieldInfo field)
        {
            if (attribute != null)
                return attribute.Type;
            else if (IsCollectionType(field.FieldType))
                return CommandLineArgumentType.MultipleUnique;
            else
                return CommandLineArgumentType.AtMostOnce;
        }
        
        private static bool IsCollectionType(Type type)
        {
            return type.IsArray;
        }
            
        private static bool IsValidElementType(Type type)
        {
            return type != null && (
                type == typeof(int) ||
                type == typeof(uint) ||
                type == typeof(string) ||
                type == typeof(bool) ||
                type.IsEnum);
        }
        
        private class Argument
        {
            public Argument(CommandLineArgumentAttribute attribute, FieldInfo field, ErrorReporter reporter)
            {
                this.longName = CommandLineArgumentParser.LongName(attribute, field);
                this.explicitShortName = CommandLineArgumentParser.ExplicitShortName(attribute);
                this.shortName = CommandLineArgumentParser.ShortName(attribute, field);
                this.elementType = ElementType(field);
                this.flags = Flags(attribute, field);
                this.field = field;
                this.seenValue = false;
                this.reporter = reporter;
                this.isDefault = attribute != null && attribute is DefaultCommandLineArgumentAttribute;
                
                if (IsCollection)
                {
                    this.collectionValues = new ArrayList();
                }
                
                Debug.Assert(this.longName != null && this.longName.Length > 0);
                Debug.Assert(!IsCollection || AllowMultiple, "Collection arguments must have allow multiple");
                Debug.Assert(!Unique || IsCollection, "Unique only applicable to collection arguments");
                Debug.Assert(IsValidElementType(Type) ||
                             IsCollectionType(Type));
                Debug.Assert((IsCollection && IsValidElementType(elementType)) ||
                             (!IsCollection && elementType == null));
            }
            
            public bool Finish(object destination)
            {
                if (this.IsCollection)
                {
                    this.field.SetValue(destination, this.collectionValues.ToArray(this.elementType));
                }
                
                return ReportMissingRequiredArgument();
            }
            
            private bool ReportMissingRequiredArgument()
            {
                if (this.IsRequired && !this.SeenValue)
                {
                    if (this.IsDefault)
                        reporter(string.Format("Missing required argument '<{0}>'.", this.LongName));
                    else
                        reporter(string.Format("Missing required argument '/{0}'.", this.LongName));
                    return true;
                }
                return false;
            }
            
            private void ReportDuplicateArgumentValue(string value)
            {
                this.reporter(string.Format("Duplicate '{0}' argument '{1}'", this.LongName, value));
            }
            
            public bool SetValue(string value, object destination)
            {
                if (SeenValue && !AllowMultiple)
                {
                    this.reporter(string.Format("Duplicate '{0}' argument", this.LongName));
                    return false;
                }
                this.seenValue = true;
                
                object newValue;
                if (!ParseValue(this.ValueType, value, out newValue))
                    return false;
                if (this.IsCollection)
                {
                    if (this.Unique && this.collectionValues.Contains(newValue))
                    {
                        ReportDuplicateArgumentValue(value);
                        return false;
                    }
                    else
                    {
                        this.collectionValues.Add(newValue);
                    }
                }
                else
                {
                    this.field.SetValue(destination, newValue);
                }
                
                return true;
            }
            
            public Type ValueType
            {
                get { return this.IsCollection ? this.elementType : this.Type; }
            }
            
            private void ReportBadArgumentValue(string value)
            {
                this.reporter(string.Format("'{0}' is not a valid value for the '{1}' command line option", value, this.LongName));
            }
            
            private bool ParseValue(Type type, string stringData, out object value)
            {
                // null is only valid for bool variables
                // empty string is never valid
                if ((stringData != null || type == typeof(bool)) && (stringData == null || stringData.Length > 0))
                {
                    try
                    {
                        if (type == typeof(string))
                        {
                            value = stringData;
                            return true;
                        }
                        else if (type == typeof(bool))
                        {
                            if (stringData == null || stringData == "+")
                            {
                                value = true;
                                return true;
                            }
                            else if (stringData == "-")
                            {
                                value = false;
                                return true;
                            }
                        }
                        else if (type == typeof(int))
                        {
                            value = int.Parse(stringData);
                            return true;
                        }
                        else if (type == typeof(uint))
                        {
                            value = int.Parse(stringData);
                            return true;
                        }
                        else
                        {
                            Debug.Assert(type.IsEnum);
                            value = Enum.Parse(type, stringData, true);
                            return true;
                        }
                    }
                    catch
                    {
                        // catch parse errors
                    }
                }
                                
                ReportBadArgumentValue(stringData);
                value = null;
                return false;
            }
            
            public string LongName
            {
                get { return this.longName; }
            }

            public bool ExplicitShortName
            {
                get { return this.explicitShortName; }
            }
            
            public string ShortName
            {
                get { return this.shortName; }
            }

            public bool IsRequired
            {
                get { return 0 != (this.flags & CommandLineArgumentType.Required); }
            }
            
            public bool SeenValue
            {
                get { return this.seenValue; }
            }
            
            public bool AllowMultiple
            {
                get { return 0 != (this.flags & CommandLineArgumentType.Multiple); }
            }
            
            public bool Unique
            {
                get { return 0 != (this.flags & CommandLineArgumentType.Unique); }
            }
            
            public Type Type
            {
                get { return field.FieldType; }
            }
            
            public bool IsCollection
            {
                get { return IsCollectionType(Type); }
            }
            
            public bool IsDefault
            {
                get { return this.isDefault; }
            }
            
            private string longName;
            private string shortName;
            private bool explicitShortName;
            private bool seenValue;
            private FieldInfo field;
            private Type elementType;
            private CommandLineArgumentType flags;
            private ArrayList collectionValues;
            private ErrorReporter reporter;
            private bool isDefault;
        }
        
        private ArrayList arguments;
        private Hashtable argumentMap;
        private Argument defaultArgument;
        private ErrorReporter reporter;
    }
}


