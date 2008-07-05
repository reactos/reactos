<module name="objdir" type="win32cui" installbase="bin" installname="objdir.exe">
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>msvcrt</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>objdir.c</file>
</module>