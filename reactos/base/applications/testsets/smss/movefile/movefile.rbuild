<module name="movefile" type="win32cui" installbase="system32" installname="movefiletest.exe">
	<include base="movefile">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<file>movefile.cpp</file>
	<file>movefile.rc</file>
</module>