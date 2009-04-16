<module name="amstream" type="win32dll" baseaddress="${BASEADDRESS_AMSTREAM}" installbase="system32" installname="amstream.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="amstream.spec" />
	<include base="amstream">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_WIN32_WINNT">0x600</define>
	<define name="__WINESRC__" />
	<file>amstream.c</file>
	<file>main.c</file>
	<file>mediastream.c</file>
	<file>mediastreamfilter.c</file>
	<file>regsvr.c</file>
	<file>amstream_i.c</file>
	<file>version.rc</file>
	<library>wine</library>
	<library>strmiids</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
