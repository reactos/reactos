using System;
using System.Web.UI;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using SysGen.BuildEngine.Backends;
using SysGen.RBuild.Framework;
using SysGen.BuildEngine;

namespace SysGen.BuildEngine.Backends
{
    public class ProjectTreeReport : Backend
    {
        public ProjectTreeReport(SysGenEngine sysgen)
            : base(sysgen)
        {
        }

        protected override string FriendlyName
        {
            get { return "Tree Report"; }
        }

        protected override void Generate()
        {
            using (StreamWriter sw = new StreamWriter(@"C:\resTree.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    WriteModule(SysGen.RootTask , writer);
                }
            }
        }

        private void WriteModule(Task task, HtmlTextWriter writer)
        {
            ITaskContainer container = task as ITaskContainer;

            writer.AddAttribute(HtmlTextWriterAttribute.Cellpadding, "5");
            writer.AddAttribute(HtmlTextWriterAttribute.Cellspacing, "5");
            writer.AddAttribute(HtmlTextWriterAttribute.Border, "1");
            writer.AddAttribute(HtmlTextWriterAttribute.Bordercolor, "#000000");
            writer.RenderBeginTag(HtmlTextWriterTag.Table);
            writer.RenderBeginTag(HtmlTextWriterTag.Tr);
            writer.RenderBeginTag(HtmlTextWriterTag.Td);
            writer.Write(task.Name);

            if (container != null)
            {
                if (container.ChildTasks.Count > 0)
                {
                    foreach (Task childTask in container.ChildTasks)
                    {
                        WriteModule(childTask, writer);
                    }
                }
            }

            writer.RenderEndTag();
            writer.RenderEndTag();
            writer.RenderEndTag();
        }
    }
}
