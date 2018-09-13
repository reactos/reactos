VERY IMPORTANT:  The master sources are kept on the Win95 sources tree.
Make sure all fixes to code here are progated back to the Win95 source tree.

Win95 Sources:
\\Trango\Future   %ROOT%\win\shell\applets\systray

Contacts:
    Bob Day      - for NT Shell issues
    Chris Guzak  - for Win95 Shell issues
    Tracy Sharpe - Wrote the orginal systray


This applet (systray.exe) is responsible for putting up the following icons
on the system tray (TASKBAR)

Volume Control service:  Launch Volume Mixer (sndvol32.exe) or Sound Mapper Tab
Power (Battery) service: Power status for battery powered portables
PCMCIA service:          PCMCIA services icon


To Do:

1.  The following include files are currently in the local directory
    but eventually need to be moved to the correct NT 
    include directories.

    systrayp.h   // Related to all specific System Tray icon services
    help.h       // Help ID's
    pccrdapi.h   // Related to Power (battery) service
    pbt.h        // Related to Power (battery) service
    pwrioctl.h   // Related to Power (battery) service
    vpowerd.h    // Related to Power (battery) service

2.  The IOCTL's in power.c need to get replaced by the
    more appopriate GetSystemPowerStatus API assuming this gets
    ported to NT.

3.  There are several Win95 assumptions in pccard.c accessing pccard.vxd
    that will have to be fixed for NT assuming pccard.vxd gets ported
    as a NT driver.

