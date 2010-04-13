using System;
using System.IO;
using System.Reflection;
using System.Xml;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Log;
using SysGen.BuildEngine.Attributes;
using SysGen.BuildEngine.Tasks;

namespace SysGen.BuildEngine 
{
    /// <summary>Models a NAnt XML element in the build file.</summary>
    /// <remarks>
    ///   <para>Automatically validates attributes in the element based on Attribute settings in the derived class.</para>
    /// </remarks>
    public class Element : IElement 
    {
        protected Location _location = Location.UnknownLocation;
        protected SysGenEngine _sysgen = null;
        protected RBuildProject _project = null;
        protected XmlNode _xmlNode = null;
        protected IElement _parent = null;
        protected bool m_FailOnMissingRequired = true;

        /// <summary>
        /// The default contstructor.
        /// </summary>
        public Element()
        {
        }

        /// <summary>A copy contstructor.</summary>
        protected Element(Element element) : this() 
        {
            _location = element._location;
            _sysgen = element._sysgen;
            _xmlNode = element._xmlNode;
        }

        /// <summary><see cref="Location"/> in the build file where the element is defined.</summary>
        protected virtual Location Location {
            get { return _location; }
            set { _location = value; }
        }

        /// <summary>
        /// The Parent object. This will be your parent Task, Target, or Project depeding on where the element is defined.
        /// </summary>
        public IElement Parent { get { return _parent; } set { _parent = value; } }

        /// <summary>Name of the XML element used to initialize this element.</summary>
        public virtual string Name 
        {
            get {
                ElementNameAttribute elementNameAttribute = (ElementNameAttribute) 
                    Attribute.GetCustomAttribute(GetType(), typeof(ElementNameAttribute));

                string name = null;
                if (elementNameAttribute != null) {
                    name = elementNameAttribute.Name;
                }
                return name;
            }
        }

        /// <summary>
        /// The <see cref="Project"/> this element belongs to.
        /// </summary>
        public virtual SysGenEngine SysGen
        {
            get { return _sysgen; }
            set { _sysgen = value; }
        }

        public RBuildProject Project
        {
            get { return _project; }
            set { _project = value; }
        }

        public RBuildModule Module
        {
            get 
            {
                RBuildModule module = RBuildElement as RBuildModule;

                if (module == null)
                    throw new BuildException(String.Format("Task <{0} ... \\> is not child of any ModuleTask." , Name), Location);

                return module;
            }
        }

        /// <summary>
        /// <see cref="Project"/> this element belongs to.
        /// </summary>
        public virtual RBuildElement RBuildElement
        {
            get
            {
                IElement element = this;
                while (element != null)
                {
                    if (element is ISysGenObject)
                        return ((ISysGenObject)element).RBuildElement;

                    //Set to his parent
                    element = element.Parent;
                }

                return SysGen.Project;
            }
        }

        public string BaseBuildLocation
        {
            get { return Path.GetDirectoryName(new Uri(_xmlNode.BaseURI).LocalPath); }
        }

        public XmlNode XmlNode
        {
            get { return _xmlNode; }
        }
     
        /// <summary>
        /// Initializes all build attributes.
        /// </summary>
        private void InitializeProperties(XmlNode elementNode) 
        {
            // Get the current element Type
            Type currentType = GetType();
            
            PropertyInfo[] propertyInfoArray = currentType.GetProperties(BindingFlags.Public|BindingFlags.Instance);
            foreach (PropertyInfo propertyInfo in propertyInfoArray ) 
            {
                // process all TaskPropertyAttribute attributes
                TaskPropertyAttribute[] propertyAttributes = (TaskPropertyAttribute[])
                    Attribute.GetCustomAttributes(propertyInfo, typeof(TaskPropertyAttribute) , false);

                foreach(TaskPropertyAttribute propertyAttribute in propertyAttributes)
                {
                    string propertyValue = null;

                    if (propertyAttribute.Location == TaskPropertyLocation.Attribute)
                    {
                        if (elementNode.Attributes[propertyAttribute.Name] != null)
                        {
                            propertyValue = elementNode.Attributes[propertyAttribute.Name].Value;
                        }
                    }
                    else if (propertyAttribute.Location == TaskPropertyLocation.Node)
                    {
                        propertyValue = elementNode.InnerText;
                    }

                    // check if its required
                    if (propertyValue == null && propertyAttribute.Required && m_FailOnMissingRequired) 
                    {
                        throw new BuildException(String.Format("'{0}' is a required '{1}' of <{2} ... \\>.", propertyAttribute.Name, propertyAttribute.Location , Name), Location);
                    }

                    if (propertyValue != null) 
                    {
                        //string attrValue = attributeNode.Value;
                        if (propertyAttribute.ExpandProperties) 
                        {
                            // expand attribute properites
                            propertyValue = SysGen.ExpandProperties(propertyValue);
                        }

                        if (propertyInfo.CanWrite)
                        {
                            // set the property value instead
                            MethodInfo info = propertyInfo.GetSetMethod();
                            object[] paramaters = new object[1];

                            Type propertyType = propertyInfo.PropertyType;

                            // If the object is an emum
                            if (propertyType.IsSubclassOf(typeof(System.Enum)))
                            {
                                try
                                {
                                    paramaters[0] = Enum.Parse(propertyType, propertyValue, true);
                                }
                                catch (Exception)
                                {
                                    // catch type conversion exceptions here
                                    string message = string.Format("Invalid value '{0}'. Valid values for this attribute are:\n", propertyValue);
                                    foreach (object value in Enum.GetValues(propertyType))
                                    {
                                        message += string.Format("\t{0}\n", value.ToString());
                                    }
                                    throw new BuildException(message, Location);
                                }
                            }
                            else
                            {
                                //validate attribute value with custom ValidatorAttribute(ors)                                                            
                                ValidatorAttribute[] validateAttributes = (ValidatorAttribute[])
                                    Attribute.GetCustomAttributes(propertyInfo, typeof(ValidatorAttribute));
                                try
                                {
                                    foreach (ValidatorAttribute validator in validateAttributes)
                                        validator.Validate(propertyValue);
                                }
                                catch (ValidationException ve)
                                {
                                    throw new ValidationException(ve.Message, Location);
                                }

                                if (propertyType == typeof(System.Boolean))
                                {
                                    paramaters[0] = Convert.ChangeType(SysGenConversion.ToBolean(propertyValue), propertyInfo.PropertyType);
                                }
                                else
                                {
                                    paramaters[0] = Convert.ChangeType(propertyValue, propertyInfo.PropertyType);
                                }
                            }

                            info.Invoke(this, paramaters);
                        }
                        else
                        {
                            new BuildException(string.Format("Property '{0}' was found but '{1}' does no implement Set", propertyAttribute.Name, Name));
                        }
                    }
                }

                // now do nested BuildElements
                BuildElementAttribute buildElementAttribute = (BuildElementAttribute) 
                    Attribute.GetCustomAttribute(propertyInfo, typeof(BuildElementAttribute));

                if (buildElementAttribute != null) 
                {
                    // get value from xml node
                    XmlNode nestedElementNode = elementNode[buildElementAttribute.Name, elementNode.OwnerDocument.DocumentElement.NamespaceURI]; 
                    // check if its required
                    if (nestedElementNode == null && buildElementAttribute.Required) {
                        throw new BuildException(String.Format("'{0}' is a required element of <{1} ...//>.", buildElementAttribute.Name, this.Name), Location);
                    }
                    if (nestedElementNode != null) {
                        Element childElement = (Element)propertyInfo.GetValue(this, null);
                        // Sanity check: Ensure property wasn't null.
                        if ( childElement == null )
                            throw new BuildException(String.Format("Property '{0}' value cannot be null for <{1} ...//>", propertyInfo.Name, this.Name), Location);
                        childElement.SysGen = SysGen;
                        childElement.Initialize(nestedElementNode);
                    }                        
                }
            }            
        }

        /// <summary>Performs default initialization.</summary>
        /// <remarks>
        ///   <para>Derived classes that wish to add custom initialization should override <see cref="InitializeElement"/>.</para>
        /// </remarks>
        public void Initialize(XmlNode elementNode) 
        {
            if (SysGen == null)
                throw new InvalidOperationException("Element has invalid BuildFileLoader property.");

            // Save the element node
            _xmlNode = elementNode;

            // Save position in buildfile for reporting useful error messages.
            try 
            {
                _location = SysGen.LocationMap.GetLocation(elementNode);
            }
            catch(ArgumentException ae) 
            {
                BuildLog.WriteLineIf(SysGen.Verbose, ae.ToString());
            }

            InitializeProperties(elementNode);

            OnInit();

            // Allow inherited classes a chance to do some custom initialization.
            InitializeElement(elementNode);

            // The Element has been completly initialized
            OnLoad();
        }

        /// <summary>
        /// Allows derived classes to provide extra initialization and validation not covered by the base class.
        /// </summary>
        /// <param name="elementNode">The xml node of the element to use for initialization.</param>
        protected virtual void InitializeElement(XmlNode elementNode) 
        {
        }

        protected virtual void OnLoad()
        {
        }

        protected virtual void OnInit()
        {
        }

    }
}
