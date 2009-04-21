<module name="kbdlayout" type="win32gui" installbase="system32" installname="kbdlayout.exe">
	<include base="kbdlayout">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>kbdlayout.c</file>
	<file>kbdlayout.rc</file>
</module>
