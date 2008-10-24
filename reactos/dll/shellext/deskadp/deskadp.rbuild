<module name="deskadp" type="win32dll" baseaddress="${BASEADDRESS_DESKADP}" installbase="system32" installname="deskadp.dll" unicode="yes">
	<importlibrary definition="deskadp.spec" />
	<include base="deskadp">.</include>
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ole32</library>
	<library>uuid</library>
	<file>deskadp.c</file>
	<file>shxiface.c</file>
	<file>deskadp.rc</file>
	<pch>precomp.h</pch>
</module>
