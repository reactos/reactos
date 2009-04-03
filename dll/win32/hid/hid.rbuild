<module name="hid" type="win32dll" baseaddress="${BASEADDRESS_HID}" installbase="system32" installname="hid.dll" unicode="yes">
	<importlibrary definition="hid.def" />
	<include base="hid">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>hid.c</file>
	<file>stubs.c</file>
	<file>hid.rc</file>
	<pch>precomp.h</pch>
</module>
