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
        public ConsoleTechBotService()
            : base(new ConsoleServiceOutput())
        {
            System.Console.WriteLine("TechBot running console service...");
        }

        public override void Run()
        {
            //Call the base class
            base.Run();

            while (true)
            {
                InjectMessage(System.Console.ReadLine());
            }
        }
    }
}
