// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: CodeSink which writes to a text file. Translates \n to \r\n.
//              Trims blank lines.
//
//              Also performs "sd add" and "sd edit" commands as appropriate.
//
//              Limitations:
//
//              * No "sd delete" functionality. Right now you need to detect
//                stale files and "sd delete" them manually.
//
//              * It's slow to call sd.exe for each edited file. Workaround:
//                manually "sd edit Blah/..." before running the script. (Then
//                this automation is more to prevent you from forgetting files.)
//

namespace MS.Internal.MilCodeGen.Runtime
{
    using System;
    using System.IO;
    using System.Text;
    using System.Diagnostics;

    public class FileCodeSink : CodeSink, IDisposable
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        public FileCodeSink(string dir, string filename): this(dir, filename, true) { }

        public FileCodeSink(string dir, string filename, bool createDirIfNecessary)
        {
            _filePath = Path.Combine(dir, filename);

            if (createDirIfNecessary)
            {
                Directory.CreateDirectory(dir);
            }
            
            _fileCreated = !File.Exists(_filePath);

            // If the file exists and is readonly attempt to check out the file.
            if (!_fileCreated && ((File.GetAttributes(_filePath) & FileAttributes.ReadOnly) != 0))
            {
                TfFile(_filePath, "edit");
            }

            _streamWriter = new StreamWriter(_filePath, false, Encoding.ASCII);
        }

        ~FileCodeSink()
        {
            Dispose(false);
        }
        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------
        
        #region Public Methods
        public void Dispose()
        {
            GC.SuppressFinalize(this);
            Dispose(true);
        }

        public static void DisableSd()
        {
            _disableSd = true;
        }

        public void TfFile(string filename, string op)
        {
            if (_tfOperation != "")
            {
                throw new Exception("Internal error");
            }

            _tfOperation = op;

            if (_disableSd) return;

            Process tfProcess = new Process();

            tfProcess.StartInfo.FileName = "tf.cmd";
            tfProcess.StartInfo.Arguments = op + " " + filename;
            tfProcess.StartInfo.CreateNoWindow = true;
            tfProcess.StartInfo.UseShellExecute = true;

            tfProcess.Start();

            tfProcess.WaitForExit();

            if (0 != tfProcess.ExitCode)
            {
                throw new ApplicationException("Non-zero return code (" + tfProcess.ExitCode + ") encountered executing:\n"+
                                               tfProcess.StartInfo.FileName + " " + tfProcess.StartInfo.Arguments);
            }
        }

        #endregion Public Methods

        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------
        
        #region Public Properties

        public string FilePath
        {
            get { return _filePath; }
        }

        public string FileName
        {
            get { return Path.GetFileName(_filePath); }
        }

        #endregion Public Properties

        //------------------------------------------------------
        //
        //  Protected Methods
        //
        //------------------------------------------------------

        #region Protected Methods
        protected virtual void Dispose(bool disposing)
        {
            if (!disposed)
            {
                if (disposing)
                {
                    if (_streamWriter != null)
                    {
                        FlushCurrentLine();
                        _streamWriter.Close();
                        _streamWriter = null;
                    }

                    if (_fileCreated)
                    {
                        // If we created the file, we now need to Sd Add it
                        TfFile(_filePath, "add");
                    }

                    LogCreation();
                }

                disposed = true;         
            }
        }
        #endregion Protected Methods
        
        
        //------------------------------------------------------
        //
        //  Protected Internal Methods
        //
        //------------------------------------------------------
        
        #region Protected Internal Methods
        protected internal override void InternalWrite(string output)
        {
            if (output.IndexOf('\r') >= 0) {
                throw new Exception("Internal error");
            }

            string[] lines = output.Split('\n');

            for (int i=0; i<lines.Length; i++)
            {
                _currentLine += lines[i];
                if (i < lines.Length - 1)
                {
                    FlushCurrentLine();
                    _streamWriter.Write("\r\n");
                }
            }
        }
        #endregion
        
        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods
        private void FlushCurrentLine()
        {
            if (_currentLine != "")
            {
                if (_currentLine.Trim() != "")
                {
                    _streamWriter.Write(_currentLine);
                }
                _currentLine = "";
            }
        }

        // Log the creation of this file, and any sd operations performed.
        private void LogCreation()
        {
            string tf = "";
            if (_tfOperation != "")
            {
                tf = " (and 'tf " + _tfOperation + "')";
            }

            // Log the creation of this file
            Console.WriteLine("\tCreated: {0}{1}", _filePath, tf);
        }
        #endregion Private Methods

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields
        static bool _disableSd = false;
        
        StreamWriter _streamWriter;
        bool _fileCreated = true;
        string _tfOperation = "";
        string _filePath;
        bool disposed = false;
        string _currentLine = "";
        #endregion Private Fields
    }
}




