<module name="dirdlg" type="win32gui" installbase="bin" installname="dirdlg.exe">
	<include base="dirdlg">.</include>

	<!-- FIXME: workarounds until we have a proper oldnames library -->
	<define name="chdir">_chdir</define>

	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>dirdlg.c</file>
	<file>dirdlg.rc</file>
</module>
