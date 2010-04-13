using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;

using Meebey.SmartIrc4net;

namespace SysGen.RBuild.IRCBot
{
    class ClientDemo
    {
        private CommandCollection m_Commands = new CommandCollection();
        private IrcClient m_IrcClient = new IrcClient();
        private string m_Server = "chat.freenode.net";
        private int m_Port = 7000;

        private static void Main()
        {
            ClientDemo demo = new ClientDemo();
        }

        public ClientDemo()
        {
            m_Commands.Add(new WhoIsCommand());

            m_IrcClient.OnConnected += new EventHandler(OnConnected);
            m_IrcClient.OnJoin += new JoinEventHandler(irc_OnJoin);
            m_IrcClient.OnChannelMessage += new IrcEventHandler(OnChannelMessage);
            m_IrcClient.OnError += new ErrorEventHandler(irc_OnError);
            m_IrcClient.OnRawMessage += new IrcEventHandler(irc_OnRawMessage);

            try
            {
                m_IrcClient.Connect(m_Server, m_Port);
            }
            catch (Exception e)
            {
                Console.Write("Failed to connect: {0}", e.Message);
                Console.ReadKey();
            }
        }

        private void irc_OnRawMessage(object sender, IrcEventArgs e)
        {
            Console.WriteLine("Received: " + e.Data.RawMessage);
        }

        private void irc_OnError(object sender, ErrorEventArgs e)
        {
            Console.WriteLine("Error: " + e.ErrorMessage);
        }

        private void OnChannelMessage(object sender, IrcEventArgs e)
        {
            Console.WriteLine(e.Data.Type + ":");
            Console.WriteLine("(" + e.Data.Channel + ") <" + e.Data.Nick + "> " + e.Data.Message);

            m_IrcClient.SendMessage(SendType.Message, e.Data.Nick, "test");
        }

        private void irc_OnJoin(object sender, JoinEventArgs e)
        {
        }

        private void OnConnected(object sender, EventArgs e)
        {
            m_IrcClient.Login("RBuildBot", "RBuild Info BOT");
            m_IrcClient.RfcJoin("#testchannel");
            m_IrcClient.Listen();
        }
    }
}