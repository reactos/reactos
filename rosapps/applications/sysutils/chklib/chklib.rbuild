<module name="chklib" type="win32cui" installbase="system32" installname="chklib.exe" allowwarnings="true">
	<include base="chklib>..</include>

	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>

	<library>win32err</library>
	<library>kernel32</library>

	<file>chklib.c</file>
	<file>chklib.rc</file>
</module>
