using System;
using System.Collections;
using System.IO;

using HtmlHelp.ChmDecoding;

namespace HtmlHelp
{
	/// <summary>
	/// The class <c>Category</c> implements methods/properties for handling an information category
	/// </summary>
	/// <remarks>Note: Information types and categories allow users to filter help contents. 
	/// They are only supported if using sitemap TOC and/or sitemap Index.</remarks>
	public class Category
	{
		private string _name = "";
		private string _description = "";
		private ArrayList _infoTypes = null;
		private int _referenceCount = 1;

		/// <summary>
		/// Standard constructor
		/// </summary>
		public Category() : this("","")
		{
		}

		/// <summary>
		/// Standard constructor
		/// </summary>
		/// <param name="name">name of the category</param>
		/// <param name="description">description</param>
		public Category(string name, string description) : this(name, description, new ArrayList())
		{
		}
		/// <summary>
		/// Standard constructor
		/// </summary>
		/// <param name="name">name of the category</param>
		/// <param name="description">description</param>
		/// <param name="linkedInformationTypes">Arraylist of InformationType instances which applies to this category</param>
		public Category(string name, string description, ArrayList linkedInformationTypes)
		{
			_name = name;
			_description = description;
			_infoTypes = linkedInformationTypes;
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			writer.Write( _name );
			writer.Write( _description );

			writer.Write( _infoTypes.Count );

			for(int i=0; i<_infoTypes.Count;i++)
			{
				InformationType curType = _infoTypes[i] as InformationType;
				writer.Write( curType.Name );
			}
				
		}

		/// <summary>
		/// Reads the object data from a dump store
		/// </summary>
		/// <param name="reader">reader to read the data</param>
		/// <param name="chmFile">current CHMFile instance which reads from dump</param>
		internal void ReadDump(ref BinaryReader reader, CHMFile chmFile)
		{
			_name = reader.ReadString();
			_description = reader.ReadString();

			int nCnt = reader.ReadInt32();

			for(int i=0; i<nCnt; i++)
			{
				string sITName = reader.ReadString();

				InformationType linkedType = chmFile.GetInformationType( sITName );

				if(linkedType != null)
				{
					linkedType.SetCategoryFlag(true);
					_infoTypes.Add(linkedType);
				}
			}
		}
		#endregion

		/// <summary>
		/// Merges the lineked information types from cat into this instance
		/// </summary>
		/// <param name="cat">category instance</param>
		internal void MergeInfoTypes(Category cat)
		{
			if(cat!=null)
			{
				if(cat.InformationTypes.Count > 0)
				{
					for(int i=0;i<cat.InformationTypes.Count;i++)
					{
						InformationType curType = cat.InformationTypes[i] as InformationType;

						if(!ContainsInformationType(curType.Name))
						{
							curType.SetCategoryFlag(true);
							_infoTypes.Add(curType);
						}
					}
				}
			}
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
		/// Gets an ArrayList with the linked Information types
		/// </summary>
		public ArrayList InformationTypes
		{
			get { return _infoTypes; }
		}

		/// <summary>
		/// Adds a new information type to the category
		/// </summary>
		/// <param name="type"></param>
		public void AddInformationType(InformationType type)
		{
			_infoTypes.Add(type);
		}

		/// <summary>
		/// Removes an information type from the category
		/// </summary>
		/// <param name="type"></param>
		public void RemoveInformationType(InformationType type)
		{
			_infoTypes.Remove(type);
		}

		/// <summary>
		/// Checks if the category contains an information type
		/// </summary>
		/// <param name="type">information type instance to check</param>
		/// <returns>Return true if the information type is part of this category</returns>
		public bool ContainsInformationType(InformationType type)
		{
			return _infoTypes.Contains(type);
		}

		/// <summary>
		/// Checks if the category contains an information type
		/// </summary>
		/// <param name="name">name of the information type</param>
		/// <returns>Return true if the information type is part of this category</returns>
		public bool ContainsInformationType(string name)
		{
			for(int i=0;i<_infoTypes.Count;i++)
			{
				InformationType curType = _infoTypes[i] as InformationType;

				if(curType.Name == name)
					return true;
			}

			return false;
		}
	}
}
