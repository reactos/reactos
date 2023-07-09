
PRS SIGNING TOOLS INITIAL SETUP
-------------------------------

1.  Set up directories

        The signing batch files rely on the following directory structure:

            TOOLS:
            \cryptsdk

            WORKING DIRECTORY:
            \newworking

2.  Copy the command files to the "WORKING DIRECTORY":

        copy \\pberkman1\ispu\pkitools\prs\*.bat \newworking\.


3.  Install CryptSYS.EXE
        
        This is the Digital Security Client files.  It will prompt you 
        to re-boot.


4.  Install CrypTool.EXE

        This is the Digital Security Tools install.  Take the default 
        directory (c:\cryptsdk).



PRS SIGNING TOOLS USAGE
-----------------------

1.  Using the system with mutliple subdirectories under the "WORKING DIRECTORY" (PRS lab):

        a.  Copy the directories from \\prslab\unsigned to "WORKING DIRECTORY"
                
                xcopy \\prslab\unsigned\*.* \newworking\*.* /s /v /e /z

        b.  From the "WORKING DIRECTORY", run the command file to sign multiple 
            directories.

                cd /D \newworking
                startsgn.bat    (for verbose output, add the '-v' flag)

        c.  From the "WORKING DIRECTORY", run the command file to check multiple 
            directories.

                cd /D \newworking
                startchk.bat    (for verbose output, add the '-v' flag)


2.  Using the system to sign a single directory (for test signing):

        a.  Create a directory on your local machine.

        b.  Copy the directory from \\prslab\unsigned to your local directory

                xcopy \\prslab\unsigned\{my directory}\*.*  c:\{my directory}\*.* /s /v /e /z

        c.  From the local directory, run the command file to sign a single directory

                cd /D c:\{my directory}
                \\prslab\unsigned\tools\signfiles.bat -T

        d.  From the local directory, run the command to verify each file

                cd /D c:\{my directory}
                c:\cryptsdk\bin\chktrust file_name
