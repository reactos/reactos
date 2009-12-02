<module name="slayer" type="win32dll" baseaddress="${BASEADDRESS_SLAYER}" installbase="system32" installname="slayer.dll" unicode="yes">
	<importlibrary definition="slayer.spec" />
	<include base="slayer">.</include>
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
