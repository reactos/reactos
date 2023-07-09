<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<!-- This transforms attributes into subelements -->
<xsl:document>

    <xsl:use-templates />

    <xsl:templates>

        <!-- This is the identity template -->
        <xsl:template for = "node()">
            <xsl:copy>
                <xsl:use-templates for = "@*" />
                <xsl:use-templates />
            </xsl:copy>                
        </xsl:template>

        <!-- This transforms element attributes into subelements -->
        <xsl:template for = "element()">
            <xsl:copy>
                <xsl:repeat for = "@*" >
                    <xsl:element><xsl:get-value /></xsl:element>
                </xsl:repeat>
                <xsl:use-templates />
            </xsl:copy>                
        </xsl:template>

    </xsl:templates>

</xsl:document>