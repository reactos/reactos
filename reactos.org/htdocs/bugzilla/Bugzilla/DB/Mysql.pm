# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Dave Miller <davem00@aol.com>
#                 Gayathri Swaminath <gayathrik00@aol.com>
#                 Jeroen Ruigrok van der Werven <asmodai@wxs.nl>
#                 Dave Lawrence <dkl@redhat.com>
#                 Tomas Kopal <Tomas.Kopal@altap.cz>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Lance Larsh <lance.larsh@oracle.com>

=head1 NAME

Bugzilla::DB::Mysql - Bugzilla database compatibility layer for MySQL

=head1 DESCRIPTION

This module overrides methods of the Bugzilla::DB module with MySQL specific
implementation. It is instantiated by the Bugzilla::DB module and should never
be used directly.

For interface details see L<Bugzilla::DB> and L<DBI>.

=cut

package Bugzilla::DB::Mysql;

use strict;

use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;

# This module extends the DB interface via inheritance
use base qw(Bugzilla::DB);

sub new {
    my ($class, $user, $pass, $host, $dbname, $port, $sock) = @_;

    # construct the DSN from the parameters we got
    my $dsn = "DBI:mysql:host=$host;database=$dbname";
    $dsn .= ";port=$port" if $port;
    $dsn .= ";mysql_socket=$sock" if $sock;
    
    my $self = $class->db_new($dsn, $user, $pass);

    # This makes sure that if the tables are encoded as UTF-8, we
    # return their data correctly.
    $self->do("SET NAMES utf8") if Bugzilla->params->{'utf8'};

    # all class local variables stored in DBI derived class needs to have
    # a prefix 'private_'. See DBI documentation.
    $self->{private_bz_tables_locked} = "";

    bless ($self, $class);
    
    # Bug 321645 - disable MySQL strict mode, if set
    my ($var, $sql_mode) = $self->selectrow_array(
        "SHOW VARIABLES LIKE 'sql\\_mode'");

    if ($sql_mode) {
        # STRICT_TRANS_TABLE or STRICT_ALL_TABLES enable MySQL strict mode,
        # causing bug 321645. TRADITIONAL sets these modes (among others) as
        # well, so it has to be stipped as well
        my $new_sql_mode =
            join(",", grep {$_ !~ /^STRICT_(?:TRANS|ALL)_TABLES|TRADITIONAL$/}
                            split(/,/, $sql_mode));

        if ($sql_mode ne $new_sql_mode) {
            $self->do("SET SESSION sql_mode = ?", undef, $new_sql_mode);
        }
    }

    return $self;
}

# when last_insert_id() is supported on MySQL by lowest DBI/DBD version
# required by Bugzilla, this implementation can be removed.
sub bz_last_key {
    my ($self) = @_;

    my ($last_insert_id) = $self->selectrow_array('SELECT LAST_INSERT_ID()');

    return $last_insert_id;
}

sub sql_regexp {
    my ($self, $expr, $pattern) = @_;

    return "$expr REGEXP $pattern";
}

sub sql_not_regexp {
    my ($self, $expr, $pattern) = @_;

    return "$expr NOT REGEXP $pattern";
}

sub sql_limit {
    my ($self, $limit, $offset) = @_;

    if (defined($offset)) {
        return "LIMIT $offset, $limit";
    } else {
        return "LIMIT $limit";
    }
}

sub sql_string_concat {
    my ($self, @params) = @_;
    
    return 'CONCAT(' . join(', ', @params) . ')';
}

sub sql_fulltext_search {
    my ($self, $column, $text) = @_;

    # Add the boolean mode modifier if the search string contains
    # boolean operators.
    my $mode = ($text =~ /[+-<>()~*"]/ ? "IN BOOLEAN MODE" : "");

    # quote the text for use in the MATCH AGAINST expression
    $text = $self->quote($text);

    # untaint the text, since it's safe to use now that we've quoted it
    trick_taint($text);

    return "MATCH($column) AGAINST($text $mode)";
}

sub sql_istring {
    my ($self, $string) = @_;
    
    return $string;
}

sub sql_from_days {
    my ($self, $days) = @_;

    return "FROM_DAYS($days)";
}

sub sql_to_days {
    my ($self, $date) = @_;

    return "TO_DAYS($date)";
}

sub sql_date_format {
    my ($self, $date, $format) = @_;

    $format = "%Y.%m.%d %H:%i:%s" if !$format;
    
    return "DATE_FORMAT($date, " . $self->quote($format) . ")";
}

sub sql_interval {
    my ($self, $interval, $units) = @_;
    
    return "INTERVAL $interval $units";
}

sub sql_position {
    my ($self, $fragment, $text) = @_;

    # mysql 4.0.1 and lower do not support CAST
    # (checksetup has a check for unsupported versions)

    my $server_version = $self->bz_server_version;
    return "INSTR(CAST($text AS BINARY), CAST($fragment AS BINARY))";
}

sub sql_group_by {
    my ($self, $needed_columns, $optional_columns) = @_;

    # MySQL allows to specify all columns as ANSI SQL requires, but also
    # allow you to specify just minimal subset to get unique result.
    # According to MySQL documentation, the less columns you specify
    # the faster the query runs.
    return "GROUP BY $needed_columns";
}


sub bz_lock_tables {
    my ($self, @tables) = @_;

    my $list = join(', ', @tables);
    # Check first if there was no lock before
    if ($self->{private_bz_tables_locked}) {
        ThrowCodeError("already_locked", { current => $self->{private_bz_tables_locked},
                                           new => $list });
    } else {
        $self->do('LOCK TABLE ' . $list); 
    
        $self->{private_bz_tables_locked} = $list;
    }
}

sub bz_unlock_tables {
    my ($self, $abort) = @_;
    
    # Check first if there was previous matching lock
    if (!$self->{private_bz_tables_locked}) {
        # Abort is allowed even without previous lock for error handling
        return if $abort;
        ThrowCodeError("no_matching_lock");
    } else {
        $self->do("UNLOCK TABLES");
    
        $self->{private_bz_tables_locked} = "";
    }
}

# As Bugzilla currently runs on MyISAM storage, which does not support
# transactions, these functions die when called.
# Maybe we should just ignore these calls for now, but as we are not
# using transactions in MySQL yet, this just hints the developers.
sub bz_start_transaction {
    die("Attempt to start transaction on DB without transaction support");
}

sub bz_commit_transaction {
    die("Attempt to commit transaction on DB without transaction support");
}

sub bz_rollback_transaction {
    die("Attempt to rollback transaction on DB without transaction support");
}


sub _bz_get_initial_schema {
    my ($self) = @_;
    return $self->_bz_build_schema_from_disk();
}

#####################################################################
# Database Setup
#####################################################################

sub bz_setup_database {
    my ($self) = @_;

    # Figure out if any existing tables are of type ISAM and convert them
    # to type MyISAM if so.  ISAM tables are deprecated in MySQL 3.23,
    # which Bugzilla now requires, and they don't support more than 16
    # indexes per table, which Bugzilla needs.
    my $sth = $self->prepare("SHOW TABLE STATUS");
    $sth->execute;
    my @isam_tables = ();
    while (my ($name, $type) = $sth->fetchrow_array) {
        push(@isam_tables, $name) if $type eq "ISAM";
    }

    if(scalar(@isam_tables)) {
        print "One or more of the tables in your existing MySQL database are\n"
              . "of type ISAM. ISAM tables are deprecated in MySQL 3.23 and\n"
              . "don't support more than 16 indexes per table, which \n"
              . "Bugzilla needs.\n  Converting your ISAM tables to type"
              . " MyISAM:\n\n";
        foreach my $table (@isam_tables) {
            print "Converting table $table... ";
            $self->do("ALTER TABLE $table TYPE = MYISAM");
            print "done.\n";
        }
        print "\nISAM->MyISAM table conversion done.\n\n";
    }

    my @tables = $self->bz_table_list_real();
    $self->_after_table_status(\@tables);

    # Versions of Bugzilla before the existence of Bugzilla::DB::Schema did 
    # not provide explicit names for the table indexes. This means
    # that our upgrades will not be reliable, because we look for the name
    # of the index, not what fields it is on, when doing upgrades.
    # (using the name is much better for cross-database compatibility 
    # and general reliability). It's also very important that our
    # Schema object be consistent with what is on the disk.
    #
    # While we're at it, we also fix some inconsistent index naming
    # from the original checkin of Bugzilla::DB::Schema.

    # We check for the existence of a particular "short name" index that
    # has existed at least since Bugzilla 2.8, and probably earlier.
    # For fixing the inconsistent naming of Schema indexes,
    # we also check for one of those inconsistently-named indexes.
    if (grep($_ eq 'bugs', @tables)
        && ($self->bz_index_info_real('bugs', 'assigned_to')
            || $self->bz_index_info_real('flags', 'flags_bidattid_idx')) )
    {

        # This is a check unrelated to the indexes, to see if people are
        # upgrading from 2.18 or below, but somehow have a bz_schema table
        # already. This only happens if they have done a mysqldump into
        # a database without doing a DROP DATABASE first.
        # We just do the check here since this check is a reliable way
        # of telling that we are upgrading from a version pre-2.20.
        if (grep($_ eq 'bz_schema', $self->bz_table_list_real())) {
            die("\nYou are upgrading from a version before 2.20, but the"
              . " bz_schema\ntable already exists. This means that you"
              . " restored a mysqldump into\nthe Bugzilla database without"
              . " first dropping the already-existing\nBugzilla database,"
              . " at some point. Whenever you restore a Bugzilla\ndatabase"
              . " backup, you must always drop the entire database first.\n\n"
              . "Please drop your Bugzilla database and restore it from a"
              . " backup that\ndoes not contain the bz_schema table. If for"
              . " some reason you cannot\ndo this, you can connect to your"
              . " MySQL database and drop the bz_schema\ntable, as a last"
              . " resort.\n");
        }

        my $bug_count = $self->selectrow_array("SELECT COUNT(*) FROM bugs");
        # We estimate one minute for each 3000 bugs, plus 3 minutes just
        # to handle basic MySQL stuff.
        my $rename_time = int($bug_count / 3000) + 3;
        # And 45 minutes for every 15,000 attachments, per some experiments.
        my ($attachment_count) = 
            $self->selectrow_array("SELECT COUNT(*) FROM attachments");
        $rename_time += int(($attachment_count * 45) / 15000);
        # If we're going to take longer than 5 minutes, we let the user know
        # and allow them to abort.
        if ($rename_time > 5) {
            print "\nWe are about to rename old indexes.\n"
                  . "The estimated time to complete renaming is "
                  . "$rename_time minutes.\n"
                  . "You cannot interrupt this action once it has begun.\n"
                  . "If you would like to cancel, press Ctrl-C now..."
                  . " (Waiting 45 seconds...)\n\n";
            # Wait 45 seconds for them to respond.
            sleep(45) unless Bugzilla->installation_answers->{NO_PAUSE};
        }
        print "Renaming indexes...\n";

        # We can't be interrupted, because of how the "if"
        # works above.
        local $SIG{INT}  = 'IGNORE';
        local $SIG{TERM} = 'IGNORE';
        local $SIG{PIPE} = 'IGNORE';

        # Certain indexes had names in Schema that did not easily conform
        # to a standard. We store those names here, so that they
        # can be properly renamed.
        # Also, sometimes an old mysqldump would incorrectly rename
        # unique indexes to "PRIMARY", so we address that here, also.
        my $bad_names = {
            # 'when' is a possible leftover from Bugzillas before 2.8
            bugs_activity => ['when', 'bugs_activity_bugid_idx',
                'bugs_activity_bugwhen_idx'],
            cc => ['PRIMARY'],
            longdescs => ['longdescs_bugid_idx',
               'longdescs_bugwhen_idx'],
            flags => ['flags_bidattid_idx'],
            flaginclusions => ['flaginclusions_tpcid_idx'],
            flagexclusions => ['flagexclusions_tpc_id_idx'],
            keywords => ['PRIMARY'],
            milestones => ['PRIMARY'],
            profiles_activity => ['profiles_activity_when_idx'],
            group_control_map => ['group_control_map_gid_idx', 'PRIMARY'],
            user_group_map => ['PRIMARY'],
            group_group_map => ['PRIMARY'],
            email_setting => ['PRIMARY'],
            bug_group_map => ['PRIMARY'],
            category_group_map => ['PRIMARY'],
            watch => ['PRIMARY'],
            namedqueries => ['PRIMARY'],
            series_data => ['PRIMARY'],
            # series_categories is dealt with below, not here.
        };

        # The series table is broken and needs to have one index
        # dropped before we begin the renaming, because it had a
        # useless index on it that would cause a naming conflict here.
        if (grep($_ eq 'series', @tables)) {
            my $dropname;
            # This is what the bad index was called before Schema.
            if ($self->bz_index_info_real('series', 'creator_2')) {
                $dropname = 'creator_2';
            }
            # This is what the bad index is called in Schema.
            elsif ($self->bz_index_info_real('series', 'series_creator_idx')) {
                    $dropname = 'series_creator_idx';
            }
            $self->bz_drop_index_raw('series', $dropname) if $dropname;
        }

        # The email_setting table also had the same problem.
        if( grep($_ eq 'email_setting', @tables) 
            && $self->bz_index_info_real('email_setting', 
                                         'email_settings_user_id_idx') ) 
        {
            $self->bz_drop_index_raw('email_setting', 
                                     'email_settings_user_id_idx');
        }

        # Go through all the tables.
        foreach my $table (@tables) {
            # Will contain the names of old indexes as keys, and the 
            # definition of the new indexes as a value. The values
            # include an extra hash key, NAME, with the new name of 
            # the index.
            my %rename_indexes;
            # And go through all the columns on each table.
            my @columns = $self->bz_table_columns_real($table);

            # We also want to fix the silly naming of unique indexes
            # that happened when we first checked-in Bugzilla::DB::Schema.
            if ($table eq 'series_categories') {
                # The series_categories index had a nonstandard name.
                push(@columns, 'series_cats_unique_idx');
            } 
            elsif ($table eq 'email_setting') { 
                # The email_setting table had a similar problem.
                push(@columns, 'email_settings_unique_idx');
            }
            else {
                push(@columns, "${table}_unique_idx");
            }
            # And this is how we fix the other inconsistent Schema naming.
            push(@columns, @{$bad_names->{$table}})
                if (exists $bad_names->{$table});
            foreach my $column (@columns) {
                # If we have an index named after this column, it's an 
                # old-style-name index.
                if (my $index = $self->bz_index_info_real($table, $column)) {
                    # Fix the name to fit in with the new naming scheme.
                    $index->{NAME} = $table . "_" .
                                     $index->{FIELDS}->[0] . "_idx";
                    print "Renaming index $column to " 
                          . $index->{NAME} . "...\n";
                    $rename_indexes{$column} = $index;
                } # if
            } # foreach column

            my @rename_sql = $self->_bz_schema->get_rename_indexes_ddl(
                $table, %rename_indexes);
            $self->do($_) foreach (@rename_sql);

        } # foreach table
    } # if old-name indexes

    # If there are no tables, but the DB isn't utf8 and it should be,
    # then we should alter the database to be utf8. We know it should be
    # if the utf8 parameter is true or there are no params at all.
    # This kind of situation happens when people create the database
    # themselves, and if we don't do this they will get the big
    # scary WARNING statement about conversion to UTF8.
    if ( !$self->bz_db_is_utf8 && !@tables 
         && (Bugzilla->params->{'utf8'} || !scalar keys %{Bugzilla->params}) )
    {
        $self->_alter_db_charset_to_utf8();
    }

    # And now we create the tables and the Schema object.
    $self->SUPER::bz_setup_database();


    # The old timestamp fields need to be adjusted here instead of in
    # checksetup. Otherwise the UPDATE statements inside of bz_add_column
    # will cause accidental timestamp updates.
    # The code that does this was moved here from checksetup.

    # 2002-08-14 - bbaetz@student.usyd.edu.au - bug 153578
    # attachments creation time needs to be a datetime, not a timestamp
    my $attach_creation = 
        $self->bz_column_info("attachments", "creation_ts");
    if ($attach_creation && $attach_creation->{TYPE} =~ /^TIMESTAMP/i) {
        print "Fixing creation time on attachments...\n";

        my $sth = $self->prepare("SELECT COUNT(attach_id) FROM attachments");
        $sth->execute();
        my ($attach_count) = $sth->fetchrow_array();

        if ($attach_count > 1000) {
            print "This may take a while...\n";
        }
        my $i = 0;

        # This isn't just as simple as changing the field type, because
        # the creation_ts was previously updated when an attachment was made
        # obsolete from the attachment creation screen. So we have to go
        # and recreate these times from the comments..
        $sth = $self->prepare("SELECT bug_id, attach_id, submitter_id " .
                               "FROM attachments");
        $sth->execute();

        # Restrict this as much as possible in order to avoid false 
        # positives, and keep the db search time down
        my $sth2 = $self->prepare("SELECT bug_when FROM longdescs 
                                    WHERE bug_id=? AND who=? 
                                          AND thetext LIKE ?
                                 ORDER BY bug_when " . $self->sql_limit(1));
        while (my ($bug_id, $attach_id, $submitter_id) 
                  = $sth->fetchrow_array()) 
        {
            $sth2->execute($bug_id, $submitter_id, 
                "Created an attachment (id=$attach_id)%");
            my ($when) = $sth2->fetchrow_array();
            if ($when) {
                $self->do("UPDATE attachments " .
                             "SET creation_ts='$when' " .
                           "WHERE attach_id=$attach_id");
            } else {
                print "Warning - could not determine correct creation"
                      . " time for attachment $attach_id on bug $bug_id\n";
            }
            ++$i;
            print "Converted $i of $attach_count attachments\n" if !($i % 1000);
        }
        print "Done - converted $i attachments\n";

        $self->bz_alter_column("attachments", "creation_ts", 
                               {TYPE => 'DATETIME', NOTNULL => 1});
    }

    # 2004-08-29 - Tomas.Kopal@altap.cz, bug 257303
    # Change logincookies.lastused type from timestamp to datetime
    my $login_lastused = $self->bz_column_info("logincookies", "lastused");
    if ($login_lastused && $login_lastused->{TYPE} =~ /^TIMESTAMP/i) {
        $self->bz_alter_column('logincookies', 'lastused', 
                               { TYPE => 'DATETIME',  NOTNULL => 1});
    }

    # 2005-01-17 - Tomas.Kopal@altap.cz, bug 257315
    # Change bugs.delta_ts type from timestamp to datetime 
    my $bugs_deltats = $self->bz_column_info("bugs", "delta_ts");
    if ($bugs_deltats && $bugs_deltats->{TYPE} =~ /^TIMESTAMP/i) {
        $self->bz_alter_column('bugs', 'delta_ts', 
                               {TYPE => 'DATETIME', NOTNULL => 1});
    }

    # 2005-09-24 - bugreport@peshkin.net, bug 307602
    # Make sure that default 4G table limit is overridden
    my $row = $self->selectrow_hashref("SHOW TABLE STATUS LIKE 'attach_data'");
    if ($$row{'Create_options'} !~ /MAX_ROWS/i) {
        print "Converting attach_data maximum size to 100G...\n";
        $self->do("ALTER TABLE attach_data
                   AVG_ROW_LENGTH=1000000,
                   MAX_ROWS=100000");
    }

    # Convert the database to UTF-8 if the utf8 parameter is on.
    # We check if any table isn't utf8, because lots of crazy
    # partial-conversion situations can happen, and this handles anything
    # that could come up (including having the DB charset be utf8 but not
    # the table charsets.
    my $utf_table_status =
        $self->selectall_arrayref("SHOW TABLE STATUS", {Slice=>{}});
    $self->_after_table_status([map($_->{Name}, @$utf_table_status)]);
    my @non_utf8_tables = grep($_->{Collation} !~ /^utf8/, @$utf_table_status);
    
    if (Bugzilla->params->{'utf8'} && scalar @non_utf8_tables) {
        print <<EOT;

WARNING: We are about to convert your table storage format to UTF8. This
         allows Bugzilla to correctly store and sort international characters.
         However, if you have any non-UTF-8 data in your database,
         it ***WILL BE DELETED*** by this process. So, before
         you continue with checksetup.pl, if you have any non-UTF-8
         data (or even if you're not sure) you should press Ctrl-C now
         to interrupt checksetup.pl, and run contrib/recode.pl to make all 
         the data in your database into UTF-8. You should also back up your
         database before continuing. This will affect every single table
         in the database, even non-Bugzilla tables.

         If you ever used a version of Bugzilla before 2.22, we STRONGLY
         recommend that you stop checksetup.pl NOW and run contrib/recode.pl.

EOT

        if (!Bugzilla->installation_answers->{NO_PAUSE}) {
            if (Bugzilla->installation_mode == 
                INSTALLATION_MODE_NON_INTERACTIVE) 
            {
                print <<EOT;
         Re-run checksetup.pl in interactive mode (without an 'answers' file)
         to continue.
EOT
                exit;
            }
            else {
                print "         Press Enter to continue or Ctrl-C to exit...";
                getc;
            }
        }

        print "Converting table storage format to UTF-8. This may take a",
              " while.\n";
        foreach my $table ($self->bz_table_list_real) {
            my $info_sth = $self->prepare("SHOW FULL COLUMNS FROM $table");
            $info_sth->execute();
            while (my $column = $info_sth->fetchrow_hashref) {
                # Our conversion code doesn't work on enum fields, but they
                # all go away later in checksetup anyway.
                next if $column->{Type} =~ /enum/i;

                # If this particular column isn't stored in utf-8
                if ($column->{Collation}
                    && $column->{Collation} ne 'NULL' 
                    && $column->{Collation} !~ /utf8/) 
                {
                    my $name = $column->{Field};

                    # The code below doesn't work on a field with a FULLTEXT
                    # index. So we drop it. The upgrade code will re-create
                    # it later.
                    if ($table eq 'longdescs' && $name eq 'thetext') {
                        $self->bz_drop_index('longdescs', 
                                             'longdescs_thetext_idx');
                    }
                    if ($table eq 'bugs' && $name eq 'short_desc') {
                        $self->bz_drop_index('bugs', 'bugs_short_desc_idx');
                    }

                    print "Converting $table.$name to be stored as UTF-8...\n";
                    my $col_info = 
                        $self->bz_column_info_real($table, $name);

                    # CHANGE COLUMN doesn't take PRIMARY KEY
                    delete $col_info->{PRIMARYKEY};

                    my $sql_def = $self->_bz_schema->get_type_ddl($col_info);
                    # We don't want MySQL to actually try to *convert*
                    # from our current charset to UTF-8, we just want to
                    # transfer the bytes directly. This is how we do that.

                    # The CHARACTER SET part of the definition has to come
                    # right after the type, which will always come first.
                    my ($binary, $utf8) = ($sql_def, $sql_def);
                    my $type = $self->_bz_schema->convert_type($col_info->{TYPE});
                    $binary =~ s/(\Q$type\E)/$1 CHARACTER SET binary/;
                    $utf8   =~ s/(\Q$type\E)/$1 CHARACTER SET utf8/;
                    $self->do("ALTER TABLE $table CHANGE COLUMN $name $name 
                              $binary");
                    $self->do("ALTER TABLE $table CHANGE COLUMN $name $name 
                              $utf8");
                }
            }

            $self->do("ALTER TABLE $table DEFAULT CHARACTER SET utf8");
        } # foreach my $table (@tables)
    }

    # Sometimes you can have a situation where all the tables are utf8,
    # but the database isn't. (This tends to happen when you've done
    # a mysqldump.) So we have this change outside of the above block,
    # so that it just happens silently if no actual *table* conversion
    # needs to happen.
    if (Bugzilla->params->{'utf8'} && !$self->bz_db_is_utf8) {
        $self->_alter_db_charset_to_utf8();
    }
}

# There is a bug in MySQL 4.1.0 - 4.1.15 that makes certain SELECT
# statements fail after a SHOW TABLE STATUS: 
# http://bugs.mysql.com/bug.php?id=13535
# This is a workaround, a dummy SELECT to reset the LAST_INSERT_ID.
sub _after_table_status {
    my ($self, $tables) = @_;
    if (grep($_ eq 'bugs', @$tables)
        && $self->bz_column_info_real("bugs", "bug_id"))
    {
        $self->do('SELECT 1 FROM bugs WHERE bug_id IS NULL');
    }
}

sub _alter_db_charset_to_utf8 {
    my $self = shift;
    my $db_name = Bugzilla->localconfig->{db_name};
    $self->do("ALTER DATABASE $db_name CHARACTER SET utf8"); 
}

sub bz_db_is_utf8 {
    my $self = shift;
    my $db_collation = $self->selectrow_arrayref(
        "SHOW VARIABLES LIKE 'character_set_database'");
    # First column holds the variable name, second column holds the value.
    return $db_collation->[1] =~ /utf8/ ? 1 : 0;
}


sub bz_enum_initial_values {
    my ($self) = @_;
    my %enum_values = %{$self->ENUM_DEFAULTS};
    # Get a complete description of the 'bugs' table; with DBD::MySQL
    # there isn't a column-by-column way of doing this.  Could use
    # $dbh->column_info, but it would go slower and we would have to
    # use the undocumented mysql_type_name accessor to get the type
    # of each row.
    my $sth = $self->prepare("DESCRIBE bugs");
    $sth->execute();
    # Look for the particular columns we are interested in.
    while (my ($thiscol, $thistype) = $sth->fetchrow_array()) {
        if (defined $enum_values{$thiscol}) {
            # this is a column of interest.
            my @value_list;
            if ($thistype and ($thistype =~ /^enum\(/)) {
                # it has an enum type; get the set of values.
                while ($thistype =~ /'([^']*)'(.*)/) {
                    push(@value_list, $1);
                    $thistype = $2;
                }
            }
            if (@value_list) {
                # record the enum values found.
                $enum_values{$thiscol} = \@value_list;
            }
        }
    }

    return \%enum_values;
}

#####################################################################
# MySQL-specific Database-Reading Methods
#####################################################################

=begin private

=head1 MYSQL-SPECIFIC DATABASE-READING METHODS

These methods read information about the database from the disk,
instead of from a Schema object. They are only reliable for MySQL 
(see bug 285111 for the reasons why not all DBs use/have functions
like this), but that's OK because we only need them for 
backwards-compatibility anyway, for versions of Bugzilla before 2.20.

=over 4

=item C<bz_column_info_real($table, $column)>

 Description: Returns an abstract column definition for a column
              as it actually exists on disk in the database.
 Params:      $table - The name of the table the column is on.
              $column - The name of the column you want info about.
 Returns:     An abstract column definition.
              If the column does not exist, returns undef.

=cut

sub bz_column_info_real {
    my ($self, $table, $column) = @_;

    # DBD::mysql does not support selecting a specific column,
    # so we have to get all the columns on the table and find 
    # the one we want.
    my $info_sth = $self->column_info(undef, undef, $table, '%');

    # Don't use fetchall_hashref as there's a Win32 DBI bug (292821)
    my $col_data;
    while ($col_data = $info_sth->fetchrow_hashref) {
        last if $col_data->{'COLUMN_NAME'} eq $column;
    }

    if (!defined $col_data) {
        return undef;
    }
    return $self->_bz_schema->column_info_to_column($col_data);
}

=item C<bz_index_info_real($table, $index)>

 Description: Returns information about an index on a table in the database.
 Params:      $table = name of table containing the index
              $index = name of an index
 Returns:     An abstract index definition, always in hashref format.
              If the index does not exist, the function returns undef.
=cut
sub bz_index_info_real {
    my ($self, $table, $index) = @_;

    my $sth = $self->prepare("SHOW INDEX FROM $table");
    $sth->execute;

    my @fields;
    my $index_type;
    # $raw_def will be an arrayref containing the following information:
    # 0 = name of the table that the index is on
    # 1 = 0 if unique, 1 if not unique
    # 2 = name of the index
    # 3 = seq_in_index (The order of the current field in the index).
    # 4 = Name of ONE column that the index is on
    # 5 = 'Collation' of the index. Usually 'A'.
    # 6 = Cardinality. Either a number or undef.
    # 7 = sub_part. Usually undef. Sometimes 1.
    # 8 = "packed". Usually undef.
    # 9 = Null. Sometimes undef, sometimes 'YES'.
    # 10 = Index_type. The type of the index. Usually either 'BTREE' or 'FULLTEXT'
    # 11 = 'Comment.' Usually undef.
    while (my $raw_def = $sth->fetchrow_arrayref) {
        if ($raw_def->[2] eq $index) {
            push(@fields, $raw_def->[4]);
            # No index can be both UNIQUE and FULLTEXT, that's why
            # this is written this way.
            $index_type = $raw_def->[1] ? '' : 'UNIQUE';
            $index_type = $raw_def->[10] eq 'FULLTEXT'
                ? 'FULLTEXT' : $index_type;
        }
    }

    my $retval;
    if (scalar(@fields)) {
        $retval = {FIELDS => \@fields, TYPE => $index_type};
    }
    return $retval;
}

=item C<bz_index_list_real($table)>

 Description: Returns a list of index names on a table in 
              the database, as it actually exists on disk.
 Params:      $table - The name of the table you want info about.
 Returns:     An array of index names.

=cut

sub bz_index_list_real {
    my ($self, $table) = @_;
    my $sth = $self->prepare("SHOW INDEX FROM $table");
    # Column 3 of a SHOW INDEX statement contains the name of the index.
    return @{ $self->selectcol_arrayref($sth, {Columns => [3]}) };
}

#####################################################################
# MySQL-Specific "Schema Builder"
#####################################################################

=back

=head1 MYSQL-SPECIFIC "SCHEMA BUILDER"

MySQL needs to be able to read in a legacy database (from before 
Schema existed) and create a Schema object out of it. That's what
this code does.

=end private

=cut

# This sub itself is actually written generically, but the subroutines
# that it depends on are database-specific. In particular, the
# bz_column_info_real function would be very difficult to create
# properly for any other DB besides MySQL.
sub _bz_build_schema_from_disk {
    my ($self) = @_;

    print "Building Schema object from database...\n";

    my $schema = $self->_bz_schema->get_empty_schema();

    my @tables = $self->bz_table_list_real();
    foreach my $table (@tables) {
        $schema->add_table($table);
        my @columns = $self->bz_table_columns_real($table);
        foreach my $column (@columns) {
            my $type_info = $self->bz_column_info_real($table, $column);
            $schema->set_column($table, $column, $type_info);
        }

        my @indexes = $self->bz_index_list_real($table);
        foreach my $index (@indexes) {
            unless ($index eq 'PRIMARY') {
                my $index_info = $self->bz_index_info_real($table, $index);
                ($index_info = $index_info->{FIELDS}) 
                    if (!$index_info->{TYPE});
                $schema->set_index($table, $index, $index_info);
            }
        }
    }

    return $schema;
}
1;
