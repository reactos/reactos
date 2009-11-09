<module name="qedit" type="win32dll" baseaddress="${BASEADDRESS_QEDIT}" installbase="system32" installname="qedit.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="qedit.spec" />
	<include base="qedit">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<file>mediadet.c</file>
	<file>regsvr.c</file>
	<library>wine</library>
	<library>strmiids</library>
	<library>uuid</library>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
