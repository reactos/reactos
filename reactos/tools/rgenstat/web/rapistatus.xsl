<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

	<xsl:output method="html" indent="no"/>
	<!--	<xsl:output method="xml"/>-->
<!-- Will not work:	<xsl:strip-space elements="*"/> -->

	<xsl:template match="/">
		<HTML>
			<HEAD>
				<TITLE>
					ReactOS API Status
				</TITLE>
				<SCRIPT src="rapistatus.js"></SCRIPT>
				<LINK rel="stylesheet" type="text/css" href="rapistatus.css"></LINK>
			</HEAD>
			<BODY onLoad="onLoad();">
				<P>
					<H1>ReactOS API Status</H1>
				</P>
				<P>
					<TABLE>
						<TR>
							<TD> <INPUT type="checkbox" ID="implemented" onClick="selectImplemented();" checked="1"/> </TD>
							<TD> <IMG src="i.gif"/> </TD>
							<TD> Implemented </TD>
							<TD width="20"/>
						</TR>
						<TR>
							<TD> <INPUT type="checkbox" ID="unimplemented" onClick="selectUnimplemented();" checked="1"/> </TD>
							<TD> <IMG src="u.gif"/> </TD>
							<TD> Unimplemented </TD>
							<TD width="20"/>
						</TR>
					</TABLE>
				</P>
				<DIV ID="ROOT">
					<xsl:apply-templates/>
				</DIV>
				<P>
					Legend :<BR/>
					<TABLE>
						<TR>
							<TD> <IMG src="c.gif"/> </TD>
							<TD> Component </TD>
							<TD> <IMG src="i.gif"/> </TD>
							<TD> Implemented </TD>
							<TD> <IMG src="sc.gif"/> </TD>
							<TD> Complete </TD>
						<TR>
						</TR>
							<TD> <IMG src="f.gif"/> </TD>
							<TD> Function </TD>
							<TD> <IMG src="u.gif"/> </TD>
							<TD> Unimplemented </TD>
							<TD></TD>
							<TD></TD>
						</TR>
					</TABLE>

				</P>
			</BODY>
		</HTML>
	</xsl:template>


	<!-- component -->
	<xsl:template match="/components">
		<xsl:apply-templates select="component">
			<xsl:sort select="@name"/>
		</xsl:apply-templates>
	</xsl:template>

	<xsl:template match="components/component[@implemented_total or @unimplemented_total]">
		<DIV>
			<xsl:call-template name="ELEMENT">
				<xsl:with-param name="class">c</xsl:with-param>
			</xsl:call-template>
			<xsl:apply-templates/>
		</DIV>
	</xsl:template>


	<!-- function -->
	<xsl:template match="functions">
		<xsl:apply-templates select="function">
			<xsl:sort select="@name"/>
		</xsl:apply-templates>
	</xsl:template>

	<xsl:template match="functions/function">
		<DIV>
			<xsl:call-template name="ELEMENT">
				<xsl:with-param name="class">f</xsl:with-param>
			</xsl:call-template>
			<xsl:apply-templates/>
		</DIV>
	</xsl:template>


	<!-- support templates -->

	<xsl:template name="ELEMENT">
		<xsl:param name="class"/>
		<xsl:param name="image"/>
			<xsl:attribute name="class">
	  		<xsl:value-of select="$class"/>
  			<xsl:text>_</xsl:text>
			</xsl:attribute>
			<xsl:call-template name="toggle"/>
			<xsl:choose>
				<xsl:when test="./node() and local-name() != 'component' and @implemented = 'true'">
          <img src="i.gif" class="i"/>
				</xsl:when>
				<xsl:when test="./node() and local-name() != 'component' and @implemented = 'false'">
          <img src="u.gif" class="u"/>
				</xsl:when>
				<xsl:when test="./node() and local-name() = 'component' and @complete >= 100">
          <img src="sc.gif"/>
				</xsl:when>
				<xsl:otherwise>
          <img src="tb.gif" with="12" height="12"/>
				</xsl:otherwise>
			</xsl:choose>
			<xsl:choose>
				<xsl:when test="$image">
					<img src="{$image}.gif" class="t"/>
				</xsl:when>
				<xsl:otherwise>
					<img src="{$class}.gif" class="t"/>
				</xsl:otherwise>
			</xsl:choose>
			<xsl:call-template name="name"/>
 			<xsl:call-template name="file"/>
			<xsl:call-template name="status"/>
	</xsl:template>

	<xsl:template name="status">
		<xsl:if test="@complete and @complete != 0">
			<SPAN class="st">
				<img src="sc.gif"/>
				<xsl:text>: </xsl:text>
				<xsl:value-of select="@complete"/>
				<xsl:text>%</xsl:text>
			</SPAN>
		</xsl:if>
		<xsl:if test="@implemented_total">
			<SPAN class="st">
				<img src="i.gif"/>: <xsl:value-of select="@implemented_total"/>
			</SPAN>
		</xsl:if>
		<xsl:if test="@unimplemented_total">
			<SPAN class="st">
				<img src="u.gif"/>: <xsl:value-of select="@unimplemented_total"/>
			</SPAN>
		</xsl:if>
	</xsl:template>

	<xsl:template name="toggle">
		<xsl:choose>
			<xsl:when test="local-name() = 'component'">
				<IMG src="tp.gif" class="t"/>
			</xsl:when>
			<xsl:otherwise>
   			<IMG src="tb.gif"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template name="name">
		<xsl:if test="@name">
			<SPAN class="l"><xsl:value-of select="@name"/></SPAN>
		</xsl:if>
	</xsl:template>

	<xsl:template name="file">
		<xsl:if test="@file">
			<SPAN class="h"><xsl:value-of select="@file"/></SPAN>
		</xsl:if>
	</xsl:template>

</xsl:stylesheet>
