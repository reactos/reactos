<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<xsl:document>

	<TBODY>
	    <TD><P>NAME</P></TD>
	</TBODY>
    <xsl:repeat for="BREAKFAST-MENU/FOOD" order-by = "NAME">
        <TR>
            <xsl:use-templates for = "*"/>
        </TR>
    </xsl:repeat>

    <xsl:templates>

        <xsl:template for="NAME">
	        <TD></TD>
        </xsl:template>

    </xsl:templates>
    
</xsl:document>