<?xml version="1.0" encoding="UTF-8" ?>
<xsl:stylesheet 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:ts="TestSuite" version="1.0"
	xmlns:xl="http://www.w3.org/1999/xlink">
	<xsl:param name="vendor" select="'NIST'"/>
    <xsl:output method="text"/>   

    <xsl:template match="/">
        <xsl:text>#!/usr/bin/python -u
# -*- coding: UTF-8 -*-
#
# This file is generated from the W3C test suite description file.
#

import xstc
from xstc import XSTCTestRunner, XSTCTestGroup, XSTCSchemaTest, XSTCInstanceTest

xstc.vendor = "</xsl:text><xsl:value-of select="$vendor"/><xsl:text>"

r = XSTCTestRunner()

# Group definitions.
                                 
</xsl:text>
		      
        <xsl:apply-templates select="ts:testSet/ts:testGroup" mode="group-def"/>
<xsl:text>

# Test definitions.

</xsl:text>
		<xsl:apply-templates select="ts:testSet/ts:testGroup" mode="test-def"/>
        <xsl:text>
           
r.run()    

</xsl:text>
            
    </xsl:template>       

	<!-- groupName, descr -->
    <xsl:template match="ts:testGroup" mode="group-def">
		<xsl:text>r.addGroup(XSTCTestGroup("</xsl:text>
		<!-- group -->
		<xsl:value-of select="@name"/><xsl:text>", "</xsl:text>
		<!-- main schema -->
		<xsl:value-of select="ts:schemaTest[1]/ts:schemaDocument/@xl:href"/><xsl:text>", """</xsl:text>
		<!-- group-description -->
		<xsl:call-template name="str">
			<xsl:with-param name="str" select="ts:annotation/ts:documentation/text()"/>
		</xsl:call-template>
		<xsl:text>"""))
</xsl:text>
	</xsl:template>
	
	<xsl:template name="str">
		<xsl:param name="str"/>
		<xsl:choose>
			<xsl:when test="contains($str, '&quot;')">
				<xsl:call-template name="str">
					<xsl:with-param name="str" select="substring-before($str, '&quot;')"/>
				</xsl:call-template>
				<xsl:text>'</xsl:text>
				<xsl:call-template name="str">
					<xsl:with-param name="str" select="substring-after($str, '&quot;')"/>
				</xsl:call-template>
			
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="$str"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="ts:testGroup" mode="test-def">	    
		<xsl:param name="group" select="@name"/>
		<xsl:for-each select="ts:schemaTest">
			<!-- groupName, isSchema, Name, Accepted, File, Val, Descr -->
			<xsl:text>r.addTest(XSTCSchemaTest("</xsl:text>
			<!-- group -->
			<xsl:value-of select="$group"/><xsl:text>", "</xsl:text>
			<!-- test-name -->
			<xsl:value-of select="@name"/><xsl:text>", </xsl:text>
			<!-- accepted -->
			<xsl:value-of select="number(ts:current/@status = 'accepted')"/><xsl:text>, "</xsl:text>
			<!-- filename -->			
			<xsl:value-of select="ts:schemaDocument/@xl:href"/><xsl:text>", </xsl:text>
			<!-- validity -->
			<xsl:value-of select="number(ts:expected/@validity = 'valid')"/><xsl:text>, "</xsl:text>
			<!-- test-description -->
			<xsl:value-of select="ts:annotation/ts:documentation/text()"/><xsl:text>"))
</xsl:text>
		</xsl:for-each>
		<xsl:for-each select="ts:instanceTest">
			<!-- groupName, isSchema, Name, Accepted, File, Val, Descr -->
			<xsl:text>r.addTest(XSTCInstanceTest("</xsl:text>
			<!-- group -->
			<xsl:value-of select="$group"/><xsl:text>", "</xsl:text>
			<!-- test-name -->
			<xsl:value-of select="@name"/><xsl:text>", </xsl:text>
			<!-- accepted -->
			<xsl:value-of select="number(ts:current/@status = 'accepted')"/><xsl:text>, "</xsl:text>
			<!-- filename -->			
			<xsl:value-of select="ts:instanceDocument/@xl:href"/><xsl:text>", </xsl:text>
			<!-- validity -->
			<xsl:value-of select="number(ts:expected/@validity = 'valid')"/><xsl:text>, "</xsl:text>
			<!-- test-description -->
			<xsl:value-of select="ts:annotation/ts:documentation/text()"/><xsl:text>"))
</xsl:text>
		</xsl:for-each>
	</xsl:template>                     
        
</xsl:stylesheet>