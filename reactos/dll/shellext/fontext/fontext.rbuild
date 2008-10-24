<module name="fontext" type="win32dll" baseaddress="${BASEADDRESS_FONTEXT}" installbase="system32" installname="fontext.dll" unicode="yes">
	<importlibrary definition="fontext.spec" />
	<include base="fontext">.</include>
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>ole32</library>
	<library>uuid</library>
	<library>shlwapi</library>
	<library>lz32</library>
	<library>advapi32</library>
	<library>setupapi</library>
	<file>fontext.c</file>
	<file>regsvr.c</file>
	<file>fontext.rc</file>
	<file>fontext.spec</file>
</module>
