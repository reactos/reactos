<module name="hid" type="win32dll" baseaddress="${BASEADDRESS_HID}" installbase="system32" installname="hid.dll">
	<importlibrary definition="hid.def" />
	<include base="hid">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>hid.c</file>
	<file>stubs.c</file>
	<file>hid.rc</file>
	<pch>precomp.h</pch>
</module>
