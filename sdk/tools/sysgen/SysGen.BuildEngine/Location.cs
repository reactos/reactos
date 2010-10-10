using System;
using System.IO;
using System.Text;

namespace SysGen.BuildEngine 
{
    /// <summary>
    /// Stores the file name, line number and column number to record a position in a text file.
    /// </summary>
    [Serializable]
    public class Location {
        string _fileName = null;
        int _lineNumber = 0;
        int _columnNumber = 0;

        public static readonly Location UnknownLocation = new Location();

        /// <summary>Creates a location consisting of a file name, line number and column number.</summary>
        /// <remarks>fileName can be a local URI resource, e.g., file:///C:/WINDOWS/setuplog.txt</remarks>
        public Location(string fileName, int lineNumber, int columnNumber) {
            Init(fileName, lineNumber, columnNumber);
        }

        /// <summary>Creates a location consisting of a file name.</summary>
        /// <remarks>fileName can be a local URI resource, e.g., file:///C:/WINDOWS/setuplog.txt</remarks>
        public Location(string fileName) {
            Init(fileName, 0, 0);
        }

        /// <summary>Creates an "unknown" location.</summary>
        private Location() {
            Init(null, 0, 0);
        }

        /// <summary>Private Init function.</summary>
        private void Init(string fileName, int lineNumber, int columnNumber) {
            if (fileName != null) {
                try {
                    // first check to see if fileName is a URI
                    Uri uri = new Uri(fileName);
                    fileName = uri.LocalPath;
                } catch {
                    // must be a simple filename
                    fileName = Path.GetFullPath(fileName);
                }
            }
            _fileName = fileName;
            _lineNumber = lineNumber;
            _columnNumber = columnNumber;
        }

        /// <summary>Gets a string containing the file name for the location.</summary>
        /// <remarks>The file name includes both the file path and the extension.</remarks>
        public string FileName {
            get { return _fileName; }
        }

        /// <summary>Gets the line number for the location.</summary>
        /// <remarks>Lines start at 1.  Will be zero if not specified.</remarks>
        public int LineNumber {
            get { return _lineNumber; }
        }

        /// <summary>Gets the column number for the location.</summary>
        /// <remarks>Columns start a 1.  Will be zero if not specified.</remarks>
        public int ColumnNumber {
            get { return _columnNumber; }
        }

        /// <summary>
        /// Returns the file name, line number and a trailing space. An error
        /// message can be appended easily. For unknown locations, returns
        /// an empty string.
        ///</summary>
        public override string ToString() {
            StringBuilder sb = new StringBuilder("");

            if (_fileName != null) {
                sb.Append(_fileName);
                if (_lineNumber != 0) {
                    sb.Append(String.Format("({0},{1})", _lineNumber, _columnNumber));
                }
                sb.Append(":");
            }

            return sb.ToString();
        }
    }
}
