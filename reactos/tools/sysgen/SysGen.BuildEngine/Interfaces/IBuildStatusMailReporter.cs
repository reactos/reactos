using System;

namespace SysGen.BuildEngine
{
    public interface IBuildStatusMailReporter
    {
        string MailAdress { get; }
        string Subject { get; }
    }
}
