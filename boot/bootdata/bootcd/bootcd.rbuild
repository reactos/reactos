<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<if property="ARCH" value="i386>
	<module name="bootcd" type="iso" output="ReactOS.iso">
		<bootsector>isoboot</bootsector>
	</module>
</if>
<ifnot property="ARCH" value="i386>
	<module name="bootcd" type="iso" output="ReactOS-$(ARCH).iso">
		<bootsector>isoboot</bootsector>
	</module>
</ifnot>
