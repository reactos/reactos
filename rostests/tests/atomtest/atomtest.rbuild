<module name="atomtest" type="win32cui" installbase="bin" installname="atomtest.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>ntdll</library>
	<file>atomtest.c</file>
</module>
