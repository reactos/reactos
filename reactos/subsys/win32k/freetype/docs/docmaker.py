#!/usr/bin/env python
#
# DocMaker is a very simple program used to generate HTML documentation
# from the source files of the FreeType packages.
#

import fileinput, sys, string

# This function is used to parse a source file, and extract comment blocks
# from it. The following comment block formats are recognized :
#
#  /**************************
#   *
#   *  FORMAT1
#   *
#   *
#   *
#   *
#   *************************/
#
#  /**************************/
#  /*                        */
#  /*  FORMAT2               */
#  /*                        */
#  /*                        */
#  /*                        */
#  /*                        */
#
#  /**************************/
#  /*                        */
#  /*  FORMAT3               */
#  /*                        */
#  /*                        */
#  /*                        */
#  /*                        */
#  /**************************/
#
# Each block is modeled as a simple list of text strings
# The function returns a list of blocks, i.e. a list of strings lists
#

def make_block_list():
    """parse a file and extract comments blocks from it"""

    list   = []
    block  = []
    format = 0

    for line in fileinput.input():

	line = string.strip(line)
	l    = len(line)

	if format == 0:
	    if l > 3 and line[0:3] == '/**':
		i = 3
		while (i < l) and (line[i] == '*'):
		    i = i+1

		if i == l:
		    # this is '/**' followed by any number of '*', the
		    # beginning of a Format 1 block
		    #
		    block  = [];
		    format = 1;

		elif (i == l-1) and (line[i] == '/'):
		    # this is '/**' followed by any number of '*', followed
		    # by a '/', i.e. the beginning of a Format 2 or 3 block
		    #
		    block  = [];
		    format = 2;
				
	##############################################################
	#
	# FORMAT 1
	#
	elif format == 1:

	    # if the line doesn't begin with a "*", something went
	    # wrong, and we must exit, and forget the current block..
	    if (l == 0) or (line[0] != '*'):
		block  = []
		format = 0
		
	    # otherwise, we test for an end of block, which is an
	    # arbitrary number of '*', followed by '/'
	    else:
		i = 1
		while (i < l) and (line[i] == '*'):
		    i = i+1

		# test for the end of the block
		if (i < l) and (line[i] == '/'):
		    if block != []: list.append( block )
		    format = 0
		    block  = []

		else:
		    block.append( line[1:] )

	##############################################################
	#
	# FORMAT 2
	#
	elif format == 2:

	    # if the line doesn't begin with '/*' and end with '*/',
	    # this is the end of the format 2 format..
	    if (l < 4 ) or (line[:2] != '/*') or (line[-2:] != '*/'):
		if block != []: list.append(block)
		block  = []
		format = 0
		continue

	    # remove the start and end comment delimiters, then right-strip
	    # the line
	    line = string.rstrip(line[2:-2])

	    # check for end of a format2 block, i.e. a run of '*'
	    if string.count(line,'*') == l-4:
		if block != []: list.append(block)
		block = []
		format = 0
	    else:
		# otherwise, add the line to the current block
		block.append(line)

    return list
    
    
# This function is only used for debugging
#
def dump_block_list( list ):
    """dump a comment block list"""
    for block in list:
	print "----------------------------------------"
	for line in block:
	    print line
    print "---------the end-----------------------"





######################################################################################
#
#
# The DocParagraph is used to store either simple text paragraph or
# source code lines
#
#
# If the paragraph contains source code (use code=1 when initializing the
# object), self.lines is a list of source code strings
#
# Otherwise, self.lines is simply a list of words for the paragraph
#
class DocParagraph:

    def __init__(self,code=0,margin=0):
	self.lines  = []
	self.code   = code
	self.margin = margin 
	
    def	add(self,line):
    
	if self.code==0:
	    # get rid of unwanted spaces in the paragraph
	    self.lines.extend( string.split(line) )
	
	else:
	    # remove margin whitespace
	    if string.strip( line[:self.margin] ) == "": line = line[self.margin:]
	    self.lines.append(line)

    
    def dump(self):

	max_width = 50

	if self.code == 0:
	    cursor = 0
	    line   = ""
	    
	    for word in self.lines:

		if cursor+len(word)+1 > max_width:
		    print line
		    cursor = 0
		    line = ""

		line   = line + word + " "
		cursor = cursor + len(word) + 1
		
	    if cursor > 0:
		print line

	else:
	    for line in self.lines:
		print "--" + line
		
	print ""


######################################################################################
#
#
# The DocContent class is used to store the content of a given marker
# Each DocContent can have its own text, plus a list of fields. Each field
# has its own text too
#
# Hence, the following lines :
#
#    """  
#    Some arbitraty text:
#
#      fieldone :: some arbitraty text for this field,
#                  note that the text stretches to several lines
#
#      fieldtwo :: some other text
#
#    """
#
# will be stored as (each text is a list of string:
#
#    self.fields = [ "", "fieldone", "fieldtwo" ]
#    self.texts  = [
#                    [ "some arbitraty text for this field," ,
#                      "note that the text stretches to several lines" ],
#
#                    [ "some other text" ]
#                  ]
#
#
class DocContent:

    def __init__(self, paragraph_lines=[]):
    
	self.fields = []
	self.texts  = []
	
	code_mode   = 0
	code_margin = 0
	
	field       = ""
	text        = []
	paragraph   = None 

	for aline in paragraph_lines:
	
	    if code_mode == 0:
		line   = string.lstrip(aline) 
	        l      = len(line)
		margin = len(aline) - l

		# if the line is empty, this is the end of the current
		# paragraph
		if line == "":
		    if paragraph:
			text.append(paragraph)
			paragraph = None
		    continue
	    
		# test for the beginning of a code block, i.e.'{' is the first
		# and only character on the line..
		#
		if line == '{':
		    code_mode   = 1
		    code_margin = margin
		    if paragraph:
			text.append(paragraph)
		    paragraph = DocParagraph( 1, margin )
		    continue
	    
		words = string.split(line)
		
		# test for a field delimiter on the start of the line, i.e.
		# the oken `::'
		#
		if len(words) >= 2 and words[1] == "::":
		    if paragraph:
			text.append(paragraph)
			paragraph = None
			
		    self.fields.append(field)
		    self.texts.append(text)
		
		    field = words[0]
		    text  = []
		    words = words[2:]
			
		if len(words) > 0:
		    line = string.join(words)
		    if not paragraph:
			paragraph = DocParagraph()
		    paragraph.add( line )
	    
	    else:
		line = aline
		
		# the code block ends with a line that has a single '}' on it
		if line == " "*code_margin+'}':
		    text.append(paragraph)
		    paragraph   = None
		    code_mode   = 0
		    code_margin = 0
		
		# otherwise, add the line to the current paragraph
		else:
		    paragraph.add(line)
	
	if paragraph:
	    text.append(paragraph)
	    
	self.fields.append( field )
	self.texts.append( text )    


    
    def dump(self):
	for i in range(len(self.fields)):
	    field = self.fields[i]
	    if field: print "<field "+field+">"

	    for paras in self.texts[i]:
		paras.dump()
	    
	    if field: print "</field>	"

    def dump_html(self):
        n = len(self.fields)
        for i in range(n):
            field = self.fields[i]
            if field==[]:
                print "<p>"
                for paras in self.texts[i]:
                    print "<p>"
                    paras.dump()
                    print "</p>"
            else:
                if i==1:
                    print "<table cellpadding=4><tr valign=top><td>"
                else:
                    print "</td></tr><tr valign=top><td>"
                
                print "<b>"+field+"</b></td><td>"
                
                for paras in self.texts[i]:
                    print "<p>"
                    paras.dump()
                    print "</p>"
                    
                print "</td></tr>"
        if n > 1:
            print "</table>"
              

######################################################################################
#
#
# The DocBlock class is used to store a given comment block. It contains
# a list of markers, as well as a list of contents for each marker.
#
#
class DocBlock:

    def __init__(self, block_line_list=[]):
	self.markers  = []
	self.contents = []
    
        marker   = ""
        content  = []
        alphanum = string.letters + string.digits + "_"
    
        for line in block_line_list:
    	    line2  = string.lstrip(line)
	    l      = len(line2)
	    margin = len(line) - l
	
	    if l > 3 and line2[0] == '<':
	        i = 1
	        while i < l and line2[i] in alphanum: i = i+1
	        if i < l and line2[i] == '>':
		    if marker or content:
    			self.add( marker, content )
		    marker  = line2[1:i]
		    content = []
		    line2   = string.lstrip(line2[i+1:])
		    l       = len(line2)
		    line    = " "*margin + line2
	
	    content.append(line)
	
	if marker or content:
	    self.add( marker, content )
	
	
    def add( self, marker, lines ):
    
	# remove the first and last empty lines from the content list
	l = len(lines)
	if l > 0:
	    i = 0
	    while l > 0 and string.strip(lines[l-1]) == "": l = l-1
	    while i < l and string.strip(lines[i]) == "": i = i+1
	    lines = lines[i:l]
	    l     = len(lines)
	    
	# add a new marker only if its marker and its content list aren't empty
	if l > 0 and marker:
	    self.markers.append(marker)
	    self.contents.append(lines)

    def dump( self ):
	for i in range( len(self.markers) ):
	    print "["+self.markers[i]+"]"
	    for line in self.contents[i]:
		print "-- "+line

    def doc_contents(self):
	contents = []
	for item in self.contents:
	  contents.append( DocContent(item) )
	return contents


def dump_doc_blocks( block_list ):
    for block in block_list:
	docblock = DocBlock(block)
	docblock.dump()
	print "<<------------------->>"


#
#
#
def dump_single_content( block_list ):

    block = block_list[0]
    docblock = DocBlock(block)

    print "<block>"
    for i in range(len(docblock.markers)):
	marker   = docblock.markers[i]
	contents = docblock.contents[i]
	
	print "<marker "+marker+">"
	doccontent = DocContent( contents )

	doccontent.dump()
		
	print "</marker>"
	
    print "</block>"        

def dump_doc_contents( block_list ):

    for block in block_list:
	docblock = DocBlock(block)
	print "<block>"
	
	for i in range(len(docblock.markers)):
	    print "<marker "+docblock.markers[i]+">"
	    content = DocContent( docblock.contents[i] )
	    content.dump()
	    print "</marker>"
	print "</block>"

def dump_html_1( block_list ):
    
    print "<html><body>"
    types = [ 'Type', 'Struct', 'FuncType', 'Function', 'Constant', 'Enumeration' ]
    for block in block_list:
          docblock = DocBlock(block)
          print "<hr>"
          for i in range(len(docblock.markers)):
              marker   = docblock.markers[i]
              content  = docblock.contents[i]
              dcontent = DocContent( content )
              
              if marker=="Description":
                  print "<ul><p>"
                  dcontent.dump()
                  print "</p></ul>"
                  
              elif marker in types:
                  print "<h3><font color=blue>"+content[0]+"</font></h3>"
              else:
                  print "<h4>"+marker+"</h4>"
                  print "<ul><p>"
                  dcontent.dump_html()
                  print "</p></ul>"
                  
              print ""
              
    print "<hr></body></html>"
              

def main(argv):
    """main program loop"""
    print "extracting comment blocks from sources .."
    list = make_block_list()
    
#    dump_block_list( list )

#    dump_doc_blocks( list )

#    print "dumping block contents .."
#    dump_doc_contents(list)

    dump_html_1(list)
     
#    dump_single_content(list)

# If called from the command line
if __name__=='__main__': main(sys.argv)
