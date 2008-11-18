#!/usr/bin/perl

## Convert data from a MySQL mediawiki database into a Postgres mediawiki database
## svn: $Id: mediawiki_mysql2postgres.pl 33556 2008-04-18 16:27:57Z greg $

## NOTE: It is probably easier to dump your wiki using maintenance/dumpBackup.php
## and then import it with maintenance/importDump.php

## If having UTF-8 problems, there are reports that adding --compatible=postgresql
## may help.

use strict;
use warnings;
use Data::Dumper;
use Getopt::Long;

use vars qw(%table %tz %special @torder $COM);
my $VERSION = '1.2';

## The following options can be changed via command line arguments:
my $MYSQLDB       = '';
my $MYSQLUSER     = '';

## If the following are zero-length, we omit their arguments entirely:
my $MYSQLHOST     = '';
my $MYSQLPASSWORD = '';
my $MYSQLSOCKET   = '';

## Name of the dump file created
my $MYSQLDUMPFILE = 'mediawiki_upgrade.pg';

## How verbose should this script be (0, 1, or 2)
my $verbose = 0;

my $help = 0;

my $USAGE = "
Usage: $0 --db=<dbname> --user=<user> [OPTION]...
Example: $0 --db=wikidb --user=wikiuser --pass=sushi

Converts a MediaWiki schema from MySQL to Postgres
Options:
  db       Name of the MySQL database
  user     MySQL database username
  pass     MySQL database password
  host     MySQL database host
  socket   MySQL database socket
  verbose  Verbosity, increases with multiple uses
";

GetOptions
	(
	 'db=s'     => \$MYSQLDB,
	 'user=s'   => \$MYSQLUSER,
	 'pass=s'   => \$MYSQLPASSWORD,
	 'host=s'   => \$MYSQLHOST,
	 'socket=s' => \$MYSQLSOCKET,
	 'verbose+' => \$verbose,
	 'help'     => \$help,
 );

die $USAGE
	if ! length $MYSQLDB
	or ! length $MYSQLUSER
	or $help;

## The Postgres schema file: should not be changed
my $PG_SCHEMA = 'tables.sql';

## What version we default to when we can't parse the old schema
my $MW_DEFAULT_VERSION = 110;

## Try and find a working version of mysqldump
$verbose and warn "Locating the mysqldump executable\n";
my @MYSQLDUMP = ('/usr/local/bin/mysqldump', '/usr/bin/mysqldump');
my $MYSQLDUMP;
for my $mytry (@MYSQLDUMP) {
	next if ! -e $mytry;
	-x $mytry or die qq{Not an executable file: "$mytry"\n};
	my $version = qx{$mytry -V};
	$version =~ /^mysqldump\s+Ver\s+\d+/ or die qq{Program at "$mytry" does not act like mysqldump\n};
	$MYSQLDUMP = $mytry;
}
$MYSQLDUMP or die qq{Could not find the mysqldump program\n};

## Flags we use for mysqldump
my @MYSQLDUMPARGS = qw(
--skip-lock-tables
--complete-insert
--skip-extended-insert
--skip-add-drop-table
--skip-add-locks
--skip-disable-keys
--skip-set-charset
--skip-comments
--skip-quote-names
);


$verbose and warn "Checking that mysqldump can handle our flags\n";
## Make sure this version can handle all the flags we want.
## Combine with user dump below
my $MYSQLDUMPARGS = join ' ' => @MYSQLDUMPARGS;
## Argh. Any way to make this work on Win32?
my $version = qx{$MYSQLDUMP $MYSQLDUMPARGS 2>&1};
if ($version =~ /unknown option/) {
	die qq{Sorry, you need to use a newer version of the mysqldump program than the one at "$MYSQLDUMP"\n};
}

push @MYSQLDUMPARGS, "--user=$MYSQLUSER";
length $MYSQLPASSWORD and push @MYSQLDUMPARGS, "--password=$MYSQLPASSWORD";
length $MYSQLHOST and push @MYSQLDUMPARGS, "--host=$MYSQLHOST";

## Open the dump file to hold the mysqldump output
open my $mdump, '+>', $MYSQLDUMPFILE or die qq{Could not open "$MYSQLDUMPFILE": $!\n};
print qq{Writing file "$MYSQLDUMPFILE"\n};

open my $mfork2, '-|' or exec $MYSQLDUMP, @MYSQLDUMPARGS, '--no-data', $MYSQLDB;
my $oldselect = select $mdump;

print while <$mfork2>;

## Slurp in the current schema
my $current_schema;
seek $mdump, 0, 0;
{
	local $/;
	$current_schema = <$mdump>;
}
seek $mdump, 0, 0;
truncate $mdump, 0;

warn qq{Trying to determine database version...\n} if $verbose;

my $current_version = 0;
if ($current_schema =~ /CREATE TABLE \S+cur /) {
	$current_version = 103;
}
elsif ($current_schema =~ /CREATE TABLE \S+brokenlinks /) {
	$current_version = 104;
}
elsif ($current_schema !~ /CREATE TABLE \S+templatelinks /) {
	$current_version = 105;
}
elsif ($current_schema !~ /CREATE TABLE \S+validate /) {
	$current_version = 106;
}
elsif ($current_schema !~ /ipb_auto tinyint/) {
	$current_version = 107;
}
elsif ($current_schema !~ /CREATE TABLE \S+profiling /) {
	$current_version = 108;
}
elsif ($current_schema !~ /CREATE TABLE \S+querycachetwo /) {
	$current_version = 109;
}
else {
	$current_version = $MW_DEFAULT_VERSION;
}

if (!$current_version) {
	warn qq{WARNING! Could not figure out the old version, assuming MediaWiki $MW_DEFAULT_VERSION\n};
	$current_version = $MW_DEFAULT_VERSION;
}

## Check for a table prefix:
my $table_prefix = '';
if ($current_schema =~ /CREATE TABLE (\S+)querycache /) {
	$table_prefix = $1;
}

warn qq{Old schema is from MediaWiki version $current_version\n} if $verbose;
warn qq{Table prefix is "$table_prefix"\n} if $verbose and length $table_prefix;

$verbose and warn qq{Writing file "$MYSQLDUMPFILE"\n};
my $now = scalar localtime;
my $conninfo = '';
$MYSQLHOST and $conninfo .= "\n--   host      $MYSQLHOST";
$MYSQLSOCKET and $conninfo .= "\n--   socket    $MYSQLSOCKET";

print qq{
-- Dump of MySQL Mediawiki tables for import into a Postgres Mediawiki schema
-- Performed by the program: $0
-- Version: $VERSION (subversion }.q{$LastChangedRevision: 33556 $}.qq{)
-- Author: Greg Sabino Mullane <greg\@turnstep.com> Comments welcome
--
-- This file was created: $now
-- Executable used: $MYSQLDUMP
-- Connection information:
--   database:  $MYSQLDB
--   user:      $MYSQLUSER$conninfo

-- This file can be imported manually with psql like so:
-- psql -p port# -h hostname -U username -f $MYSQLDUMPFILE databasename
-- This will overwrite any existing MediaWiki information, so be careful

};

## psql specific stuff
print q{
\\set ON_ERROR_STOP
BEGIN;
SET client_min_messages = 'WARNING';
SET timezone = 'GMT';
SET DateStyle = 'ISO, YMD';
};

warn qq{Reading in the Postgres schema information\n} if $verbose;
open my $schema, '<', $PG_SCHEMA
	or die qq{Could not open "$PG_SCHEMA": make sure this script is run from maintenance/postgres/\n};
my $t;
while (<$schema>) {
	if (/CREATE TABLE\s+(\S+)/) {
		$t = $1;
		$table{$t}={};
		$verbose > 1 and warn qq{  Found table $t\n};
	}
	elsif (/^ +(\w+)\s+TIMESTAMP/) {
		$tz{$t}{$1}++;
		$verbose > 1 and warn qq{    Got a timestamp for column $1\n};
	}
	elsif (/REFERENCES\s*([^( ]+)/) {
		my $ref = $1;
		exists $table{$ref} or die qq{No parent table $ref found for $t\n};
		$table{$t}{$ref}++;
	}
}
close $schema or die qq{Could not close "$PG_SCHEMA": $!\n};

## Read in special cases and table/version information
$verbose and warn qq{Reading in schema exception information\n};
my %version_tables;
while (<DATA>) {
	if (/^VERSION\s+(\d+\.\d+):\s+(.+)/) {
		my $list = join '|' => split /\s+/ => $2;
		$version_tables{$1} = qr{\b$list\b};
		next;
	}
	next unless /^(\w+)\s*(.*)/;
	$special{$1} = $2||'';
	$special{$2} = $1 if length $2;
}

## Determine the order of tables based on foreign key constraints
$verbose and warn qq{Figuring out order of tables to dump\n};
my %dumped;
my $bail = 0;
{
	my $found=0;
	T: for my $t (sort keys %table) {
		next if exists $dumped{$t} and $dumped{$t} >= 1;
		$found=1;
		for my $dep (sort keys %{$table{$t}}) {
			next T if ! exists $dumped{$dep} or $dumped{$dep} < 0;
		}
		$dumped{$t} = -1 if ! exists $dumped{$t};
		## Skip certain tables that are not imported
		next if exists $special{$t} and !$special{$t};
		push @torder, $special{$t} || $t;
	}
	last if !$found;
	push @torder, '---';
	for (values %dumped) { $_+=2; }
	die "Too many loops!\n" if $bail++ > 1000;
	redo;
}

## Prepare the Postgres database for the move
$verbose and warn qq{Writing Postgres transformation information\n};

print "\n-- Empty out all existing tables\n";
$verbose and warn qq{Writing truncates to empty existing tables\n};


for my $t (@torder, 'objectcache', 'querycache') {
	next if $t eq '---';
	my $tname = $special{$t}||$t;
	printf qq{TRUNCATE TABLE %-20s;\n}, qq{"$tname"};
}
print "\n\n";

print qq{-- Temporarily rename pagecontent to "text"\n};
print qq{ALTER TABLE pagecontent RENAME TO "text";\n\n};

print qq{-- Allow rc_ip to contain empty string, will convert at end\n};
print qq{ALTER TABLE recentchanges ALTER rc_ip TYPE text USING host(rc_ip);\n\n};

print "-- Changing all timestamp fields to handle raw integers\n";
for my $t (sort keys %tz) {
	next if $t eq 'archive2';
	for my $c (sort keys %{$tz{$t}}) {
		printf "ALTER TABLE %-18s ALTER %-25s TYPE TEXT;\n", $t, $c;
	}
}
print "\n";

print q{
INSERT INTO page VALUES (0,-1,'Dummy Page','',0,0,0,default,now(),0,10);
};

## If we have a table _prefix, we need to temporarily rename all of our Postgres
## tables temporarily for the import. Perhaps consider making this an auto-schema
## thing in the future.
if (length $table_prefix) {
	print qq{\n\n-- Temporarily renaming tables to accomodate the table_prefix "$table_prefix"\n\n};
	for my $t (@torder) {
		next if $t eq '---';
		my $tname = $special{$t}||$t;
		printf qq{ALTER TABLE %-18s RENAME TO "${table_prefix}$tname"\n}, qq{"$tname"};
	}
}


## Try and dump the ill-named "user" table:
## We do this table alone because "user" is a reserved word.
print q{

SET escape_string_warning TO 'off';
\\o /dev/null

-- Postgres uses a table name of "mwuser" instead of "user"

-- Create a dummy user to satisfy fk contraints especially with revisions
SELECT setval('user_user_id_seq',0,'false');
INSERT INTO mwuser
  VALUES (DEFAULT,'Anonymous','',NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,now(),now());

};

push @MYSQLDUMPARGS, '--no-create-info';

$verbose and warn qq{Dumping "user" table\n};
$verbose > 2 and warn Dumper \@MYSQLDUMPARGS;
my $usertable = "${table_prefix}user";
open my $mfork, '-|' or exec $MYSQLDUMP, @MYSQLDUMPARGS, $MYSQLDB, $usertable;
## Unfortunately, there is no easy way to catch errors
my $numusers = 0;
while (<$mfork>) {
	++$numusers and print if s/INSERT INTO $usertable/INSERT INTO mwuser/;
}
close $mfork;
if ($numusers < 1) {
	warn qq{No users found, probably a connection error.\n};
	print qq{ERROR: No users found, connection failed, or table "$usertable" does not exist. Dump aborted.\n};
	close $mdump or die qq{Could not close "$MYSQLDUMPFILE": $!\n};
	exit;
}
print "\n-- Users loaded: $numusers\n\n-- Loading rest of the mediawiki schema:\n";

warn qq{Dumping all other tables from the MySQL schema\n} if $verbose;

## Dump the rest of the tables, in chunks based on constraints
## We do not need the user table:
my @dumplist = grep { $_ ne 'user'} @torder;
my @alist;
{
	undef @alist;
	PICKATABLE: {
		my $tname = shift @dumplist;
		## XXX Make this dynamic below
		for my $ver (sort {$b <=> $a } keys %version_tables) {
			redo PICKATABLE if $tname =~ $version_tables{$ver};
		}
		$tname = "${table_prefix}$tname" if length $table_prefix;
		next if $tname !~ /^\w/;
		push @alist, $tname;
		$verbose and warn "  $tname...\n";
		pop @alist and last if index($alist[-1],'---') >= 0;
		redo if @dumplist;
	}

	## Dump everything else
	open my $mfork2, '-|' or exec $MYSQLDUMP, @MYSQLDUMPARGS, $MYSQLDB, @alist;
	print while <$mfork2>;
	close $mfork2;
	warn qq{Finished dumping from MySQL\n} if $verbose;

	redo if @dumplist;
}

warn qq{Writing information to return Postgres database to normal\n} if $verbose;
print qq{ALTER TABLE "${table_prefix}text" RENAME TO pagecontent;\n};
print qq{ALTER TABLE ${table_prefix}recentchanges ALTER rc_ip TYPE cidr USING\n};
print qq{  CASE WHEN rc_ip = '' THEN NULL ELSE rc_ip::cidr END;\n};

## Return tables to their original names if a table prefix was used.
if (length $table_prefix) {
	print qq{\n\n-- Renaming tables by removing table prefix "$table_prefix"\n\n};
	my $maxsize = 18;
	for (@torder) {
		$maxsize = length "$_$table_prefix" if length "$_$table_prefix" > $maxsize;
	}
	for my $t (@torder) {
		next if $t eq '---' or $t eq 'text';
		my $tname = $special{$t}||$t;
		printf qq{ALTER TABLE %*s RENAME TO "$tname"\n}, $maxsize+1, qq{"${table_prefix}$tname"};
	}
}

print qq{\n\n--Returning timestamps to normal\n};
for my $t (sort keys %tz) {
	next if $t eq 'archive2';
	for my $c (sort keys %{$tz{$t}}) {
		printf "ALTER TABLE %-18s ALTER %-25s TYPE timestamptz\n".
				"  USING TO_TIMESTAMP($c,'YYYYMMDDHHMISS');\n", $t, $c;
	}
}

## Reset sequences
print q{
SELECT setval('filearchive_fa_id_seq', 1+coalesce(max(fa_id)  ,0),false) FROM filearchive;
SELECT setval('ipblocks_ipb_id_val',   1+coalesce(max(ipb_id) ,0),false) FROM ipblocks;
SELECT setval('job_job_id_seq',        1+coalesce(max(job_id) ,0),false) FROM job;
SELECT setval('log_log_id_seq',        1+coalesce(max(log_id) ,0),false) FROM logging;
SELECT setval('page_page_id_seq',      1+coalesce(max(page_id),0),false) FROM page;
SELECT setval('pr_id_val',             1+coalesce(max(pr_id)  ,0),false) FROM page_restrictions;
SELECT setval('rc_rc_id_seq',          1+coalesce(max(rc_id)  ,0),false) FROM recentchanges;
SELECT setval('rev_rev_id_val',        1+coalesce(max(rev_id) ,0),false) FROM revision;
SELECT setval('text_old_id_val',       1+coalesce(max(old_id) ,0),false) FROM pagecontent;
SELECT setval('trackbacks_tb_id_seq',  1+coalesce(max(tb_id)  ,0),false) FROM trackbacks;
SELECT setval('user_user_id_seq',      1+coalesce(max(user_id),0),false) FROM mwuser;
};

## Finally, make a record in the mediawiki_version table about this import
print qq{
INSERT INTO mediawiki_version (type,mw_version,notes) VALUES ('MySQL import','??',
'Imported from file created on $now. Old version: $current_version');
};

print "COMMIT;\n\\o\n\n-- End of dump\n\n";
select $oldselect;
close $mdump or die qq{Could not close "$MYSQLDUMPFILE": $!\n};
exit;


__DATA__
## Known remappings: either indicate the MySQL name,
## or leave blank if it should be skipped
pagecontent text
mwuser user
mediawiki_version
archive2
profiling
objectcache

## Which tables to ignore depending on the version
VERSION 1.5: trackback
VERSION 1.6: externallinks job templatelinks transcache
VERSION 1.7: filearchive langlinks querycache_info
VERSION 1.9: querycachetwo page_restrictions redirect

