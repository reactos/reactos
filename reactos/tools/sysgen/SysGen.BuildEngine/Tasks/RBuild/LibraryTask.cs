using System;
using SysGen.BuildEngine.Attributes;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("library")]
    public class LibraryTask : ValueBaseTask
    {
        /// <summary>
        /// The define value.
        /// </summary>
        [TaskValue(Required=true)]
        public override string Value { get { return _value; } set { _value = value; } }

        protected override void ExecuteTask()
        {
            RBuildModule libModule = Project.Modules.GetByName(Value);

            if (libModule == null)
                throw new BuildException("Unknown library dependency '{0}' referenced by module '{1}'", Value, Module.Name);

            if (Module.Host != libModule.Host)
                throw new BuildException("Module '{0}' is trying to link against library '{1}' but can't mix target and hosts",
                    Module.Name,
                    libModule.Name);

            if ((libModule.Type != ModuleType.NativeDLL) &&
                (libModule.Type != ModuleType.Win32DLL) &&
                (libModule.Type != ModuleType.StaticLibrary) &&
                (libModule.Type != ModuleType.ObjectLibrary) &&
                (libModule.Type != ModuleType.Kernel) &&
                (libModule.Type != ModuleType.KernelModeDLL) &&
                (libModule.Type != ModuleType.KernelModeDriver) &&
                (libModule.Type != ModuleType.KeyboardLayout) &&
                (libModule.Type != ModuleType.RpcServer) &&
                (libModule.Type != ModuleType.RpcClient) &&
                (libModule.Type != ModuleType.RpcProxy) &&
                (libModule.Type != ModuleType.HostStaticLibrary))
            {
                throw new BuildException("Module '{0}' is trying to use Module '{1}' as a library but it is a '{2}'",
                    Module.Name,
                    libModule.Name,
                    libModule.Type);
            }

            Module.Libraries.Add(libModule);
        }
    }
}
