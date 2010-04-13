using System;
using System.Web.UI;
using System.IO;
using System.Collections.Generic;
using System.Text;

using SysGen.BuildEngine;
using SysGen.BuildEngine.Framework;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public abstract class HtmlDocumenterBaseBacked : Backend
    {
        public HtmlDocumenterBaseBacked(SysGenEngine sysgen)
            : base(sysgen)
        {
        }

        public string ReportFileExtension
        {
            get { return "htm"; }
        }

        protected string GetHtmlFileName(IRBuildNamed namedObject)
        {
            return string.Format("{0}.{1}",
                namedObject.Name,
                ReportFileExtension);
        }

        protected void WriteDocumentStart(HtmlTextWriter writer, string title)
        {
            writer.RenderBeginTag(HtmlTextWriterTag.Html);//<html>
            writer.RenderBeginTag(HtmlTextWriterTag.Head);// <head>
            writer.RenderBeginTag(HtmlTextWriterTag.Title); // <title>
            writer.Write(title);
            writer.RenderEndTag(); // </title>
            writer.AddAttribute(HtmlTextWriterAttribute.Rel, "stylesheet");
            writer.AddAttribute(HtmlTextWriterAttribute.Type, "text/css");
            writer.AddAttribute(HtmlTextWriterAttribute.Href, "style.css");
            writer.RenderBeginTag(HtmlTextWriterTag.Link);
            writer.RenderEndTag();

            writer.RenderEndTag(); // </head>
            writer.RenderBeginTag(HtmlTextWriterTag.Body);// <body>

            writer.AddAttribute(HtmlTextWriterAttribute.Class, "header");
            writer.RenderBeginTag(HtmlTextWriterTag.Div);
            writer.AddAttribute(HtmlTextWriterAttribute.Href, "default.htm");
            writer.RenderBeginTag(HtmlTextWriterTag.A);
            writer.Write("ReactOS RBuild Documentation");
            writer.RenderEndTag();

            writer.RenderBeginTag(HtmlTextWriterTag.P);
            
            if (!Project.Properties["ARCH"].IsEmpty)
            {
                if (!Project.Properties["SARCH"].IsEmpty)
                {
                    writer.Write("RBuild Documentation for the <b>'{0}'</b> architecture, sub-architecture <b>'{1}'</b>. Project used <b>'{2}'</b>",
                        Project.Properties["ARCH"].Value,
                        Project.Properties["SARCH"].Value,
                        Project.RBuildFile);
                }
                else
                {
                    writer.Write("RBuild Documentation for the <b>'{0}'</b> architecture. Project used <b>'{1}'</b>", 
                        Project.Properties["ARCH"].Value,
                        Project.RBuildFile);
                }
            }

            writer.RenderEndTag();
            writer.RenderEndTag();
        }

        protected void WriteDocumentEnd(HtmlTextWriter writer)
        {
            writer.WriteBreak();
            writer.AddAttribute(HtmlTextWriterAttribute.Class, "footer");
            writer.RenderBeginTag(HtmlTextWriterTag.Div);
            WriteDocumentLastUpdate(writer);
            writer.RenderEndTag();

            writer.RenderEndTag(); // </body>
            writer.RenderEndTag(); // </html>
        }

        protected void WriteDocumentLastUpdate(HtmlTextWriter writer)
        {
            //writer.RenderBeginTag(HtmlTextWriterTag.P);
            writer.Write("Document last updated on {0}", DateTime.Now);
            //writer.RenderEndTag();
        }
    }
}
