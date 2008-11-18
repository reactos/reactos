try:
# Using Psyco makes it about 25% faster, but there's a bug in psyco in
# handling of eval causing it to use unlimited memory with the magic
# file enabled.
#    import psyco
#    psyco.full()
#    from psyco.classes import *
    pass
except:
    pass
import re
import base64
import cStringIO
import specialuu
import array
import email.Utils
import zlib
import magic

# Comment out if you don't want magic detection
magicf = magic.MagicFile()

# Open our output file
outfile = open("gnats2bz_data.sql", "w")

# List of GNATS fields
fieldnames = ("Number", "Category", "Synopsis", "Confidential", "Severity",
              "Priority", "Responsible", "State", "Quarter", "Keywords",
              "Date-Required", "Class", "Submitter-Id", "Arrival-Date",
              "Closed-Date", "Last-Modified", "Originator", "Release",
              "Organization", "Environment", "Description", "How-To-Repeat",
              "Fix", "Release-Note", "Audit-Trail", "Unformatted")

# Dictionary telling us which GNATS fields are multiline
multilinefields = {"Organization":1, "Environment":1, "Description":1,
                   "How-To-Repeat":1, "Fix":1, "Release-Note":1,
                   "Audit-Trail":1, "Unformatted":1}

# Mapping of GCC release to version. Our version string is updated every
# so we need to funnel all release's with 3.4 in the string to be version
# 3.4 for bug tracking purposes
# The key is a regex to match, the value is the version it corresponds
# with
releasetovermap = {r"3\.4":"3.4", r"3\.3":"3.3", r"3\.2\.2":"3.2.2",
                   r"3\.2\.1":"3.2.1", r"3\.2":"3.2", r"3\.1\.2":"3.1.2",
                   r"3\.1\.1":"3.1.1", r"3\.1":"3.1", r"3\.0\.4":"3.0.4",
                   r"3\.0\.3":"3.0.3", r"3\.0\.2":"3.0.2", r"3\.0\.1":"3.0.1",
                   r"3\.0":"3.0", r"2\.95\.4":"2.95.4", r"2\.95\.3":"2.95.3",
                   r"2\.95\.2":"2.95.2", r"2\.95\.1":"2.95.1",
                   r"2\.95":"2.95", r"2\.97":"2.97",
                   r"2\.96.*[rR][eE][dD].*[hH][aA][tT]":"2.96 (redhat)",
                   r"2\.96":"2.96"}

# These map the field name to the field id bugzilla assigns. We need
# the id when doing bug activity.
fieldids = {"State":8, "Responsible":15}

# These are the keywords we use in gcc bug tracking. They are transformed
# into bugzilla keywords.  The format here is <keyword>-><bugzilla keyword id>
keywordids = {"wrong-code":1, "ice-on-legal-code":2, "ice-on-illegal-code":3,
              "rejects-legal":4, "accepts-illegal":5, "pessimizes-code":6}

# Map from GNATS states to Bugzilla states.  Duplicates and reopened bugs
# are handled when parsing the audit trail, so no need for them here.
state_lookup = {"":"NEW", "open":"ASSIGNED", "analyzed":"ASSIGNED",
                "feedback":"WAITING", "closed":"CLOSED",
                "suspended":"SUSPENDED"}

# Table of versions that exist in the bugs, built up as we go along
versions_table = {}

# Delimiter gnatsweb uses for attachments
attachment_delimiter = "----gnatsweb-attachment----\n"

# Here starts the various regular expressions we use
# Matches an entire GNATS single line field
gnatfieldre = re.compile(r"""^([>\w\-]+)\s*:\s*(.*)\s*$""")

# Matches the name of a GNATS field
fieldnamere = re.compile(r"""^>(.*)$""")

# Matches the useless part of an envelope
uselessre = re.compile(r"""^(\S*?):\s*""", re.MULTILINE)

# Matches the filename in a content disposition
dispositionre = re.compile("(\\S+);\\s*filename=\"([^\"]+)\"")

# Matches the last changed date in the entire text of a bug
# If you have other editable fields that get audit trail entries, modify this
# The field names are explicitly listed in order to speed up matching
lastdatere = re.compile(r"""^(?:(?:State|Responsible|Priority|Severity)-Changed-When: )(.+?)$""", re.MULTILINE)

# Matches the From line of an email or the first line of an audit trail entry
# We use this re to find the begin lines of all the audit trail entries
# The field names are explicitly listed in order to speed up matching
fromtore=re.compile(r"""^(?:(?:State|Responsible|Priority|Severity)-Changed-From-To: |From: )""", re.MULTILINE)

# These re's match the various parts of an audit trail entry
changedfromtore=re.compile(r"""^(\w+?)-Changed-From-To: (.+?)$""", re.MULTILINE)
changedbyre=re.compile(r"""^\w+?-Changed-By: (.+?)$""", re.MULTILINE)
changedwhenre=re.compile(r"""^\w+?-Changed-When: (.+?)$""", re.MULTILINE)
changedwhyre=re.compile(r"""^\w+?-Changed-Why:\s*(.*?)$""", re.MULTILINE)

# This re matches audit trail text saying that the current bug is a duplicate of another
duplicatere=re.compile(r"""(?:")?Dup(?:licate)?(?:d)?(?:")? of .*?(\d+)""", re.IGNORECASE | re.MULTILINE)

# Get the text of a From: line
fromre=re.compile(r"""^From: (.*?)$""", re.MULTILINE)

# Get the text of a Date: Line
datere=re.compile(r"""^Date: (.*?)$""", re.MULTILINE)

#  Map of the responsible file to email addresses
responsible_map = {}
#  List of records in the responsible file
responsible_list = []
#  List of records in the categories file
categories_list = []
# List of pr's in the index
pr_list = []
# Map usernames to user ids
usermapping = {}
# Start with this user id
userid_base = 2

# Name of gnats user
gnats_username = "gnats@gcc.gnu.org"
# Name of unassigned user
unassigned_username = "unassigned@gcc.gnu.org"

gnats_db_dir = "."
product = "gcc"
productdesc = "GNU Compiler Connection"
milestoneurl = "http://gcc/gnu.org"
defaultmilestone = "3.4"

def write_non_bug_tables():
    """ Write out the non-bug related tables, such as products, profiles, etc."""
    # Set all non-unconfirmed bugs's everconfirmed flag
    print >>outfile, "update bugs set everconfirmed=1 where bug_status != 'UNCONFIRMED';"

    # Set all bugs assigned to the unassigned user to NEW
    print >>outfile, "update bugs set bug_status='NEW',assigned_to='NULL' where bug_status='ASSIGNED' AND assigned_to=3;"
    
    # Insert the products
    print >>outfile, "\ninsert into products ("
    print >>outfile, "  product, description, milestoneurl, disallownew,"
    print >>outfile, "  defaultmilestone, votestoconfirm) values ("
    print >>outfile, "  '%s', '%s', '%s', 0, '%s', 1);" % (product,
                                                           productdesc,
                                                           milestoneurl,
                                                           defaultmilestone)

    # Insert the components    
    for category in categories_list:
        component = SqlQuote(category[0])
        productstr = SqlQuote(product)
        description = SqlQuote(category[1])
        initialowner = SqlQuote("3")
        print >>outfile, "\ninsert into components (";
        print >>outfile, "  value, program, initialowner, initialqacontact,"
        print >>outfile, "  description) values ("
        print >>outfile, "  %s, %s, %s, '', %s);" % (component, productstr,
                                                     initialowner, description)
        
    # Insert the versions
    for productstr, version_list in versions_table.items():
        productstr = SqlQuote(productstr)
        for version in version_list:
            version = SqlQuote(version)
            print >>outfile, "\ninsert into versions (value, program) "
            print >>outfile, "  values (%s, %s);" % (version, productstr)
            
    # Insert the users
    for username, userid in usermapping.items():
        realname = map_username_to_realname(username)
        username = SqlQuote(username)
        realname = SqlQuote(realname)
        print >>outfile, "\ninsert into profiles ("
        print >>outfile, "  userid, login_name, password, cryptpassword, realname, groupset"
        print >>outfile, ") values ("
        print >>outfile, "%s,%s,'password',encrypt('password'), %s, 0);" % (userid, username, realname)
    print >>outfile, "update profiles set groupset=1 << 32 where login_name like '%\@gcc.gnu.org';"
    
def unixdate2datetime(unixdate):
    """ Convert a unix date to a datetime value """
    year, month, day, hour, min, sec, x, x, x, x = email.Utils.parsedate_tz(unixdate)
    return "%d-%02d-%02d %02d:%02d:%02d" % (year,month,day,hour,min,sec)

def unixdate2timestamp(unixdate):
    """ Convert a unix date to a timestamp value """
    year, month, day, hour, min, sec, x, x, x, x = email.Utils.parsedate_tz(unixdate)
    return "%d%02d%02d%02d%02d%02d" % (year,month,day,hour,min,sec)

def SqlQuote(str):
    """ Perform SQL quoting on a string """
    return "'%s'" % str.replace("'", """''""").replace("\\", "\\\\").replace("\0","\\0")

def convert_gccver_to_ver(gccver):
    """ Given a gcc version, convert it to a Bugzilla version. """
    for k in releasetovermap.keys():
        if re.search(".*%s.*" % k, gccver) is not None:
            return releasetovermap[k]
    result = re.search(r""".*(\d\.\d) \d+ \(experimental\).*""", gccver)
    if result is not None:
        return result.group(1)
    return "unknown"

def load_index(fname):
    """ Load in the GNATS index file """
    global pr_list
    ifp = open(fname)
    for record in ifp.xreadlines():
        fields = record.split("|")
        pr_list.append(fields[0])
    ifp.close()
    
def load_categories(fname):
    """ Load in the GNATS categories file """
    global categories_list
    cfp = open(fname)
    for record in cfp.xreadlines():
        if re.search("^#", record) is not None:
            continue
        categories_list.append(record.split(":"))
    cfp.close()
    
def map_username_to_realname(username): 
    """ Given a username, find the real name """
    name = username
    name = re.sub("@.*", "", name)
    for responsible_record in responsible_list:
	if responsible_record[0] == name:
	    return responsible_record[1]
    if len(responsible_record) > 2:
        if responsible_record[2] == username:
	    return responsible_record[1]
    return ""


def get_userid(responsible):
    """ Given an email address, get the user id """
    global responsible_map
    global usermapping
    global userid_base
    if responsible is None:
        return -1
    responsible = responsible.lower()
    responsible = re.sub("sources.redhat.com", "gcc.gnu.org", responsible)
    if responsible_map.has_key(responsible):
        responsible = responsible_map[responsible]
    if usermapping.has_key(responsible):
        return usermapping[responsible]
    else:
        usermapping[responsible] = userid_base
        userid_base += 1
    return usermapping[responsible]

def load_responsible(fname):
    """ Load in the GNATS responsible file """
    global responsible_map
    global responsible_list
    rfp = open(fname)
    for record in rfp.xreadlines():
        if re.search("^#", record) is not None:
            continue
        split_record = record.split(":")
        responsible_map[split_record[0]] = split_record[2].rstrip()
        responsible_list.append(record.split(":"))
    rfp.close()

def split_csl(list):
    """ Split a comma separated list """
    newlist = re.split(r"""\s*,\s*""", list)
    return newlist

def fix_email_addrs(addrs):
    """ Perform various fixups and cleaning on an e-mail address """
    addrs = split_csl(addrs)
    trimmed_addrs = []
    for addr in addrs:
        addr = re.sub(r"""\(.*\)""","",addr)
        addr = re.sub(r""".*<(.*)>.*""","\\1",addr)
        addr = addr.rstrip()
        addr = addr.lstrip()
        trimmed_addrs.append(addr)
    addrs = ", ".join(trimmed_addrs)
    return addrs

class Bugzillabug(object):
    """ Class representing a bugzilla bug """
    def __init__(self, gbug):
        """ Initialize a bugzilla bug from a GNATS bug.  """
        self.bug_id = gbug.bug_id
        self.long_descs = []
        self.bug_ccs = [get_userid("gcc-bugs@gcc.gnu.org")]
        self.bug_activity = []
        self.attachments = gbug.attachments
        self.gnatsfields = gbug.fields
        self.need_unformatted = gbug.has_unformatted_attach == 0
        self.need_unformatted &= gbug.fields.has_key("Unformatted")
        self.translate_pr()
        self.update_versions()
        if self.fields.has_key("Audit-Trail"):
            self.parse_audit_trail()
            self.write_bug()
            
    def parse_fromto(type, string):
        """ Parses the from and to parts of a changed-from-to line """
        fromstr = ""
        tostr = ""

        # Some slightly messed up changed lines have unassigned-new,
        # instead of unassigned->new. So we make the > optional.        
        result = re.search(r"""(.*)-(?:>?)(.*)""", string)
        
        # Only know how to handle parsing of State and Responsible
        # changed-from-to right now
        if type == "State":
            fromstr = state_lookup[result.group(1)]
            tostr = state_lookup[result.group(2)]
        elif type == "Responsible":
            if result.group(1) != "":
                fromstr = result.group(1)
            if result.group(2) != "":
                tostr = result.group(2)
            if responsible_map.has_key(fromstr):
                fromstr = responsible_map[fromstr]
            if responsible_map.has_key(tostr):
                tostr = responsible_map[tostr]  
        return (fromstr, tostr)
    parse_fromto = staticmethod(parse_fromto)
    
    def parse_audit_trail(self):
        """ Parse a GNATS audit trail """
        trail = self.fields["Audit-Trail"]
        # Begin to split the audit trail into pieces
        result = fromtore.finditer(trail)
        starts = []
        ends = []
        pieces = []
        # Make a list of the pieces
        for x in result:
            pieces.append (x)
        # Find the start and end of each piece
        if len(pieces) > 0:
            for x in xrange(len(pieces)-1):
                starts.append(pieces[x].start())
                ends.append(pieces[x+1].start())
            starts.append(pieces[-1].start())
            ends.append(len(trail))
        pieces = []
        # Now make the list of actual text of the pieces
        for x in xrange(len(starts)):
            pieces.append(trail[starts[x]:ends[x]])
        # And parse the actual pieces
        for piece in pieces:
            result = changedfromtore.search(piece)
            # See what things we actually have inside this entry, and
            # handle them appropriately
            if result is not None:
                type = result.group(1)
                changedfromto = result.group(2)
                # If the bug was reopened, mark it as such
                if changedfromto.find("closed->analyzed") != -1:
                    if self.fields["bug_status"] == "'NEW'":
                        self.fields["bug_status"] = "'REOPENED'"
                if type == "State" or type == "Responsible":
                    oldstate, newstate = self.parse_fromto (type, changedfromto)
                result = changedbyre.search(piece)
                if result is not None:
                    changedby = result.group(1)
                result = changedwhenre.search(piece)
                if result is not None:
                    changedwhen = result.group(1)
                    changedwhen = unixdate2datetime(changedwhen)
                    changedwhen = SqlQuote(changedwhen)
                result = changedwhyre.search(piece)
                changedwhy = piece[result.start(1):]
                #changedwhy = changedwhy.lstrip()
                changedwhy = changedwhy.rstrip()
                changedby = get_userid(changedby)
		# Put us on the cc list if we aren't there already
                if changedby != self.fields["userid"] \
                       and changedby not in self.bug_ccs:
                    self.bug_ccs.append(changedby)
                # If it's a duplicate, mark it as such
                result = duplicatere.search(changedwhy)
                if result is not None:
                    newtext = "*** This bug has been marked as a duplicate of %s ***" % result.group(1)
                    newtext = SqlQuote(newtext)
                    self.long_descs.append((self.bug_id, changedby,
                                            changedwhen, newtext))
                    self.fields["bug_status"] = "'RESOLVED'"
                    self.fields["resolution"] = "'DUPLICATE'"
                    self.fields["userid"] = changedby
                else:
                    newtext = "%s-Changed-From-To: %s\n%s-Changed-Why: %s\n" % (type, changedfromto, type, changedwhy)
                    newtext = SqlQuote(newtext)
                    self.long_descs.append((self.bug_id, changedby,
                                            changedwhen, newtext))
                if type == "State" or type == "Responsible":
                    newstate = SqlQuote("%s" % newstate)
                    oldstate = SqlQuote("%s" % oldstate)
                    fieldid = fieldids[type]
                    self.bug_activity.append((newstate, oldstate, fieldid, changedby, changedwhen))
                    
            else:
		# It's an email
                result = fromre.search(piece)
                if result is None:
                    continue
                fromstr = result.group(1)
                fromstr = fix_email_addrs(fromstr)
                fromstr = get_userid(fromstr)
                result = datere.search(piece)
                if result is None:
                    continue
                datestr = result.group(1)
                datestr = SqlQuote(unixdate2timestamp(datestr))
                if fromstr != self.fields["userid"] \
                       and fromstr not in self.bug_ccs:
                    self.bug_ccs.append(fromstr)
                self.long_descs.append((self.bug_id, fromstr, datestr,
                                        SqlQuote(piece)))
                
                    

    def write_bug(self):
	""" Output a bug to the data file """
        fields = self.fields
        print >>outfile, "\ninsert into bugs("
        print >>outfile, "  bug_id, assigned_to, bug_severity, priority, bug_status, creation_ts, delta_ts,"
        print >>outfile, "  short_desc,"
        print >>outfile, "  reporter, version,"
        print >>outfile, "  product, component, resolution, target_milestone, qa_contact,"
        print >>outfile, "  gccbuild, gcctarget, gcchost, keywords"
        print >>outfile, "  ) values ("
        print >>outfile, "%s, %s, %s, %s, %s, %s, %s," % (self.bug_id, fields["userid"], fields["bug_severity"], fields["priority"], fields["bug_status"], fields["creation_ts"], fields["delta_ts"])
        print >>outfile, "%s," % (fields["short_desc"])
        print >>outfile, "%s, %s," % (fields["reporter"], fields["version"])
        print >>outfile, "%s, %s, %s, %s, 0," %(fields["product"], fields["component"], fields["resolution"], fields["target_milestone"])
        print >>outfile, "%s, %s, %s, %s" % (fields["gccbuild"], fields["gcctarget"], fields["gcchost"], fields["keywords"])
        print >>outfile, ");"
        if self.fields["keywords"] != 0:
            print >>outfile, "\ninsert into keywords (bug_id, keywordid) values ("
            print >>outfile, " %s, %s);" % (self.bug_id, fields["keywordid"])
        for id, who, when, text in self.long_descs:
            print >>outfile, "\ninsert into longdescs ("
            print >>outfile, "  bug_id, who, bug_when, thetext) values("
            print >>outfile, "  %s, %s, %s, %s);" % (id, who, when, text)
        for name, data, who in self.attachments:
            print >>outfile, "\ninsert into attachments ("
            print >>outfile, "  bug_id, filename, description, mimetype, ispatch, submitter_id) values ("
	    ftype = None
	    # It's *magic*!
	    if name.endswith(".ii") == 1:
		ftype = "text/x-c++"
 	    elif name.endswith(".i") == 1:
		ftype = "text/x-c"
	    else:
		ftype = magicf.detect(cStringIO.StringIO(data))
            if ftype is None:
                ftype = "application/octet-stream"
            
            print >>outfile, "%s,%s,%s, %s,0, %s,%s);" %(self.bug_id, SqlQuote(name), SqlQuote(name), SqlQuote (ftype), who)
            print >>outfile, "\ninsert into attach_data ("
            print >>outfile, "\n(id, thedata) values (last_insert_id(),"
            print >>outfile, "%s);" % (SqlQuote(zlib.compress(data)))
        for newstate, oldstate, fieldid, changedby, changedwhen in self.bug_activity:
            print >>outfile, "\ninsert into bugs_activity ("
            print >>outfile, "  bug_id, who, bug_when, fieldid, added, removed) values ("
            print >>outfile, "  %s, %s, %s, %s, %s, %s);" % (self.bug_id,
                                                             changedby,
                                                             changedwhen,
                                                             fieldid,
                                                             newstate,
                                                             oldstate)
        for cc in self.bug_ccs:
            print >>outfile, "\ninsert into cc(bug_id, who) values (%s, %s);" %(self.bug_id, cc)
    def update_versions(self):
	""" Update the versions table to account for the version on this bug """
        global versions_table
        if self.fields.has_key("Release") == 0 \
               or self.fields.has_key("Category") == 0:
            return
        curr_product = "gcc"
        curr_version = self.fields["Release"]
        if curr_version == "":
            return
        curr_version = convert_gccver_to_ver (curr_version)
        if versions_table.has_key(curr_product) == 0:
            versions_table[curr_product] = []
        for version in versions_table[curr_product]:
            if version == curr_version:
                return
        versions_table[curr_product].append(curr_version)
    def translate_pr(self):
	""" Transform a GNATS PR into a Bugzilla bug """
        self.fields = self.gnatsfields
        if (self.fields.has_key("Organization") == 0) \
           or self.fields["Organization"].find("GCC"):
            self.fields["Originator"] = ""
            self.fields["Organization"] = ""
        self.fields["Organization"].lstrip()
        if (self.fields.has_key("Release") == 0) \
               or self.fields["Release"] == "" \
               or self.fields["Release"].find("unknown-1.0") != -1:
            self.fields["Release"]="unknown"
        if self.fields.has_key("Responsible"):
            result = re.search(r"""\w+""", self.fields["Responsible"])
            self.fields["Responsible"] = "%s%s" % (result.group(0), "@gcc.gnu.org")
        self.fields["gcchost"] = ""
        self.fields["gcctarget"] = ""
        self.fields["gccbuild"] = ""
        if self.fields.has_key("Environment"):
            result = re.search("^host: (.+?)$", self.fields["Environment"],
                               re.MULTILINE)
            if result is not None:
                self.fields["gcchost"] = result.group(1)
            result = re.search("^target: (.+?)$", self.fields["Environment"],
                               re.MULTILINE)
            if result is not None:
                self.fields["gcctarget"] = result.group(1)
            result = re.search("^build: (.+?)$", self.fields["Environment"],
                               re.MULTILINE)
            if result is not None:
                self.fields["gccbuild"] = result.group(1)
        self.fields["userid"] = get_userid(self.fields["Responsible"])
        self.fields["bug_severity"] = "normal"
        if self.fields["Class"] == "change-request":
            self.fields["bug_severity"] = "enhancement"
        elif self.fields.has_key("Severity"):
            if self.fields["Severity"] == "critical":
                self.fields["bug_severity"] = "critical"
            elif self.fields["Severity"] == "serious":
                self.fields["bug_severity"] = "major"
        elif self.fields.has_key("Synopsis"):
            if re.search("crash|assert", self.fields["Synopsis"]):
                self.fields["bug_severity"] = "critical"
            elif re.search("wrong|error", self.fields["Synopsis"]):
                self.fields["bug_severity"] = "major"
        self.fields["bug_severity"] = SqlQuote(self.fields["bug_severity"])
        self.fields["keywords"] = 0
        if keywordids.has_key(self.fields["Class"]):
            self.fields["keywords"] = self.fields["Class"]
            self.fields["keywordid"] = keywordids[self.fields["Class"]]
            self.fields["keywords"] = SqlQuote(self.fields["keywords"])
        self.fields["priority"] = "P1"
        if self.fields.has_key("Severity") and self.fields.has_key("Priority"):
            severity = self.fields["Severity"]
            priority = self.fields["Priority"]
            if severity == "critical":
                if priority == "high":
                    self.fields["priority"] = "P1"
                else:
                    self.fields["priority"] = "P2"
            elif severity == "serious":
                if priority == "low":
                    self.fields["priority"] = "P4"
                else:
                    self.fields["priority"] = "P3"
            else:
                if priority == "high":
                    self.fields["priority"] = "P4"
                else:
                    self.fields["priority"] = "P5"
        self.fields["priority"] = SqlQuote(self.fields["priority"])
        state = self.fields["State"]
        if (state == "open" or state == "analyzed") and self.fields["userid"] != 3:
            self.fields["bug_status"] = "ASSIGNED"
            self.fields["resolution"] = ""
        elif state == "feedback":
            self.fields["bug_status"] = "WAITING"
            self.fields["resolution"] = ""
        elif state == "closed":
            self.fields["bug_status"] = "CLOSED"
            if self.fields.has_key("Class"):
                theclass = self.fields["Class"]
                if theclass.find("duplicate") != -1:
                    self.fields["resolution"]="DUPLICATE"
                elif theclass.find("mistaken") != -1:
                    self.fields["resolution"]="INVALID"                    
                else:
                    self.fields["resolution"]="FIXED"
            else:
                self.fields["resolution"]="FIXED"
        elif state == "suspended":
            self.fields["bug_status"] = "SUSPENDED"
            self.fields["resolution"] = ""
        elif state == "analyzed" and self.fields["userid"] == 3:
            self.fields["bug_status"] = "NEW"
            self.fields["resolution"] = ""
        else:
            self.fields["bug_status"] = "UNCONFIRMED"
            self.fields["resolution"] = ""
        self.fields["bug_status"] = SqlQuote(self.fields["bug_status"])
        self.fields["resolution"] = SqlQuote(self.fields["resolution"])
        self.fields["creation_ts"] = ""
        if self.fields.has_key("Arrival-Date") and self.fields["Arrival-Date"] != "":
            self.fields["creation_ts"] = unixdate2datetime(self.fields["Arrival-Date"])
        self.fields["creation_ts"] = SqlQuote(self.fields["creation_ts"])
        self.fields["delta_ts"] = ""
        if self.fields.has_key("Audit-Trail"):
            result = lastdatere.findall(self.fields["Audit-Trail"])
            result.reverse()
            if len(result) > 0:
                self.fields["delta_ts"] = unixdate2timestamp(result[0])
        if self.fields["delta_ts"] == "":
            if self.fields.has_key("Arrival-Date") and self.fields["Arrival-Date"] != "":
                self.fields["delta_ts"] = unixdate2timestamp(self.fields["Arrival-Date"])
        self.fields["delta_ts"] = SqlQuote(self.fields["delta_ts"])
        self.fields["short_desc"] = SqlQuote(self.fields["Synopsis"])
        if self.fields.has_key("Reply-To") and self.fields["Reply-To"] != "":
            self.fields["reporter"] = get_userid(self.fields["Reply-To"])
        elif self.fields.has_key("Mail-Header"):
            result = re.search(r"""From .*?([\w.]+@[\w.]+)""", self.fields["Mail-Header"])
            if result:
                self.fields["reporter"] = get_userid(result.group(1))
            else:
                self.fields["reporter"] = get_userid(gnats_username)
        else:
            self.fields["reporter"] = get_userid(gnats_username)
        long_desc = self.fields["Description"]
        long_desc2 = ""
        for field in ["Release", "Environment", "How-To-Repeat"]:
            if self.fields.has_key(field) and self.fields[field] != "":
                long_desc += ("\n\n%s:\n" % field) + self.fields[field]
        if self.fields.has_key("Fix") and self.fields["Fix"] != "":
            long_desc2 = "Fix:\n" + self.fields["Fix"]
        if self.need_unformatted  == 1 and self.fields["Unformatted"] != "":
            long_desc += "\n\nUnformatted:\n" + self.fields["Unformatted"]
        if long_desc != "":
            self.long_descs.append((self.bug_id, self.fields["reporter"],
                                    self.fields["creation_ts"],
                                    SqlQuote(long_desc)))
        if long_desc2 != "":
            self.long_descs.append((self.bug_id, self.fields["reporter"],
                                    self.fields["creation_ts"],
                                    SqlQuote(long_desc2)))
        for field in ["gcchost", "gccbuild", "gcctarget"]:
            self.fields[field] = SqlQuote(self.fields[field])
        self.fields["version"] = ""
        if self.fields["Release"] != "":
            self.fields["version"] = convert_gccver_to_ver (self.fields["Release"])
        self.fields["version"] = SqlQuote(self.fields["version"])
        self.fields["product"] = SqlQuote("gcc")
        self.fields["component"] = "invalid"
        if self.fields.has_key("Category"):
            self.fields["component"] = self.fields["Category"]
        self.fields["component"] = SqlQuote(self.fields["component"])
        self.fields["target_milestone"] = "---"
        if self.fields["version"].find("3.4") != -1:
            self.fields["target_milestone"] = "3.4"
        self.fields["target_milestone"] = SqlQuote(self.fields["target_milestone"])
        if self.fields["userid"] == 2:
            self.fields["userid"] = "\'NULL\'"

class GNATSbug(object):
    """ Represents a single GNATS PR """
    def __init__(self, filename):
        self.attachments = []
        self.has_unformatted_attach = 0
        fp = open (filename)
        self.fields = self.parse_pr(fp.xreadlines())
        self.bug_id = int(self.fields["Number"])
        if self.fields.has_key("Unformatted"):
            self.find_gnatsweb_attachments()
        if self.fields.has_key("How-To-Repeat"):
            self.find_regular_attachments("How-To-Repeat")
        if self.fields.has_key("Fix"):
            self.find_regular_attachments("Fix")

    def get_attacher(fields):
        if fields.has_key("Reply-To") and fields["Reply-To"] != "":
            return get_userid(fields["Reply-To"])
        else:
            result = None
            if fields.has_key("Mail-Header"):
                result = re.search(r"""From .*?([\w.]+\@[\w.]+)""",
                                   fields["Mail-Header"])
            if result is not None:
                reporter = get_userid(result.group(1))
            else:
                reporter = get_userid(gnats_username)
    get_attacher = staticmethod(get_attacher)
    def find_regular_attachments(self, which):
        fields = self.fields
        while re.search("^begin [0-7]{3}", fields[which],
                        re.DOTALL | re.MULTILINE):
            outfp = cStringIO.StringIO()
            infp = cStringIO.StringIO(fields[which])
            filename, start, end = specialuu.decode(infp, outfp, quiet=0)
            fields[which]=fields[which].replace(fields[which][start:end],
                                                "See attachments for %s\n" % filename)
            self.attachments.append((filename, outfp.getvalue(),
                                     self.get_attacher(fields)))

    def decode_gnatsweb_attachment(self, attachment):
        result = re.split(r"""\n\n""", attachment, 1)
        if len(result) == 1:
            return -1
        envelope, body = result
        envelope = uselessre.split(envelope)
        envelope.pop(0)
        # Turn the list of key, value into a dict of key => value
        attachinfo = dict([(envelope[i], envelope[i+1]) for i in xrange(0,len(envelope),2)])
        for x in attachinfo.keys():
            attachinfo[x] = attachinfo[x].rstrip()
        if (attachinfo.has_key("Content-Type") == 0) or \
           (attachinfo.has_key("Content-Disposition") == 0):
            raise ValueError, "Unable to parse file attachment"
        result = dispositionre.search(attachinfo["Content-Disposition"])
        filename = result.group(2)
        filename = re.sub(".*/","", filename)
        filename = re.sub(".*\\\\","", filename)
        attachinfo["filename"]=filename
        result = re.search("""(\S+);.*""", attachinfo["Content-Type"])
        if result is not None:
            attachinfo["Content-Type"] = result.group(1)
        if attachinfo.has_key("Content-Transfer-Encoding"):
            if attachinfo["Content-Transfer-Encoding"] == "base64":
                attachinfo["data"] = base64.decodestring(body)
        else:
            attachinfo["data"]=body

        return (attachinfo["filename"], attachinfo["data"],
                self.get_attacher(self.fields))

    def find_gnatsweb_attachments(self):
        fields = self.fields
        attachments = re.split(attachment_delimiter, fields["Unformatted"])
        fields["Unformatted"] = attachments.pop(0)
        for attachment in attachments:
            result = self.decode_gnatsweb_attachment (attachment)
            if result != -1:
                self.attachments.append(result)
            self.has_unformatted_attach = 1
    def parse_pr(lines):
        #fields = {"envelope":[]}
        fields = {"envelope":array.array("c")}
        hdrmulti = "envelope"
        for line in lines:
            line = line.rstrip('\n')
            line += '\n'
            result = gnatfieldre.search(line)
            if result is None:
                if hdrmulti != "":
                    if fields.has_key(hdrmulti):
                        #fields[hdrmulti].append(line)
                        fields[hdrmulti].fromstring(line)
                    else:
                        #fields[hdrmulti] = [line]
                        fields[hdrmulti] = array.array("c", line)
                continue
            hdr, arg = result.groups()
            ghdr = "*not valid*"
            result = fieldnamere.search(hdr)
            if result != None:
                ghdr = result.groups()[0]
            if ghdr in fieldnames:
                if multilinefields.has_key(ghdr):
                    hdrmulti = ghdr
                    #fields[ghdr] = [""]
                    fields[ghdr] = array.array("c")
                else:
                    hdrmulti = ""
                    #fields[ghdr] = [arg]
                    fields[ghdr] = array.array("c", arg)
            elif hdrmulti != "":
                #fields[hdrmulti].append(line)
                fields[hdrmulti].fromstring(line)
            if hdrmulti == "envelope" and \
               (hdr == "Reply-To" or hdr == "From" \
                or hdr == "X-GNATS-Notify"):
                arg = fix_email_addrs(arg)
                #fields[hdr] = [arg]
                fields[hdr] = array.array("c", arg)
	if fields.has_key("Reply-To") and len(fields["Reply-To"]) > 0:
            fields["Reply-To"] = fields["Reply-To"]
        else:
            fields["Reply-To"] = fields["From"]
        if fields.has_key("From"):
            del fields["From"]
        if fields.has_key("X-GNATS-Notify") == 0:
            fields["X-GNATS-Notify"] = array.array("c")
            #fields["X-GNATS-Notify"] = ""
        for x in fields.keys():
            fields[x] = fields[x].tostring()
            #fields[x] = "".join(fields[x])            
        for x in fields.keys():
            if multilinefields.has_key(x):
                fields[x] = fields[x].rstrip()

        return fields
    parse_pr = staticmethod(parse_pr)
load_index("%s/gnats-adm/index" % gnats_db_dir)
load_categories("%s/gnats-adm/categories" % gnats_db_dir)
load_responsible("%s/gnats-adm/responsible" % gnats_db_dir)
get_userid(gnats_username)
get_userid(unassigned_username)
for x in pr_list:
    print "Processing %s..." % x
    a = GNATSbug ("%s/%s" % (gnats_db_dir, x))
    b = Bugzillabug(a)
write_non_bug_tables()
outfile.close()
