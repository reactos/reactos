<group>
<module name="qmgr" type="win32dll" baseaddress="${BASEADDRESS_QMGR}" installbase="system32" installname="qmgr.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="qmgr.spec" />
	<include base="qmgr">.</include>
	<include base="qmgr" root="intermediate">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<dependency>qmgr_local_header</dependency>
	<library>wine</library>
	<library>uuid</library>
	<library>wininet</library>
	<library>urlmon</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>enum_files.c</file>
	<file>enum_jobs.c</file>
	<file>factory.c</file>
	<file>file.c</file>
	<file>job.c</file>
	<file>qmgr.c</file>
	<file>qmgr_main.c</file>
	<file>service.c</file>
	<file>rsrc.rc</file>
</module>
<module name="qmgr_local_header" type="idlheader">
	<file>qmgr_local.idl</file>
</module>
</group>
