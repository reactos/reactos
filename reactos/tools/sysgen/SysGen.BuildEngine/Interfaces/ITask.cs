using System;

namespace SysGen.BuildEngine
{
    public interface ITask : IElement
    {
        void Execute();
        bool FailOnError { get; set; }
        bool IfDefined { get; set; }
        bool IfNotDefined { get; set; }
        string LogPrefix { get; }
        string Name { get; }
        void PostExecute();
        void PreExecute();
        string ToString();
        bool Verbose { get; set; }
    }
}
