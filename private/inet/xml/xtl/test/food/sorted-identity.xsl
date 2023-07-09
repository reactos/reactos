<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<!-- The identity transform -->
<xsl:document>

    <xsl:use-templates />

    <xsl:templates>

        <xsl:template for = "node()">
            <xsl:copy>
                <xsl:use-templates for = "@*" />
                <xsl:use-templates />
            </xsl:copy>                
        </xsl:template>

        <xsl:template for = "BREAKFAST-MENU">
            <xsl:copy>
                <xsl:use-templates for = "@*" />
                <xsl:use-templates for = "FOOD" order-by="NAME" />
            </xsl:copy>
        </xsl:template>

    </xsl:templates>

</xsl:document>