<module name="dosmbr" type="bootsector" >
	<bootstrap base="loader" nameoncd="dosmbr.bin" />
	<file>dosmbr.asm</file>
</module>
<module name="ext2" type="bootsector">
	<bootstrap base="loader" nameoncd="ext2.bin" />
	<file>ext2.asm</file>
</module>
<module name="fat32" type="bootsector">
	<bootstrap base="loader" nameoncd="fat32.bin" />
	<file>fat32.asm</file>
</module>
<module name="fat" type="bootsector">
	<bootstrap base="loader" nameoncd="fat.bin" />
	<file>fat.asm</file>
</module>
<module name="isoboot" type="bootsector">
	<bootstrap base="loader" nameoncd="isoboot.bin" />
	<file>isoboot.asm</file>
</module>
<module name="isobtrt" type="bootsector">
	<bootstrap base="loader" nameoncd="isobtrt.bin" />
	<file>isobtrt.asm</file>
</module>
<if property="ARCH" value="powerpc">
	<module name="ofwldr" type="bootprogram" payload="freeldr">
		<bootstrap base="loader" nameoncd="boot/ofwldr" />
		<file>ofwboot.s</file>
	</module>
</if>