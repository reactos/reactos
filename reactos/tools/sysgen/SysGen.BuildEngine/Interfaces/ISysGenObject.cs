using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine
{
    /// <summary>
    /// Represent one of the root element object types for SysGen : Project and Module
    /// </summary>
    public interface ISysGenObject
    {
        RBuildElement RBuildElement { get; }
    }

    public interface ISysGenObjectFileContainer
    {
        RBuildFileCollection Files { get; }
    }
}