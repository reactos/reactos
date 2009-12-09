<module name="w32kdll_2k3sp2" type="win32dll" entrypoint="0" installname="w32kdll_2k3sp2.dll">
	<importlibrary definition="w32kdll_2k3sp2-$(ARCH).def" />
	<if property="ARCH" value="i386">
		<file>w32kdll_2k3sp2-i386.S</file>
	</if>
	<if property="ARCH" value="amd64">
		<file>w32kdll_2k3sp2-amd64.S</file>
	</if>
	<file>main.c</file>
</module>
