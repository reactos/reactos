using System;
using System.Web.UI;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using SysGen.BuildEngine.Framework.VisualStudio;
using SysGen.BuildEngine.Backends;
using SysGen.RBuild.Framework;
using SysGen.BuildEngine;

namespace SysGen.BuildEngine.Backends
{
    public class MSVisualStudio : Backend
    {
        public MSVisualStudio(SysGenEngine sysgen)
            : base(sysgen)
        {
        }

        protected override string FriendlyName
        {
            get { return "Visual Studio 6.0-2005"; }
        }

        protected override void Generate()
        {
            VSSolution solution = new VSSolution();

            solution.Name = "ReactOS";
            solution.FileName = "reactos.sln";

            foreach (RBuildModule module in SysGen.Project.Modules)
            {
                VSProject project = new VSProject();

                //project.Name = module.Name;
                project.FileName = module.Name + ".vcproj";

                solution.Projects.Add(project);
            }
        }
    }
}
