<module name="advapi32" type="win32dll" baseaddress="${BASEADDRESS_ADVAPI32}"  installbase="system32" installname="advapi32.dll">
	<importlibrary definition="advapi32.def" />
	<include base="advapi32">.</include>
	<include base="scm_client">.</include>
	<include base="lsa_client">.</include>
	<define name="__USE_W32API" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="UNICODE"></define>
	<define name="_UNICODE"></define>
	<library>scm_client</library>
	<library>lsa_client</library>
	<library>ntdll</library>
	<library>rpcrt4</library>
	<library>wine</library>
	<library>kernel32</library>
	<pch>advapi32.h</pch>
	<directory name="crypt">
		<compilationunit name="crypt_unit.c">
			<file>crypt.c</file>
			<file>crypt_des.c</file>
			<file>crypt_lmhash.c</file>
			<file>crypt_md4.c</file>
			<file>crypt_md5.c</file>
			<file>crypt_sha.c</file>
		</compilationunit>
	</directory>
	<directory name="misc">
		<compilationunit name="misc.c">
			<file>dllmain.c</file>
			<file>hwprofiles.c</file>
			<file>logon.c</file>
			<file>msi.c</file>
			<file>shutdown.c</file>
			<file>sysfunc.c</file>
		</compilationunit>
	</directory>
	<directory name="reg">
		<file>reg.c</file>
	</directory>
	<directory name="sec">
		<compilationunit name="sec_unit.c">
			<file>ac.c</file>
			<file>audit.c</file>
			<file>lsa.c</file>
			<file>misc.c</file>
			<file>sec.c</file>
			<file>sid.c</file>
			<file>trustee.c</file>
		</compilationunit>
	</directory>
	<directory name="service">
		<compilationunit name="service.c">
			<file>eventlog.c</file>
			<file>scm.c</file>
			<file>sctrl.c</file>
			<file>undoc.c</file>
		</compilationunit>
	</directory>
	<directory name="token">
		<compilationunit name="token_unit.c">
			<file>privilege.c</file>
			<file>token.c</file>
		</compilationunit>
	</directory>
	<file>advapi32.rc</file>
</module>
