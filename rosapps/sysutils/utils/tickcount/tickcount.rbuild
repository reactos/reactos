<module name="tickcount" type="win32cui" installbase="bin" installname="tickcount.exe" unicode="true" >
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>msvcrt</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>tickcount.c</file>
</module>