/*
 * Created by SharpDevelop.
 * User: lextm
 * Date: 2008/5/17
 * Time: 17:14
 * 
 * To change this template use Tools | Options | Coding | Edit Standard Headers.
 */

using System;
using System.Configuration;
using System.Globalization;
using System.Text;

namespace Lextm.SharpSnmpLib.Mib
{
    /// <summary>
    /// Description of Symbol.
    /// </summary>
    public sealed class Symbol : IEquatable<Symbol>
    {
        private readonly string _text;
        private readonly int _row;
        private readonly int _column;
        private readonly string _file;
        
        private Symbol(string text) : this(null, text, -1, -1)
        {
        }
        
        /// <summary>
        /// Creates a <see cref="Symbol"/>.
        /// </summary>
        /// <param name="file">File</param>
        /// <param name="text">Text</param>
        /// <param name="row">Row number</param>
        /// <param name="column">column number</param>
        public Symbol(string file, string text, int row, int column)
        {
            _file = file;
            _text = text;
            _row = row;
            _column = column;
        }
        
        /// <summary>
        /// File.
        /// </summary>
        public string File
        {
            get
            {
                return _file;
            }
        }
        
        /// <summary>
        /// Row number.
        /// </summary>
        public int Row
        {
            get
            {
                return _row;
            }
        }
        
        /// <summary>
        /// Column number.
        /// </summary>
        public int Column
        {
            get
            {
                return _column;
            }
        }        
        
        /// <summary>
        /// Returns a <see cref="String"/> that represents this <see cref="Symbol"/>.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return _text;
        }
        
        /// <summary>
        /// Determines whether the specified <see cref="Object"/> is equal to the current <see cref="Symbol"/>.
        /// </summary>
        /// <param name="obj">The <see cref="Object"/> to compare with the current <see cref="Symbol"/>. </param>
        /// <returns><value>true</value> if the specified <see cref="Object"/> is equal to the current <see cref="Symbol"/>; otherwise, <value>false</value>.
        /// </returns>
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }
            
            if (ReferenceEquals(this, obj))
            {
                return true;
            }
            
            return GetType() == obj.GetType() && Equals((Symbol)obj);
        }
        
        /// <summary>
        /// Serves as a hash function for a particular type.
        /// </summary>
        /// <returns>A hash code for the current <see cref="Symbol"/>.</returns>
        public override int GetHashCode()
        {
            return _text.GetHashCode();
        }

        /// <summary>
        /// The equality operator.
        /// </summary>
        /// <param name="left">Left <see cref="Symbol"/> object</param>
        /// <param name="right">Right <see cref="Symbol"/> object</param>
        /// <returns>
        /// Returns <c>true</c> if the values of its operands are equal, <c>false</c> otherwise.</returns>
        public static bool operator ==(Symbol left, Symbol right)
        {
            return Equals(left, right);
        }
        
        /// <summary>
        /// Determines whether the specified <see cref="Symbol"/> is equal to the current <see cref="Symbol"/>.
        /// </summary>
        /// <param name="left">Left <see cref="Symbol"/> object</param>
        /// <param name="right">Right <see cref="Symbol"/> object</param>
        /// <returns>
        /// Returns <c>true</c> if the values of its operands are equal, <c>false</c> otherwise.</returns>
        public static bool Equals(Symbol left, Symbol right)
        {
            object l = left;
            object r = right;
            if (l == r)
            {
                return true;
            }

            if (l == null || r == null)
            {
                return false;
            }

            return left._text.Equals(right._text);
        }
        
        /// <summary>
        /// The inequality operator.
        /// </summary>
        /// <param name="left">Left <see cref="Symbol"/> object</param>
        /// <param name="right">Right <see cref="Symbol"/> object</param>
        /// <returns>
        /// Returns <c>true</c> if the values of its operands are not equal, <c>false</c> otherwise.</returns>
        public static bool operator !=(Symbol left, Symbol right)
        {
            return !(left == right);
        }

        #region IEquatable<Symbol> Members
        /// <summary>
        /// Indicates whether the current object is equal to another object of the same type.
        /// </summary>
        /// <param name="other">An object to compare with this object.</param>
        /// <returns><value>true</value> if the current object is equal to the <paramref name="other"/> parameter; otherwise, <value>false</value>.
        /// </returns>
        public bool Equals(Symbol other)
        {
            return Equals(this, other);
        }

        #endregion
        
        public static readonly Symbol Definitions = new Symbol("DEFINITIONS");        
        public static readonly Symbol Begin = new Symbol("BEGIN");        
        public static readonly Symbol Object = new Symbol("OBJECT");        
        public static readonly Symbol Identifier = new Symbol("IDENTIFIER");
        public static readonly Symbol Assign = new Symbol("::=");
        public static readonly Symbol OpenBracket = new Symbol("{");
        public static readonly Symbol CloseBracket = new Symbol("}");
        public static readonly Symbol Comment = new Symbol("--");
        public static readonly Symbol Imports = new Symbol("IMPORTS");
        public static readonly Symbol Semicolon = new Symbol(";");
        public static readonly Symbol From = new Symbol("FROM");
        public static readonly Symbol ModuleIdentity = new Symbol("MODULE-IDENTITY");
        public static readonly Symbol ObjectType = new Symbol("OBJECT-TYPE");
        public static readonly Symbol ObjectGroup = new Symbol("OBJECT-GROUP");
        public static readonly Symbol NotificationGroup = new Symbol("NOTIFICATION-GROUP");
        public static readonly Symbol ModuleCompliance = new Symbol("MODULE-COMPLIANCE");
        public static readonly Symbol Sequence = new Symbol("SEQUENCE");
        public static readonly Symbol NotificationType = new Symbol("NOTIFICATION-TYPE");
        public static readonly Symbol EOL = new Symbol(Environment.NewLine);
        public static readonly Symbol ObjectIdentity = new Symbol("OBJECT-IDENTITY");
        public static readonly Symbol End = new Symbol("END");
        public static readonly Symbol Macro = new Symbol("MACRO");
        public static readonly Symbol Choice = new Symbol("CHOICE");
        public static readonly Symbol TrapType = new Symbol("TRAP-TYPE");
        public static readonly Symbol AgentCapabilities = new Symbol("AGENT-CAPABILITIES");
        public static readonly Symbol Comma = new Symbol(",");
        public static readonly Symbol TextualConvention = new Symbol("TEXTUAL-CONVENTION");
        public static readonly Symbol Syntax = new Symbol("SYNTAX");
        public static readonly Symbol Bits = new Symbol("BITS");
        public static readonly Symbol Octet = new Symbol("OCTET");
        public static readonly Symbol String = new Symbol("STRING");
        public static readonly Symbol OpenParentheses = new Symbol("(");
        public static readonly Symbol CloseParentheses = new Symbol(")");
        public static readonly Symbol Exports = new Symbol("EXPORTS");
        public static readonly Symbol DisplayHint = new Symbol("DISPLAY-HINT");
        public static readonly Symbol Status = new Symbol("STATUS");
        public static readonly Symbol Description = new Symbol("DESCRIPTION");
        public static readonly Symbol Reference = new Symbol("REFERENCE");
        public static readonly Symbol DoubleDot = new Symbol("..");
        public static readonly Symbol Pipe = new Symbol("|");
        public static readonly Symbol Size = new Symbol("SIZE");
        public static readonly Symbol Units = new Symbol("UNITS");
        public static readonly Symbol MaxAccess = new Symbol("MAX-ACCESS");
        public static readonly Symbol Access = new Symbol("ACCESS");
        public static readonly Symbol Index = new Symbol("INDEX");
        public static readonly Symbol Augments = new Symbol("AUGMENTS");
        public static readonly Symbol DefVal = new Symbol("DEFVAL");
        public static readonly Symbol Of = new Symbol("OF");
        public static readonly Symbol Integer = new Symbol("INTEGER");
        public static readonly Symbol Integer32 = new Symbol("Integer32");
        public static readonly Symbol IpAddress = new Symbol("IpAddress");
        public static readonly Symbol Counter32 = new Symbol("Counter32");
        public static readonly Symbol Counter = new Symbol("Counter");
        public static readonly Symbol TimeTicks = new Symbol("TimeTicks");
        public static readonly Symbol Opaque = new Symbol("Opaque");
        public static readonly Symbol Counter64 = new Symbol("Counter64");
        public static readonly Symbol Unsigned32 = new Symbol("Unsigned32");
        public static readonly Symbol Gauge32 = new Symbol("Gauge32");
        public static readonly Symbol Gauge = new Symbol("Gauge");
        public static readonly Symbol TruthValue = new Symbol("TruthValue");
        public static readonly Symbol Implied = new Symbol("IMPLIED");

        internal void Expect(Symbol expected, params Symbol[] orExpected)
        {
            bool isExpected = (this == expected);

            if (!isExpected && (orExpected != null) && (orExpected.Length > 0))
            {
                // check the alternatives
                for (int i=0; i<orExpected.Length; i++)
                {
                    if (this == orExpected[i])
                    {
                        isExpected = true;
                        break;
                    }
                }              
            }

            if (!isExpected)
            {
                if ((orExpected == null) || (orExpected.Length == 0))
                {
                    Assert(false, "Unexpected symbol found! Expected '" + expected.ToString() + "'");
                }
                else
                {
                    StringBuilder msg = new StringBuilder("Unexpected symbol found! Expected one of the following: '");
                    msg.Append(expected);

                    // check the alternatives
                    for (int i=0; i<orExpected.Length; i++)
                    {
                        msg.Append("', '");
                        msg.Append(expected);
                    }
                    msg.Append("'");

                    Assert(false, msg.ToString());
                }
            }
        }

        internal void Assert(bool condition, string message)
        {
            if (!condition)
            {
                throw MibException.Create(message, this);
            }
        }

        internal void AssertIsValidIdentifier()
        {
            string message;
            bool isValid = IsValidIdentifier(ToString(), out message);
            Assert(isValid, message);
        }

        internal bool IsValidIdentifier()
        {      
            string message;
            return IsValidIdentifier(ToString(), out message);
        }

        private static bool IsValidIdentifier(string name, out string message)
        {
            if (UseStricterValidation && (name.Length < 1 || name.Length > 64))
            {
                message = "an identifier must consist of 1 to 64 letters, digits, and hyphens";
                return false;
            }

            if (!Char.IsLetter(name[0]))
            {
                message = "the initial character must be a letter";
                return false;
            }

            if (name.EndsWith("-", StringComparison.Ordinal))
            {
                message = "a hyphen cannot be the last character of an identifier";
                return false;
            }

            if (name.Contains("--"))
            {
                message = "a hyphen cannot be immediately followed by another hyphen in an identifier";
                return false;
            }

            if (UseStricterValidation && name.Contains("_"))
            {
                message = "underscores are not allowed in identifiers";
                return false;
            }

            // TODO: SMIv2 forbids "-" except in module names and keywords
            message = null;
            return true;
        }

        private static bool? _useStricterValidation;

        private static bool UseStricterValidation
        {
            get
            {
                if (_useStricterValidation == null)
                {
                    object setting = ConfigurationManager.AppSettings["StricterValidationEnabled"];
                    _useStricterValidation = setting != null && Convert.ToBoolean(setting.ToString(), CultureInfo.InvariantCulture);
                }

                return _useStricterValidation.Value;
            }
        }
    }
}