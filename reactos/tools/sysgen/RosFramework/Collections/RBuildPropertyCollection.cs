using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Text.RegularExpressions;

namespace SysGen.RBuild.Framework
{
    public class RBuildPropertyCollection : List<RBuildProperty>
    {
        /// <summary>
        /// Adds a property that cannot be changed.
        /// </summary>
        /// <remarks>
        /// Properties added with this method can never be changed.  Note that
        /// they are removed if the <c>Clear</c> method is called.
        /// </remarks>
        /// <param name="name">Name of property</param>
        /// <param name="value">Value of property</param>
        public virtual void AddReadOnly(string name, string value) 
        {
            Add(name, value, true);
        }

        /// <summary>
        /// Adds a property to the collection.
        /// </summary>
        /// <param name="name">Name of property</param>
        /// <param name="value">Value of property</param>
        public virtual void Add(string name, string value)
        {
            Add(name, value, false);
        }

        /// <summary>
        /// Adds a property to the collection.
        /// </summary>
        /// <param name="name">Name of property</param>
        /// <param name="value">Value of property</param>
        public virtual void Add(string name, bool value)
        {
            Add(name, value.ToString());
        }

        /// <summary>
        /// Adds a property to the collection.
        /// </summary>
        /// <param name="name">Name of property</param>
        /// <param name="value">Value of property</param>
        public virtual void Add(string name, string value, bool readOnly) 
        {
            if (!PropertyExists(name))
            {
                Add(new RBuildProperty(name, value , readOnly));
            }
        }

        /// <summary>
        /// Adds a property to the collection.
        /// </summary>
        /// <param name="name">Name of property</param>
        /// <param name="value">Value of property</param>
        public virtual void Add(string name, string value, bool readOnly, bool isInternal)
        {
            if (!PropertyExists(name))
            {
                Add(new RBuildProperty(name, value, readOnly,isInternal));
            }
        }
        
        /// <summary>
        /// Returns true if a property is listed as read only
        /// </summary>
        /// <param name="name">Property to check</param>
        /// <returns>true if readonly, false otherwise</returns>
        public virtual bool IsReadOnlyProperty(string name) 
        {
            if (PropertyExists(name))
                return this[name].ReadOnly;
            return false;
        }

        /// <summary>
        /// Returns true if a property exists
        /// </summary>
        /// <param name="name">Property to check</param>
        /// <returns>true if exists, false otherwise</returns>
        public bool PropertyExists(string name)
        {
            return (this[name] != null);
        }

        /// <summary>
        /// Indexer property. 
        /// </summary>
        public virtual RBuildProperty this[string name]
        {
            get
            {
                foreach (RBuildProperty property in this)
                    if (property.Name == name)
                        return property;

                return null;
            }
        }
    }
}
