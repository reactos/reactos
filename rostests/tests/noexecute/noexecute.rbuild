<module name="noexecute" type="win32cui" installbase="bin" installname="noexecute.exe">
	<define name="__USE_W32API" />
	<library>pseh</library>
	<library>kernel32</library>
	<library>user32</library>
	<file>noexecute.c</file>
</module>
