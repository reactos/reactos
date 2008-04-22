<module name="slayer" type="win32dll" baseaddress="${BASEADDRESS_SLAYER}" installbase="system32" installname="slayer.dll">
	<importlibrary definition="slayer.def" />
	<include base="slayer">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x601</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>shell32</library>
	<library>uuid</library>
	<file>slayer.c</file>
	<file>slayer.rc</file>
	<pch>precomp.h</pch>
</module>
