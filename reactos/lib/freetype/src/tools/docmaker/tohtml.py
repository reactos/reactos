from sources import *
from content import *
from formatter import *

import time

# The following defines the HTML header used by all generated pages.
#
html_header_1 = """\
<html>
<header>
<title>"""

html_header_2= """ API Reference</title>
<basefont face="Verdana,Geneva,Arial,Helvetica">
<style content="text/css">
  P { text-align=justify }
  H1 { text-align=center }
  LI { text-align=justify }
</style>
</header>
<body text=#000000
      bgcolor=#FFFFFF
      link=#0000EF
      vlink=#51188E
      alink=#FF0000>
<center><h1>"""

html_header_3=""" API Reference</h1></center>
"""



# The HTML footer used by all generated pages.
#
html_footer = """\
</body>
</html>"""

# The header and footer used for each section.
#
section_title_header = "<center><h1>"
section_title_footer = "</h1></center>"

# The header and footer used for code segments.
#
code_header = "<font color=blue><pre>"
code_footer = "</pre></font>"

# Paragraph header and footer.
#
para_header = "<p>"
para_footer = "</p>"

# Block header and footer.
#
block_header = "<center><table width=75%><tr><td>"
block_footer = "</td></tr></table><hr width=75%></center>"

# Description header/footer.
#
description_header = "<center><table width=87%><tr><td>"
description_footer = "</td></tr></table></center><br>"

# Marker header/inter/footer combination.
#
marker_header = "<center><table width=87% cellpadding=5><tr bgcolor=#EEEEFF><td><em><b>"
marker_inter  = "</b></em></td></tr><tr><td>"
marker_footer = "</td></tr></table></center>"

# Source code extracts header/footer.
#
source_header = "<center><table width=87%><tr bgcolor=#D6E8FF width=100%><td><pre>\n"
source_footer = "\n</pre></table></center><br>"

# Chapter header/inter/footer.
#
chapter_header = "<br><center><table width=75%><tr><td><h2>"
chapter_inter  = "</h2><ul>"
chapter_footer = "</ul></td></tr></table></center>"


# source language keyword coloration/styling
#
keyword_prefix = '<font color="darkblue">'
keyword_suffix = '</font>'

section_synopsis_header = '<h2>Synopsys</h2><font color="cyan">'
section_synopsis_footer = '</font>'

# Translate a single line of source to HTML.  This will convert
# a "<" into "&lt.", ">" into "&gt.", etc.
#
def html_quote( line ):
    result = string.replace( line,   "&", "&amp;" )
    result = string.replace( result, "<", "&lt;" )
    result = string.replace( result, ">", "&gt;" )
    return result


# same as 'html_quote', but ignores left and right brackets
#
def html_quote0( line ):
    return string.replace( line, "&", "&amp;" )


def dump_html_code( lines, prefix = "" ):
    # clean the last empty lines
    #
    l = len( self.lines )
    while l > 0 and string.strip( self.lines[l - 1] ) == "":
        l = l - 1

    # The code footer should be directly appended to the last code
    # line to avoid an additional blank line.
    #
    print prefix + code_header,
    for line in self.lines[0 : l+1]:
        print '\n' + prefix + html_quote(line),
    print prefix + code_footer,



class HtmlFormatter(Formatter):

    def __init__( self, processor, project_title, file_prefix ):

        Formatter.__init__( self, processor )

        global html_header_1, html_header_2, html_header_3, html_footer

        if file_prefix:
            file_prefix = file_prefix + "-"
        else:
            file_prefix = ""

        self.project_title = project_title
        self.file_prefix   = file_prefix
        self.html_header   = html_header_1 + project_title + html_header_2 + \
                             project_title + html_header_3

        self.html_footer = "<p><center><font size=""-2"">generated on " +   \
                            time.asctime( time.localtime( time.time() ) ) + \
                           "</font></p></center>" + html_footer

        self.columns = 3

    def  make_section_url( self, section ):
        return self.file_prefix + section.name + ".html"


    def  make_block_url( self, block ):
        return self.make_section_url( block.section ) + "#" + block.name


    def  make_html_words( self, words ):
        """ convert a series of simple words into some HTML text """
        line = ""
        if words:
            line = html_quote( words[0] )
            for w in words[1:]:
                line = line + " " + html_quote( w )

        return line


    def  make_html_word( self, word ):
        """analyze a simple word to detect cross-references and styling"""
        # look for cross-references
        #
        m = re_crossref.match( word )
        if m:
            try:
                name = m.group(1)
                block = self.identifiers[ name ]
                url   = self.make_block_url( block )
                return '<a href="' + url + '">' + name + '</a>'
            except:
                return '?' + name + '?'

        # look for italics and bolds
        m = re_italic.match( word )
        if m:
            name = m.group(1)
            return '<i>'+name+'</i>'

        m = re_bold.match( word )
        if m:
            name = m.group(1)
            return '<b>'+name+'</b>'

        return html_quote(word)


    def  make_html_para( self, words ):
        """ convert a paragraph's words into tagged HTML text, handle xrefs """
        line = ""
        if words:
            line = self.make_html_word( words[0] )
            for word in words[1:]:
                line = line + " " + self.make_html_word( word )

        return "<p>" + line + "</p>"


    def  make_html_code( self, lines ):
        """ convert a code sequence to HTML """
        line = code_header + '\n'
        for l in lines:
            line = line + html_quote( l ) + '\n'

        return line + code_footer


    def  make_html_items( self, items ):
        """ convert a field's content into some valid HTML """
        lines = []
        for item in items:
            if item.lines:
                lines.append( self.make_html_code( item.lines ) )
            else:
                lines.append( self.make_html_para( item.words ) )

        return string.join( lines, '\n' )


    def  print_html_items( self, items ):
        print self.make_html_items( items )


    def print_html_field( self, field ):
        if field.name:
            print "<table valign=top><tr><td><b>"+field.name+"</b></td><td>"

        print self.make_html_items( field.items )

        if field.name:
            print "</td></tr></table>"


    def html_source_quote( self, line, block_name = None ):
        result = ""
        while line:
            m = re_source_crossref.match( line )
            if m:
                name   = m.group(2)
                prefix = html_quote( m.group(1) )
                length = len( m.group(0) )

                if name == block_name:
                    # this is the current block name, if any
                    result = result + prefix + '<b>' + name + '</b>'

                elif re_source_keywords.match(name):
                    # this is a C keyword
                    result = result + prefix + keyword_prefix + name + keyword_suffix

                elif self.identifiers.has_key(name):
                    # this is a known identifier
                    block = self.identifiers[name]
                    result = result + prefix + '<a href="' + \
                             self.make_block_url(block) + '">' + name + '</a>'
                else:
                    result = result + html_quote(line[ : length ])

                line = line[ length : ]
            else:
                result = result + html_quote(line)
                line   = []

        return result


    def print_html_field_list( self, fields ):
        print "<table valign=top cellpadding=3>"
        for field in fields:
            print "<tr valign=top><td><b>" + field.name + "</b></td><td>"
            self.print_html_items( field.items )
            print "</td></tr>"
        print "</table>"


    def print_html_markup( self, markup ):
        table_fields = []
        for field in markup.fields:
            if field.name:
                # we begin a new series of field or value definitions, we
                # will record them in the 'table_fields' list before outputting
                # all of them as a single table
                #
                table_fields.append( field )

            else:
                if table_fields:
                    self.print_html_field_list( table_fields )
                    table_fields = []

                self.print_html_items( field.items )

        if table_fields:
            self.print_html_field_list( table_fields )

    #
    #  Formatting the index
    #

    def  index_enter( self ):
        print self.html_header
        self.index_items = {}

    def  index_name_enter( self, name ):
        block = self.identifiers[ name ]
        url   = self.make_block_url( block )
        self.index_items[ name ] = url

    def  index_exit( self ):

        # block_index already contains the sorted list of index names
        count = len( self.block_index )
        rows  = (count + self.columns - 1)/self.columns

        print "<center><table border=0 cellpadding=0 cellspacing=0>"
        for r in range(rows):
            line = "<tr>"
            for c in range(self.columns):
                i = r + c*rows
                if i < count:
                    bname = self.block_index[ r + c*rows ]
                    url   = self.index_items[ bname ]
                    line = line + '<td><a href="' + url + '">' + bname + '</a></td>'
                else:
                    line = line + '<td></td>'
            line = line + "</tr>"
            print line

        print "</table></center>"
        print self.html_footer
        self.index_items = {}

    def  index_dump( self, index_filename = None ):

        if index_filename == None:
            index_filename = self.file_prefix + "index.html"

        Formatter.index_dump( self, index_filename )

    #
    #  Formatting the table of content
    #
    def  toc_enter( self ):
        print self.html_header
        print "<center><h1>Table of Contents</h1></center>"

    def  toc_chapter_enter( self, chapter ):
        print  chapter_header + string.join(chapter.title) + chapter_inter
        print "<table cellpadding=5>"

    def  toc_section_enter( self, section ):
        print "<tr valign=top><td>"
        print '<a href="' + self.make_section_url( section ) + '">' + \
               section.title + '</a></td><td>'

        print self.make_html_para( section.abstract )

    def  toc_section_exit( self, section ):
        print "</td></tr>"

    def  toc_chapter_exit( self, chapter ):
        print "</table>"
        print  chapter_footer

    def  toc_index( self, index_filename ):
        print chapter_header + '<a href="' + index_filename + '">Global Index</a>' + chapter_inter + chapter_footer

    def  toc_exit( self ):
        print "</table></center>"
        print self.html_footer

    def  toc_dump( self, toc_filename = None, index_filename = None ):
        if toc_filename == None:
            toc_filename = self.file_prefix + "toc.html"

        if index_filename == None:
            index_filename = self.file_prefix + "index.html"

        Formatter.toc_dump( self, toc_filename, index_filename )

    #
    #  Formatting sections
    #
    def  section_enter( self, section ):
        print self.html_header

        print section_title_header
        print section.title
        print section_title_footer

        # print section synopsys
        print section_synopsis_header
        print "<center><table cellspacing=5 cellpadding=0 border=0>"

        maxwidth = 0
        for b in section.blocks.values():
            if len(b.name) > maxwidth:
                maxwidth = len(b.name)

        width  = 70  # XXX magic number
        columns = width / maxwidth
        if columns < 1:
            columns = 1

        count   = len(section.block_names)
        rows    = (count + columns-1)/columns
        for r in range(rows):
            line = "<tr>"
            for c in range(columns):
                i = r + c*rows
                line = line + '<td></td><td>'
                if i < count:
                    name = section.block_names[i]
                    line = line + '<a href="#' + name + '">' + name + '</a>'

                line = line + '</td>'
            line = line + "</tr>"
            print line

        print "</table></center><br><br>"
        print section_synopsis_footer

        print description_header
        print self.make_html_items( section.description )
        print description_footer

    def  block_enter( self, block ):
        print block_header

        # place html anchor if needed
        if block.name:
            print '<a name="' + block.name + '">'
            print "<h4>" + block.name + "</h4>"
            print "</a>"

        # dump the block C source lines now
        if block.code:
            print source_header
            for l in block.code:
                print self.html_source_quote( l, block.name )
            print source_footer


    def  markup_enter( self, markup, block ):
        if markup.tag == "description":
            print description_header
        else:
            print marker_header + markup.tag + marker_inter

        self.print_html_markup( markup )

    def  markup_exit( self, markup, block ):
        if markup.tag == "description":
            print description_footer
        else:
            print marker_footer

    def  block_exit( self, block ):
        print block_footer


    def  section_exit( self, section ):
        print html_footer


    def section_dump_all( self ):
        for section in self.sections:
            self.section_dump( section, self.file_prefix + section.name + '.html' )
        