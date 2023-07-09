<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<!-- The identity transform -->
<xsl:document>

    <xsl:use-templates />

    <xsl:templates>

        <xsl:template for = "node()">
            <xsl:create-node>
                <xsl:use-templates for = "@*" />
                <xsl:use-templates />
            </xsl:create-node>                
        </xsl:template>

    </xsl:templates>

</xsl:document>