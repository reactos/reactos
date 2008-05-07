using System;
using System.Text.RegularExpressions;

//Code taken from : http://www.codeproject.com/KB/recipes/commandlineparser.aspx

namespace TechBot.Library
{
	/// <summary>Implementation of a command-line parsing class.  Is capable of
	/// having switches registered with it directly or can examine a registered
	/// class for any properties with the appropriate attributes appended to
	/// them.</summary>
	public class ParametersParser
	{
		/// <summary>A simple internal class for passing back to the caller
		/// some information about the switch.  The internals/implementation
		/// of this class has privillaged access to the contents of the
		/// SwitchRecord class.</summary>
		public class SwitchInfo 
		{
			#region Private Variables
			private object m_Switch = null;
			#endregion

			#region Public Properties
			public string Name				{ get { return (m_Switch as SwitchRecord).Name; } }
			public string Description		{ get { return (m_Switch as SwitchRecord).Description; } }
			public string[] Aliases			{ get { return (m_Switch as SwitchRecord).Aliases; } }
			public System.Type Type			{ get { return (m_Switch as SwitchRecord).Type; } }
			public object Value				{ get { return (m_Switch as SwitchRecord).Value; } }
			public object InternalValue	{ get { return (m_Switch as SwitchRecord).InternalValue; } }
			public bool   IsEnum				{ get { return (m_Switch as SwitchRecord).Type.IsEnum; } }
			public string[] Enumerations	{ get { return (m_Switch as SwitchRecord).Enumerations; } }
			#endregion

			/// <summary>
			/// Constructor for the SwitchInfo class.  Note, in order to hide to the outside world
			/// information not necessary to know, the constructor takes a System.Object (aka
			/// object) as it's registering type.  If the type isn't of the correct type, an exception
			/// is thrown.
			/// </summary>
			/// <param name="rec">The SwitchRecord for which this class store information.</param>
			/// <exception cref="ArgumentException">Thrown if the rec parameter is not of
			/// the type SwitchRecord.</exception>
			public SwitchInfo( object rec )
			{
				if ( rec is SwitchRecord )
					m_Switch = rec;
				else
					throw new ArgumentException();
			}
		}

		/// <summary>
		/// The SwitchRecord is stored within the parser's collection of registered
		/// switches.  This class is private to the outside world.
		/// </summary>
		private class SwitchRecord
		{
			#region Private Variables
			private string m_name = "";
			private string m_description = "";
			private object m_value = null;
			private System.Type m_switchType = typeof(bool);
			private System.Collections.ArrayList m_Aliases = null;
			private string m_Pattern = "";

			// The following advanced functions allow for callbacks to be
			// made to manipulate the associated data type.
			private System.Reflection.MethodInfo m_SetMethod = null;
			private System.Reflection.MethodInfo m_GetMethod = null;
			private object m_PropertyOwner = null;
			#endregion

			#region Private Utility Functions
			private void Initialize( string name, string description )
			{
				m_name = name;
				m_description = description;

				BuildPattern();
			}

			private void BuildPattern()
			{
				string matchString = Name;

				if ( Aliases != null && Aliases.Length > 0 )
					foreach( string s in Aliases )
						matchString += "|" + s;

				string strPatternStart = @"(\s|^)(?<match>(-{1,2}|/)(";
				string strPatternEnd;  // To be defined below.

				// The common suffix ensures that the switches are followed by
				// a white-space OR the end of the string.  This will stop
				// switches such as /help matching /helpme
				//
				string strCommonSuffix = @"(?=(\s|$))";

				if ( Type == typeof(bool) )
					strPatternEnd = @")(?<value>(\+|-){0,1}))";
				else if ( Type == typeof(string) )
					strPatternEnd = @")(?::|\s+))((?:"")(?<value>.+)(?:"")|(?<value>\S+))";
				else if ( Type == typeof(int) )
					strPatternEnd = @")(?::|\s+))((?<value>(-|\+)[0-9]+)|(?<value>[0-9]+))";
				else if ( Type.IsEnum )
				{
					string[] enumNames = Enumerations;
					string e_str = enumNames[0];
					for ( int e=1; e<enumNames.Length; e++ )
						e_str += "|" + enumNames[e];
					strPatternEnd = @")(?::|\s+))(?<value>" + e_str + @")";
				}
				else
					throw new System.ArgumentException();

				// Set the internal regular expression pattern.
				m_Pattern = strPatternStart + matchString + strPatternEnd + strCommonSuffix;
			}
			#endregion

			#region Public Properties
			public object Value 
			{
				get 
				{
					if ( ReadValue != null )
						return ReadValue;
					else
						return m_value;
				}
			}

			public object InternalValue 
			{
				get { return m_value; }
			}

			public string Name 
			{
				get { return m_name;  }
				set { m_name = value; }
			}

			public string Description 
			{
				get { return m_description;  }
				set { m_description = value; }
			}

			public System.Type Type 
			{
				get { return m_switchType; }
			}

			public string[] Aliases 
			{
				get { return (m_Aliases != null) ? (string[])m_Aliases.ToArray(typeof(string)): null; }
			}

			public string Pattern 
			{
				get { return m_Pattern; }
			}
			
			public System.Reflection.MethodInfo SetMethod 
			{
				set { m_SetMethod = value; }
			}
			
			public System.Reflection.MethodInfo GetMethod 
			{
				set { m_GetMethod = value; }
			}

			public object PropertyOwner 
			{
				set { m_PropertyOwner = value; }
			}

			public object ReadValue 
			{
				get 
				{
					object o = null;
					if ( m_PropertyOwner != null && m_GetMethod != null )
						o = m_GetMethod.Invoke( m_PropertyOwner, null );
					return o;
				}
			}

			public string[] Enumerations 
			{
				get 
				{
					if ( m_switchType.IsEnum )
						return System.Enum.GetNames( m_switchType );
					else
						return null;
				}
			}
			#endregion

			#region Constructors
			public SwitchRecord( string name, string description )
			{
				Initialize( name, description );
			}

			public SwitchRecord( string name, string description, System.Type type )
			{
				if ( type == typeof(bool)   ||
					  type == typeof(string) ||
					  type == typeof(int)    ||
					  type.IsEnum )
				{
					m_switchType = type;
					Initialize( name, description );
				}
				else
					throw new ArgumentException("Currently only Ints, Bool and Strings are supported");
			}
			#endregion

			#region Public Methods
			public void AddAlias( string alias )
			{
				if ( m_Aliases == null )
					m_Aliases = new System.Collections.ArrayList();
				m_Aliases.Add( alias );

				BuildPattern();
			}

			public void Notify( object value )
			{
				if ( m_PropertyOwner != null && m_SetMethod != null )
				{
					object[] parameters = new object[1];
					parameters[0] = value;
					m_SetMethod.Invoke( m_PropertyOwner, parameters );
				}
				m_value = value;
			}
			
			#endregion
		}


		#region Private Variables
		private string m_commandLine = "";
		private string m_workingString = "";
		private string m_applicationName = "";
		private string[] m_splitParameters = null;
		private System.Collections.ArrayList m_switches = null;
		#endregion

		#region Private Utility Functions
		private void ExtractApplicationName()
		{
			Regex r = new Regex(@"^(?<commandLine>("".+""|(\S)+))(?<remainder>.+)",
				RegexOptions.ExplicitCapture);
			Match m = r.Match(m_commandLine);
			if ( m != null && m.Groups["commandLine"] != null )
			{
				m_applicationName = m.Groups["commandLine"].Value;
				m_workingString = m.Groups["remainder"].Value;
			}
		}

		private void SplitParameters()
		{
			// Populate the split parameters array with the remaining parameters.
			// Note that if quotes are used, the quotes are removed.
			// e.g.   one two three "four five six"
			//						0 - one
			//						1 - two
			//						2 - three
			//						3 - four five six
			// (e.g. 3 is not in quotes).
			Regex r = new Regex(@"((\s*(""(?<param>.+?)""|(?<param>\S+))))",
				RegexOptions.ExplicitCapture);
			MatchCollection m = r.Matches( m_workingString );

			if ( m != null )
			{
				m_splitParameters = new string[ m.Count ];
				for ( int i=0; i < m.Count; i++ )
					m_splitParameters[i] = m[i].Groups["param"].Value;
			}
		}

		private void HandleSwitches()
		{
			if ( m_switches != null )
			{
				foreach ( SwitchRecord s in m_switches )
				{
					Regex r = new Regex( s.Pattern,
						RegexOptions.ExplicitCapture
						| RegexOptions.IgnoreCase );
					MatchCollection m = r.Matches( m_workingString );
					if ( m != null )
					{
						for ( int i=0; i < m.Count; i++ )
						{
							string value = null;
							if ( m[i].Groups != null && m[i].Groups["value"] != null )
								value = m[i].Groups["value"].Value;

							if ( s.Type == typeof(bool))
							{
								bool state = true;
								// The value string may indicate what value we want.
								if ( m[i].Groups != null && m[i].Groups["value"] != null )
								{
									switch ( value )
									{
										case "+": state = true;
											break;
										case "-": state = false;
											break;
										case "":  if ( s.ReadValue != null )
														 state = !(bool)s.ReadValue;
											break;
										default:  break;
									}
								}
								s.Notify( state );
								break;
							}
							else if ( s.Type == typeof(string) )
								s.Notify( value );
							else if ( s.Type == typeof(int) )
								s.Notify( int.Parse( value ) );
							else if ( s.Type.IsEnum )
								s.Notify( System.Enum.Parse(s.Type,value,true) );
						}
					}

					m_workingString = r.Replace(m_workingString, " ");
				}
			}
		}

		#endregion

		#region Public Properties
		public string ApplicationName 
		{
			get { return m_applicationName; }
		}

		public string[] Parameters 
		{
			get { return m_splitParameters; }
		}

		public SwitchInfo[] Switches 
		{
			get 
			{
				if ( m_switches == null )
					return null;
				else
				{
					SwitchInfo[] si = new SwitchInfo[ m_switches.Count ];
					for ( int i=0; i<m_switches.Count; i++ )
						si[i] = new SwitchInfo( m_switches[i] );
					return si;
				}
			}
		}

		public object this[string name] 
		{
			get
			{
				if ( m_switches != null )
					for ( int i=0; i<m_switches.Count; i++ )
						if ( string.Compare( (m_switches[i] as SwitchRecord).Name, name, true )==0 )
							return (m_switches[i] as SwitchRecord).Value;
				return null;
			}
		}

		/// <summary>This function returns a list of the unhandled switches
		/// that the parser has seen, but not processed.</summary>
		/// <remark>The unhandled switches are not removed from the remainder
		/// of the command-line.</remark>
		public string[] UnhandledSwitches 
		{
			get
			{
				string switchPattern = @"(\s|^)(?<match>(-{1,2}|/)(.+?))(?=(\s|$))";
				Regex r = new Regex( switchPattern,
					RegexOptions.ExplicitCapture
					| RegexOptions.IgnoreCase );
				MatchCollection m = r.Matches( m_workingString );

				if ( m != null )
				{
					string[] unhandled = new string[ m.Count ];
					for ( int i=0; i < m.Count; i++ )
						unhandled[i] = m[i].Groups["match"].Value;
					return unhandled;
				}
				else
					return null;
			}
		}
		#endregion

		#region Public Methods
		public void AddSwitch( string name, string description )
		{
			if ( m_switches == null )
				m_switches = new System.Collections.ArrayList();

			SwitchRecord rec = new SwitchRecord( name, description );
			m_switches.Add( rec );
		}

		public void AddSwitch( string[] names, string description )
		{
			if ( m_switches == null )
				m_switches = new System.Collections.ArrayList();
			SwitchRecord rec = new SwitchRecord( names[0], description );
			for ( int s=1; s<names.Length; s++ )
				rec.AddAlias( names[s] );
			m_switches.Add( rec );
		}

		public bool Parse()
		{
			ExtractApplicationName();

			// Remove switches and associated info.
			HandleSwitches();

			// Split parameters.
			SplitParameters();

			return true;
		}

		public object InternalValue(string name)
		{
			if ( m_switches != null )
				for ( int i=0; i<m_switches.Count; i++ )
					if ( string.Compare( (m_switches[i] as SwitchRecord).Name, name, true )==0 )
						return (m_switches[i] as SwitchRecord).InternalValue;
			return null;
		}
		#endregion

		#region Constructors
		public ParametersParser( string commandLine )
		{
			m_commandLine = commandLine;
		}

		public ParametersParser( string commandLine,
							object classForAutoAttributes )
		{
			m_commandLine = commandLine;

			Type type = classForAutoAttributes.GetType();
			System.Reflection.MemberInfo[] members = type.GetMembers();

			for(int i=0; i<members.Length; i++)
			{
				object[] attributes = members[i].GetCustomAttributes(false);
				if(attributes.Length > 0)
				{
					SwitchRecord rec = null;

					foreach ( Attribute attribute in attributes )
					{
						if ( attribute is CommandParameterAttribute )
						{
							CommandParameterAttribute switchAttrib =
								(CommandParameterAttribute) attribute;

							// Get the property information.  We're only handling
							// properties at the moment!
							if ( members[i] is System.Reflection.PropertyInfo )
							{
								System.Reflection.PropertyInfo pi = (System.Reflection.PropertyInfo) members[i];

								rec = new SwitchRecord( switchAttrib.Name,
																switchAttrib.Description,
																pi.PropertyType );

								// Map in the Get/Set methods.
								rec.SetMethod = pi.GetSetMethod();
								rec.GetMethod = pi.GetGetMethod();
								rec.PropertyOwner = classForAutoAttributes;

								// Can only handle a single switch for each property
								// (otherwise the parsing of aliases gets silly...)
								break;
							}
						}
					}

					// See if any aliases are required.  We can only do this after
					// a switch has been registered and the framework doesn't make
					// any guarantees about the order of attributes, so we have to
					// walk the collection a second time.
					if ( rec != null )
					{
						foreach ( Attribute attribute in attributes )
						{
                            if (attribute is CommandParameterAliasAttribute)
							{
                                CommandParameterAliasAttribute aliasAttrib =
                                    (CommandParameterAliasAttribute)attribute;
								rec.AddAlias( aliasAttrib.Alias );
							}
						}
					}

					// Assuming we have a switch record (that may or may not have
					// aliases), add it to the collection of switches.
					if ( rec != null )
					{
						if ( m_switches == null )
							m_switches = new System.Collections.ArrayList();
						m_switches.Add( rec );
					}
				}
			}
		}
		#endregion
	}
}
