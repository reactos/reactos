<module name="hivetest" type="win32gui" installbase="bin" installname="hivetest.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>ntdll</library>
	<library>advapi32</library>
	<file>hivetest.c</file>
</module>
