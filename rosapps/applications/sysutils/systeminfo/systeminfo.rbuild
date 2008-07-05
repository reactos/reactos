<module name="systeminfo" type="win32cui" installbase="system32" installname="systeminfo.exe">
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>ntdll</library>
	<library>advapi32</library>
	<file>systeminfo.c</file>
	<file>systeminfo.rc</file>
	<file>rsrc.rc</file>
</module>
