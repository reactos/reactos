<module name="vgafontedit" type="win32gui" installname="vgafontedit.exe" unicode="yes">
	<include base="vgafontedit">.</include>
	
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comdlg32</library>
	
	<file>aboutdlg.c</file>
	<file>editglyphdlg.c</file>
	<file>fontboxeswnd.c</file>
	<file>fontwnd.c</file>
	<file>main.c</file>
	<file>main.rc</file>
	<file>mainwnd.c</file>
	<file>misc.c</file>
	<file>opensave.c</file>
	<pch>precomp.h</pch>
</module>
