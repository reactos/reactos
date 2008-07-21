<module name="tee" type="win32cui" installbase="system32" installname="tee.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<file>tee.c</file>
	<file>tee.rc</file>
</module>
