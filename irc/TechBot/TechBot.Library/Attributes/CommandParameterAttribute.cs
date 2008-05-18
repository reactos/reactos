using System;

namespace TechBot.Library
{
	/// <summary>Implements a basic command-line switch by taking the
	/// switching name and the associated description.</summary>
	/// <remark>Only currently is implemented for properties, so all
	/// auto-switching variables should have a get/set method supplied.</remark>
	[AttributeUsage( AttributeTargets.Property )]
	public class CommandParameterAttribute : Attribute
	{
		#region Private Variables
		private string m_name = "";
		private string m_description = "";
        private bool m_Required = true;
        private bool m_Default = false;
		#endregion

		#region Public Properties
		/// <summary>Accessor for retrieving the switch-name for an associated
		/// property.</summary>
		public string Name 			{ get { return m_name; } }

		/// <summary>Accessor for retrieving the description for a switch of
		/// an associated property.</summary>
		public string Description	{ get { return m_description; } }

        public bool Required { get { return m_Required; } }

        public bool DefaultParameter 
        {
            get { return m_Default; }
            set { m_Default = value; }
        }

		#endregion

		#region Constructors
		
        /// <summary>
        /// Attribute constructor.
        /// </summary>
        public CommandParameterAttribute(string name, string description)
        {
            m_name = name;
            m_description = description;
        }
		#endregion
	}
}
