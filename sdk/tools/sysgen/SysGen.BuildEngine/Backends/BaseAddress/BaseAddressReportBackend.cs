using System;
using System.Globalization;
using System.Text.RegularExpressions;
using System.Web.UI;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Tasks;
using SysGen.BuildEngine.Backends;

namespace SysGen.BuildEngine.Backends
{
    public class BaseAddressReportBackend : Backend
    {
        private List<BaseAddressModule> m_Modules = new List<BaseAddressModule> ();

        public class BaseAddressModule
        {
            private FileInfo m_FileInfo = null;
            private RBuildModule m_Module = null;

            public BaseAddressModule(RBuildModule module , string file)
            {
                m_Module = module;
                m_FileInfo = new FileInfo(file);
            }

            public string Name
            {
                get { return m_Module.Name; }
            }

            public string BaseAddress
            {
                get { return m_Module.BaseAddress; }
            }

            public long Size
            {
                get { return m_FileInfo.Length; }
            }

            public string HexBaseAddressStart
            {
                get { return m_Module.BaseAddress.Replace("0x", string.Empty); }
            }

            public long BaseAddressStart
            {
                get { return Int64.Parse(HexBaseAddressStart, NumberStyles.AllowHexSpecifier); }
            }

            public long BaseAddressEnd
            {
                get { return m_FileInfo.Length + BaseAddressStart; }
            }
        }

        public BaseAddressReportBackend(SysGenEngine sysgen)
            : base(sysgen)
        {
        }

        protected override string FriendlyName
        {
            get { return "Base Address Report"; }
        }

        protected override void Generate()
        {
            foreach (RBuildModule module in Project.Modules)
            {
                if (module.Type == ModuleType.Win32DLL ||
                    module.Type == ModuleType.Win32OCX)
                {
                    if (module.BaseAddress != module.DefaultBaseAdress)
                    {
                        BaseAddressModule baseAddressModule = new BaseAddressModule(module , SysGen.ResolveRBuildFilePath (module.TargetFile));

                        Console.WriteLine(baseAddressModule.Name);

                        Console.WriteLine("    {0} {1}", 
                            baseAddressModule.BaseAddressStart,
                            baseAddressModule.BaseAddressEnd);
                        
                        m_Modules.Add (baseAddressModule);
                    }
                }


            }

            using (StreamWriter sw = new StreamWriter(Directory.GetCurrentDirectory() + "\\overlapping.txt"))
            {
                foreach (BaseAddressModule module in m_Modules)
                {
                    foreach (BaseAddressModule testModule in m_Modules)
                    {
                        if (module.Name != testModule.Name)
                        {
                            if ((testModule.BaseAddressStart >= module.BaseAddressStart && testModule.BaseAddressStart <= module.BaseAddressEnd) ||
                                (testModule.BaseAddressEnd >= module.BaseAddressStart && testModule.BaseAddressEnd <= module.BaseAddressEnd) ||
                                (testModule.BaseAddressStart <= module.BaseAddressStart && testModule.BaseAddressEnd >= module.BaseAddressEnd))
                            {
                                sw.WriteLine("- Module '{0}' [size '{1} and base address '{2}' [start:{3} end:{4}] is provably being overlapped by module '{5}' [size '{6} and base address '{7}' [start:{3} end:{8}]",
                                    module.Name,
                                    module.Size,
                                    module.BaseAddress,
                                    module.BaseAddressStart,
                                    module.BaseAddressEnd,
                                    testModule.Name,
                                    testModule.Size,
                                    testModule.BaseAddress,
                                    testModule.BaseAddressStart,
                                    testModule.BaseAddressEnd);
                            }
                        }
                    }
                }
            }
        }
    }
}
