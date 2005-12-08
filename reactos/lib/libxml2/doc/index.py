#!/usr/bin/python -u
#
# imports the API description and fills up a database with
# name relevance to modules, functions or web pages
#
# Operation needed:
# =================
#
# install mysqld, the python wrappers for mysql and libxml2, start mysqld
# Change the root passwd of mysql:
#    mysqladmin -u root password new_password
# Create the new database xmlsoft
#    mysqladmin -p create xmlsoft
# Create a database user 'veillard' and give him passord access
# change veillard and abcde with the right user name and passwd
#    mysql -p
#    password:
#    mysql> GRANT ALL PRIVILEGES ON xmlsoft TO veillard@localhost
#           IDENTIFIED BY 'abcde' WITH GRANT OPTION;
#
# As the user check the access:
#    mysql -p xmlsoft
#    Enter password:
#    Welcome to the MySQL monitor....
#    mysql> use xmlsoft
#    Database changed
#    mysql> quit
#    Bye
#
# Then run the script in the doc subdir, it will create the symbols and
# word tables and populate them with informations extracted from 
# the libxml2-api.xml API description, and make then accessible read-only
# by nobody@loaclhost the user expected to be Apache's one
#
# On the Apache configuration, make sure you have php support enabled
#

import MySQLdb
import libxml2
import sys
import string
import os

#
# We are not interested in parsing errors here
#
def callback(ctx, str):
    return
libxml2.registerErrorHandler(callback, None)

#
# The dictionnary of tables required and the SQL command needed
# to create them
#
TABLES={
  "symbols" : """CREATE TABLE symbols (
           name varchar(255) BINARY NOT NULL,
	   module varchar(255) BINARY NOT NULL,
           type varchar(25) NOT NULL,
	   descr varchar(255),
	   UNIQUE KEY name (name),
	   KEY module (module))""",
  "words" : """CREATE TABLE words (
           name varchar(50) BINARY NOT NULL,
	   symbol varchar(255) BINARY NOT NULL,
           relevance int,
	   KEY name (name),
	   KEY symbol (symbol),
	   UNIQUE KEY ID (name, symbol))""",
  "wordsHTML" : """CREATE TABLE wordsHTML (
           name varchar(50) BINARY NOT NULL,
	   resource varchar(255) BINARY NOT NULL,
	   section varchar(255),
	   id varchar(50),
           relevance int,
	   KEY name (name),
	   KEY resource (resource),
	   UNIQUE KEY ref (name, resource))""",
  "wordsArchive" : """CREATE TABLE wordsArchive (
           name varchar(50) BINARY NOT NULL,
	   ID int(11) NOT NULL,
           relevance int,
	   KEY name (name),
	   UNIQUE KEY ref (name, ID))""",
  "pages" : """CREATE TABLE pages (
           resource varchar(255) BINARY NOT NULL,
	   title varchar(255) BINARY NOT NULL,
	   UNIQUE KEY name (resource))""",
  "archives" : """CREATE TABLE archives (
           ID int(11) NOT NULL auto_increment,
           resource varchar(255) BINARY NOT NULL,
	   title varchar(255) BINARY NOT NULL,
	   UNIQUE KEY id (ID,resource(255)),
	   INDEX (ID),
	   INDEX (resource))""",
  "Queries" : """CREATE TABLE Queries (
           ID int(11) NOT NULL auto_increment,
	   Value varchar(50) NOT NULL,
	   Count int(11) NOT NULL,
	   UNIQUE KEY id (ID,Value(35)),
	   INDEX (ID))""",
  "AllQueries" : """CREATE TABLE AllQueries (
           ID int(11) NOT NULL auto_increment,
	   Value varchar(50) NOT NULL,
	   Count int(11) NOT NULL,
	   UNIQUE KEY id (ID,Value(35)),
	   INDEX (ID))""",
}

#
# The XML API description file to parse
#
API="libxml2-api.xml"
DB=None

#########################################################################
#									#
#                  MySQL database interfaces				#
#									#
#########################################################################
def createTable(db, name):
    global TABLES

    if db == None:
        return -1
    if name == None:
        return -1
    c = db.cursor()

    ret = c.execute("DROP TABLE IF EXISTS %s" % (name))
    if ret == 1:
        print "Removed table %s" % (name)
    print "Creating table %s" % (name)
    try:
        ret = c.execute(TABLES[name])
    except:
        print "Failed to create table %s" % (name)
	return -1
    return ret

def checkTables(db, verbose = 1):
    global TABLES

    if db == None:
        return -1
    c = db.cursor()
    nbtables = c.execute("show tables")
    if verbose:
	print "Found %d tables" % (nbtables)
    tables = {}
    i = 0
    while i < nbtables:
        l = c.fetchone()
	name = l[0]
	tables[name] = {}
        i = i + 1

    for table in TABLES.keys():
        if not tables.has_key(table):
	    print "table %s missing" % (table)
	    createTable(db, table)
	try:
	    ret = c.execute("SELECT count(*) from %s" % table);
	    row = c.fetchone()
	    if verbose:
		print "Table %s contains %d records" % (table, row[0])
	except:
	    print "Troubles with table %s : repairing" % (table)
	    ret = c.execute("repair table %s" % table);
	    print "repairing returned %d" % (ret)
	    ret = c.execute("SELECT count(*) from %s" % table);
	    row = c.fetchone()
	    print "Table %s contains %d records" % (table, row[0])
    if verbose:
	print "checkTables finished"

    # make sure apache can access the tables read-only
    try:
	ret = c.execute("GRANT SELECT ON xmlsoft.* TO nobody@localhost")
	ret = c.execute("GRANT INSERT,SELECT,UPDATE  ON xmlsoft.Queries TO nobody@localhost")
    except:
        pass
    return 0
    
def openMySQL(db="xmlsoft", passwd=None, verbose = 1):
    global DB

    if passwd == None:
        try:
	    passwd = os.environ["MySQL_PASS"]
	except:
	    print "No password available, set environment MySQL_PASS"
	    sys.exit(1)

    DB = MySQLdb.connect(passwd=passwd, db=db)
    if DB == None:
        return -1
    ret = checkTables(DB, verbose)
    return ret

def updateWord(name, symbol, relevance):
    global DB

    if DB == None:
        openMySQL()
    if DB == None:
        return -1
    if name == None:
        return -1
    if symbol == None:
        return -1

    c = DB.cursor()
    try:
	ret = c.execute(
"""INSERT INTO words (name, symbol, relevance) VALUES ('%s','%s', %d)""" %
		(name, symbol, relevance))
    except:
        try:
	    ret = c.execute(
    """UPDATE words SET relevance = %d where name = '%s' and symbol = '%s'""" %
		    (relevance, name, symbol))
	except:
	    print "Update word (%s, %s, %s) failed command" % (name, symbol, relevance)
	    print "UPDATE words SET relevance = %d where name = '%s' and symbol = '%s'" % (relevance, name, symbol)
	    print sys.exc_type, sys.exc_value
	    return -1
	     
    return ret

def updateSymbol(name, module, type, desc):
    global DB

    updateWord(name, name, 50)
    if DB == None:
        openMySQL()
    if DB == None:
        return -1
    if name == None:
        return -1
    if module == None:
        return -1
    if type == None:
        return -1

    try:
	desc = string.replace(desc, "'", " ")
	l = string.split(desc, ".")
	desc = l[0]
	desc = desc[0:99]
    except:
        desc = ""

    c = DB.cursor()
    try:
	ret = c.execute(
"""INSERT INTO symbols (name, module, type, descr) VALUES ('%s','%s', '%s', '%s')""" %
                    (name, module, type, desc))
    except:
        try:
	    ret = c.execute(
"""UPDATE symbols SET module='%s', type='%s', descr='%s' where name='%s'""" %
                    (module, type, desc, name))
        except:
	    print "Update symbol (%s, %s, %s) failed command" % (name, module, type)
	    print """UPDATE symbols SET module='%s', type='%s', descr='%s' where name='%s'""" % (module, type, desc, name)
	    print sys.exc_type, sys.exc_value
	    return -1
	     
    return ret
        
def addFunction(name, module, desc = ""):
    return updateSymbol(name, module, 'function', desc)

def addMacro(name, module, desc = ""):
    return updateSymbol(name, module, 'macro', desc)

def addEnum(name, module, desc = ""):
    return updateSymbol(name, module, 'enum', desc)

def addStruct(name, module, desc = ""):
    return updateSymbol(name, module, 'struct', desc)

def addConst(name, module, desc = ""):
    return updateSymbol(name, module, 'const', desc)

def addType(name, module, desc = ""):
    return updateSymbol(name, module, 'type', desc)

def addFunctype(name, module, desc = ""):
    return updateSymbol(name, module, 'functype', desc)

def addPage(resource, title):
    global DB

    if DB == None:
        openMySQL()
    if DB == None:
        return -1
    if resource == None:
        return -1

    c = DB.cursor()
    try:
	ret = c.execute(
	    """INSERT INTO pages (resource, title) VALUES ('%s','%s')""" %
                    (resource, title))
    except:
        try:
	    ret = c.execute(
		"""UPDATE pages SET title='%s' WHERE resource='%s'""" %
                    (title, resource))
        except:
	    print "Update symbol (%s, %s, %s) failed command" % (name, module, type)
	    print """UPDATE pages SET title='%s' WHERE resource='%s'""" % (title, resource)
	    print sys.exc_type, sys.exc_value
	    return -1
	     
    return ret

def updateWordHTML(name, resource, desc, id, relevance):
    global DB

    if DB == None:
        openMySQL()
    if DB == None:
        return -1
    if name == None:
        return -1
    if resource == None:
        return -1
    if id == None:
        id = ""
    if desc == None:
        desc = ""
    else:
	try:
	    desc = string.replace(desc, "'", " ")
	    desc = desc[0:99]
	except:
	    desc = ""

    c = DB.cursor()
    try:
	ret = c.execute(
"""INSERT INTO wordsHTML (name, resource, section, id, relevance) VALUES ('%s','%s', '%s', '%s', '%d')""" %
                    (name, resource, desc, id, relevance))
    except:
        try:
	    ret = c.execute(
"""UPDATE wordsHTML SET section='%s', id='%s', relevance='%d' where name='%s' and resource='%s'""" %
                    (desc, id, relevance, name, resource))
        except:
	    print "Update symbol (%s, %s, %d) failed command" % (name, resource, relevance)
	    print """UPDATE wordsHTML SET section='%s', id='%s', relevance='%d' where name='%s' and resource='%s'""" % (desc, id, relevance, name, resource)
	    print sys.exc_type, sys.exc_value
	    return -1
	     
    return ret

def checkXMLMsgArchive(url):
    global DB

    if DB == None:
        openMySQL()
    if DB == None:
        return -1
    if url == None:
        return -1

    c = DB.cursor()
    try:
	ret = c.execute(
	    """SELECT ID FROM archives WHERE resource='%s'""" % (url))
	row = c.fetchone()
	if row == None:
	    return -1
    except:
	return -1
	     
    return row[0]
    
def addXMLMsgArchive(url, title):
    global DB

    if DB == None:
        openMySQL()
    if DB == None:
        return -1
    if url == None:
        return -1
    if title == None:
        title = ""
    else:
	title = string.replace(title, "'", " ")
	title = title[0:99]

    c = DB.cursor()
    try:
        cmd = """INSERT INTO archives (resource, title) VALUES ('%s','%s')""" % (url, title)
        ret = c.execute(cmd)
	cmd = """SELECT ID FROM archives WHERE resource='%s'""" % (url)
        ret = c.execute(cmd)
	row = c.fetchone()
	if row == None:
	    print "addXMLMsgArchive failed to get the ID: %s" % (url)
	    return -1
    except:
        print "addXMLMsgArchive failed command: %s" % (cmd)
	return -1
	     
    return((int)(row[0]))

def updateWordArchive(name, id, relevance):
    global DB

    if DB == None:
        openMySQL()
    if DB == None:
        return -1
    if name == None:
        return -1
    if id == None:
        return -1

    c = DB.cursor()
    try:
	ret = c.execute(
"""INSERT INTO wordsArchive (name, id, relevance) VALUES ('%s', '%d', '%d')""" %
                    (name, id, relevance))
    except:
        try:
	    ret = c.execute(
"""UPDATE wordsArchive SET relevance='%d' where name='%s' and ID='%d'""" %
                    (relevance, name, id))
        except:
	    print "Update word archive (%s, %d, %d) failed command" % (name, id, relevance)
	    print """UPDATE wordsArchive SET relevance='%d' where name='%s' and ID='%d'""" % (relevance, name, id)
	    print sys.exc_type, sys.exc_value
	    return -1
	     
    return ret

#########################################################################
#									#
#                  Word dictionnary and analysis routines		#
#									#
#########################################################################

#
# top 100 english word without the one len < 3 + own set
#
dropWords = {
    'the':0, 'this':0, 'can':0, 'man':0, 'had':0, 'him':0, 'only':0,
    'and':0, 'not':0, 'been':0, 'other':0, 'even':0, 'are':0, 'was':0,
    'new':0, 'most':0, 'but':0, 'when':0, 'some':0, 'made':0, 'from':0,
    'who':0, 'could':0, 'after':0, 'that':0, 'will':0, 'time':0, 'also':0,
    'have':0, 'more':0, 'these':0, 'did':0, 'was':0, 'two':0, 'many':0,
    'they':0, 'may':0, 'before':0, 'for':0, 'which':0, 'out':0, 'then':0,
    'must':0, 'one':0, 'through':0, 'with':0, 'you':0, 'said':0,
    'first':0, 'back':0, 'were':0, 'what':0, 'any':0, 'years':0, 'his':0,
    'her':0, 'where':0, 'all':0, 'its':0, 'now':0, 'much':0, 'she':0,
    'about':0, 'such':0, 'your':0, 'there':0, 'into':0, 'like':0, 'may':0,
    'would':0, 'than':0, 'our':0, 'well':0, 'their':0, 'them':0, 'over':0,
    'down':0,
    'net':0, 'www':0, 'bad':0, 'Okay':0, 'bin':0, 'cur':0,
}

wordsDict = {}
wordsDictHTML = {}
wordsDictArchive = {}

def cleanupWordsString(str):
    str = string.replace(str, ".", " ")
    str = string.replace(str, "!", " ")
    str = string.replace(str, "?", " ")
    str = string.replace(str, ",", " ")
    str = string.replace(str, "'", " ")
    str = string.replace(str, '"', " ")
    str = string.replace(str, ";", " ")
    str = string.replace(str, "(", " ")
    str = string.replace(str, ")", " ")
    str = string.replace(str, "{", " ")
    str = string.replace(str, "}", " ")
    str = string.replace(str, "<", " ")
    str = string.replace(str, ">", " ")
    str = string.replace(str, "=", " ")
    str = string.replace(str, "/", " ")
    str = string.replace(str, "*", " ")
    str = string.replace(str, ":", " ")
    str = string.replace(str, "#", " ")
    str = string.replace(str, "\\", " ")
    str = string.replace(str, "\n", " ")
    str = string.replace(str, "\r", " ")
    str = string.replace(str, "\xc2", " ")
    str = string.replace(str, "\xa0", " ")
    return str
    
def cleanupDescrString(str):
    str = string.replace(str, "'", " ")
    str = string.replace(str, "\n", " ")
    str = string.replace(str, "\r", " ")
    str = string.replace(str, "\xc2", " ")
    str = string.replace(str, "\xa0", " ")
    l = string.split(str)
    str = string.join(str)
    return str

def splitIdentifier(str):
    ret = []
    while str != "":
        cur = string.lower(str[0])
	str = str[1:]
	if ((cur < 'a') or (cur > 'z')):
	    continue
	while (str != "") and (str[0] >= 'A') and (str[0] <= 'Z'):
	    cur = cur + string.lower(str[0])
	    str = str[1:]
	while (str != "") and (str[0] >= 'a') and (str[0] <= 'z'):
	    cur = cur + str[0]
	    str = str[1:]
	while (str != "") and (str[0] >= '0') and (str[0] <= '9'):
	    str = str[1:]
	ret.append(cur)
    return ret

def addWord(word, module, symbol, relevance):
    global wordsDict

    if word == None or len(word) < 3:
        return -1
    if module == None or symbol == None:
        return -1
    if dropWords.has_key(word):
        return 0
    if ord(word[0]) > 0x80:
        return 0

    if wordsDict.has_key(word):
        d = wordsDict[word]
	if d == None:
	    return 0
	if len(d) > 500:
	    wordsDict[word] = None
	    return 0
	try:
	    relevance = relevance + d[(module, symbol)]
	except:
	    pass
    else:
        wordsDict[word] = {}
    wordsDict[word][(module, symbol)] = relevance
    return relevance
    
def addString(str, module, symbol, relevance):
    if str == None or len(str) < 3:
        return -1
    ret = 0
    str = cleanupWordsString(str)
    l = string.split(str)
    for word in l:
	if len(word) > 2:
	    ret = ret + addWord(word, module, symbol, 5)

    return ret

def addWordHTML(word, resource, id, section, relevance):
    global wordsDictHTML

    if word == None or len(word) < 3:
        return -1
    if resource == None or section == None:
        return -1
    if dropWords.has_key(word):
        return 0
    if ord(word[0]) > 0x80:
        return 0

    section = cleanupDescrString(section)

    if wordsDictHTML.has_key(word):
        d = wordsDictHTML[word]
	if d == None:
	    print "skipped %s" % (word)
	    return 0
	try:
	    (r,i,s) = d[resource]
	    if i != None:
	        id = i
	    if s != None:
	        section = s
	    relevance = relevance + r
	except:
	    pass
    else:
        wordsDictHTML[word] = {}
    d = wordsDictHTML[word];
    d[resource] = (relevance, id, section)
    return relevance
    
def addStringHTML(str, resource, id, section, relevance):
    if str == None or len(str) < 3:
        return -1
    ret = 0
    str = cleanupWordsString(str)
    l = string.split(str)
    for word in l:
	if len(word) > 2:
	    try:
		r = addWordHTML(word, resource, id, section, relevance)
		if r < 0:
		    print "addWordHTML failed: %s %s" % (word, resource)
		ret = ret + r
	    except:
		print "addWordHTML failed: %s %s %d" % (word, resource, relevance)
		print sys.exc_type, sys.exc_value

    return ret

def addWordArchive(word, id, relevance):
    global wordsDictArchive

    if word == None or len(word) < 3:
        return -1
    if id == None or id == -1:
        return -1
    if dropWords.has_key(word):
        return 0
    if ord(word[0]) > 0x80:
        return 0

    if wordsDictArchive.has_key(word):
        d = wordsDictArchive[word]
	if d == None:
	    print "skipped %s" % (word)
	    return 0
	try:
	    r = d[id]
	    relevance = relevance + r
	except:
	    pass
    else:
        wordsDictArchive[word] = {}
    d = wordsDictArchive[word];
    d[id] = relevance
    return relevance
    
def addStringArchive(str, id, relevance):
    if str == None or len(str) < 3:
        return -1
    ret = 0
    str = cleanupWordsString(str)
    l = string.split(str)
    for word in l:
        i = len(word)
	if i > 2:
	    try:
		r = addWordArchive(word, id, relevance)
		if r < 0:
		    print "addWordArchive failed: %s %s" % (word, id)
		else:
		    ret = ret + r
	    except:
		print "addWordArchive failed: %s %s %d" % (word, id, relevance)
		print sys.exc_type, sys.exc_value
    return ret

#########################################################################
#									#
#                  XML API description analysis				#
#									#
#########################################################################

def loadAPI(filename):
    doc = libxml2.parseFile(filename)
    print "loaded %s" % (filename)
    return doc

def foundExport(file, symbol):
    if file == None:
        return 0
    if symbol == None:
        return 0
    addFunction(symbol, file)
    l = splitIdentifier(symbol)
    for word in l:
	addWord(word, file, symbol, 10)
    return 1
     
def analyzeAPIFile(top):
    count = 0
    name = top.prop("name")
    cur = top.children
    while cur != None:
        if cur.type == 'text':
	    cur = cur.next
	    continue
	if cur.name == "exports":
	    count = count + foundExport(name, cur.prop("symbol"))
	else:
	    print "unexpected element %s in API doc <file name='%s'>" % (name)
        cur = cur.next
    return count

def analyzeAPIFiles(top):
    count = 0
    cur = top.children
        
    while cur != None:
        if cur.type == 'text':
	    cur = cur.next
	    continue
	if cur.name == "file":
	    count = count + analyzeAPIFile(cur)
	else:
	    print "unexpected element %s in API doc <files>" % (cur.name)
        cur = cur.next
    return count

def analyzeAPIEnum(top):
    file = top.prop("file")
    if file == None:
        return 0
    symbol = top.prop("name")
    if symbol == None:
        return 0

    addEnum(symbol, file)
    l = splitIdentifier(symbol)
    for word in l:
	addWord(word, file, symbol, 10)

    return 1

def analyzeAPIConst(top):
    file = top.prop("file")
    if file == None:
        return 0
    symbol = top.prop("name")
    if symbol == None:
        return 0

    addConst(symbol, file)
    l = splitIdentifier(symbol)
    for word in l:
	addWord(word, file, symbol, 10)

    return 1

def analyzeAPIType(top):
    file = top.prop("file")
    if file == None:
        return 0
    symbol = top.prop("name")
    if symbol == None:
        return 0

    addType(symbol, file)
    l = splitIdentifier(symbol)
    for word in l:
	addWord(word, file, symbol, 10)
    return 1

def analyzeAPIFunctype(top):
    file = top.prop("file")
    if file == None:
        return 0
    symbol = top.prop("name")
    if symbol == None:
        return 0

    addFunctype(symbol, file)
    l = splitIdentifier(symbol)
    for word in l:
	addWord(word, file, symbol, 10)
    return 1

def analyzeAPIStruct(top):
    file = top.prop("file")
    if file == None:
        return 0
    symbol = top.prop("name")
    if symbol == None:
        return 0

    addStruct(symbol, file)
    l = splitIdentifier(symbol)
    for word in l:
	addWord(word, file, symbol, 10)

    info = top.prop("info")
    if info != None:
	info = string.replace(info, "'", " ")
	info = string.strip(info)
	l = string.split(info)
	for word in l:
	    if len(word) > 2:
		addWord(word, file, symbol, 5)
    return 1

def analyzeAPIMacro(top):
    file = top.prop("file")
    if file == None:
        return 0
    symbol = top.prop("name")
    if symbol == None:
        return 0
    symbol = string.replace(symbol, "'", " ")
    symbol = string.strip(symbol)

    info = None
    cur = top.children
    while cur != None:
        if cur.type == 'text':
	    cur = cur.next
	    continue
	if cur.name == "info":
	    info = cur.content
	    break
        cur = cur.next

    l = splitIdentifier(symbol)
    for word in l:
	addWord(word, file, symbol, 10)

    if info == None:
	addMacro(symbol, file)
        print "Macro %s description has no <info>" % (symbol)
        return 0

    info = string.replace(info, "'", " ")
    info = string.strip(info)
    addMacro(symbol, file, info)
    l = string.split(info)
    for word in l:
	if len(word) > 2:
	    addWord(word, file, symbol, 5)
    return 1

def analyzeAPIFunction(top):
    file = top.prop("file")
    if file == None:
        return 0
    symbol = top.prop("name")
    if symbol == None:
        return 0

    symbol = string.replace(symbol, "'", " ")
    symbol = string.strip(symbol)
    info = None
    cur = top.children
    while cur != None:
        if cur.type == 'text':
	    cur = cur.next
	    continue
	if cur.name == "info":
	    info = cur.content
	elif cur.name == "return":
	    rinfo = cur.prop("info")
	    if rinfo != None:
		rinfo = string.replace(rinfo, "'", " ")
		rinfo = string.strip(rinfo)
	        addString(rinfo, file, symbol, 7)
	elif cur.name == "arg":
	    ainfo = cur.prop("info")
	    if ainfo != None:
		ainfo = string.replace(ainfo, "'", " ")
		ainfo = string.strip(ainfo)
	        addString(ainfo, file, symbol, 5)
	    name = cur.prop("name")
	    if name != None:
		name = string.replace(name, "'", " ")
		name = string.strip(name)
	        addWord(name, file, symbol, 7)
        cur = cur.next
    if info == None:
        print "Function %s description has no <info>" % (symbol)
	addFunction(symbol, file, "")
    else:
        info = string.replace(info, "'", " ")
	info = string.strip(info)
	addFunction(symbol, file, info)
        addString(info, file, symbol, 5)

    l = splitIdentifier(symbol)
    for word in l:
	addWord(word, file, symbol, 10)

    return 1

def analyzeAPISymbols(top):
    count = 0
    cur = top.children
        
    while cur != None:
        if cur.type == 'text':
	    cur = cur.next
	    continue
	if cur.name == "macro":
	    count = count + analyzeAPIMacro(cur)
	elif cur.name == "function":
	    count = count + analyzeAPIFunction(cur)
	elif cur.name == "const":
	    count = count + analyzeAPIConst(cur)
	elif cur.name == "typedef":
	    count = count + analyzeAPIType(cur)
	elif cur.name == "struct":
	    count = count + analyzeAPIStruct(cur)
	elif cur.name == "enum":
	    count = count + analyzeAPIEnum(cur)
	elif cur.name == "functype":
	    count = count + analyzeAPIFunctype(cur)
	else:
	    print "unexpected element %s in API doc <files>" % (cur.name)
        cur = cur.next
    return count

def analyzeAPI(doc):
    count = 0
    if doc == None:
        return -1
    root = doc.getRootElement()
    if root.name != "api":
        print "Unexpected root name"
        return -1
    cur = root.children
    while cur != None:
        if cur.type == 'text':
	    cur = cur.next
	    continue
	if cur.name == "files":
	    pass
#	    count = count + analyzeAPIFiles(cur)
	elif cur.name == "symbols":
	    count = count + analyzeAPISymbols(cur)
	else:
	    print "unexpected element %s in API doc" % (cur.name)
        cur = cur.next
    return count

#########################################################################
#									#
#                  Web pages parsing and analysis			#
#									#
#########################################################################

import glob

def analyzeHTMLText(doc, resource, p, section, id):
    words = 0
    try:
	content = p.content
	words = words + addStringHTML(content, resource, id, section, 5)
    except:
        return -1
    return words

def analyzeHTMLPara(doc, resource, p, section, id):
    words = 0
    try:
	content = p.content
	words = words + addStringHTML(content, resource, id, section, 5)
    except:
        return -1
    return words

def analyzeHTMLPre(doc, resource, p, section, id):
    words = 0
    try:
	content = p.content
	words = words + addStringHTML(content, resource, id, section, 5)
    except:
        return -1
    return words

def analyzeHTML(doc, resource, p, section, id):
    words = 0
    try:
	content = p.content
	words = words + addStringHTML(content, resource, id, section, 5)
    except:
        return -1
    return words

def analyzeHTML(doc, resource):
    para = 0;
    ctxt = doc.xpathNewContext()
    try:
	res = ctxt.xpathEval("//head/title")
	title = res[0].content
    except:
        title = "Page %s" % (resource)
    addPage(resource, title)
    try:
	items = ctxt.xpathEval("//h1 | //h2 | //h3 | //text()")
	section = title
	id = ""
	for item in items:
	    if item.name == 'h1' or item.name == 'h2' or item.name == 'h3':
	        section = item.content
		if item.prop("id"):
		    id = item.prop("id")
		elif item.prop("name"):
		    id = item.prop("name")
	    elif item.type == 'text':
	        analyzeHTMLText(doc, resource, item, section, id)
		para = para + 1
	    elif item.name == 'p':
	        analyzeHTMLPara(doc, resource, item, section, id)
		para = para + 1
	    elif item.name == 'pre':
	        analyzeHTMLPre(doc, resource, item, section, id)
		para = para + 1
	    else:
	        print "Page %s, unexpected %s element" % (resource, item.name)
    except:
        print "Page %s: problem analyzing" % (resource)
	print sys.exc_type, sys.exc_value

    return para

def analyzeHTMLPages():
    ret = 0
    HTMLfiles = glob.glob("*.html") + glob.glob("tutorial/*.html")
    for html in HTMLfiles:
	if html[0:3] == "API":
	    continue
	if html == "xml.html":
	    continue
	try:
	    doc = libxml2.parseFile(html)
	except:
	    doc = libxml2.htmlParseFile(html, None)
	try:
	    res = analyzeHTML(doc, html)
	    print "Parsed %s : %d paragraphs" % (html, res)
	    ret = ret + 1
	except:
	    print "could not parse %s" % (html)
    return ret

#########################################################################
#									#
#                  Mail archives parsing and analysis			#
#									#
#########################################################################

import time

def getXMLDateArchive(t = None):
    if t == None:
	t = time.time()
    T = time.gmtime(t)
    month = time.strftime("%B", T)
    year = T[0]
    url = "http://mail.gnome.org/archives/xml/%d-%s/date.html" % (year, month)
    return url

def scanXMLMsgArchive(url, title, force = 0):
    if url == None or title == None:
        return 0

    ID = checkXMLMsgArchive(url)
    if force == 0 and ID != -1:
        return 0

    if ID == -1:
	ID = addXMLMsgArchive(url, title)
	if ID == -1:
	    return 0

    try:
        print "Loading %s" % (url)
        doc = libxml2.htmlParseFile(url, None);
    except:
        doc = None
    if doc == None:
        print "Failed to parse %s" % (url)
	return 0

    addStringArchive(title, ID, 20)
    ctxt = doc.xpathNewContext()
    texts = ctxt.xpathEval("//pre//text()")
    for text in texts:
        addStringArchive(text.content, ID, 5)

    return 1

def scanXMLDateArchive(t = None, force = 0):
    global wordsDictArchive

    wordsDictArchive = {}

    url = getXMLDateArchive(t)
    print "loading %s" % (url)
    try:
	doc = libxml2.htmlParseFile(url, None);
    except:
        doc = None
    if doc == None:
        print "Failed to parse %s" % (url)
	return -1
    ctxt = doc.xpathNewContext()
    anchors = ctxt.xpathEval("//a[@href]")
    links = 0
    newmsg = 0
    for anchor in anchors:
	href = anchor.prop("href")
	if href == None or href[0:3] != "msg":
	    continue
        try:
	    links = links + 1

	    msg = libxml2.buildURI(href, url)
	    title = anchor.content
	    if title != None and title[0:4] == 'Re: ':
	        title = title[4:]
	    if title != None and title[0:6] == '[xml] ':
	        title = title[6:]
	    newmsg = newmsg + scanXMLMsgArchive(msg, title, force)

	except:
	    pass

    return newmsg
    

#########################################################################
#									#
#          Main code: open the DB, the API XML and analyze it		#
#									#
#########################################################################
def analyzeArchives(t = None, force = 0):
    global wordsDictArchive

    ret = scanXMLDateArchive(t, force)
    print "Indexed %d words in %d archive pages" % (len(wordsDictArchive), ret)

    i = 0
    skipped = 0
    for word in wordsDictArchive.keys():
	refs = wordsDictArchive[word]
	if refs  == None:
	    skipped = skipped + 1
	    continue;
	for id in refs.keys():
	    relevance = refs[id]
	    updateWordArchive(word, id, relevance)
	    i = i + 1

    print "Found %d associations in HTML pages" % (i)

def analyzeHTMLTop():
    global wordsDictHTML

    ret = analyzeHTMLPages()
    print "Indexed %d words in %d HTML pages" % (len(wordsDictHTML), ret)

    i = 0
    skipped = 0
    for word in wordsDictHTML.keys():
	refs = wordsDictHTML[word]
	if refs  == None:
	    skipped = skipped + 1
	    continue;
	for resource in refs.keys():
	    (relevance, id, section) = refs[resource]
	    updateWordHTML(word, resource, section, id, relevance)
	    i = i + 1

    print "Found %d associations in HTML pages" % (i)

def analyzeAPITop():
    global wordsDict
    global API

    try:
	doc = loadAPI(API)
	ret = analyzeAPI(doc)
	print "Analyzed %d blocs" % (ret)
	doc.freeDoc()
    except:
	print "Failed to parse and analyze %s" % (API)
	print sys.exc_type, sys.exc_value
	sys.exit(1)

    print "Indexed %d words" % (len(wordsDict))
    i = 0
    skipped = 0
    for word in wordsDict.keys():
	refs = wordsDict[word]
	if refs  == None:
	    skipped = skipped + 1
	    continue;
	for (module, symbol) in refs.keys():
	    updateWord(word, symbol, refs[(module, symbol)])
	    i = i + 1

    print "Found %d associations, skipped %d words" % (i, skipped)

def usage():
    print "Usage index.py [--force] [--archive]  [--archive-year year] [--archive-month month] [--API] [--docs]"
    sys.exit(1)

def main():
    try:
	openMySQL()
    except:
	print "Failed to open the database"
	print sys.exc_type, sys.exc_value
	sys.exit(1)

    args = sys.argv[1:]
    force = 0
    if args:
        i = 0
	while i < len(args):
	    if args[i] == '--force':
	        force = 1
	    elif args[i] == '--archive':
	        analyzeArchives(None, force)
	    elif args[i] == '--archive-year':
	        i = i + 1;
		year = args[i]
		months = ["January" , "February", "March", "April", "May",
			  "June", "July", "August", "September", "October",
			  "November", "December"];
	        for month in months:
		    try:
		        str = "%s-%s" % (year, month)
			T = time.strptime(str, "%Y-%B")
			t = time.mktime(T) + 3600 * 24 * 10;
			analyzeArchives(t, force)
		    except:
			print "Failed to index month archive:"
			print sys.exc_type, sys.exc_value
	    elif args[i] == '--archive-month':
	        i = i + 1;
		month = args[i]
		try:
		    T = time.strptime(month, "%Y-%B")
		    t = time.mktime(T) + 3600 * 24 * 10;
		    analyzeArchives(t, force)
		except:
		    print "Failed to index month archive:"
		    print sys.exc_type, sys.exc_value
	    elif args[i] == '--API':
	        analyzeAPITop()
	    elif args[i] == '--docs':
	        analyzeHTMLTop()
	    else:
	        usage()
	    i = i + 1
    else:
        usage()

if __name__ == "__main__":
    main()
