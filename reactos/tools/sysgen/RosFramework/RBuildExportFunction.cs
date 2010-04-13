using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public enum CallingConventionType
    {
        None,
        StdCall,
        Pascal,
        VarArgs
    }

    public class RBuildExportFunction
    {
        private string m_Ordinal;
        private string m_FunctionName;
        private CallingConventionType m_CallingConvention = CallingConventionType.None;
        private bool m_Stub;

        public string Ordinal
        {
            get { return m_Ordinal; }
            set { m_Ordinal = value; }
        }

        public string FunctionName
        {
            get { return m_FunctionName; }
            set { m_FunctionName = value; }
        }

        public CallingConventionType CallingConvention
        {
            get { return m_CallingConvention; }
            set { m_CallingConvention = value; }
        }

        public bool IsStub
        {
            get { return m_Stub; }
            set { m_Stub = value; }
        }
    }
}
