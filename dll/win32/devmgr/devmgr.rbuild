<module name="devmgr" type="win32dll" baseaddress="${BASEADDRESS_DEVENUM}" installbase="system32" installname="devmgr.dll" unicode="yes">
	<include base="devmgr">.</include>
	<importlibrary definition="devmgr.spec" />
	<library>kernel32</library>
	<library>ntdll</library>
	<library>setupapi</library>
	<library>advapi32</library>
	<library>user32</library>
	<file>devmgr.rc</file>
	<file>advprop.c</file>
	<file>devprblm.c</file>
	<file>hwpage.c</file>
	<file>misc.c</file>
	<file>stubs.c</file>
	<pch>precomp.h</pch>
</module>
