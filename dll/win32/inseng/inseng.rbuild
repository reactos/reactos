<module name="inseng" type="win32dll" baseaddress="${BASEADDRESS_INSENG}" installbase="system32" installname="inseng.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="inseng.spec" />
	<include base="inseng">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_WINNT">0x600</define>
	<file>inseng_main.c</file>
	<file>regsvr.c</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
