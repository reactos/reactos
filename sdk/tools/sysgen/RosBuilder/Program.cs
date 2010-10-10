using System;
using System.Reflection;
using System.Collections.Generic;
using System.Windows.Forms;

using TriStateTreeViewDemo;

namespace TriStateTreeViewDemo
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            MainForm mainForm = new MainForm();

            if (args.Length == 0)
            {
                //Create file associations from .dnml files to the script engine
                CreateFileAssociation();
            }
            else if (args[0].ToLower() == "remove")
            {
                //Remove file associations for .dnml files to the script engine
                RemoveFileAssociation();
            }
            else
            {
                mainForm.ProjectController.Open(args[0]);

                if (args.Length > 1)
                {
                    if (args[1].ToLower() == "x86")
                    {
                        MessageBox.Show("86");
                    }
                    else if (args[1].ToLower() == "ppc")
                    {
                        MessageBox.Show("ppc");
                    }
                    else if (args[1].ToLower() == "arm")
                    {
                        MessageBox.Show("arm");
                    }
                    else
                    {
                        MessageBox.Show("Unknown Architecture");
                    }

                    if (args.Length > 2)
                    {
                        if (args[2].ToLower() == "debug")
                        {
                            mainForm.ProjectController.SysGenProject.Debug = true;
                        }
                        else if (args[2].ToLower() == "release")
                        {
                            mainForm.ProjectController.SysGenProject.Debug = false;
                        }
                        else
                        {
                            MessageBox.Show("Unknown Target Mode");
                        }
                    }
                }
            }

            //Run the application
            Application.Run(mainForm);
        }

        internal static void CreateFileAssociation()
        {
            FileAssociation FA = new FileAssociation();
            FA.Extension = "sgpd";
            FA.ContentType = "application/sysgenproject";
            FA.FullName = "SysGenProject";
            FA.ProperName = "SysGenProject";
            FA.AddCommand("open", "\"" + Assembly.GetExecutingAssembly().Location + "\" \" %1\"");
            FA.AddCommand("edit", "notepad.exe %1");
            FA.AddCommand("Generate x86", "\"" + Assembly.GetExecutingAssembly().Location + "\" \" %1\" X86 RELEASE");
            FA.AddCommand("Generate x86 [DEBUG]", "\"" + Assembly.GetExecutingAssembly().Location + "\" \" %1\" X86 DEBUG");
            FA.IconPath = Assembly.GetExecutingAssembly().Location;
            FA.IconIndex = 0;
            FA.Create();
        }

        internal static void RemoveFileAssociation()
        {
            FileAssociation FA = new FileAssociation();
            FA.Extension = "sgpd";
            FA.ContentType = "application/sysgenproject";
            FA.FullName = "SysGenProject";
            FA.ProperName = "SysGenProject";
            FA.AddCommand("open", Assembly.GetExecutingAssembly().Location + " %1");
            FA.AddCommand("edit", "notepad.exe %1");
            FA.AddCommand("edit123", "notepad.exe %1");
            FA.IconPath = Assembly.GetExecutingAssembly().Location;
            FA.IconIndex = 0;
            FA.Remove();
        }
    }
}