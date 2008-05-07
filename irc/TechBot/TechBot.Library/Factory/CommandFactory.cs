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
            FileInfo objExeInfo = new FileInfo(@"C:\Ros\current\irc\TechBot\TechBot.Console\bin\Debug\");

            foreach (FileInfo objInfo in objExeInfo.Directory.GetFiles("*.dll"))
            {
                LoadPluginsFromDLLFile(objInfo.FullName);
            }
        }

        private static void LoadPluginsFromDLLFile(string sFile)
        {
            Assembly assPlugin = Assembly.LoadFile(sFile);

            if (assPlugin != null)
            {
                foreach (Type pluginType in assPlugin.GetTypes())
                {
                    if (pluginType.IsSubclassOf(typeof(Command)))
                    {
                        if (pluginType.IsAbstract == false)
                        {
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
