<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                xmlns:xlink="http://www.w3.org/1999/xlink"
                xmlns:stext="http://nwalsh.com/xslt/ext/com.nwalsh.saxon.TextFactory"
                xmlns:xtext="com.nwalsh.xalan.Text"
                xmlns:lxslt="http://xml.apache.org/xslt"
                exclude-result-prefixes="xlink stext xtext lxslt"
                extension-element-prefixes="stext xtext"
                version='1.0'>

<!-- ********************************************************************
     $Id: graphics.xsl,v 1.1 2002/06/13 20:32:21 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     Contributors:
     Colin Paul Adams, <colin@colina.demon.co.uk>
     Paul Grosso, <pgrosso@arbortext.com>

     ******************************************************************** -->

<!-- ==================================================================== -->
<!-- Graphic format tests for the FO backend -->

<!--
FIXME: make is.graphic.* work correctly depending on the backend!
<xsl:param name="passivetex.extensions" select="0" doc:type='boolean'/>
<xsl:param name="fop.extensions" select="0" doc:type='boolean'/>
-->

<xsl:template name="is.graphic.format">
  <xsl:param name="format"></xsl:param>
  <xsl:if test="$format = 'PNG'
                or $format = 'PDF'
                or $format = 'JPG'
                or $format = 'JPEG'
                or $format = 'linespecific'
                or $format = 'GIF'
                or $format = 'GIF87a'
                or $format = 'GIF89a'
                or $format = 'TIFF'
                or $format = 'BMP'">1</xsl:if>
</xsl:template>

<xsl:template name="is.graphic.extension">
  <xsl:param name="ext"></xsl:param>
  <xsl:if test="$ext = 'png'
                or $ext = 'pdf'
                or $ext = 'jpeg'
                or $ext = 'jpg'
                or $ext = 'gif'
                or $ext = 'tif'
                or $ext = 'tiff'
                or $ext = 'bmp'">1</xsl:if>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="screenshot">
  <fo:block>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="screeninfo">
</xsl:template>

<!-- ==================================================================== -->
<!-- Override these templates for FO -->
<!-- ==================================================================== -->

<xsl:template name="X.process.image">
  <!-- When this template is called, the current node should be  -->
  <!-- a graphic, inlinegraphic, imagedata, or videodata. All    -->
  <!-- those elements have the same set of attributes, so we can -->
  <!-- handle them all in one place.                             -->
  <xsl:variable name="filename">
    <xsl:choose>
      <xsl:when test="local-name(.) = 'graphic'
                      or local-name(.) = 'inlinegraphic'">
        <!-- handle legacy graphic and inlinegraphic by new template --> 
        <xsl:call-template name="mediaobject.filename">
          <xsl:with-param name="object" select="."/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <!-- imagedata, videodata, audiodata -->
        <xsl:call-template name="mediaobject.filename">
          <xsl:with-param name="object" select=".."/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="scale">
    <xsl:choose>
      <xsl:when test="@scale"><xsl:value-of select="@scale"/>%</xsl:when>
      <xsl:otherwise>auto</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="width">
    <xsl:choose>
      <xsl:when test="@width">
        <xsl:call-template name="length-spec">
          <xsl:with-param name="length" select="@width"/>
          <xsl:with-param name="default.units" select="$default.units"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>auto</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="height">
    <xsl:choose>
      <xsl:when test="@depth">
        <xsl:call-template name="length-spec">
          <xsl:with-param name="length" select="@depth"/>
          <xsl:with-param name="default.units" select="$default.units"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>auto</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- DocBook has four attributes related to image size: width, depth,
       scalefit, and scale. Width and depth identify the desired dimensions
       of the image (what CALS calls the repro area dimensions). Scale
       indicates the percentage by which the image should be scaled, and
       scalefit indicates whether or not the actual image should be scaled
       to fit the specified width and/or depth. It is illogical to specify
       both scale and scalefit: scale wins. My thanks to Paul Grosso for
       analyzing this situation with me. -->

  <xsl:variable name="scalefit">
    <xsl:choose>
      <xsl:when test="@scalefit">
        <xsl:value-of select="@scalefit"/>
      </xsl:when>
      <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- if scalefit != 0, set both content-height and content-width to
       scale-to-fit elseif scale != auto, set both content-height and
       content-width to it else set both content-height and
       content-width to auto.  -->

  <xsl:variable name="content-width">
    <xsl:choose>
      <xsl:when test="$scale != 'auto'">
        <xsl:value-of select="$scale"/>
      </xsl:when>
      <xsl:when test="$scalefit != '0'">scale-to-fit</xsl:when>
      <xsl:otherwise>auto</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="content-height">
    <xsl:choose>
      <xsl:when test="$scale != 'auto'">
        <xsl:value-of select="$scale"/>
      </xsl:when>
      <xsl:when test="$scalefit != '0'">scale-to-fit</xsl:when>
      <xsl:otherwise>auto</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$passivetex.extensions != 0
                    or $fop.extensions != 0
                    or $arbortext.extensions != 0">
      <fo:external-graphic src="{$filename}"
                           width="{$width}"
                           height="{$height}"
                           content-width="{$content-width}"
                           content-height="{$content-height}"/>
    </xsl:when>
    <xsl:otherwise>
      <fo:external-graphic src="url({$filename})"
                           width="{$width}"
                           height="{$height}"
                           content-width="{$content-width}"
                           content-height="{$content-height}"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="process.image">
  <!-- When this template is called, the current node should be  -->
  <!-- a graphic, inlinegraphic, imagedata, or videodata. All    -->
  <!-- those elements have the same set of attributes, so we can -->
  <!-- handle them all in one place.                             -->

  <xsl:variable name="scalefit">
    <xsl:choose>
      <xsl:when test="@contentwidth or @contentdepth">0</xsl:when>
      <xsl:when test="@scale">0</xsl:when>
      <xsl:when test="@scalefit"><xsl:value-of select="@scalefit"/></xsl:when>
      <xsl:when test="@width or @depth">1</xsl:when>
      <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="scale">
    <xsl:choose>
      <xsl:when test="@contentwidth or @contentdepth">1.0</xsl:when>
      <xsl:when test="@scale">
        <xsl:value-of select="@scale div 100.0"/>
      </xsl:when>
      <xsl:otherwise>1.0</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="filename">
    <xsl:choose>
      <xsl:when test="local-name(.) = 'graphic'
                      or local-name(.) = 'inlinegraphic'">
        <!-- handle legacy graphic and inlinegraphic by new template --> 
        <xsl:call-template name="mediaobject.filename">
          <xsl:with-param name="object" select="."/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <!-- imagedata, videodata, audiodata -->
        <xsl:call-template name="mediaobject.filename">
          <xsl:with-param name="object" select=".."/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="bgcolor">
    <xsl:call-template name="dbfo-attribute">
      <xsl:with-param name="pis"
                      select="../processing-instruction('dbfo')"/>
      <xsl:with-param name="attribute" select="'background-color'"/>
    </xsl:call-template>
  </xsl:variable>

  <fo:external-graphic>
    <xsl:attribute name="src">
      <xsl:choose>
        <xsl:when test="$passivetex.extensions != 0
                        or $fop.extensions != 0
                        or $arbortext.extensions != 0">
          <xsl:value-of select="$filename"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('url(', $filename, ')')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>

    <xsl:attribute name="width">
      <xsl:choose>
        <xsl:when test="contains(@width,'%')">
          <xsl:value-of select="@width"/>
        </xsl:when>
        <xsl:when test="@width">
          <xsl:call-template name="length-spec">
            <xsl:with-param name="length" select="@width"/>
            <xsl:with-param name="default.units" select="'px'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>auto</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>

    <xsl:attribute name="height">
      <xsl:choose>
        <xsl:when test="contains(@depth,'%')">
          <xsl:value-of select="@depth"/>
        </xsl:when>
        <xsl:when test="@depth">
          <xsl:call-template name="length-spec">
            <xsl:with-param name="length" select="@depth"/>
            <xsl:with-param name="default.units" select="'px'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>auto</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>

    <xsl:attribute name="content-width">
      <xsl:choose>
        <xsl:when test="contains(@contentwidth,'%')">
          <xsl:value-of select="@contentwidth"/>
        </xsl:when>
        <xsl:when test="@contentwidth">
          <xsl:call-template name="length-spec">
            <xsl:with-param name="length" select="@contentwidth"/>
            <xsl:with-param name="default.units" select="'px'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$scale != 1.0">
          <xsl:value-of select="$scale * 100"/>
          <xsl:text>%</xsl:text>
        </xsl:when>
        <xsl:otherwise>auto</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>

    <xsl:attribute name="content-height">
      <xsl:choose>
        <xsl:when test="contains(@contentdepth,'%')">
          <xsl:value-of select="@contentdepth"/>
        </xsl:when>
        <xsl:when test="@contentdepth">
          <xsl:call-template name="length-spec">
            <xsl:with-param name="length" select="@contentdepth"/>
            <xsl:with-param name="default.units" select="'px'"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$scale != 1.0">
          <xsl:value-of select="$scale * 100"/>
          <xsl:text>%</xsl:text>
        </xsl:when>
        <xsl:otherwise>auto</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>

    <xsl:if test="$bgcolor != ''">
      <xsl:attribute name="background-color">
        <xsl:value-of select="$bgcolor"/>
      </xsl:attribute>
    </xsl:if>

    <xsl:if test="@align">
      <xsl:attribute name="text-align">
        <xsl:value-of select="@align"/>
      </xsl:attribute>
    </xsl:if>

    <xsl:if test="@valign">
      <xsl:attribute name="display-align">
        <xsl:choose>
          <xsl:when test="@valign = 'top'">before</xsl:when>
          <xsl:when test="@valign = 'middle'">center</xsl:when>
          <xsl:when test="@valign = 'bottom'">after</xsl:when>
          <xsl:otherwise>auto</xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </xsl:if>
  </fo:external-graphic>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="graphic">
  <xsl:choose>
    <xsl:when test="../inlineequation">
      <xsl:call-template name="process.image"/>
    </xsl:when>
    <xsl:otherwise>
      <fo:block>
        <xsl:call-template name="process.image"/>
      </fo:block>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="inlinegraphic">
  <xsl:variable name="vendor" select="system-property('xsl:vendor')"/>
  <xsl:variable name="filename">
    <xsl:choose>
      <xsl:when test="@entityref">
        <xsl:value-of select="unparsed-entity-uri(@entityref)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="@fileref"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="@format='linespecific'">
      <xsl:choose>
        <xsl:when test="$use.extensions != '0'
                        and $textinsert.extension != '0'">
          <xsl:choose>
            <xsl:when test="contains($vendor, 'SAXON')">
              <stext:insertfile href="{$filename}"/>
            </xsl:when>
            <xsl:when test="contains($vendor, 'Apache Software Foundation')">
              <xtext:insertfile href="{$filename}"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:text>Don't know how to insert files with </xsl:text>
                <xsl:value-of select="$vendor"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <a xlink:type="simple" xlink:show="embed" xlink:actuate="onLoad"
             href="{$filename}"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="process.image"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="mediaobject|mediaobjectco">
  <fo:block>
    <xsl:call-template name="select.mediaobject"/>
    <xsl:apply-templates select="caption"/>
  </fo:block>
</xsl:template>

<xsl:template match="inlinemediaobject">
  <xsl:call-template name="select.mediaobject"/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="imageobjectco">
  <xsl:apply-templates select="imageobject"/>
  <xsl:apply-templates select="calloutlist"/>
</xsl:template>

<xsl:template match="imageobject">
  <xsl:apply-templates select="imagedata"/>
</xsl:template>

<xsl:template match="imagedata">
  <xsl:variable name="vendor" select="system-property('xsl:vendor')"/>
  <xsl:variable name="filename">
    <xsl:call-template name="mediaobject.filename">
      <xsl:with-param name="object" select=".."/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="@format='linespecific'">
      <xsl:choose>
        <xsl:when test="$use.extensions != '0'
                        and $textinsert.extension != '0'">
          <xsl:choose>
            <xsl:when test="contains($vendor, 'SAXON')">
              <stext:insertfile href="{$filename}"/>
            </xsl:when>
            <xsl:when test="contains($vendor, 'Apache Software Foundation')">
              <xtext:insertfile href="{$filename}"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:message terminate="yes">
                <xsl:text>Don't know how to insert files with </xsl:text>
                <xsl:value-of select="$vendor"/>
              </xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <a xlink:type="simple" xlink:show="embed" xlink:actuate="onLoad"
             href="{$filename}"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="process.image"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="videoobject">
  <xsl:apply-templates select="videodata"/>
</xsl:template>

<xsl:template match="videodata">
  <xsl:call-template name="process.image"/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="audioobject">
  <xsl:apply-templates select="audiodata"/>
</xsl:template>

<xsl:template match="audiodata">
  <xsl:call-template name="process.image"/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="textobject">
  <xsl:apply-templates/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="caption">
  <fo:block>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

</xsl:stylesheet>
