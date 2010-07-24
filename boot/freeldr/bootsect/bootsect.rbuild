<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="dosmbr" type="bootsector" >
	<bootstrap installbase="loader" nameoncd="dosmbr.bin" />
	<file>dosmbr.asm</file>
</module>
<module name="ext2" type="bootsector">
	<bootstrap installbase="loader" nameoncd="ext2.bin" />
	<file>ext2.asm</file>
</module>
<module name="fat32" type="bootsector">
	<bootstrap installbase="loader" nameoncd="fat32.bin" />
	<file>fat32.asm</file>
</module>
<module name="fat" type="bootsector">
	<bootstrap installbase="loader" nameoncd="fat.bin" />
	<file>fat.asm</file>
</module>
<module name="isoboot" type="bootsector">
	<bootstrap installbase="loader" nameoncd="isoboot.bin" />
	<file>isoboot.asm</file>
</module>
<module name="isobtrt" type="bootsector">
	<bootstrap installbase="loader" nameoncd="isobtrt.bin" />
	<file>isobtrt.asm</file>
</module>
</group>
