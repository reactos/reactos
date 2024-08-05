using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Reflection;

namespace Lextm.SharpSnmpLib.Mib
{
    public interface IMibResolver
    {
        IModule Resolve(string moduleName);
    }

    public class FileSystemMibResolver : IMibResolver
    {
        private string _path;
        private bool _recursive;

        public FileSystemMibResolver(string path, bool recursive)
        {
            _path = path;
            _recursive = recursive;
        }

        #region IMibResolver Member

        public IModule Resolve(string moduleName)
        {
            if (Directory.Exists(_path))
            {
                string[] matchedFiles = Directory.GetFiles(
                    _path,
                    "*",
                    (_recursive) ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly);

                if ((matchedFiles != null) && (matchedFiles.Length >= 1))
                {
                    foreach (string matchedFile in matchedFiles)
                    {
						if (Path.GetFileNameWithoutExtension(matchedFile.ToLowerInvariant()) == moduleName.ToLowerInvariant())
						{
							try
							{
								MibDocument md = new MibDocument (matchedFile);
								if (md.Modules.Count > 0)
								{
									return md.Modules [0];
								}
							} catch
							{
							}
						}
                    }                  
                }
            }

            return null;
        }

        #endregion

    }

    // earlier code for search of versioned MIBs:
    //
    //private const string Pattern = "-V[0-9]+$";
    //public static bool AllDependentsAvailable(MibModule module, IDictionary<string, MibModule> modules)
    //{
    //    foreach (string dependent in module.Dependents)
    //    {
    //        if (!DependentFound(dependent, modules))
    //        {
    //            return false;
    //        }
    //    }

    //    return true;
    //}

    //private static bool DependentFound(string dependent, IDictionary<string, MibModule> modules)
    //{
    //    if (!Regex.IsMatch(dependent, Pattern))
    //    {
    //        return modules.ContainsKey(dependent);
    //    }

    //    if (modules.ContainsKey(dependent))
    //    {
    //        return true;
    //    }

    //    string dependentNonVersion = Regex.Replace(dependent, Pattern, string.Empty);
    //    return modules.ContainsKey(dependentNonVersion);
    //}

}
