<module name="w32kdll_vista" type="win32dll" entrypoint="0" installname="w32kdll_vista.dll">
	<importlibrary definition="w32kdll_vista-$(ARCH).def" />
	<if property="ARCH" value="i386">
		<file>w32kdll_vista.S</file>
	</if>
	<if property="ARCH" value="amd64">
		<file>w32kdll_vista-amd64.S</file>
	</if>
	<file>main.c</file>
</module>
