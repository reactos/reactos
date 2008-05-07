using System;
using System.Collections.Generic;
using System.Text;

using TechBot.Library;

namespace TechBot.Console
{
    public class ConsoleServiceOutput : IServiceOutput
    {
        public void WriteLine(MessageContext context,
                              string message)
        {
            System.Console.WriteLine(message);
        }
    }

    public class ConsoleTechBotService : TechBotService
    {
        public ConsoleTechBotService(
                          string chmPath,
                          string mainChm)
            : base(new ConsoleServiceOutput(), chmPath, mainChm)
        {
        }

        public override void Run()
        {
            //Call the base class
            base.Run();

            while (true)
            {
                string s = System.Console.ReadLine();
                InjectMessage(null, s);
            }
        }
    }
}
