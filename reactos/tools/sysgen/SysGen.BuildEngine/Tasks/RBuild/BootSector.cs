using System;
using System.IO;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("bootsector")]
    public class BootSector : ValueBaseTask
    {
        protected override void ExecuteTask()
        {
            RBuildModule bootModule = Project.Modules.GetByName(Value);

            if (bootModule != null)
            {
                if (bootModule.Type == ModuleType.BootSector)
                {
                    if (Module.Type == ModuleType.Iso ||
                        Module.Type == ModuleType.IsoRegTest ||
                        Module.Type == ModuleType.LiveIso ||
                        Module.Type == ModuleType.LiveIsoRegTest)
                    {
                        if (Module.BootSector == null)
                            Module.BootSector = bootModule;
                    }
                    else
                        throw new BuildException("<bootsector> is not applicable for this module type.", Location);
                }
                else
                    throw new BuildException("<bootsector> for module '{0}' is referencing a non BootSector module '{1}'",
                        Module.Name,
                        bootModule.Name,
                        Location);
            }
            else
                throw new BuildException("<bootsector> for module '{0}' is referencing a non existing module '{1}'",
                    Module.Name,
                    Value,
                    Location);
        }
    }
}
