<module name="chklib" type="win32cui" installbase="system32" installname="chklib.exe" allowwarnings="true">
	<include base="chklib">..</include>

	<library>win32err</library>
	<library>kernel32</library>

	<file>chklib.c</file>
	<file>chklib.rc</file>
</module>
