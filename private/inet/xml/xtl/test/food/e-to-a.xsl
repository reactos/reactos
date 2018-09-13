<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<!-- This transforms subelements into attributes.  It assumes the document is only two levels-->
<xsl:document>

    <!-- This transforms subelements into attributes -->

    <xsl:repeat>
        <!-- copy the element -->
        <xsl:create-node>

            <!-- process the attributes -->
            <xsl:use-templates for = "@*" />

            <!-- process the subelements -->
            <xsl:repeat>
                <xsl:comment>This comment is for the output</xsl:comment><xsl:create-node>

                   <!-- copy the existing attributes -->

                    <xsl:repeat for = "@*" >
                        <xsl:attribute><xsl:get-value /></xsl:attribute>
                    </xsl:repeat>                        
                    
                    <!-- create an attribute for each element -->

                    <xsl:repeat>
                        <xsl:attribute><xsl:get-value /></xsl:attribute>
                    </xsl:repeat>

                </xsl:create-node>                
            </xsl:repeat>
        </xsl:create-node>                
    </xsl:repeat>

    <xsl:templates>

        <!-- This is the identity tranform -->

        <xsl:template for = "node()">

            <!-- copy the element -->
            <xsl:create-node>
                <!-- process the attributes -->
                <xsl:use-templates for = "@*" />

                <!-- process the subelements -->
                <xsl:use-templates />
            </xsl:create-node>

        </xsl:template>

    </xsl:templates>

</xsl:document>

