<module name="hnetcfg" type="win32dll" baseaddress="${BASEADDRESS_HNETCFG}" installbase="system32" installname="hnetcfg.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="hnetcfg.spec" />
	<include base="hnetcfg">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>ole32</library>
	<library>advapi32</library>
	<file>apps.c</file>
	<file>hnetcfg.c</file>
	<file>manager.c</file>
	<file>policy.c</file>
	<file>port.c</file>
	<file>profile.c</file>
	<file>regsvr.c</file>
	<file>service.c</file>
</module>
