
This sample demonstrates one way that a Windows NT kernel-mode device driver
can share and explicitly signal an Event Object with a Win32 application.
It is composed of two parts, a Windows NT kernel-mode device driver and a Win32 
console test application. Both are built using the Windows NT DDK.


Instructions:
-------------

1) Build the driver and test application in either the FREE or CHECKED build environment:

        BLD

    Both the driver and application are put in %NTDDK%\LIB\*\FREE | CHECKED on your build machine.


2) Copy the newly built driver to your Target machine's %SystemRoot%\system32\drivers
directory. Copy the newly built application to your target machine.
Also copy the EVENT.INI file to your Target machine.


3) Update the Target machine's Registry by running REGINI.EXE on the EVENT.INI file, i.e.:

        REGINI EVENT.INI

   This adds a driver key under the 
   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services tree in the Registry.
   You can verify this by running REGEDIT32.EXE and looking at the \Event key.


4) Reboot the Target machine for the Registry changes to take effect.
Your driver will not load until you reboot.


5) Load the driver from the command line:

        NET START EVENT


6) Run the test app from the command line:

        EVENT <DELAY>

            where DELAY = time to delay the Event signal in seconds.


7) Unload the driver from the command line:
    
        NET STOP EVENT
