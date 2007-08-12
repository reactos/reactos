<module name="cacls" type="win32cui" installbase="system32" installname="cacls.exe" unicode="true">
	<include base="cacls">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>shell32</library>
	<file>cacls.c</file>
	<file>cacls.rc</file>
	<pch>precomp.h</pch>
</module>
