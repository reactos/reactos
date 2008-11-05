<module name="deskmon" type="win32dll" baseaddress="${BASEADDRESS_DESKMON}" installbase="system32" installname="deskmon.dll" unicode="yes">
	<importlibrary definition="deskmon.spec" />
	<include base="deskmon">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ole32</library>
	<library>uuid</library>
	<file>deskmon.c</file>
	<file>shxiface.c</file>
	<file>deskmon.rc</file>
	<pch>precomp.h</pch>
</module>
