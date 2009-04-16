<module name="dirdlg" type="win32gui" installbase="bin" installname="dirdlg.exe">
	<define name="__USE_W32API" />
	<include base="dirdlg">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>dirdlg.c</file>
	<file>dirdlg.rc</file>
</module>
