# Found on a russian zope mailing list, and modified to fix bugs in parsing
# the magic file and string making
# -- Daniel Berlin <dberlin@dberlin.org>
import sys, struct, time, re, exceptions, pprint, stat, os, pwd, grp

_mew = 0

# _magic='/tmp/magic'
# _magic='/usr/share/magic.mime'
_magic='/usr/share/magic.mime'
mime = 1

_ldate_adjust = lambda x: time.mktime( time.gmtime(x) )

BUFFER_SIZE = 1024 * 128 # 128K should be enough...

class MagicError(exceptions.Exception): pass

def _handle(fmt='@x',adj=None): return fmt, struct.calcsize(fmt), adj

KnownTypes = {
        # 'byte':_handle('@b'),
        'byte':_handle('@B'),
        'ubyte':_handle('@B'),

        'string':('s',0,None),
        'pstring':_handle('p'),

#       'short':_handle('@h'),
#       'beshort':_handle('>h'),
#       'leshort':_handle('<h'),
        'short':_handle('@H'),
        'beshort':_handle('>H'),
        'leshort':_handle('<H'),
        'ushort':_handle('@H'),
        'ubeshort':_handle('>H'),
        'uleshort':_handle('<H'),

        'long':_handle('@l'),
        'belong':_handle('>l'),
        'lelong':_handle('<l'),
        'ulong':_handle('@L'),
        'ubelong':_handle('>L'),
        'ulelong':_handle('<L'),

        'date':_handle('=l'),
        'bedate':_handle('>l'),
        'ledate':_handle('<l'),
        'ldate':_handle('=l',_ldate_adjust),
        'beldate':_handle('>l',_ldate_adjust),
        'leldate':_handle('<l',_ldate_adjust),
}

_mew_cnt = 0
def mew(x):
    global _mew_cnt
    if _mew :
        if x=='.' :
            _mew_cnt += 1
            if _mew_cnt % 64 == 0 : sys.stderr.write( '\n' )
            sys.stderr.write( '.' )
        else:
            sys.stderr.write( '\b'+x )

def has_format(s):
    n = 0
    l = None
    for c in s :
        if c == '%' :
            if l == '%' : n -= 1
            else        : n += 1
        l = c
    return n

def read_asciiz(file,size=None,pos=None):
    s = []
    if pos :
        mew('s')
        file.seek( pos, 0 )
    mew('z')
    if size is not None :
        s = [file.read( size ).split('\0')[0]]
    else:
        while 1 :
            c = file.read(1)
            if (not c) or (ord(c)==0) or (c=='\n') : break
            s.append (c)
    mew('Z')
    return ''.join(s)

def a2i(v,base=0):
    if v[-1:] in 'lL' : v = v[:-1]
    return int( v, base )

_cmap = {
        '\\' : '\\',
        '0' : '\0',
}
for c in range(ord('a'),ord('z')+1) :
    try               : e = eval('"\\%c"' % chr(c))
    except ValueError : pass
    else              : _cmap[chr(c)] = e
else:
    del c
    del e

def make_string(s):
    return eval( '"'+s.replace('"','\\"')+'"')

class MagicTestError(MagicError): pass

class MagicTest:
    def __init__(self,offset,mtype,test,message,line=None,level=None):
        self.line, self.level = line, level
        self.mtype = mtype
        self.mtest = test
        self.subtests = []
        self.mask = None
        self.smod = None
        self.nmod = None
        self.offset, self.type, self.test, self.message = \
                        offset,mtype,test,message
        if self.mtype == 'true' : return # XXX hack to enable level skips
        if test[-1:]=='\\' and test[-2:]!='\\\\' :
            self.test += 'n' # looks like someone wanted EOL to match?
        if mtype[:6]=='string' :
            if '/' in mtype : # for strings
                self.type, self.smod = \
                                        mtype[:mtype.find('/')], mtype[mtype.find('/')+1:]
        else:
            for nm in '&+-' :
                if nm in mtype : # for integer-based
                    self.nmod, self.type, self.mask = (
                            nm,
                            mtype[:mtype.find(nm)],
                            # convert mask to int, autodetect base
                            int( mtype[mtype.find(nm)+1:], 0 )
                    )
                    break
        self.struct, self.size, self.cast = KnownTypes[ self.type ]
    def __str__(self):
        return '%s %s %s %s' % (
                self.offset, self.mtype, self.mtest, self.message
        )
    def __repr__(self):
        return 'MagicTest(%s,%s,%s,%s,line=%s,level=%s,subtests=\n%s%s)' % (
                `self.offset`, `self.mtype`, `self.mtest`, `self.message`,
                `self.line`, `self.level`,
                '\t'*self.level, pprint.pformat(self.subtests)
        )
    def run(self,file):
        result = ''
        do_close = 0
        try:
            if type(file) == type('x') :
                file = open( file, 'r', BUFFER_SIZE )
                do_close = 1
#                       else:
#                               saved_pos = file.tell()
            if self.mtype != 'true' :
                data = self.read(file)
                last = file.tell()
            else:
                data = last = None
            if self.check( data ) :
                result = self.message+' '
                if has_format( result ) : result %= data
                for test in self.subtests :
                    m = test.run(file)
                    if m is not None : result += m
                return make_string( result )
        finally:
            if do_close :
                file.close()
#                       else:
#                               file.seek( saved_pos, 0 )
    def get_mod_and_value(self):
        if self.type[-6:] == 'string' :
            # "something like\tthis\n"
            if self.test[0] in '=<>' :
                mod, value = self.test[0], make_string( self.test[1:] )
            else:
                mod, value = '=', make_string( self.test )
        else:
            if self.test[0] in '=<>&^' :
                mod, value = self.test[0], a2i(self.test[1:])
            elif self.test[0] == 'x':
                mod = self.test[0]
                value = 0
            else:
                mod, value = '=', a2i(self.test)
        return mod, value
    def read(self,file):
        mew( 's' )
        file.seek( self.offset(file), 0 ) # SEEK_SET
        mew( 'r' )
        try:
            data = rdata = None
            # XXX self.size might be 0 here...
            if self.size == 0 :
                # this is an ASCIIZ string...
                size = None
                if self.test != '>\\0' : # magic's hack for string read...
                    value = self.get_mod_and_value()[1]
                    size = (value=='\0') and None or len(value)
                rdata = data = read_asciiz( file, size=size )
            else:
                rdata = file.read( self.size )
                if not rdata or (len(rdata)!=self.size) : return None
                data = struct.unpack( self.struct, rdata )[0] # XXX hack??
        except:
            print >>sys.stderr, self
            print >>sys.stderr, '@%s struct=%s size=%d rdata=%s' % (
                    self.offset, `self.struct`, self.size,`rdata`)
            raise
        mew( 'R' )
        if self.cast : data = self.cast( data )
        if self.mask :
            try:
                if   self.nmod == '&' : data &= self.mask
                elif self.nmod == '+' : data += self.mask
                elif self.nmod == '-' : data -= self.mask
                else: raise MagicTestError(self.nmod)
            except:
                print >>sys.stderr,'data=%s nmod=%s mask=%s' % (
                        `data`, `self.nmod`, `self.mask`
                )
                raise
        return data
    def check(self,data):
        mew('.')
        if self.mtype == 'true' :
            return '' # not None !
        mod, value = self.get_mod_and_value()
        if self.type[-6:] == 'string' :
            # "something like\tthis\n"
            if self.smod :
                xdata = data
                if 'b' in self.smod : # all blanks are optional
                    xdata = ''.join( data.split() )
                    value = ''.join( value.split() )
                if 'c' in self.smod : # all blanks are optional
                    xdata = xdata.upper()
                    value = value.upper()
            # if 'B' in self.smod : # compact blanks
            ### XXX sorry, i don't understand this :-(
            #       data = ' '.join( data.split() )
            #       if ' ' not in data : return None
            else:
                xdata = data
        try:
            if   mod == '=' : result = data == value
            elif mod == '<' : result = data < value
            elif mod == '>' : result = data > value
            elif mod == '&' : result = data & value
            elif mod == '^' : result = (data & (~value)) == 0
            elif mod == 'x' : result = 1
            else            : raise MagicTestError(self.test)
            if result :
                zdata, zval = `data`, `value`
                if self.mtype[-6:]!='string' :
                    try: zdata, zval = hex(data), hex(value)
                    except: zdata, zval = `data`, `value`
                if 0 : print >>sys.stderr, '%s @%s %s:%s %s %s => %s (%s)' % (
                        '>'*self.level, self.offset,
                        zdata, self.mtype, `mod`, zval, `result`,
                        self.message
                )
            return result
        except:
            print >>sys.stderr,'mtype=%s data=%s mod=%s value=%s' % (
                    `self.mtype`, `data`, `mod`, `value`
            )
            raise
    def add(self,mt):
        if not isinstance(mt,MagicTest) :
            raise MagicTestError((mt,'incorrect subtest type %s'%(type(mt),)))
        if mt.level == self.level+1 :
            self.subtests.append( mt )
        elif self.subtests :
            self.subtests[-1].add( mt )
        elif mt.level > self.level+1 :
            # it's possible to get level 3 just after level 1 !!! :-(
            level = self.level + 1
            while level < mt.level :
                xmt = MagicTest(None,'true','x','',line=self.line,level=level)
                self.add( xmt )
                level += 1
            else:
                self.add( mt ) # retry...
        else:
            raise MagicTestError((mt,'incorrect subtest level %s'%(`mt.level`,)))
    def last_test(self):
        return self.subtests[-1]
#end class MagicTest

class OffsetError(MagicError): pass

class Offset:
    pos_format = {'b':'<B','B':'>B','s':'<H','S':'>H','l':'<I','L':'>I',}
    pattern0 = re.compile(r'''    # mere offset
                ^
                &?                                          # possible ampersand
                (       0                                       # just zero
                |       [1-9]{1,1}[0-9]*        # decimal
                |       0[0-7]+                         # octal
                |       0x[0-9a-f]+                     # hex
                )
                $
                ''', re.X|re.I
    )
    pattern1 = re.compile(r'''    # indirect offset
                ^\(
                (?P<base>&?0                  # just zero
                        |&?[1-9]{1,1}[0-9]* # decimal
                        |&?0[0-7]*          # octal
                        |&?0x[0-9A-F]+      # hex
                )
                (?P<type>
                        \.         # this dot might be alone
                        [BSL]? # one of this chars in either case
                )?
                (?P<sign>
                        [-+]{0,1}
                )?
                (?P<off>0              # just zero
                        |[1-9]{1,1}[0-9]*  # decimal
                        |0[0-7]*           # octal
                        |0x[0-9a-f]+       # hex
                )?
                \)$''', re.X|re.I
    )
    def __init__(self,s):
        self.source = s
        self.value  = None
        self.relative = 0
        self.base = self.type = self.sign = self.offs = None
        m = Offset.pattern0.match( s )
        if m : # just a number
            if s[0] == '&' :
                self.relative, self.value = 1, int( s[1:], 0 )
            else:
                self.value = int( s, 0 )
            return
        m = Offset.pattern1.match( s )
        if m : # real indirect offset
            try:
                self.base = m.group('base')
                if self.base[0] == '&' :
                    self.relative, self.base = 1, int( self.base[1:], 0 )
                else:
                    self.base = int( self.base, 0 )
                if m.group('type') : self.type = m.group('type')[1:]
                self.sign = m.group('sign')
                if m.group('off') : self.offs = int( m.group('off'), 0 )
                if self.sign == '-' : self.offs = 0 - self.offs
            except:
                print >>sys.stderr, '$$', m.groupdict()
                raise
            return
        raise OffsetError(`s`)
    def __call__(self,file=None):
        if self.value is not None : return self.value
        pos = file.tell()
        try:
            if not self.relative : file.seek( self.offset, 0 )
            frmt = Offset.pos_format.get( self.type, 'I' )
            size = struct.calcsize( frmt )
            data = struct.unpack( frmt, file.read( size ) )
            if self.offs : data += self.offs
            return data
        finally:
            file.seek( pos, 0 )
    def __str__(self): return self.source
    def __repr__(self): return 'Offset(%s)' % `self.source`
#end class Offset

class MagicFileError(MagicError): pass

class MagicFile:
    def __init__(self,filename=_magic):
        self.file = None
        self.tests = []
        self.total_tests = 0
        self.load( filename )
        self.ack_tests = None
        self.nak_tests = None
    def __del__(self):
        self.close()
    def load(self,filename=None):
        self.open( filename )
        self.parse()
        self.close()
    def open(self,filename=None):
        self.close()
        if filename is not None :
            self.filename = filename
        self.file = open( self.filename, 'r', BUFFER_SIZE )
    def close(self):
        if self.file :
            self.file.close()
            self.file = None
    def parse(self):
        line_no = 0
        for line in self.file.xreadlines() :
            line_no += 1
            if not line or line[0]=='#' : continue
            line = line.lstrip().rstrip('\r\n')
            if not line or line[0]=='#' : continue
            try:
                x = self.parse_line( line )
                if x is None :
                    print >>sys.stderr, '#[%04d]#'%line_no, line
                    continue
            except:
                print >>sys.stderr, '###[%04d]###'%line_no, line
                raise
            self.total_tests += 1
            level, offset, mtype, test, message = x
            new_test = MagicTest(offset,mtype,test,message,
                    line=line_no,level=level)
            try:
                if level == 0 :
                    self.tests.append( new_test )
                else:
                    self.tests[-1].add( new_test )
            except:
                if 1 :
                    print >>sys.stderr, 'total tests=%s' % (
                            `self.total_tests`,
                    )
                    print >>sys.stderr, 'level=%s' % (
                            `level`,
                    )
                    print >>sys.stderr, 'tests=%s' % (
                            pprint.pformat(self.tests),
                    )
                raise
        else:
            while self.tests[-1].level > 0 :
                self.tests.pop()
    def parse_line(self,line):
        # print >>sys.stderr, 'line=[%s]' % line
        if (not line) or line[0]=='#' : return None
        level = 0
        offset = mtype = test = message = ''
        mask = None
        # get optional level (count leading '>')
        while line and line[0]=='>' :
            line, level = line[1:], level+1
        # get offset
        while line and not line[0].isspace() :
            offset, line = offset+line[0], line[1:]
        try:
            offset = Offset(offset)
        except:
            print >>sys.stderr, 'line=[%s]' % line
            raise
        # skip spaces
        line = line.lstrip()
        # get type
        c = None
        while line :
            last_c, c, line = c, line[0], line[1:]
            if last_c!='\\' and c.isspace() :
                break # unescaped space - end of field
            else:
                mtype += c
                if last_c == '\\' :
                    c = None # don't fuck my brain with sequential backslashes
        # skip spaces
        line = line.lstrip()
        # get test
        c = None
        while line :
            last_c, c, line = c, line[0], line[1:]
            if last_c!='\\' and c.isspace() :
                break # unescaped space - end of field
            else:
                test += c
                if last_c == '\\' :
                    c = None # don't fuck my brain with sequential backslashes
        # skip spaces
        line = line.lstrip()
        # get message
        message = line
        if mime and line.find("\t") != -1:
            message=line[0:line.find("\t")]
        #
        # print '>>', level, offset, mtype, test, message
        return level, offset, mtype, test, message
    def detect(self,file):
        self.ack_tests = 0
        self.nak_tests = 0
        answers = []
        for test in self.tests :
            message = test.run( file )
            if message :
                self.ack_tests += 1
                answers.append( message )
            else:
                self.nak_tests += 1
        if answers :
            return '; '.join( answers )
#end class MagicFile

def username(uid):
    try:
        return pwd.getpwuid( uid )[0]
    except:
        return '#%s'%uid

def groupname(gid):
    try:
        return grp.getgrgid( gid )[0]
    except:
        return '#%s'%gid

def get_file_type(fname,follow):
    t = None
    if not follow :
        try:
            st = os.lstat( fname ) # stat that entry, don't follow links!
        except os.error, why :
            pass
        else:
            if stat.S_ISLNK(st[stat.ST_MODE]) :
                t = 'symbolic link'
                try:
                    lnk = os.readlink( fname )
                except:
                    t += ' (unreadable)'
                else:
                    t += ' to '+lnk
    if t is None :
        try:
            st = os.stat( fname )
        except os.error, why :
            return "can't stat `%s' (%s)." % (why.filename,why.strerror)

    dmaj, dmin = (st.st_rdev>>8)&0x0FF, st.st_rdev&0x0FF

    if 0 : pass
    elif stat.S_ISSOCK(st.st_mode) : t = 'socket'
    elif stat.S_ISLNK (st.st_mode) : t = follow and 'symbolic link' or t
    elif stat.S_ISREG (st.st_mode) : t = 'file'
    elif stat.S_ISBLK (st.st_mode) : t = 'block special (%d/%d)'%(dmaj,dmin)
    elif stat.S_ISDIR (st.st_mode) : t = 'directory'
    elif stat.S_ISCHR (st.st_mode) : t = 'character special (%d/%d)'%(dmaj,dmin)
    elif stat.S_ISFIFO(st.st_mode) : t = 'pipe'
    else: t = '<unknown>'

    if st.st_mode & stat.S_ISUID :
        t = 'setuid(%d=%s) %s'%(st.st_uid,username(st.st_uid),t)
    if st.st_mode & stat.S_ISGID :
        t = 'setgid(%d=%s) %s'%(st.st_gid,groupname(st.st_gid),t)
    if st.st_mode & stat.S_ISVTX :
        t = 'sticky '+t

    return t

HELP = '''%s [options] [files...]

Options:

        -?, --help -- this help
        -m, --magic=<file> -- use this magic <file> instead of %s
        -f, --files=<namefile> -- read filenames for <namefile>
*       -C, --compile -- write "compiled" magic file
        -b, --brief -- don't prepend filenames to output lines
+       -c, --check -- check the magic file
        -i, --mime -- output MIME types
*       -k, --keep-going -- don't stop st the first match
        -n, --flush -- flush stdout after each line
        -v, --verson -- print version and exit
*       -z, --compressed -- try to look inside compressed files
        -L, --follow -- follow symlinks
        -s, --special -- don't skip special files

*       -- not implemented so far ;-)
+       -- implemented, but in another way...
'''

def main():
    import getopt
    global _magic
    try:
        brief = 0
        flush = 0
        follow= 0
        mime  = 0
        check = 0
        special=0
        try:
            opts, args = getopt.getopt(
                    sys.argv[1:],
                    '?m:f:CbciknvzLs',
                    (       'help',
                            'magic=',
                            'names=',
                            'compile',
                            'brief',
                            'check',
                            'mime',
                            'keep-going',
                            'flush',
                            'version',
                            'compressed',
                            'follow',
                            'special',
                    )
            )
        except getopt.error, why:
            print >>sys.stderr, sys.argv[0], why
            return 1
        else:
            files = None
            for o,v in opts :
                if o in ('-?','--help'):
                    print HELP % (
                            sys.argv[0],
                            _magic,
                    )
                    return 0
                elif o in ('-f','--files='):
                    files = v
                elif o in ('-m','--magic='):
                    _magic = v[:]
                elif o in ('-C','--compile'):
                    pass
                elif o in ('-b','--brief'):
                    brief = 1
                elif o in ('-c','--check'):
                    check = 1
                elif o in ('-i','--mime'):
                    mime = 1
                    if os.path.exists( _magic+'.mime' ) :
                        _magic += '.mime'
                        print >>sys.stderr,sys.argv[0]+':',\
                                                        "Using regular magic file `%s'" % _magic
                elif o in ('-k','--keep-going'):
                    pass
                elif o in ('-n','--flush'):
                    flush = 1
                elif o in ('-v','--version'):
                    print 'VERSION'
                    return 0
                elif o in ('-z','--compressed'):
                    pass
                elif o in ('-L','--follow'):
                    follow = 1
                elif o in ('-s','--special'):
                    special = 1
            else:
                if files :
                    files = map(lambda x: x.strip(), v.split(','))
                    if '-' in files and '-' in args :
                        error( 1, 'cannot use STDIN simultaneously for file list and data' )
                    for file in files :
                        for name in (
                                        (file=='-')
                                                and sys.stdin
                                                or open(file,'r',BUFFER_SIZE)
                        ).xreadlines():
                            name = name.strip()
                            if name not in args :
                                args.append( name )
        try:
            if check : print >>sys.stderr, 'Loading magic database...'
            t0 = time.time()
            m = MagicFile(_magic)
            t1 = time.time()
            if check :
                print >>sys.stderr, \
                                        m.total_tests, 'tests loaded', \
                                        'for', '%.2f' % (t1-t0), 'seconds'
                print >>sys.stderr, len(m.tests), 'tests at top level'
                return 0 # XXX "shortened" form ;-)

            mlen = max( map(len, args) )+1
            for arg in args :
                if not brief : print (arg + ':').ljust(mlen),
                ftype = get_file_type( arg, follow )
                if (special and ftype.find('special')>=0) \
                                or ftype[-4:] == 'file' :
                    t0 = time.time()
                    try:
                        t = m.detect( arg )
                    except (IOError,os.error), why:
                        t = "can't read `%s' (%s)" % (why.filename,why.strerror)
                    if ftype[-4:] == 'file' : t = ftype[:-4] + t
                    t1 = time.time()
                    print t and t or 'data'
                    if 0 : print \
                                                        '#\t%d tests ok, %d tests failed for %.2f seconds'%\
                                                        (m.ack_tests, m.nak_tests, t1-t0)
                else:
                    print mime and 'application/x-not-regular-file' or ftype
                if flush : sys.stdout.flush()
        # print >>sys.stderr, 'DONE'
        except:
            if check : return 1
            raise
        else:
            return 0
    finally:
        pass

if __name__ == '__main__' :
    sys.exit( main() )
# vim:ai
# EOF #
