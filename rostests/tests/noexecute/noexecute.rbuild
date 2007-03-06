<module name="noexecute" type="win32cui" installbase="bin" installname="noexecute.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<library>pseh</library>
	<library>kernel32</library>
	<library>user32</library>

	<file>noexecute.c</file>
</module>
