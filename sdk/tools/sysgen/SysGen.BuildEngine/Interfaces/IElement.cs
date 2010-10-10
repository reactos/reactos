using System;
using System.Xml;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine
{
    public interface IElement
    {
        string BaseBuildLocation { get; }
        void Initialize(System.Xml.XmlNode elementNode);
        RBuildModule Module { get; }
        string Name { get; }
        IElement Parent { get; set; }
        RBuildProject Project { get; set; }
        //PropertyCollection Properties { get; }
        RBuildElement RBuildElement { get; }
        SysGenEngine SysGen { get; set; }
        XmlNode XmlNode { get; }
    }
}