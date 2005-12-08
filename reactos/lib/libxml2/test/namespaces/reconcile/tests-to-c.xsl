<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text"/>

	<xsl:template match="/">
		<xsl:text><![CDATA[
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
]]>
</xsl:text>
		<xsl:call-template name="serializer"/>		
		<xsl:apply-templates select="tests/test"/>
		<xsl:text>

int main(int argc, char **argv) {&#xA;</xsl:text>
		<xsl:apply-templates select="tests/test" mode="call"/>
		<xsl:text>
	/* printf("finished.\n"); */
	return (0);
}
</xsl:text>
	</xsl:template>	

	<xsl:template match="tests/test" mode="call">
		<xsl:text>	nsTest_</xsl:text>
		<xsl:value-of select="@name"/>
		<xsl:text>();&#xA;</xsl:text>
	</xsl:template>

	<xsl:template name="xml-text">
		<xsl:param name="text"/>
		<xsl:call-template name="replace-string">
			<!-- Substitute #10 for " -->	        			
	        <xsl:with-param name="from" select="'&#10;'"/>
	        <xsl:with-param name="to" select="'&quot;&#10;&quot;'"/>
			<xsl:with-param name="text">
				<xsl:call-template name="replace-string">
					<!-- Substitute " for \" -->
	        		<xsl:with-param name="from" select="'&quot;'"/>
	        		<xsl:with-param name="to" select="'\&quot;'"/>
					<xsl:with-param name="text">
						<xsl:call-template name="replace-string">
							<!-- Remove tabs. -->
			        		<xsl:with-param name="from" select="'&#9;'"/>
			        		<xsl:with-param name="to" select="''"/>
							<xsl:with-param name="text" select="$text"/>
	    				</xsl:call-template>
					</xsl:with-param>
    			</xsl:call-template>
			</xsl:with-param>
    	</xsl:call-template>
		
	</xsl:template>

	<xsl:template match="doc" mode="define">
		<xsl:text>	xmlDocPtr </xsl:text>
		<xsl:value-of select="@name"/>
		<xsl:text>;&#xA;</xsl:text>
		<xsl:text>	const char * </xsl:text>
		<xsl:value-of select="@name"/>
		<xsl:text>_str = "</xsl:text>
		<xsl:call-template name="xml-text">
	        <xsl:with-param name="text" select="."/>
    	</xsl:call-template>		
		<xsl:text>";&#xA;</xsl:text>
	</xsl:template>

	<xsl:template match="expected" mode="define">
		<xsl:text>	const char * </xsl:text>	
		<xsl:text>exp_str = "</xsl:text>
		<xsl:call-template name="xml-text">
	        <xsl:with-param name="text" select="."/>
    	</xsl:call-template>		
		<xsl:text>";&#xA;</xsl:text>
	</xsl:template>

	<xsl:template match="doc">
		<xsl:text>	</xsl:text>
		<xsl:value-of select="@name"/>
		<xsl:text> = xmlReadDoc(BAD_CAST </xsl:text>
		<xsl:value-of select="@name"/>
		<xsl:text>_str, NULL, NULL, 0);&#xA;</xsl:text>
			
		<xsl:apply-templates select="following-sibling::*[1]"/>

		<xsl:text>	xmlFreeDoc(</xsl:text>
		<xsl:value-of select="@name"/>
		<xsl:text>);&#xA;</xsl:text>
	</xsl:template>

	<xsl:template match="xpath">
	</xsl:template>

	<xsl:template match="var" mode="define">
		<xsl:text>	xmlNodePtr </xsl:text>
		<xsl:value-of select="@name"/>
		<xsl:text>;&#xA;</xsl:text>
	</xsl:template>

	<xsl:template match="var">
		<xsl:if test="xpath">
			<!-- Create XPath context. -->
			<xsl:text>	/* Selecting node "</xsl:text><xsl:value-of select="@name"/><xsl:text>". */&#xA;</xsl:text>
			<xsl:text>	xp = xmlXPathNewContext(</xsl:text>
			<xsl:value-of select="xpath/@doc"/>
			<xsl:text>);&#xA;</xsl:text>
			<!-- Register namespaces. -->
			<xsl:for-each select="xpath/reg-ns">
				<xsl:text>	xmlXPathRegisterNs(xp, BAD_CAST "</xsl:text>
				<xsl:value-of select="@prefix"/>
				<xsl:text>", BAD_CAST "</xsl:text>
				<xsl:value-of select="@ns"/>
				<xsl:text>");&#xA;</xsl:text>
			</xsl:for-each>
			<!-- Evaluate. -->
			<xsl:text>	</xsl:text>
			<xsl:value-of select="@name"/>
			<xsl:text> = nsSelectNode(xp, "</xsl:text>
			<xsl:value-of select="xpath/@select-node"/>
			<xsl:text>");&#xA;</xsl:text>
			<xsl:text>	xmlXPathFreeContext(xp);&#xA;</xsl:text>
		</xsl:if>
		<xsl:apply-templates select="following-sibling::*[1]"/>
	</xsl:template>

	<xsl:template match="reconcile-ns">
		<xsl:text>	/* Reconcile node "</xsl:text><xsl:value-of select="@ref"/><xsl:text>". */&#xA;</xsl:text>
		<xsl:text>	xmlDOMWrapReconcileNamespaces(NULL, </xsl:text>
		<xsl:value-of select="@node"/>
		<xsl:text>, 0);&#xA;</xsl:text>
		<xsl:apply-templates select="following-sibling::*[1]"/>
	</xsl:template>

	<xsl:template match="remove">
		<xsl:text>	xmlDOMWrapRemoveNode(NULL, </xsl:text>
		<xsl:value-of select="@node"/>
		<xsl:text>->doc, </xsl:text>
		<xsl:value-of select="@node"/>
		<xsl:text>, 0);&#xA;</xsl:text>
		<xsl:apply-templates select="following-sibling::*[1]"/>
	</xsl:template>

	<xsl:template match="adopt">
		<xsl:text>	/* Adopt "</xsl:text><xsl:value-of select="@node"/><xsl:text>". */&#xA;</xsl:text>
		<xsl:text>	xmlDOMWrapAdoptNode(NULL, </xsl:text>
		<xsl:value-of select="@node"/>
		<xsl:text>->doc, </xsl:text>
		<xsl:value-of select="@node"/>
		<xsl:text>, </xsl:text>
		<xsl:value-of select="@dest-doc"/>
		<xsl:text>, </xsl:text>
		<xsl:choose>
			<xsl:when test="@dest-parent">
				<xsl:value-of select="@dest-parent"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:text>NULL</xsl:text>
			</xsl:otherwise>
		</xsl:choose>		
		<xsl:text>, 0);&#xA;</xsl:text>
		<xsl:apply-templates select="following-sibling::*[1]"/>
	</xsl:template>

	<xsl:template match="append-child">
		<xsl:text>	xmlAddChild(</xsl:text>
		<xsl:value-of select="@parent"/>
		<xsl:text>, </xsl:text>
		<xsl:value-of select="@child"/>
		<xsl:text>);&#xA;</xsl:text>
		<xsl:apply-templates select="following-sibling::*[1]"/>
	</xsl:template>

	<xsl:template match="expected">		
		<xsl:text>	/* Serialize "</xsl:text><xsl:value-of select="@doc"/><xsl:text>". */&#xA;</xsl:text>
		<xsl:text>	result_str = nsSerializeNode(xmlDocGetRootElement(</xsl:text>
		<xsl:value-of select="@doc"/>
		<xsl:text>));&#xA;</xsl:text>
		<xsl:text>	/* Compare result. */
	if (! xmlStrEqual(BAD_CAST result_str, BAD_CAST exp_str)) {
		printf("FAILED\n");
		printf("%s\n", (const char *) result_str);
		printf("- - -\n");
		printf("Expected:\n%s\n", exp_str);
		printf("= = =\n");
	}
	xmlFree(result_str);&#xA;</xsl:text>
	</xsl:template>

	<!--********
	    * TEST *
	    ********-->
	<xsl:template match="test">		
		<xsl:text>void nsTest_</xsl:text>
		<xsl:value-of select="@name"/>
		<xsl:text>(void) {
	xmlChar * result_str;
	xmlXPathContextPtr xp;
	int memory;&#xA;</xsl:text>
		<xsl:apply-templates select="*" mode="define"/>
		<xsl:text>
	memory = xmlMemUsed();
	xmlInitParser();&#xA;&#xA;</xsl:text>
		<xsl:apply-templates select="child::*[1]"/>
		<xsl:text>
	xmlCleanupParser();
	memory = xmlMemUsed() - memory;

	if (memory != 0) {		
		printf("## '%s' MEMORY leak: %d\n", "</xsl:text>
		<xsl:value-of select="@name"/>
		<xsl:text>", memory);
    }		
}
</xsl:text>	
	</xsl:template>

	<xsl:template name="serializer">
		<xsl:text>
		
xmlChar * nsSerializeNode(xmlNodePtr node) {
	xmlChar * ret;

	xmlOutputBufferPtr buf;
	buf = xmlAllocOutputBuffer(NULL);
	xmlNodeDumpOutput(buf, node->doc, node, 0, 0, NULL);
	xmlOutputBufferFlush(buf);
	ret = (xmlChar *) buf->buffer->content;
	buf->buffer->content = NULL;
	(void) xmlOutputBufferClose(buf);
	return (ret);
}

xmlNodePtr nsSelectNode(xmlXPathContextPtr xp, const char * xpath) {
	xmlXPathObjectPtr xpres;
	xmlNodePtr ret;	
		
	xpres = xmlXPathEval(BAD_CAST xpath, xp);
	ret = xpres->nodesetval->nodeTab[0];
	xmlXPathFreeObject(xpres);
	return (ret);
}

</xsl:text>
	</xsl:template>

	<xsl:template name="replace-string">
    <xsl:param name="text"/>
    <xsl:param name="from"/>
    <xsl:param name="to"/>

    <xsl:choose>
      <xsl:when test="contains($text, $from)">

	<xsl:variable name="before" select="substring-before($text, $from)"/>
	<xsl:variable name="after" select="substring-after($text, $from)"/>
	<xsl:variable name="prefix" select="concat($before, $to)"/>

	<xsl:value-of select="$before"/>
	<xsl:value-of select="$to"/>
        <xsl:call-template name="replace-string">
	  <xsl:with-param name="text" select="$after"/>
	  <xsl:with-param name="from" select="$from"/>
	  <xsl:with-param name="to" select="$to"/>
	</xsl:call-template>
      </xsl:when> 
      <xsl:otherwise>
        <xsl:value-of select="$text"/>  
      </xsl:otherwise>
    </xsl:choose>            
 </xsl:template>
		
	
</xsl:stylesheet>
