using System;

namespace TechBot.Library
{
	/// <summary>
	/// This class implements an alias attribute to work in conjunction
	/// with the <see cref="CommandLineSwitchAttribute">CommandLineSwitchAttribute</see>
	/// attribute.  If the CommandLineSwitchAttribute exists, then this attribute
	/// defines an alias for it.
	/// </summary>
	[AttributeUsage( AttributeTargets.Property )]
	public class CommandParameterAliasAttribute : Attribute
	{
		#region Private Variables
		protected string m_Alias = "";
		#endregion

		#region Public Properties
		public string Alias 
		{
			get { return m_Alias; }
		}

		#endregion

		#region Constructors
        public CommandParameterAliasAttribute(string alias) 
		{
			m_Alias = alias;
		}
		#endregion
	}

}
