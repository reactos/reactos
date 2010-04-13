using System;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace SysGen.BuildEngine.Log
{
    /// <summary>
    /// Used to wrap log messages in xml  &lt;message/&gt; elements
    /// </summary>
    public class XmlLogger : LogListener, IBuildEventConsumer
    {
        public class Elements
        {
            public const string BUILD_RESULTS = "buildresults";
            public const string MESSAGE = "message";
            public const string TARGET = "target";
            public const string TASK = "task";
            public const string STATUS = "status";
        }

        public class Attributes
        {
            public const string PROJECT = "project";
            public const string MESSAGETYPE = "type";
        }

        private TextWriter _writer = Console.Out;
        private XmlTextWriter _xmlWriter = new XmlTextWriter(Console.Out);

        public XmlLogger()
        {

        }

        public XmlLogger(TextWriter writer)
        {
            _writer = writer;
            _xmlWriter = new XmlTextWriter(_writer);
            _xmlWriter.Formatting = Formatting.Indented;
        }

        public string StripFormatting(string message)
        {
            //looking for zero or more white space from front of line followed by
            //one or more of just about anything between [ and ] followed by a message
            //which we will capture. '    [blah] 
            Regex r = new Regex(@"(?ms)^\s*?\[[\s\w\d]+\](.+)");

            Match m = r.Match(message);
            if (m.Success)
            {
                return m.Groups[1].Captures[0].Value.Trim();
            }
            return message;
        }

        public bool IsJustWhiteSpace(string message)
        {
            Regex r = new Regex(@"^\s*$");

            return r.Match(message).Success;
        }

        #region LogListener Overrides

        public override void Write(string formattedMessage)
        {
            WriteLine(formattedMessage, null);
        }

        public override void WriteLine(string message)
        {
            WriteLine(message, null);
        }

        public override void WriteLine(string message, string messageType)
        {
            string rawMessage = StripFormatting(message.Trim());
            if (IsJustWhiteSpace(rawMessage))
            {
                return;
            }

            _xmlWriter.WriteStartElement(Elements.MESSAGE);

            if (messageType != null && messageType != String.Empty)
            {
                _xmlWriter.WriteAttributeString(Attributes.MESSAGETYPE, messageType);
            }

            if (IsValidXml(rawMessage))
            {
                rawMessage = Regex.Replace(rawMessage, @"<\?.*\?>", String.Empty);
                _xmlWriter.WriteRaw(rawMessage);
            }
            else
            {
                _xmlWriter.WriteCData(StripCData(rawMessage));
            }
            _xmlWriter.WriteEndElement();
            _xmlWriter.Flush();
        }

        private bool IsValidXml(string message)
        {
            if (Regex.Match(message, @"^<.*>").Success)
            {
                // validate xml
                XmlValidatingReader reader = new XmlValidatingReader(message, XmlNodeType.Element, null);
                try { while (reader.Read()) { } }
                catch (Exception) { return false; }
                finally { reader.Close(); }
                return true;
            }
            return false;
        }

        private string StripCData(string message)
        {
            string strippedMessage = Regex.Replace(message, @"<!\[CDATA\[", String.Empty);
            return Regex.Replace(strippedMessage, @"\]\]>", String.Empty);
        }

        public override void Flush()
        {
            _writer.Flush();
        }

        /// <summary>Returns the contents of log captured.</summary>
        public override string ToString()
        {
            return _writer.ToString();
        }

        #endregion

        #region IBuildEventConsumer Implementation

        public void BuildStarted(object obj, BuildEventArgs args)
        {
            _xmlWriter.WriteStartElement(Elements.BUILD_RESULTS);
            _xmlWriter.WriteAttributeString(Attributes.PROJECT, args.Name);
        }

        public void BuildFinished(object obj, BuildEventArgs args)
        {
            _xmlWriter.WriteEndElement();
        }

        public void TargetStarted(object obj, BuildEventArgs args)
        {
            _xmlWriter.WriteStartElement(Elements.TARGET);
            WriteNameAttribute(args.Name);
            _xmlWriter.Flush();
        }

        public void TargetFinished(object obj, BuildEventArgs args)
        {
            _xmlWriter.WriteEndElement();
            _xmlWriter.Flush();
        }

        public void TaskStarted(object obj, BuildEventArgs args)
        {
            _xmlWriter.WriteStartElement(Elements.TASK);
            WriteNameAttribute(args.Name);
            _xmlWriter.Flush();
        }

        public void TaskFinished(object obj, BuildEventArgs args)
        {
            _xmlWriter.WriteEndElement();
            _xmlWriter.Flush();
        }

        private void WriteNameAttribute(string name)
        {
            _xmlWriter.WriteAttributeString("name", name);
        }

        private void WriteStatus(string status)
        {
            _xmlWriter.WriteStartElement(Elements.STATUS);
            _xmlWriter.WriteAttributeString("value", status);
            _xmlWriter.WriteEndElement();
        }
        #endregion
    }
}
