This file documents the process of converting the 15 or so WebFolders 
NSE files into a single MSI package file.  First some definitions...

NSE         == Name Space Extension (a shell namespace extension).
MSI         == Microsoft Installer (darwin).
Package     == Database file used to control MSI installs.  
Components  == Various DLLs, REG files etc associated with an MSI install.
msiedit.exe == Editor for MSI database files (i.e. webfldrs.msi)
msiinfo.exe == Tool for setting MSI database file values.
msi.hlp     == Help file for MSI stuff.

... then a caveat from me (brianau)...

My goal is to have the Office team generate a single MSI package file
containing all the necessary component files, enlist in our source tree
and check in their binary whenever they make changes.  Currently, we just
get this set of files and have to do it ourselves.  This file documents
how to do it.


Here are the steps:

Step 1 (Get files from Office team) -------------------------------------------

Obtain new MSI package file and component sources from 
David Switzer (davidswi).  These files come organized in the following tree:

    webfldrs.msi
    PFiles\Common\MSShared\webfldrs\msonsext.dll
    PFiles\Common\MSShared\webfldrs\msows409.dll
    PFiles\Common\MSShared\webfldrs\pubplace.htt
    PFiles\Common\MSShared\webfldrs\ragent.dll
    PFiles\Common\MSShared\webfldrs\ragent.tlb
    PFiles\Common\MSShared\webfldrs\msonsext.dll
    PFiles\Common\MSShared\WebSrvEx\bin\fp4anwi.dll
    PFiles\Common\MSShared\WebSrvEx\bin\fp4autl.dll
    PFiles\Common\MSShared\WebSrvEx\bin\fp4awec.dll
    PFiles\Common\MSShared\WebSrvEx\bin\fp4awel.dll
    PFiles\Common\System\Oledb\msdadc.dll
    PFiles\Common\System\Oledb\msdaipp.dll
    PFiles\Common\System\Oledb\msdapml.dll
    PFiles\Common\System\Oledb\msdaurl.dll

Step 2 (Create CAB file from MSI components) ----------------------------------

Feed this script to MAKECAB to generate a single CAB file containing the
14 component files.  Run MAKECAB with the default directory set to the
PFiles directory or modify the script accordingly.

IMPORTANT: Order of the files in this DDF files is CRITICAL.  Files
           MUST be listed in the order of the corresponding "Sequence"
           number in the "File" table in the MSI database.  Use msiedit
           to view the MSI content.

C:\PFiles> MAKECAB /f webfldrs.ddf

    ;
    ;*** WebFolders NSE CAB directive file (webfldrs.ddf).
    ;
    .OPTION EXPLICIT   ; Generate errors on variable typos

    .Set Cabinet=on
    .Set Compress=on
    .Set MaxDiskSize=0 ; Storing on hard drive/CDROM

    "common\MSShared\WebSrvEx\vers40\bin\fp4autl.dll" "fp4autl.dll"
    "common\MSShared\WebSrvEx\vers40\bin\fp4awel.dll" "fp4awel.dll"
    "common\MSShared\WebSrvEx\vers40\bin\fp4awec.dll" "fp4awec.dll"
    "common\MSShared\WebSrvEx\vers40\bin\fp4anwi.dll" "fp4anwi.dll"
    "common\System\Oledb\msdadc.dll" "msdadc.dll"
    "common\MSShared\webfldrs\msows409.dll" "msows409.dll"
    "common\System\Oledb\msdaipp.dll" "msdaipp.dll"
    "common\System\Oledb\msdapml.dll" "msdapml.dll"
    "common\System\Oledb\msdaurl.dll" "msdaurl.dll"
    "common\MSShared\webfldrs\msonsext.dll" "msonsext.dll"
    "common\MSShared\webfldrs\pubplace.htt" "pubplace.htt"
    "common\MSShared\webfldrs\ragent.dll" "ragent.dll"
    "common\MSShared\webfldrs\ragent.tlb" "ragent.tlb"
    ;*** <the end>


Step 3 (Make changes to MSI database file)-------------------------------------

Open the MSI database using msiedit.exe and make the following 
changes:

a) Open the "Media" table.  Enter the following text string in the
   "Cabinet" field of the first record (there should only be 1 record).

        #Icon.WebfldrsCabFile

b) Open the "Icon" table.  Create a new record and enter the following string
   in the "Name" field.
   
        "WebfldrsCabFile".

c) Highlight the "Data" field of this new record and right click.  Select the
   "Import Stream" command.  In the FileOpen dialog, find the CAB file generated
   in step 2 and select it.  Select "Open" to import the CAB file.  This will
   import the CAB file into the MSI package file so that we have only a single
   file to deal with in NT setup.

d) Save the changes and close msiedit.exe


Step 4 (Change Source Type Summary) -------------------------------------------

Issue the following command:

    msiinfo webfldrs.msi /W 2

This will tell Darwin to find the component files inside the MSI package.


Step 5 (Check in the new MSI package) -----------------------------------------

Check out the current MSI package file.
Check in the new MSI package file.


Step 6 (Install the MSI package) ----------------------------------------------

To install the MSI package, call the MsiInstallProduct() API with the path
to the MSI package file.  

