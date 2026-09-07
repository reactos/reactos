// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: This is the "Csp" tool - a C# project 'interpreter'.
//
//              'Interpreter' is one of the two useful things about it. It makes
//              an easy way to "compile and execute" a set of source files,
//              making it feel just like an interpreted language (e.g. Perl).
//              This is good for prototyping.
//
//              The other useful thing is "C# Prime", or "C#'". This is a simple
//              extension to C# which makes it very useful for writing a
//              code-generation tool. (We use it for mil/codegen).
//
//------------------------------------------------------------------------------

namespace MS.Internal.Csp
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.CodeDom.Compiler;

    internal sealed class MainClass
    {
        // Parameters affecting csp.exe as a whole.
        // (For parameters affecting the project, see Project.Parameters)

        private class Parameters
        {
            public bool SuppressExceptions = true;
            public bool BreakBeforeExit = false;
        }

        // Return codes that csp may return. (The project is free to return others.)

        const int _rc_success = 0;
        const int _rc_usageError = 127;
        const int _rc_fileNotFound = 128;
        const int _rc_exitAndReturnSuccess = 129;
        const int _rc_cspProjectException = 130;
        const int _rc_projectBuildFailed = 131;
        const int _rc_projectThrewException = 132;

        //--------------------------------------------------------------------
        //
        // Program Entry Method
        //
        //---------------------------------------------------------------------
        #region Entry

        //
        // Process command-line arguments.
        // Compile and execute the project.
        // Returns 0 if successful.
        //
        static int Main(string[] argsIn)
        {
            Project.Parameters projectParams = null;
            Parameters cspParams = null;

            int returnCode = ProcessArgs(
                argsIn, 
                out projectParams, 
                out cspParams
                );

            if (returnCode != _rc_success)
            {
                if (returnCode == _rc_exitAndReturnSuccess)
                {
                    return _rc_success;
                }
                else
                {
                    return returnCode;
                }
            }

            int projectReturnValue = _rc_projectBuildFailed;

            try
            {
                Project project = Project.Build(projectParams);

                if (project != null)
                {
                    projectReturnValue = project.ExecuteMain(projectParams.MainClass, projectParams.ProjectArgs);
                }
                else
                {
                    // Do nothing. If the project didn't build, we expect "Project" to have
                    // emitted some useful error messages.
                }
            }
            catch (CspProjectException e)
            {
                if (cspParams.SuppressExceptions)
                {
                    WriteErrorLine(e.Message);

                    return _rc_cspProjectException;
                }
                else
                {
                    throw;
                }
            }
            catch (Exception e)
            {
                if (cspParams.SuppressExceptions)
                {
                    WriteErrorLine(e.Message);
                    WriteErrorLine("(To debug, rerun under a debugger, with the -debugMode parameter.)");

                    return _rc_projectThrewException;
                }
                else
                {
                    throw;
                }
            }

            if (cspParams.BreakBeforeExit)
            {
                // Final breakpoint just before csp.exe quits.
                //
                // This is useful for seeing the output in the console window.
                System.Diagnostics.Debugger.Break();
            }

            return projectReturnValue;
        }
        #endregion Entry

        //--------------------------------------------------------------------
        //
        // Private Methods
        //
        //---------------------------------------------------------------------
        #region Private Methods


        //+-----------------------------------------------------------------------------
        //
        //  Function: ProcessArgs
        //
        //  Synopsis: Processes the command-line arguments and returns a structured
        //            form in argsOut.
        //
        //  Return value: A return code; if not _rc_success, caller should quit.
        //                May return _rc_exitAndReturnSuccess, which caller needs
        //                to translate to _rc_success.
        //
        //------------------------------------------------------------------------------

        static private int ProcessArgs(
            string[] argsIn, 
            out Project.Parameters projParamsOut,
            out Parameters cspParamsOut
            )
        {
            projParamsOut = new Project.Parameters();
            cspParamsOut = new Parameters();


            // Convert argsIn to a list

            ArrayList rgsArgs = new ArrayList();
            foreach (string arg in argsIn)
            {
                rgsArgs.Add(arg);
            }

            ArrayList alSourceFiles = new ArrayList();
            ArrayList alReferencedAssemblies = new ArrayList();
            bool noBreakBeforeInvoke = false;
            bool debugMode = false;

            if (rgsArgs.Count > 0)
            {
                string prefix = GetStandardizedPrefix((string) rgsArgs[0]);

                if (   (prefix == "-h")
                    || (prefix == "-?"))
                {
                    _Usage();
                    return _rc_exitAndReturnSuccess;
                }
            }

            //
            // Process the arguments (until we hit "--").
            //

            bool fHitProjectParams = false;

            int idxCurrentArg;

            for (idxCurrentArg = 0; idxCurrentArg<rgsArgs.Count; idxCurrentArg++)
            {
                string arg = ((string) rgsArgs[idxCurrentArg]).Trim();
                string prefix = GetStandardizedPrefix(arg);

                switch (prefix)
                {
                    // response file - a file which contains more arguments for us to parse.
                    case "-rsp:":
                    {
                        string responseFile = arg.Substring(5);

                        if (!File.Exists(responseFile))
                        {
                            WriteErrorLine(String.Format(
                                "Response file '{0}' not found.", responseFile));
                            return _rc_fileNotFound;
                        }

                        // Remove this argument - replace it with the new arguments.

                        rgsArgs.RemoveAt(idxCurrentArg);
                        GetArgsFromFile(responseFile, idxCurrentArg, ref rgsArgs);

                        // Make the next iteration use the same index.
                        idxCurrentArg--;

                        break;
                    }

                    // source file - a .cs file which we will parse and then
                    // compile.
                    case "-s:":
                    {
                        string sourceFile = arg.Substring(3);
                        if (!File.Exists(sourceFile))
                        {
                            WriteErrorLine(String.Format(
                                "Source file '{0}' not found.", sourceFile));
                            return _rc_fileNotFound;
                        }
                        alSourceFiles.Add(sourceFile);
                        break;
                    }

                    case "-r:":
                    {
                        alReferencedAssemblies.Add(arg.Substring(3));
                        break;
                    }

                    case "-clrdir:":
                    {
                        projParamsOut.ClrDir = arg.Substring(8);
                        break;
                    }

                    case "-debugmode":
                    {
                        debugMode = true;
                        continue;
                    }

                    case "-enablecsprime":
                    {
                        projParamsOut.EnableCsPrime = true;
                        continue;
                    }

                    case "-nobreakpoint":
                    {
                        noBreakBeforeInvoke = true;
                        continue;
                    }

                    case "-main:":
                    {
                        if (projParamsOut.MainClass != null)
                        {
                            _Usage();
                            return _rc_usageError;
                        }
                        projParamsOut.MainClass = arg.Substring(6);
                        continue;
                    }

                    case "--":
                    {
                        idxCurrentArg++;
                        fHitProjectParams = true;
                        break;
                    }

                    default:
                    {
                        _Usage();
                        WriteErrorLine(String.Format(
                            "Unrecognized option: '{0}'", arg));
                        return _rc_usageError;
                    }
                }

                if (fHitProjectParams)
                {
                    break;
                }
            }

            if (debugMode)
            {
                if (!noBreakBeforeInvoke)
                {
                    projParamsOut.BreakBeforeInvoke = true;
                }
                cspParamsOut.SuppressExceptions = false;
                cspParamsOut.BreakBeforeExit = true;

                // This will leave files in the %temp% directory.
                // But seems necessary to get symbols.
                projParamsOut.DebugModeHack = true;
            }

            if (alSourceFiles.Count == 0)
            {
                _Usage();
                return _rc_usageError;
            }

            projParamsOut.SourceFiles = (string[]) alSourceFiles.ToArray(typeof(string));
            projParamsOut.ReferencedAssemblies = (string[]) alReferencedAssemblies.ToArray(typeof(string));


            // Set projParamsOut.ProjectArgs, to the parameters after the "--".

            int idxProjectArgs = idxCurrentArg;
            int nProjectArgs = rgsArgs.Count - idxProjectArgs;
            if (nProjectArgs < 0)
            {
                nProjectArgs = 0;
            }

            projParamsOut.ProjectArgs=new string[nProjectArgs];

            if (nProjectArgs > 0)
            {
                rgsArgs.CopyTo(idxProjectArgs, projParamsOut.ProjectArgs, 0, nProjectArgs);
            }

            return _rc_success;
        }


        static private string GetStandardizedPrefix(string arg)
        {
            int prefixLength = arg.IndexOf(':');
            if (prefixLength == -1)
            {
                prefixLength = arg.Length;
            }
            else
            {
                prefixLength++;
            }

            if (prefixLength == 0)
            {
                return "";
            }
    
            string ret = arg.Substring(0, prefixLength);

            // Convert uppercase

            ret = ret.ToLower();

            // Convert initial '/'

            if (ret.Substring(0, 1) == "/")
            {
                ret = "-" + ret.Substring(1);
            }

            return ret;
        }


        //+---------------------------------------------------------------------
        //
        //  Function:  GetArgsFromFile
        //
        //  Synopsis:  Read arguments from the given file and insert them as
        //             strings into the given ArrayList.
        //
        //             The file format puts each argument on a separate line.
        //
        //             Recommended file extension: ".rsp" ("response" file).
        //
        //----------------------------------------------------------------------

        static private void GetArgsFromFile(string fileName, int idxInsertAt, ref ArrayList args)
        {
            using (StreamReader sr = File.OpenText(fileName))
            {
                String inputLine;

                while ((inputLine = sr.ReadLine()) != null)
                {
                    if (inputLine.Trim() != "")
                    {
                        args.Insert(idxInsertAt, inputLine);
                        idxInsertAt++;
                    }
                }
            }
        }


        static private void WriteErrorLine(string s)
        {
            // This "csp(0) : error :" makes build.exe emit the error to the console.
            // Otherwise, it just swallows it and you have to look in build.log.

            Console.Error.WriteLine("\ncsp(0) : error : " + s);
            Debug.WriteLine(s);
        }

        static private void WriteWarning(string s)
        {
            Console.Error.WriteLine("\ncsp(0) : warning : " + s);
            Debug.WriteLine(s);
        }

        static private void _DisplayLogo()
        {
            string mcgPath = System.Reflection.Assembly.GetExecutingAssembly().Location;
            FileVersionInfo mcgFileVersionInfo = FileVersionInfo.GetVersionInfo(mcgPath);

            string avalonFileVersion = mcgFileVersionInfo.FileVersion;

            string avalonVersion = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();

            string clrVersion = "v" + Environment.Version.Major.ToString() + "." + Environment.Version.Minor.ToString() + "." + Environment.Version.Build.ToString();

            //string clrAssemVersion = typeof(System.Object).Assembly.GetName().Version.ToString();

            Console.WriteLine();
            Console.WriteLine("Microsoft (R) Windows C#/C#' Project Utility version " + avalonFileVersion);
            Console.WriteLine("for Microsoft (R) .NET Framework " + clrVersion);
            Console.WriteLine();
        }

        static private void _Usage()
        {
            _DisplayLogo();

            // display usage
            Console.WriteLine(
@"Usage: Csp -rsp:<file> -s:<file> -r:<file> [...] [-- <project args>]

'Interpreter' for C#/""C# prime"" projects. Compiles a project, then runs 
it with the given arguments.

Required args:
-rsp:<file>      A file from which other parameters will be read, and inserted
                 into the command line at the current position.
-s:<file>        A source file in the project. Can specify multiple.
-r:<file>        A referenced assembly for the project. Can specify multiple.

Optional args:
--               Separator. Args after this are passed to the project.

-debugMode       Enable ""debug mode"". This:
                   1) Leaves files in %temp%. (Seems unavoidable.)
                   2) Enables an initial breakpoint, just before the project's
                      Main method.
                   3) Enables a final breakpoint, just before csp.exe exits.
                   4) Allows exceptions from building & executing the project,
                      to be propagated. Then the debugger can catch them.

-enableCsPrime   Enable ""C# prime"" parsing - a language extension.

-main:<class>    Specifies the class which contains the Main method.
                 (Only needed when multiple Main methods are present.)

-clrdir:<dir>    Location of the CLR (used for implicit referenced assemblies).

-h or -?         This help.

Optional arg for -debugMode:
-noBreakpoint    Suppress the initial breakpoint.
");
        }

        #endregion
    }
}



