<module name="user32" type="win32dll" baseaddress="${BASEADDRESS_USER32}" installbase="system32" installname="user32.dll" unicode="yes">
	<importlibrary definition="user32.spec" />
	<include base="user32">.</include>
	<include base="user32">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="WINVER">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>wine</library>
	<library>ntdll</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>imm32</library>
	<library>win32ksys</library>
	<library>pseh</library>

	<file>winpos.c</file>
	<file>user32.rc</file>
</module>
