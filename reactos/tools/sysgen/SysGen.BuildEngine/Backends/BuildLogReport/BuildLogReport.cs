using System;
using System.Text.RegularExpressions;
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
    public class BuildLogReportEntry
    {
        public string File;
        public string Message;
        public string Position;
        public string Type;
    }

    public class BuildLogReport : Backend
    {
        private List<BuildLogReportEntry> m_Errors = new List<BuildLogReportEntry>();

        public BuildLogReport(SysGenEngine sysgen)
            : base(sysgen)
        {
        }

        protected override string FriendlyName
        {
            get { return "Build Log analizer"; }
        }

        private List<BuildLogReportEntry> Errors
        {
            get { return m_Errors; }
        }

        protected override void Generate()
        {
            using (StreamReader sr = new StreamReader(@"C:\Ros\Trunk\reactos\RosBE-Logs\BuildLog-4.1.3-20070210-0630.txt"))
            {
                Regex regex = new Regex(@"(.*?):(.*?): (.*?): (.*?)$",
                                RegexOptions.IgnoreCase |
                                RegexOptions.Multiline |
                                RegexOptions.Compiled);

                MatchCollection matches = regex.Matches(sr.ReadToEnd());
                foreach (Match match in matches)
                {
                    BuildLogReportEntry error = new BuildLogReportEntry();

                    error.File = match.Groups[1].ToString();
                    error.Position = match.Groups[2].ToString();
                    error.Type = match.Groups[3].ToString();
                    error.Message = match.Groups[4].ToString();

                    Errors.Add(error);
                }
            }

            using (StreamWriter sw = new StreamWriter(@"C:\rosbuildwarnings.htm"))
            {
                using (HtmlTextWriter writer = new HtmlTextWriter(sw))
                {
                    writer.WriteLine("{0} Warnings" , Errors.Count);

                    writer.RenderBeginTag(HtmlTextWriterTag.Table);

                    writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("File");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Line/Column");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Type");
                    writer.RenderEndTag();
                    writer.RenderBeginTag(HtmlTextWriterTag.Th);
                    writer.Write("Error");
                    writer.RenderEndTag();
                    writer.RenderEndTag();

                    foreach (BuildLogReportEntry report in m_Errors)
                    {
                        writer.RenderBeginTag(HtmlTextWriterTag.Tr);
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(report.File);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(report.Position);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(report.Type);
                        writer.RenderEndTag();
                        writer.RenderBeginTag(HtmlTextWriterTag.Td);
                        writer.Write(report.Message);
                        writer.RenderEndTag();
                        writer.RenderEndTag();
                    }
                }
            }
        }
    }
}
