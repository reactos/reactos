<module name="appwiz.exe" type="win32cui" installbase="system32" installname="appwiz.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>shell32</library>
	<file>appwiz.c</file>
	<file>appwiz.rc</file>
</module>
