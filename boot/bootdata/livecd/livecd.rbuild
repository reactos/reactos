<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<if property="ARCH" value="i386>
	<module name="livecd" type="liveiso" output="ReactOS-LiveCD.iso">
		<bootsector>isoboot</bootsector>
	</module>
</if>
<ifnot property="ARCH" value="i386>
	<module name="livecd" type="liveiso" output="ReactOS-LiveCD-$(ARCH).iso">
		<bootsector>isoboot</bootsector>
	</module>
</ifnot>
