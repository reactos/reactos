<?xml:namespace ns="uri:xsl" prefix="xsl"?>
<xsl:document>

    <DIV STYLE="font-family:Courier; font-size:10pt">
        <xsl:use-templates/>
    </DIV>

    <xsl:templates>

        <xsl:template for = "cdata()">
            <SPAN STYLE="color:red"><pre><xsl:get-value /></pre></SPAN>
        </xsl:template>

        <xsl:template for = "textnode()">
            <SPAN STYLE="color:red"><xsl:get-value /></SPAN>
        </xsl:template>

	    <xsl:template for = "attribute()">
    	    <SPAN STYLE="color:black"><xsl:eval>" " + nodeName</xsl:eval></SPAN>="<SPAN STYLE="color:blue"><xsl:get-value />"</SPAN>
	    </xsl:template>

	    <xsl:template for = "element()">
		    <DIV STYLE="margin-left:1em">
			    <SPAN STYLE="color:black">&amp;lt;<xsl:eval>nodeName</xsl:eval><xsl:use-templates for = "@*"/>/&amp;gt;</SPAN>
		    </DIV>
	    </xsl:template>

	    <xsl:template for = "element()[node()]">
		    <DIV STYLE="margin-left:1em">
			    <SPAN STYLE="color:black">&amp;lt;<xsl:eval>nodeName</xsl:eval><xsl:use-templates for = "@*"/>&amp;gt;</SPAN><xsl:use-templates /><SPAN STYLE="color:black">&amp;lt;/<xsl:eval>nodeName</xsl:eval>&amp;gt;</SPAN>
		    </DIV>
	    </xsl:template>

    </xsl:templates>

</xsl:document>
