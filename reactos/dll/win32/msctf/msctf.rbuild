<module name="msctf" type="win32dll" baseaddress="${BASEADDRESS_MSCTF}" installbase="system32" installname="msctf.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="msctf.spec" />
	<include base="msctf">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<file>categorymgr.c</file>
	<file>context.c</file>
	<file>documentmgr.c</file>
	<file>inputprocessor.c</file>
	<file>msctf.c</file>
	<file>regsvr.c</file>
	<file>threadmgr.c</file>
	<file>version.rc</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
