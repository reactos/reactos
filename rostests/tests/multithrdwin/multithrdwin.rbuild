<module name="multithrdwin" type="win32gui" installbase="bin" installname="multithrdwin.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>ntdll</library>
	<file>multithrdwin.c</file>
</module>
