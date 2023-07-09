BVT Check-in Test Cases for ShellUrl parsing:

1. Absolute Shell Urls:
===========================================
1.1 Navigation:
-------------------------------------------
desktop
desktop/
/desktop
/desktop/
\desktop
\desktop/
desktop
desktop/
/desktop
/desktop/
\desktop
\deskTop/
My compUter    <Asserts?>
My compUter/
/My compUter
/My compUter/
\My compUter
\My compUter/
My compUter
My compUter/
/My compUter
/My compUter/
\My compUter
\My compUter/
My compUter/Control Panel
E:\
E:\dir1
E:\dir1\
C:\Program Files
C:\Program Files\
\\bryanst2\test
\\bryanst\test
\\bryanst2
\\bryanst2\
\\zekelsvr\     (Check for Perf)

1.2 ShellExec:
-------------------------------------------
My compUter/Control Panel/Mouse
My compUter/Control Panel/Add/Remove Programs
My compUter/E:\dir1\batch.bat
My compUter/E:\dir1\batch.bat Arg1 Arg2
E:\dir1\batch.bat           (batch.bat is available from \\bryanst2\test)
E:\dir1\batch.bat Arg1 Arg2
E:\Program Files\batch.bat
E:\Program Files\batch.bat Arg1 Arg2
\\bryanst2\test\dos\debug\dos
\\bryanst2\test\dos\debug\dos.exe
\\bryanst2\test\simp\debug\simp.exe
\\bryanst2\test\simp\debug\simp Arg1 Arg2
\\bryanst2\test\simp\debug\simp.exe Arg1 Arg2


2. Shell Urls Relative to the Current Working Directory (CWD):
   NOTE: This is only available to AddressBars that are connected
         to a browser window.
===========================================
2.1 Navigation:
-------------------------------------------
..
../
..\.. (CWD=My Computer\Control Panel)
..\.. (CWD=http://bryanst2/)
..\.. (CWD=E:\)
..\printers (CWD=My Computer\Control Panel)
..\Program Files\ (CWD=E:\dir1)
Program Files\ (CWD=E:\)
..\..\..\Printers\..\Control Panel\Mouse (CWD: \My compUter/C:\windows\system\)


2.2 ShellExec:
-------------------------------------------
Mouse (CWD=My Computer/Control Panel)
../Control Panel\Mouse (CWD=My Computer/Printers)
Add/Remove Programs (CWD=My Computer/Control Panel)
../Control Panel/Add/Remove Programs  (CWD=My Computer/Printers)
batch.bat  (CWD=E:\dir1)
batch.bat Arg1 Arg2  (CWD=E:\dir1)
..\batch.bat  (CWD=E:\dir1\dir2)
..\batch.bat Arg1 Arg2  (CWD=E:\dir1\dir2)



3. Shell Urls Relative to a Path Directory ("Desktop"; "Desktop/My Computer"):
===========================================
3.1 Navigation:
-------------------------------------------
Printers
Control Panel
My Computer
My Computer/
My Computer/Control Panel

3.2 ShellExec:
-------------------------------------------
My Computer/Control Panel/Mouse
Control Panel/Mouse
My Computer/Control Panel/Add/Remove Programs
Control Panel/Add/Remove Programs
My Computer\E:\dir1\batch.bat
My Computer\E:\dir1\batch.bat Arg1 Arg2



4. Other URLs/AutoSearch:
===========================================
http://bryanst2/
ie40
//bryanst
ftp://ftp.microsoft.com
file://E:\dir1
file://\\bryanst2\public

One Two
? Three Four
go Five Six
? SevenEight
go NineTen



5. PERF
===========================================
NOTE: Iterating through ISFs that contain a large number of
      items or folders takes a long time.  For this reason,
      we special case the File System and UNC (Network Neighborhood)
      sections of the Shell Name Space to parse in quicker
      method.  This is possible because those items
      cannot contain '\' chars. 

Make the following Shell Folders and items:
C:\winnt\system32\one two.exe
C:\winnt\system32\one two three (Directory)
C:\winnt\system32\one two three four.exe

Enter the following commands:
C:\winnt\system32\one two Arg1 Arg2
C:\winnt\system32\one two three  (Okay if "one two.exe" is launched because of ambiguity)
C:\winnt\system32\one two three\ (Folder should open)
C:\winnt\system32\one two three four Arg1 Arg2
C:\winnt\system32\Zoo Zed Zad  (File doesn't exist, and it will cause navigation)
