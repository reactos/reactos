// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: This class represents a temporary directory, which
//              is cleared and deleted on dispose.
//
//------------------------------------------------------------------------------

namespace MS.Internal.Csp
{
    using System;
    using System.IO;

    internal class TempDirectory : IDisposable
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        public TempDirectory()
        {
            _dir = GetUniqueTempDirectory();
        }

        ~TempDirectory()
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

        //+---------------------------------------------------------------------
        //
        //  Function: SetToLeak
        //
        //  Synopsis: Sets the TempDirectory to "leak" - i.e. not clean up on
        //            Dispose.
        //
        //  Notes: This is needed when the TempDirectory is to hold a compiled
        //         assembly, and we're debugging under Rascal.
        //
        //         In that case, someone (either Rascal or the CLR) holds onto
        //         either the .exe or .pdb (or both), so that when Dispose runs,
        //         it is "too early" to delete the directory (and the attempt
        //         would throw an exception).
        //
        //----------------------------------------------------------------------

        public void SetToLeak()
        {
            _leak = true;
        }
        #endregion Public Methods

        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------
        
        #region Public Properties
        public string PathName
        {
            get
            {
                return _dir;
            }
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
                if (!_leak)
                {
                    if (_dir != "")
                    {
                        Directory.Delete(_dir, /* recursive = */ true);
                        _dir = "";
                    }
                }

                disposed = true;         
            }
        }
        #endregion Protected Methods


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods
        //+-----------------------------------------------------------------------------
        //
        //  Member:    GetUniqueTempDirectory
        //
        //  Synopsis:  The CLR only provides a reliable way to get a uniquely-named
        //             temporary file. This method uses that to build a mostly-reliable way
        //             to get a uniquely-named temporary directory.
        //
        //------------------------------------------------------------------------------

        private static string GetUniqueTempDirectory()
        {
            string root = Path.GetTempPath();
            string ret = null;

            for (int iAttempts=0; true; iAttempts++)
            {
                // Get a temporary file. (i.e. let the CLR do the work of finding an
                // unused name).

                string file = Path.GetTempFileName();
                try
                {
                    // Now delete the file and create a directory of the same name, instead.

                    File.Delete(file);
                    ret = file;

                    // Small time window when someone else could create a file/directory of the
                    // same name.

                    Directory.CreateDirectory(ret);
                }
                catch (Exception)
                {
                    // If this ever happens, we expect it's because someone else
                    // created a file of the same name. So, assume we were
                    // unlucky, and try again.

                    if (iAttempts==10)
                    {
                        // But if we get here, there's probably some other cause
                        // for failure - so give up and report the failure.

                        throw;
                    }
                    continue;
                }
                return ret;
            }
        }
        #endregion Private Methods

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields
        string _dir = "";
        bool disposed = false;
        bool _leak = false;
        #endregion Private Fields
    }
}



