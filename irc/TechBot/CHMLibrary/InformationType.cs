using System;
using System.IO;

namespace HtmlHelp
{
	/// <summary>
	/// Enumeration for specifying the mode of the information type
	/// </summary>
	public enum InformationTypeMode
	{
		/// <summary>
		/// Inclusive information type. The user will be allowed to select from one or more information types.
		/// </summary>
		Inclusive = 0,
		/// <summary>
		/// Exclusive information type. The user will be allowed to choose only one information type within each category
		/// </summary>
		Exclusive = 1,
		/// <summary>
		/// Hidden information type. The user cannot see this information types (only for API calls).
		/// </summary>
		Hidden = 2
	}

	/// <summary>
	/// The class <c>InformationType</c> implements a methods/properties for an information type.
	/// </summary>
	/// <remarks>Note: Information types and categories allow users to filter help contents. 
	/// They are only supported if using sitemap TOC and/or sitemap Index.</remarks>
	public class InformationType
	{
		private string _name = "";
		private string _description = "";
		private InformationTypeMode _typeMode = InformationTypeMode.Inclusive;
		private bool _isInCategory = false;
		private int _referenceCount = 1;

		/// <summary>
		/// Standard constructor
		/// </summary>
		/// <remarks>the mode is set to InformationTypeMode.Inclusive by default</remarks>
		public InformationType() : this("","")
		{
		}

		/// <summary>
		/// Standard constructor
		/// </summary>
		/// <param name="name">name of the information type</param>
		/// <param name="description">description</param>
		/// <remarks>the mode is set to InformationTypeMode.Inclusive by default</remarks>
		public InformationType(string name, string description) : this(name, description, InformationTypeMode.Inclusive)
		{
		}

		/// <summary>
		/// Standard constructor
		/// </summary>
		/// <param name="name">name of the information type</param>
		/// <param name="description">description</param>
		/// <param name="mode">mode of the information type</param>
		public InformationType(string name, string description, InformationTypeMode mode)
		{
			_name = name;
			_description = description;
			_typeMode = mode;
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			writer.Write( (int)_typeMode );
			writer.Write( _name );
			writer.Write( _description );
		}

		/// <summary>
		/// Reads the object data from a dump store
		/// </summary>
		/// <param name="reader">reader to read the data</param>
		internal void ReadDump(ref BinaryReader reader)
		{
			_typeMode = (InformationTypeMode)reader.ReadInt32();
			_name = reader.ReadString();
			_description = reader.ReadString();
		}
		#endregion

		/// <summary>
		/// Sets the flag if this information type is nested in at least one category 
		/// </summary>
		/// <param name="newValue">true or false</param>
		internal void SetCategoryFlag(bool newValue)
		{
			_isInCategory = newValue;
		}

		/// <summary>
		/// Gets/Sets the reference count of this information type instance
		/// </summary>
		internal int ReferenceCount
		{
			get { return _referenceCount; }
			set { _referenceCount = value; }
		}

		/// <summary>
		/// Gets true if this information type is nested in at least one category
		/// </summary>
		public bool IsInCategory
		{
			get { return _isInCategory; }
		}

		/// <summary>
		/// Gets/Sets the name of the information type
		/// </summary>
		public string Name
		{
			get { return _name; }
			set { _name = value; }
		}

		/// <summary>
		/// Gets/Sets the description of the information type
		/// </summary>
		public string Description
		{
			get { return _description; }
			set { _name = value; }
		}

		/// <summary>
		/// Gets/Sets the mode of the information type
		/// </summary>
		public InformationTypeMode Mode
		{
			get { return _typeMode; }
			set { _typeMode = value; }
		}
	}
}
