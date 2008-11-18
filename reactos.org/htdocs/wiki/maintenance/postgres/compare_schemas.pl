#!/usr/bin/perl

## Rough check that the base and postgres "tables.sql" are in sync
## Should be run from maintenance/postgres
## Checks a few other things as well...

use strict;
use warnings;
use Data::Dumper;
use Cwd;

check_valid_sql();

my @old = ('../tables.sql');
my $new = 'tables.sql';
my @xfile;

## Read in exceptions and other metadata
my %ok;
while (<DATA>) {
	next unless /^(\w+)\s*:\s*([^#]+)/;
	my ($name,$val) = ($1,$2);
	chomp $val;
	if ($name eq 'RENAME') {
		die "Invalid rename\n" unless $val =~ /(\w+)\s+(\w+)/;
		$ok{OLD}{$1} = $2;
		$ok{NEW}{$2} = $1;
		next;
	}
	if ($name eq 'XFILE') {
		push @xfile, $val;
		next;
	}
	for (split /\s+/ => $val) {
		$ok{$name}{$_} = 0;
	}
}

my $datatype = join '|' => qw(
bool
tinyint int bigint real float
tinytext mediumtext text char varchar varbinary binary
timestamp datetime
tinyblob mediumblob blob
);
$datatype .= q{|ENUM\([\"\w, ]+\)};
$datatype = qr{($datatype)};

my $typeval = qr{(\(\d+\))?};

my $typeval2 = qr{ signed| unsigned| binary| NOT NULL| NULL| auto_increment| default ['\-\d\w"]+| REFERENCES .+CASCADE};

my $indextype = join '|' => qw(INDEX KEY FULLTEXT), 'PRIMARY KEY', 'UNIQUE INDEX', 'UNIQUE KEY';
$indextype = qr{$indextype};

my $engine = qr{TYPE|ENGINE};

my $tabletype = qr{InnoDB|MyISAM|HEAP|HEAP MAX_ROWS=\d+|InnoDB MAX_ROWS=\d+ AVG_ROW_LENGTH=\d+};

my $charset = qr{utf8|binary};

open my $newfh, '<', $new or die qq{Could not open $new: $!\n};


my ($table,%old);

## Read in the xfiles
my %xinfo;
for my $xfile (@xfile) {
	print "Loading $xfile\n";
	my $info = parse_sql($xfile);
	for (keys %$info) {
		$xinfo{$_} = $info->{$_};
	}
}

for my $oldfile (@old) {
	print "Loading $oldfile\n";
	my $info = parse_sql($oldfile);
	for (keys %xinfo) {
		$info->{$_} = $xinfo{$_};
	}
	$old{$oldfile} = $info;
}

sub parse_sql {

	my $oldfile = shift;

	open my $oldfh, '<', $oldfile or die qq{Could not open $oldfile: $!\n};

	my %info;
	while (<$oldfh>) {
		next if /^\s*\-\-/ or /^\s+$/;
		s/\s*\-\- [\w ]+$//;
		chomp;

		if (/CREATE\s*TABLE/i) {
			m{^CREATE TABLE /\*\$wgDBprefix\*/(\w+) \($}
				or die qq{Invalid CREATE TABLE at line $. of $oldfile\n};
			$table = $1;
			$info{$table}{name}=$table;
		}
		elsif (m{^\) /\*\$wgDBTableOptions\*/}) {
			$info{$table}{engine} = 'ENGINE';
			$info{$table}{type} = 'variable';
		}
		elsif (/^\) ($engine)=($tabletype);$/) {
			$info{$table}{engine}=$1;
			$info{$table}{type}=$2;
		}
		elsif (/^\) ($engine)=($tabletype), DEFAULT CHARSET=($charset);$/) {
			$info{$table}{engine}=$1;
			$info{$table}{type}=$2;
			$info{$table}{charset}=$3;
		}
		elsif (/^  (\w+) $datatype$typeval$typeval2{0,3},?$/) {
			$info{$table}{column}{$1} = $2;
			my $extra = $3 || '';
			$info{$table}{columnfull}{$1} = "$2$extra";
		}
		elsif (/^  ($indextype)(?: (\w+))? \(([\w, \(\)]+)\),?$/) {
			$info{$table}{lc $1.'_name'} = $2 ? $2 : '';
			$info{$table}{lc $1.'pk_target'} = $3;
		}
		else {
			die "Cannot parse line $. of $oldfile:\n$_\n";
		}

	}
	close $oldfh or die qq{Could not close "$oldfile": $!\n};

	return \%info;

} ## end of parse_sql

## Read in the parser test information
my $parsefile = '../parserTests.inc';
open my $pfh, '<', $parsefile or die qq{Could not open "$parsefile": $!\n};
my $stat = 0;
my %ptable;
while (<$pfh>) {
	if (!$stat) {
		if (/function listTables/) {
			$stat = 1;
		}
		next;
	}
	$ptable{$1}=2 while m{'(\w+)'}g;
	last if /\);/;
}
close $pfh or die qq{Could not close "$parsefile": $!\n};

my $OK_NOT_IN_PTABLE = '
filearchive
logging
profiling
querycache_info
searchindex
trackbacks
transcache
user_newtalk
updatelog
';

## Make sure all tables in main tables.sql are accounted for in the parsertest.
for my $table (sort keys %{$old{'../tables.sql'}}) {
	$ptable{$table}++;
	next if $ptable{$table} > 2;
	next if $OK_NOT_IN_PTABLE =~ /\b$table\b/;
	print qq{Table "$table" is in the schema, but not used inside of parserTest.inc\n};
}
## Any that are used in ptables but no longer exist in the schema?
for my $table (sort grep { $ptable{$_} == 2 } keys %ptable) {
	print qq{Table "$table" ($ptable{$table}) used in parserTest.inc, but not found in schema\n};
}

for my $oldfile (@old) {

## Begin non-standard indent

## MySQL sanity checks
for my $table (sort keys %{$old{$oldfile}}) {
	my $t = $old{$oldfile}{$table};
	if ($t->{engine} eq 'TYPE') {
		die "Invalid engine for $oldfile: $t->{engine}\n" unless $t->{name} eq 'profiling';
	}
	my $charset = $t->{charset} || '';
	if ($oldfile !~ /binary/ and $charset eq 'binary') {
		die "Invalid charset for $oldfile: $charset\n";
	}
}

my $dtype = join '|' => qw(
SMALLINT INTEGER BIGINT NUMERIC SERIAL
TEXT CHAR VARCHAR
BYTEA
TIMESTAMPTZ
CIDR
);
$dtype = qr{($dtype)};
my %new;
my ($infunction,$inview,$inrule,$lastcomma) = (0,0,0,0);
seek $newfh, 0, 0;
while (<$newfh>) {
	next if /^\s*\-\-/ or /^\s*$/;
	s/\s*\-\- [\w ']+$//;
	next if /^BEGIN;/ or /^SET / or /^COMMIT;/;
	next if /^CREATE SEQUENCE/;
	next if /^CREATE(?: UNIQUE)? INDEX/;
	next if /^CREATE FUNCTION/;
	next if /^CREATE TRIGGER/ or /^  FOR EACH ROW/;
	next if /^INSERT INTO/ or /^  VALUES \(/;
	next if /^ALTER TABLE/;
	chomp;

	if (/^\$mw\$;?$/) {
		$infunction = $infunction ? 0 : 1;
		next;
	}
	next if $infunction;

	next if /^CREATE VIEW/ and $inview = 1;
	if ($inview) {
		/;$/ and $inview = 0;
		next;
	}

	next if /^CREATE RULE/ and $inrule = 1;
	if ($inrule) {
		/;$/ and $inrule = 0;
		next;
	}

	if (/^CREATE TABLE "?(\w+)"? \($/) {
		$table = $1;
		$new{$table}{name}=$table;
		$lastcomma = 1;
	}
	elsif (/^\);$/) {
		if ($lastcomma) {
			warn "Stray comma before line $.\n";
		}
	}
	elsif (/^  (\w+) +$dtype.*?(,?)(?: --.*)?$/) {
		$new{$table}{column}{$1} = $2;
		if (!$lastcomma) {
			print "Missing comma before line $. of $new\n";
		}
		$lastcomma = $3 ? 1 : 0;
	}
	else {
		die "Cannot parse line $. of $new:\n$_\n";
	}
}

## Which column types are okay to map from mysql to postgres?
my $COLMAP = q{
## INTS:
tinyint SMALLINT
int INTEGER SERIAL
bigint BIGINT
real NUMERIC
float NUMERIC

## TEXT:
varchar(15) TEXT
varchar(32) TEXT
varchar(70) TEXT
varchar(255) TEXT
varchar TEXT
text TEXT
tinytext TEXT
ENUM TEXT

## TIMESTAMPS:
varbinary(14) TIMESTAMPTZ
binary(14) TIMESTAMPTZ
datetime TIMESTAMPTZ
timestamp TIMESTAMPTZ

## BYTEA:
mediumblob BYTEA

## OTHER:
bool SMALLINT # Sigh

};
## Allow specific exceptions to the above
my $COLMAPOK = q{
## User inputted text strings:
ar_comment      tinyblob       TEXT
fa_description  tinyblob       TEXT
img_description tinyblob       TEXT
ipb_reason      tinyblob       TEXT
log_action      varbinary(10)  TEXT
oi_description  tinyblob       TEXT
rev_comment     tinyblob       TEXT
rc_log_action   varbinary(255) TEXT
rc_log_type     varbinary(255) TEXT

## Simple text-only strings:
ar_flags          tinyblob       TEXT
fa_minor_mime     varbinary(32)  TEXT
fa_storage_group  varbinary(16)  TEXT # Just 'deleted' for now, should stay plain text
fa_storage_key    varbinary(64)  TEXT # sha1 plus text extension
ipb_address       tinyblob       TEXT # IP address or username
ipb_range_end     tinyblob       TEXT # hexadecimal
ipb_range_start   tinyblob       TEXT # hexadecimal
img_minor_mime    varbinary(32)  TEXT
img_sha1          varbinary(32)  TEXT
job_cmd           varbinary(60)  TEXT # Should we limit to 60 as well?
keyname           varbinary(255) TEXT # No tablename prefix (objectcache)
ll_lang           varbinary(20)  TEXT # Language code
log_params        blob           TEXT # LF separated list of args
log_type          varbinary(10)  TEXT
oi_minor_mime     varbinary(32)  TEXT
oi_sha1           varbinary(32)  TEXT
old_flags         tinyblob       TEXT
old_text          mediumblob     TEXT
pp_propname       varbinary(60)  TEXT
pp_value          blob           TEXT
page_restrictions tinyblob       TEXT # CSV string
pf_server         varchar(30)    TEXT
pr_level          varbinary(60)  TEXT
pr_type           varbinary(60)  TEXT
pt_create_perm    varbinary(60)  TEXT
pt_reason         tinyblob       TEXT
qc_type           varbinary(32)  TEXT
qcc_type          varbinary(32)  TEXT
qci_type          varbinary(32)  TEXT
rc_params         blob           TEXT
rlc_to_blob       blob           TEXT
ug_group          varbinary(16)  TEXT
user_email_token  binary(32)     TEXT
user_ip           varbinary(40)  TEXT
user_newpassword  tinyblob       TEXT
user_options      blob           TEXT
user_password     tinyblob       TEXT
user_token        binary(32)     TEXT

## Text URLs:
el_index blob           TEXT
el_to    blob           TEXT
iw_url   blob           TEXT
tb_url   blob           TEXT
tc_url   varbinary(255) TEXT

## Deprecated or not yet used:
ar_text     mediumblob TEXT
job_params  blob       TEXT
log_deleted tinyint    INTEGER # Not used yet, but keep it INTEGER for safety
rc_type     tinyint    CHAR

## Number tweaking:
fa_bits   int SMALLINT # bits per pixel
fa_height int SMALLINT
fa_width  int SMALLINT # Hope we don't see an image this wide...
hc_id     int BIGINT   # Odd that site_stats is all bigint...
img_bits  int SMALLINT # bits per image should stay sane
oi_bits   int SMALLINT

## True binary fields, usually due to gzdeflate and/or serialize:
math_inputhash  varbinary(16) BYTEA
math_outputhash varbinary(16) BYTEA

## Namespaces: not need for such a high range
ar_namespace     int SMALLINT
job_namespace    int SMALLINT
log_namespace    int SMALLINT
page_namespace   int SMALLINT
pl_namespace     int SMALLINT
pt_namespace     int SMALLINT
qc_namespace     int SMALLINT
rc_namespace     int SMALLINT
rd_namespace     int SMALLINT
rlc_to_namespace int SMALLINT
tl_namespace     int SMALLINT
wl_namespace     int SMALLINT

## Easy enough to change if a wiki ever does grow this big:
ss_good_articles bigint INTEGER
ss_total_edits   bigint INTEGER
ss_total_pages   bigint INTEGER
ss_total_views   bigint INTEGER
ss_users         bigint INTEGER

## True IP - keep an eye on these, coders tend to make textual assumptions
rc_ip varbinary(40) CIDR # Want to keep an eye on this

## Others:
tc_time int TIMESTAMPTZ


};

my %colmap;
for (split /\n/ => $COLMAP) {
	next unless /^\w/;
	s/(.*?)#.*/$1/;
	my ($col,@maps) = split / +/, $_;
	for (@maps) {
		$colmap{$col}{$_} = 1;
	}
}

my %colmapok;
for (split /\n/ => $COLMAPOK) {
	next unless /^\w/;
	my ($col,$old,$new) = split / +/, $_;
	$colmapok{$col}{$old}{$new} = 1;
}

## Old but not new
for my $t (sort keys %{$old{$oldfile}}) {
	if (!exists $new{$t} and !exists $ok{OLD}{$t}) {
		print "Table not in $new: $t\n";
		next;
	}
	next if exists $ok{OLD}{$t} and !$ok{OLD}{$t};
	my $newt = exists $ok{OLD}{$t} ? $ok{OLD}{$t} : $t;
	my $oldcol = $old{$oldfile}{$t}{column};
	my $oldcolfull = $old{$oldfile}{$t}{columnfull};
	my $newcol = $new{$newt}{column};
	for my $c (keys %$oldcol) {
		if (!exists $newcol->{$c}) {
			print "Column $t.$c not in $new\n";
			next;
		}
	}
	for my $c (sort keys %$newcol) {
		if (!exists $oldcol->{$c}) {
			print "Column $t.$c not in $oldfile\n";
			next;
		}
		## Column types (roughly) match up?
		my $new = $newcol->{$c};
		my $old = $oldcolfull->{$c};

		## Known exceptions:
		next if exists $colmapok{$c}{$old}{$new};

		$old =~ s/ENUM.*/ENUM/;
		if (! exists $colmap{$old}{$new}) {
			print "Column types for $t.$c do not match: $old does not map to $new\n";
		}
	}
}
## New but not old:
for (sort keys %new) {
	if (!exists $old{$oldfile}{$_} and !exists $ok{NEW}{$_}) {
		print "Not in $oldfile: $_\n";
		next;
	}
}


} ## end each file to be parsed


sub check_valid_sql {

	## Check for a few common problems in most php files

	my $olddir = getcwd();
	chdir("../..");
	for my $basedir (qw/includes extensions/) {
		scan_dir($basedir);
	}
	chdir $olddir;

	return;

} ## end of check_valid_sql


sub scan_dir {

	my $dir = shift;

	opendir my $dh, $dir or die qq{Could not opendir $dir: $!\n};
	print "Scanning $dir...\n";
	for my $file (grep { -f "$dir/$_" and /\.php$/ } readdir $dh) {
		find_problems("$dir/$file");
	}
	rewinddir $dh;
	for my $subdir (grep { -d "$dir/$_" and ! /\./ } readdir $dh) {
		scan_dir("$dir/$subdir");
	}
	closedir $dh or die qq{Closedir failed: $!\n};
	return;

} ## end of scan_dir

sub find_problems {

	my $file = shift;
	open my $fh, '<', $file or die qq{Could not open "$file": $!\n};
	while (<$fh>) {
		if (/FORCE INDEX/ and $file !~ /Database\w*\.php/) {
			warn "Found FORCE INDEX string at line $. of $file\n";
		}
		if (/REPLACE INTO/ and $file !~ /Database\w*\.php/) {
			warn "Found REPLACE INTO string at line $. of $file\n";
		}
		if (/\bIF\s*\(/ and $file !~ /DatabaseMySQL\.php/) {
			warn "Found IF string at line $. of $file\n";
		}
		if (/\bCONCAT\b/ and $file !~ /Database\w*\.php/) {
			warn "Found CONCAT string at line $. of $file\n";
		}
		if (/\bGROUP\s+BY\s*\d\b/i and $file !~ /Database\w*\.php/) {
			warn "Found GROUP BY # at line $. of $file\n";
		}
	}
	close $fh or die qq{Could not close "$file": $!\n};
	return;

} ## end of find_problems


__DATA__
## Known exceptions
OLD: searchindex          ## We use tsearch2 directly on the page table instead
RENAME: user mwuser       ## Reserved word causing lots of problems
RENAME: text pagecontent  ## Reserved word
NEW: mediawiki_version    ## Just us, for now
XFILE: ../archives/patch-profiling.sql
