<module name="netid" type="win32dll" baseaddress="${BASEADDRESS_NETID}" installbase="system32" installname="netid.dll" unicode="true">
	<importlibrary definition="netid.spec.def" />
	<include base="netid">.</include>
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
	<file>netid.c</file>
	<file>netid.rc</file>
	<file>netid.spec</file>
</module>
