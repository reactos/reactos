<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<xsl:document>

    <xsl:templates>

        <xsl:template for="PRICE">
            <P><xsl:get-value /></P>
        </xsl:template>

        <xsl:template for="NAME">
            <P><xsl:get-value /></P>
        </xsl:template>

    </xsl:templates>

    <HTML>
        <xsl:repeat for="BREAKFAST-MENU/FOOD">

            <xsl:templates>
        
                <xsl:template for="NAME">
                    <P-NESTED><xsl:get-value /></P-NESTED>
                </xsl:template>

            </xsl:templates>

    	    <DIV x="y" z="t"><xsl:use-templates for="NAME" /></DIV>
    	    <DIV><xsl:use-templates for="PRICE" /></DIV>
	    </xsl:repeat>
    </HTML>

</xsl:document>

