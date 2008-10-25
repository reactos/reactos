<module name="msgina" type="win32dll" baseaddress="${BASEADDRESS_MSGINA}" installbase="system32" installname="msgina.dll">
	<importlibrary definition="msgina.spec" />
	<include base="msgina">.</include>
	<include base="msgina">include</include>
	<library>ntdll</library>
	<library>wine</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>userenv</library>
	<file>gui.c</file>
	<file>msgina.c</file>
	<file>stubs.c</file>
	<file>tui.c</file>
	<file>msgina.rc</file>
	<file>msgina.spec</file>
</module>
