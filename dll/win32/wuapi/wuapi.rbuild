<module name="wuapi" type="win32dll" baseaddress="${BASEADDRESS_WUAPI}" installbase="system32" installname="wuapi.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="wuapi.spec" />
	<include base="wuapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<file>downloader.c</file>
	<file>installer.c</file>
	<file>main.c</file>
	<file>regsvr.c</file>
	<file>searcher.c</file>
	<file>session.c</file>
	<file>updates.c</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
