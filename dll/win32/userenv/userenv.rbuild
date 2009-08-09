<module name="userenv" type="win32dll" baseaddress="${BASEADDRESS_USERENV}" installbase="system32" installname="userenv.dll">
	<importlibrary definition="userenv.spec" />
	<include base="userenv">.</include>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<pch>precomp.h</pch>
	<file>desktop.c</file>
	<file>directory.c</file>
	<file>environment.c</file>
	<file>gpolicy.c</file>
	<file>misc.c</file>
	<file>profile.c</file>
	<file>registry.c</file>
	<file>setup.c</file>
	<file>userenv.c</file>
	<file>userenv.rc</file>
</module>
