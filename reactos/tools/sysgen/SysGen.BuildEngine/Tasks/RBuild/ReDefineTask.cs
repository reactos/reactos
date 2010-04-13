using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("redefine")]
    public class ReDefineTask : Task
    {
        protected override void ExecuteTask() 
        {
//            RBuildElement.Defines.Add(new RBuildDefine(DefineName, DefineValue));
        }
    }
}