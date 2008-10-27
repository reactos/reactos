<module name="ntmarta" type="win32dll" baseaddress="${BASEADDRESS_NTMARTA}" installbase="system32" installname="ntmarta.dll" unicode="yes">
	<importlibrary definition="ntmarta.spec" />
	<include base="ntmarta">.</include>
	<define name="_WIN32_WINNT">0x600</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<file>ntmarta.c</file>
	<file>ntmarta.rc</file>
	<file>ntmarta.spec</file>
	<pch>ntmarta.h</pch>
</module>
