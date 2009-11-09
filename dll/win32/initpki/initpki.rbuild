<module name="initpki" type="win32dll" baseaddress="${BASEADDRESS_INITPKI}" installbase="system32" installname="initpki.dll" allowwarnings="true">
	<!--autoregister infsection="OleControlDlls" type="Both" /-->
	<importlibrary definition="initpki.spec" />
	<include base="initpki">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
