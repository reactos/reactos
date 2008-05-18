using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Reflection;

namespace TechBot.Library
{
    public class CommandFactory
    {
        private static CommandBuilderCollection m_Commands = new CommandBuilderCollection();

        private CommandFactory()
        {
        }

        public static void LoadPlugins()
        {
            //get the file names of the dll files in the current directory.
            foreach (string fileName in Directory.GetFiles(Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location), "*.dll"))
            {
                LoadPluginsFromDLLFile(fileName);
            }
        }

        private static void LoadPluginsFromDLLFile(string sFile)
        {
            Assembly assPlugin = Assembly.LoadFile(sFile);

            Console.WriteLine("Loading plugins from : {0}", assPlugin.Location);

            if (assPlugin != null)
            {
                foreach (Type pluginType in assPlugin.GetTypes())
                {
                    if (pluginType.IsSubclassOf(typeof(Command)))
                    {
                        if (pluginType.IsAbstract == false)
                        {
                            CommandBuilder cmdBuilder = new CommandBuilder(pluginType);

                            Console.WriteLine("{0}:{1}", 
                                cmdBuilder.Name,
                                cmdBuilder.Description);

                            //Add it to the list.
                            Commands.Add(new CommandBuilder(pluginType));
                        }
                    }
                }
            }
        }

        public static CommandBuilderCollection Commands
        {
            get { return m_Commands; }
        }
    }
}
