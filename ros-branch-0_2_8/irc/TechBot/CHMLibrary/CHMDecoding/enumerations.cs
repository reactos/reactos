using System;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// Enumeration for specifying the extraction mode of an toc or index item.
	/// </summary>
	public enum DataMode
	{
		/// <summary>
		/// TextBased - this item comes from a text-based sitemap file
		/// </summary>
		TextBased = 0,
		/// <summary>
		/// Binary - this item was extracted out of a binary stream
		/// </summary>
		Binary = 1
	}
}
