using System;
using System.Xml;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("baseadress")]
    public class BaseAdressTask : PropertyBaseTask
    {
        protected override void OnLoad()
        {
            Project.Properties.Add(new RBuildBaseAdress(m_Name, m_Value));
        }
    }
}