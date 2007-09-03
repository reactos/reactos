<group>
<directory name="hal">
	<xi:include href="hal/hal.rbuild" />
</directory>
<if property="ARCH" value="i386">
        <directory name="halx86">
        	<xi:include href="halx86/directory.rbuild" />
        </directory>
</if>
<if property="ARCH" value="powerpc">
        <directory name="halppc">
	        <xi:include href="halppc/directory.rbuild" />
        </directory>
</if>
</group>
