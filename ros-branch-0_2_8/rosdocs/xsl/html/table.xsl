<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
                xmlns:stbl="http://nwalsh.com/xslt/ext/com.nwalsh.saxon.Table"
                xmlns:xtbl="com.nwalsh.xalan.Table"
                xmlns:lxslt="http://xml.apache.org/xslt"
                exclude-result-prefixes="doc stbl xtbl lxslt"
                version='1.0'>

<xsl:include href="../common/table.xsl"/>

<!-- ********************************************************************
     $Id: table.xsl,v 1.1 2002/06/13 20:32:40 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<lxslt:component prefix="xtbl"
                 functions="adjustColumnWidths"/>

<xsl:template name="empty.table.cell">
  <xsl:param name="colnum" select="0"/>

  <xsl:variable name="rowsep">
    <xsl:call-template name="inherited.table.attribute">
      <xsl:with-param name="entry" select="NOT-AN-ELEMENT-NAME"/>
      <xsl:with-param name="colnum" select="$colnum"/>
      <xsl:with-param name="attribute" select="'rowsep'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="colsep">
    <xsl:call-template name="inherited.table.attribute">
      <xsl:with-param name="entry" select="NOT-AN-ELEMENT-NAME"/>
      <xsl:with-param name="colnum" select="$colnum"/>
      <xsl:with-param name="attribute" select="'colsep'"/>
    </xsl:call-template>
  </xsl:variable>

  <td class="auto-generated">
    <xsl:if test="$table.borders.with.css != 0">
      <xsl:attribute name="style">
        <xsl:if test="$colsep &gt; 0">
          <xsl:call-template name="border">
            <xsl:with-param name="side" select="'right'"/>
          </xsl:call-template>
        </xsl:if>
        <xsl:if test="$rowsep &gt; 0">
          <xsl:call-template name="border">
            <xsl:with-param name="side" select="'bottom'"/>
          </xsl:call-template>
        </xsl:if>
      </xsl:attribute>
    </xsl:if>
    <xsl:text>&#160;</xsl:text>
  </td>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="border">
  <xsl:param name="side" select="'left'"/>
  <xsl:param name="padding" select="0"/>

  <xsl:text>border-</xsl:text>
  <xsl:value-of select="$side"/>
  <xsl:text>: </xsl:text>
  <xsl:value-of select="$table.border.thickness"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="$table.border.style"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="$table.border.color"/>
  <xsl:text>; </xsl:text>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="tgroup">
  <table>
    <xsl:choose>
      <!-- If there's a textobject/phrase for the table summary, use it -->
      <xsl:when test="../textobject/phrase">
        <xsl:attribute name="summary">
          <xsl:value-of select="../textobject/phrase"/>
        </xsl:attribute>
      </xsl:when>

      <!-- If there's a <?dbhtml table-summary="foo"?> PI, use it for
           the HTML table summary attribute -->
      <xsl:when test="processing-instruction('dbhtml')">
        <xsl:variable name="summary">
          <xsl:call-template name="dbhtml-attribute">
            <xsl:with-param name="pis"
                            select="processing-instruction('dbhtml')[1]"/>
            <xsl:with-param name="attribute" select="'table-summary'"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$summary != ''">
          <xsl:attribute name="summary">
            <xsl:value-of select="$summary"/>
          </xsl:attribute>
        </xsl:if>
      </xsl:when>

      <!-- Otherwise, if there's a title, use that -->
      <xsl:when test="../title">
        <xsl:attribute name="summary">
          <xsl:value-of select="string(../title)"/>
        </xsl:attribute>
      </xsl:when>

      <!-- Otherwise, forget the whole idea -->
      <xsl:otherwise><!-- nevermind --></xsl:otherwise>
    </xsl:choose>

    <xsl:if test="../@pgwide=1">
      <xsl:attribute name="width">100%</xsl:attribute>
    </xsl:if>

    <xsl:choose>
      <xsl:when test="$table.borders.with.css != 0">
        <xsl:attribute name="border">0</xsl:attribute>
        <xsl:choose>
          <xsl:when test="../@frame='all'">
            <xsl:attribute name="style">
              <xsl:text>border-collapse: collapse;</xsl:text>
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'top'"/>
              </xsl:call-template>
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'bottom'"/>
              </xsl:call-template>
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'left'"/>
              </xsl:call-template>
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'right'"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:when>
          <xsl:when test="../@frame='topbot'">
            <xsl:attribute name="style">
              <xsl:text>border-collapse: collapse;</xsl:text>
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'top'"/>
              </xsl:call-template>
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'bottom'"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:when>
          <xsl:when test="../@frame='top'">
            <xsl:attribute name="style">
              <xsl:text>border-collapse: collapse;</xsl:text>
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'top'"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:when>
          <xsl:when test="../@frame='bottom'">
            <xsl:attribute name="style">
              <xsl:text>border-collapse: collapse;</xsl:text>
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'bottom'"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:when>
          <xsl:when test="../@frame='sides'">
            <xsl:attribute name="style">
              <xsl:text>border-collapse: collapse;</xsl:text>
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'left'"/>
              </xsl:call-template>
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'right'"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:when>
          <xsl:otherwise>
            <xsl:attribute name="style">
              <xsl:text>border-collapse: collapse;</xsl:text>
            </xsl:attribute>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="../@frame='none'">
        <xsl:attribute name="border">0</xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
        <xsl:attribute name="border">1</xsl:attribute>
      </xsl:otherwise>
    </xsl:choose>

    <xsl:variable name="colgroup">
      <colgroup>
        <xsl:call-template name="generate.colgroup">
          <xsl:with-param name="cols" select="@cols"/>
        </xsl:call-template>
      </colgroup>
    </xsl:variable>

    <xsl:variable name="explicit.table.width">
      <xsl:call-template name="dbhtml-attribute">
        <xsl:with-param name="pis"
                        select="../processing-instruction('dbhtml')[1]"/>
        <xsl:with-param name="attribute" select="'table-width'"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="table.width">
      <xsl:choose>
        <xsl:when test="$explicit.table.width != ''">
          <xsl:value-of select="$explicit.table.width"/>
        </xsl:when>
        <xsl:when test="$default.table.width = ''">
          <xsl:text>100%</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$default.table.width"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:if test="$default.table.width != ''
                  or $explicit.table.width != ''">
      <xsl:attribute name="width">
        <xsl:choose>
          <xsl:when test="contains($table.width, '%')">
            <xsl:value-of select="$table.width"/>
          </xsl:when>
          <xsl:when test="$use.extensions != 0
                          and $tablecolumns.extension != 0">
            <xsl:choose>
              <xsl:when test="function-available('stbl:convertLength')">
                <xsl:value-of select="stbl:convertLength($table.width)"/>
              </xsl:when>
              <xsl:when test="function-available('xtbl:convertLength')">
                <xsl:value-of select="xtbl:convertLength($table.width)"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:message terminate="yes">
                  <xsl:text>No convertLength function available.</xsl:text>
                </xsl:message>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$table.width"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </xsl:if>

    <xsl:choose>
      <xsl:when test="$use.extensions != 0
                      and $tablecolumns.extension != 0">
        <xsl:choose>
          <xsl:when test="function-available('stbl:adjustColumnWidths')">
            <xsl:copy-of select="stbl:adjustColumnWidths($colgroup)"/>
          </xsl:when>
          <xsl:when test="function-available('xtbl:adjustColumnWidths')">
            <xsl:copy-of select="xtbl:adjustColumnWidths($colgroup)"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:message terminate="yes">
              <xsl:text>No adjustColumnWidths function available.</xsl:text>
            </xsl:message>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="$colgroup"/>
      </xsl:otherwise>
    </xsl:choose>

    <xsl:apply-templates select="thead"/>
    <xsl:apply-templates select="tfoot"/>
    <xsl:apply-templates select="tbody"/>

    <xsl:if test=".//footnote">
      <tbody class="footnotes">
        <tr>
          <td colspan="{@cols}">
            <xsl:apply-templates select=".//footnote" mode="table.footnote.mode"/>
          </td>
        </tr>
      </tbody>
    </xsl:if>
  </table>
</xsl:template>

<xsl:template match="tgroup/processing-instruction('dbhtml')">
  <xsl:variable name="summary">
    <xsl:call-template name="dbhtml-attribute">
      <xsl:with-param name="pis" select="."/>
      <xsl:with-param name="attribute" select="'table-summary'"/>
    </xsl:call-template>
  </xsl:variable>

  <!-- Suppress the table-summary PI -->
  <xsl:if test="$summary = ''">
    <xsl:processing-instruction name="dbhtml">
      <xsl:value-of select="."/>
    </xsl:processing-instruction>
  </xsl:if>
</xsl:template>

<xsl:template match="colspec"></xsl:template>

<xsl:template match="spanspec"></xsl:template>

<xsl:template match="thead|tfoot">
  <xsl:element name="{name(.)}">
    <xsl:if test="@align">
      <xsl:attribute name="align">
        <xsl:value-of select="@align"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@char">
      <xsl:attribute name="char">
        <xsl:value-of select="@char"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@charoff">
      <xsl:attribute name="charoff">
        <xsl:value-of select="@charoff"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@valign">
      <xsl:attribute name="valign">
        <xsl:value-of select="@valign"/>
      </xsl:attribute>
    </xsl:if>

    <xsl:apply-templates select="row[1]">
      <xsl:with-param name="spans">
        <xsl:call-template name="blank.spans">
          <xsl:with-param name="cols" select="../@cols"/>
        </xsl:call-template>
      </xsl:with-param>
    </xsl:apply-templates>

  </xsl:element>
</xsl:template>

<xsl:template match="tbody">
  <tbody>
    <xsl:if test="@align">
      <xsl:attribute name="align">
        <xsl:value-of select="@align"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@char">
      <xsl:attribute name="char">
        <xsl:value-of select="@char"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@charoff">
      <xsl:attribute name="charoff">
        <xsl:value-of select="@charoff"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@valign">
      <xsl:attribute name="valign">
        <xsl:value-of select="@valign"/>
      </xsl:attribute>
    </xsl:if>

    <xsl:apply-templates select="row[1]">
      <xsl:with-param name="spans">
        <xsl:call-template name="blank.spans">
          <xsl:with-param name="cols" select="../@cols"/>
        </xsl:call-template>
      </xsl:with-param>
    </xsl:apply-templates>

  </tbody>
</xsl:template>

<xsl:template match="row">
  <xsl:param name="spans"/>

  <xsl:variable name="row-height">
    <xsl:if test="processing-instruction('dbhtml')">
      <xsl:call-template name="dbhtml-attribute">
        <xsl:with-param name="pis" select="processing-instruction('dbhtml')"/>
        <xsl:with-param name="attribute" select="'row-height'"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:variable>

  <tr>
    <xsl:if test="$row-height != ''">
      <xsl:attribute name="height">
        <xsl:value-of select="$row-height"/>
      </xsl:attribute>
    </xsl:if>

    <xsl:if test="$table.borders.with.css != 0">
      <xsl:if test="@rowsep = 1">
        <xsl:attribute name="style">
          <xsl:call-template name="border">
            <xsl:with-param name="side" select="'bottom'"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:if>
    </xsl:if>

    <xsl:if test="@align">
      <xsl:attribute name="align">
        <xsl:value-of select="@align"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@char">
      <xsl:attribute name="char">
        <xsl:value-of select="@char"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@charoff">
      <xsl:attribute name="charoff">
        <xsl:value-of select="@charoff"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@valign">
      <xsl:attribute name="valign">
        <xsl:value-of select="@valign"/>
      </xsl:attribute>
    </xsl:if>

    <xsl:apply-templates select="entry[1]">
      <xsl:with-param name="spans" select="$spans"/>
    </xsl:apply-templates>
  </tr>

  <xsl:if test="following-sibling::row">
    <xsl:variable name="nextspans">
      <xsl:apply-templates select="entry[1]" mode="span">
        <xsl:with-param name="spans" select="$spans"/>
      </xsl:apply-templates>
    </xsl:variable>

    <xsl:apply-templates select="following-sibling::row[1]">
      <xsl:with-param name="spans" select="$nextspans"/>
    </xsl:apply-templates>
  </xsl:if>
</xsl:template>

<xsl:template match="entry" name="entry">
  <xsl:param name="col" select="1"/>
  <xsl:param name="spans"/>

  <xsl:variable name="cellgi">
    <xsl:choose>
      <xsl:when test="ancestor::thead">th</xsl:when>
      <xsl:when test="ancestor::tfoot">th</xsl:when>
      <xsl:otherwise>td</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="empty.cell" select="count(node()) = 0"/>

  <xsl:variable name="named.colnum">
    <xsl:call-template name="entry.colnum"/>
  </xsl:variable>

  <xsl:variable name="entry.colnum">
    <xsl:choose>
      <xsl:when test="$named.colnum &gt; 0">
        <xsl:value-of select="$named.colnum"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$col"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="entry.colspan">
    <xsl:choose>
      <xsl:when test="@spanname or @namest">
        <xsl:call-template name="calculate.colspan"/>
      </xsl:when>
      <xsl:otherwise>1</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="following.spans">
    <xsl:call-template name="calculate.following.spans">
      <xsl:with-param name="colspan" select="$entry.colspan"/>
      <xsl:with-param name="spans" select="$spans"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="rowsep">
    <xsl:call-template name="inherited.table.attribute">
      <xsl:with-param name="entry" select="."/>
      <xsl:with-param name="colnum" select="$entry.colnum"/>
      <xsl:with-param name="attribute" select="'rowsep'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="colsep">
    <xsl:call-template name="inherited.table.attribute">
      <xsl:with-param name="entry" select="."/>
      <xsl:with-param name="colnum" select="$entry.colnum"/>
      <xsl:with-param name="attribute" select="'colsep'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="valign">
    <xsl:call-template name="inherited.table.attribute">
      <xsl:with-param name="entry" select="."/>
      <xsl:with-param name="colnum" select="$entry.colnum"/>
      <xsl:with-param name="attribute" select="'valign'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="align">
    <xsl:call-template name="inherited.table.attribute">
      <xsl:with-param name="entry" select="."/>
      <xsl:with-param name="colnum" select="$entry.colnum"/>
      <xsl:with-param name="attribute" select="'align'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="char">
    <xsl:call-template name="inherited.table.attribute">
      <xsl:with-param name="entry" select="."/>
      <xsl:with-param name="colnum" select="$entry.colnum"/>
      <xsl:with-param name="attribute" select="'char'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="charoff">
    <xsl:call-template name="inherited.table.attribute">
      <xsl:with-param name="entry" select="."/>
      <xsl:with-param name="colnum" select="$entry.colnum"/>
      <xsl:with-param name="attribute" select="'charoff'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$spans != '' and not(starts-with($spans,'0:'))">
      <xsl:call-template name="entry">
        <xsl:with-param name="col" select="$col+1"/>
        <xsl:with-param name="spans" select="substring-after($spans,':')"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:when test="$entry.colnum &gt; $col">
      <xsl:call-template name="empty.table.cell"/>
      <xsl:call-template name="entry">
        <xsl:with-param name="col" select="$col+1"/>
        <xsl:with-param name="spans" select="substring-after($spans,':')"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:otherwise>
      <xsl:variable name="entry-bgcolor">
        <xsl:if test="processing-instruction('dbhtml')">
          <xsl:call-template name="dbhtml-attribute">
            <xsl:with-param name="pis" select="processing-instruction('dbhtml')"/>
            <xsl:with-param name="attribute" select="'entry-bgcolor'"/>
          </xsl:call-template>
        </xsl:if>
      </xsl:variable>

      <xsl:element name="{$cellgi}">
        <xsl:if test="$entry-bgcolor != ''">
          <xsl:attribute name="bgcolor">
            <xsl:value-of select="$entry-bgcolor"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="$show.revisionflag and @revisionflag">
          <xsl:attribute name="class">
            <xsl:value-of select="@revisionflag"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="$table.borders.with.css != 0">
          <xsl:attribute name="style">
            <xsl:if test="$colsep &gt; 0">
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'right'"/>
              </xsl:call-template>
            </xsl:if>
            <xsl:if test="$rowsep &gt; 0">
              <xsl:call-template name="border">
                <xsl:with-param name="side" select="'bottom'"/>
              </xsl:call-template>
            </xsl:if>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="@morerows &gt; 0">
          <xsl:attribute name="rowspan">
            <xsl:value-of select="1+@morerows"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="$entry.colspan &gt; 1">
          <xsl:attribute name="colspan">
            <xsl:value-of select="$entry.colspan"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="$align != ''">
          <xsl:attribute name="align">
            <xsl:value-of select="$align"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="$valign != ''">
          <xsl:attribute name="valign">
            <xsl:value-of select="$valign"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="$char != ''">
          <xsl:attribute name="char">
            <xsl:value-of select="$char"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="$charoff != ''">
          <xsl:attribute name="charoff">
            <xsl:value-of select="$charoff"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="not(preceding-sibling::*) and ancestor::row/@id">
          <xsl:call-template name="anchor">
            <xsl:with-param name="node" select="ancestor::row[1]"/>
          </xsl:call-template>
        </xsl:if>

        <xsl:call-template name="anchor"/>

        <xsl:choose>
          <xsl:when test="$empty.cell">
            <xsl:text>&#160;</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:element>

      <xsl:choose>
        <xsl:when test="following-sibling::entry">
          <xsl:apply-templates select="following-sibling::entry[1]">
            <xsl:with-param name="col" select="$col+$entry.colspan"/>
            <xsl:with-param name="spans" select="$following.spans"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="finaltd">
            <xsl:with-param name="spans" select="$following.spans"/>
            <xsl:with-param name="col" select="$col+$entry.colspan"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="entry" name="sentry" mode="span">
  <xsl:param name="col" select="1"/>
  <xsl:param name="spans"/>

  <xsl:variable name="entry.colnum">
    <xsl:call-template name="entry.colnum"/>
  </xsl:variable>

  <xsl:variable name="entry.colspan">
    <xsl:choose>
      <xsl:when test="@spanname or @namest">
        <xsl:call-template name="calculate.colspan"/>
      </xsl:when>
      <xsl:otherwise>1</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="following.spans">
    <xsl:call-template name="calculate.following.spans">
      <xsl:with-param name="colspan" select="$entry.colspan"/>
      <xsl:with-param name="spans" select="$spans"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$spans != '' and not(starts-with($spans,'0:'))">
      <xsl:value-of select="substring-before($spans,':')-1"/>
      <xsl:text>:</xsl:text>
      <xsl:call-template name="sentry">
        <xsl:with-param name="col" select="$col+1"/>
        <xsl:with-param name="spans" select="substring-after($spans,':')"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:when test="$entry.colnum &gt; $col">
      <xsl:text>0:</xsl:text>
      <xsl:call-template name="sentry">
        <xsl:with-param name="col" select="$col+$entry.colspan"/>
        <xsl:with-param name="spans" select="$following.spans"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="copy-string">
        <xsl:with-param name="count" select="$entry.colspan"/>
        <xsl:with-param name="string">
          <xsl:choose>
            <xsl:when test="@morerows">
              <xsl:value-of select="@morerows"/>
            </xsl:when>
            <xsl:otherwise>0</xsl:otherwise>
          </xsl:choose>
          <xsl:text>:</xsl:text>
        </xsl:with-param>
      </xsl:call-template>

      <xsl:choose>
        <xsl:when test="following-sibling::entry">
          <xsl:apply-templates select="following-sibling::entry[1]"
                               mode="span">
            <xsl:with-param name="col" select="$col+$entry.colspan"/>
            <xsl:with-param name="spans" select="$following.spans"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="sfinaltd">
            <xsl:with-param name="spans" select="$following.spans"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="generate.colgroup">
  <xsl:param name="cols" select="1"/>
  <xsl:param name="count" select="1"/>
  <xsl:choose>
    <xsl:when test="$count &gt; $cols"></xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="generate.col">
        <xsl:with-param name="countcol" select="$count"/>
      </xsl:call-template>
      <xsl:call-template name="generate.colgroup">
        <xsl:with-param name="cols" select="$cols"/>
        <xsl:with-param name="count" select="$count+1"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="generate.col">
  <xsl:param name="countcol">1</xsl:param>
  <xsl:param name="colspecs" select="./colspec"/>
  <xsl:param name="count">1</xsl:param>
  <xsl:param name="colnum">1</xsl:param>

  <xsl:choose>
    <xsl:when test="$count>count($colspecs)">
      <col/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="colspec" select="$colspecs[$count=position()]"/>
      <xsl:variable name="colspec.colnum">
        <xsl:choose>
          <xsl:when test="$colspec/@colnum">
            <xsl:value-of select="$colspec/@colnum"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$colnum"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <xsl:choose>
        <xsl:when test="$colspec.colnum=$countcol">
          <col>
            <xsl:if test="$colspec/@colwidth
                          and $use.extensions != 0
                          and $tablecolumns.extension != 0">
              <xsl:attribute name="width">
                <xsl:value-of select="$colspec/@colwidth"/>
              </xsl:attribute>
            </xsl:if>

            <xsl:choose>
              <xsl:when test="$colspec/@align">
                <xsl:attribute name="align">
                  <xsl:value-of select="$colspec/@align"/>
                </xsl:attribute>
              </xsl:when>
              <!-- Suggested by Pavel ZAMPACH <zampach@nemcb.cz> -->
              <xsl:when test="$colspecs/ancestor::tgroup/@align">
                <xsl:attribute name="align">
                  <xsl:value-of select="$colspecs/ancestor::tgroup/@align"/>
                </xsl:attribute>
              </xsl:when>
            </xsl:choose>

            <xsl:if test="$colspec/@char">
              <xsl:attribute name="char">
                <xsl:value-of select="$colspec/@char"/>
              </xsl:attribute>
            </xsl:if>
            <xsl:if test="$colspec/@charoff">
              <xsl:attribute name="charoff">
                <xsl:value-of select="$colspec/@charoff"/>
              </xsl:attribute>
            </xsl:if>
          </col>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="generate.col">
            <xsl:with-param name="countcol" select="$countcol"/>
            <xsl:with-param name="colspecs" select="$colspecs"/>
            <xsl:with-param name="count" select="$count+1"/>
            <xsl:with-param name="colnum">
              <xsl:choose>
                <xsl:when test="$colspec/@colnum">
                  <xsl:value-of select="$colspec/@colnum + 1"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="$colnum + 1"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:with-param>
           </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>

<xsl:template name="colspec.colwidth">
  <!-- when this macro is called, the current context must be an entry -->
  <xsl:param name="colname"></xsl:param>
  <!-- .. = row, ../.. = thead|tbody, ../../.. = tgroup -->
  <xsl:param name="colspecs" select="../../../../tgroup/colspec"/>
  <xsl:param name="count">1</xsl:param>
  <xsl:choose>
    <xsl:when test="$count>count($colspecs)"></xsl:when>
    <xsl:otherwise>
      <xsl:variable name="colspec" select="$colspecs[$count=position()]"/>
      <xsl:choose>
        <xsl:when test="$colspec/@colname=$colname">
          <xsl:value-of select="$colspec/@colwidth"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="colspec.colwidth">
            <xsl:with-param name="colname" select="$colname"/>
            <xsl:with-param name="colspecs" select="$colspecs"/>
            <xsl:with-param name="count" select="$count+1"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>

