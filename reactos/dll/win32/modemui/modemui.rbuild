<module name="modemui" type="win32dll" baseaddress="${BASEADDRESS_MODEMUI}" installbase="system32" installname="modemui.dll" unicode="true">
	<importlibrary definition="modemui.spec" />
	<include base="modemui">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<library>netapi32</library>
	<file>modemui.c</file>
	<file>modemui.rc</file>
	<file>modemui.spec</file>
</module>
