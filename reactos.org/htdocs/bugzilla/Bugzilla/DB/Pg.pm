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

Bugzilla::DB::Pg - Bugzilla database compatibility layer for PostgreSQL

=head1 DESCRIPTION

This module overrides methods of the Bugzilla::DB module with PostgreSQL
specific implementation. It is instantiated by the Bugzilla::DB module
and should never be used directly.

For interface details see L<Bugzilla::DB> and L<DBI>.

=cut

package Bugzilla::DB::Pg;

use strict;

use Bugzilla::Error;
use DBD::Pg;

# This module extends the DB interface via inheritance
use base qw(Bugzilla::DB);

use constant BLOB_TYPE => { pg_type => DBD::Pg::PG_BYTEA };

sub new {
    my ($class, $user, $pass, $host, $dbname, $port) = @_;

    # The default database name for PostgreSQL. We have
    # to connect to SOME database, even if we have
    # no $dbname parameter.
    $dbname ||= 'template1';

    # construct the DSN from the parameters we got
    my $dsn = "DBI:Pg:dbname=$dbname";
    $dsn .= ";host=$host" if $host;
    $dsn .= ";port=$port" if $port;

    # This stops Pg from printing out lots of "NOTICE" messages when
    # creating tables.
    $dsn .= ";options='-c client_min_messages=warning'";

    my $self = $class->db_new($dsn, $user, $pass);

    # all class local variables stored in DBI derived class needs to have
    # a prefix 'private_'. See DBI documentation.
    $self->{private_bz_tables_locked} = "";

    bless ($self, $class);

    return $self;
}

# if last_insert_id is supported on PostgreSQL by lowest DBI/DBD version
# supported by Bugzilla, this implementation can be removed.
sub bz_last_key {
    my ($self, $table, $column) = @_;

    my $seq = $table . "_" . $column . "_seq";
    my ($last_insert_id) = $self->selectrow_array("SELECT CURRVAL('$seq')");

    return $last_insert_id;
}

sub sql_regexp {
    my ($self, $expr, $pattern) = @_;

    return "$expr ~* $pattern";
}

sub sql_not_regexp {
    my ($self, $expr, $pattern) = @_;

    return "$expr !~* $pattern" 
}

sub sql_limit {
    my ($self, $limit, $offset) = @_;

    if (defined($offset)) {
        return "LIMIT $limit OFFSET $offset";
    } else {
        return "LIMIT $limit";
    }
}

sub sql_from_days {
    my ($self, $days) = @_;

    return "TO_TIMESTAMP(${days}::int, 'J')::date";
}

sub sql_to_days {
    my ($self, $date) = @_;

    return "TO_CHAR(${date}::date, 'J')::int";
}

sub sql_date_format {
    my ($self, $date, $format) = @_;
    
    $format = "%Y.%m.%d %H:%i:%s" if !$format;

    $format =~ s/\%Y/YYYY/g;
    $format =~ s/\%y/YY/g;
    $format =~ s/\%m/MM/g;
    $format =~ s/\%d/DD/g;
    $format =~ s/\%a/Dy/g;
    $format =~ s/\%H/HH24/g;
    $format =~ s/\%i/MI/g;
    $format =~ s/\%s/SS/g;

    return "TO_CHAR($date, " . $self->quote($format) . ")";
}

sub sql_interval {
    my ($self, $interval, $units) = @_;
    
    return "$interval * INTERVAL '1 $units'";
}

sub sql_string_concat {
    my ($self, @params) = @_;
    
    # Postgres 7.3 does not support concatenating of different types, so we
    # need to cast both parameters to text. Version 7.4 seems to handle this
    # properly, so when we stop support 7.3, this can be removed.
    return '(CAST(' . join(' AS text) || CAST(', @params) . ' AS text))';
}

sub bz_lock_tables {
    my ($self, @tables) = @_;
   
    my $list = join(', ', @tables);
    # Check first if there was no lock before
    if ($self->{private_bz_tables_locked}) {
        ThrowCodeError("already_locked", { current => $self->{private_bz_tables_locked},
                                           new => $list });
    } else {
        my %read_tables;
        my %write_tables;
        foreach my $table (@tables) {
            $table =~ /^([\d\w]+)([\s]+AS[\s]+[\d\w]+)?[\s]+(WRITE|READ)$/i;
            my $table_name = $1;
            if ($3 =~ /READ/i) {
                if (!exists $read_tables{$table_name}) {
                    $read_tables{$table_name} = undef;
                }
            }
            else {
                if (!exists $write_tables{$table_name}) {
                    $write_tables{$table_name} = undef;
                }
            }
        }
    
        # Begin Transaction
        $self->bz_start_transaction();
        
        Bugzilla->dbh->do('LOCK TABLE ' . join(', ', keys %read_tables) .
                          ' IN ROW SHARE MODE') if keys %read_tables;
        Bugzilla->dbh->do('LOCK TABLE ' . join(', ', keys %write_tables) .
                          ' IN ROW EXCLUSIVE MODE') if keys %write_tables;
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
        $self->{private_bz_tables_locked} = "";
        # End transaction, tables will be unlocked automatically
        if ($abort) {
            $self->bz_rollback_transaction();
        } else {
            $self->bz_commit_transaction();
        }
    }
}

# Tell us whether or not a particular sequence exists in the DB.
sub bz_sequence_exists {
    my ($self, $seq_name) = @_;
    my $exists = $self->selectrow_array(
        'SELECT 1 FROM pg_statio_user_sequences WHERE relname = ?',
        undef, $seq_name);
    return $exists || 0;
}

#####################################################################
# Custom Database Setup
#####################################################################

sub bz_setup_database {
    my $self = shift;
    $self->SUPER::bz_setup_database(@_);

    # PostgreSQL doesn't like having *any* index on the thetext
    # field, because it can't have index data longer than 2770
    # characters on that field.
    $self->bz_drop_index('longdescs', 'longdescs_thetext_idx');

    # PostgreSQL also wants an index for calling LOWER on
    # login_name, which we do with sql_istrcmp all over the place.
    $self->bz_add_index('profiles', 'profiles_login_name_lower_idx', 
        {FIELDS => ['LOWER(login_name)'], TYPE => 'UNIQUE'});

    # Now that Bugzilla::Object uses sql_istrcmp, other tables
    # also need a LOWER() index.
    _fix_case_differences('fielddefs', 'name');
    $self->bz_add_index('fielddefs', 'fielddefs_name_lower_idx',
        {FIELDS => ['LOWER(name)'], TYPE => 'UNIQUE'});
    _fix_case_differences('keyworddefs', 'name');
    $self->bz_add_index('keyworddefs', 'keyworddefs_name_lower_idx',
        {FIELDS => ['LOWER(name)'], TYPE => 'UNIQUE'});
    _fix_case_differences('products', 'name');
    $self->bz_add_index('products', 'products_name_lower_idx',
        {FIELDS => ['LOWER(name)'], TYPE => 'UNIQUE'});

    # bz_rename_column didn't correctly rename the sequence.
    if ($self->bz_column_info('fielddefs', 'id')
        && $self->bz_sequence_exists('fielddefs_fieldid_seq')) 
    {
        print "Fixing fielddefs_fieldid_seq sequence...\n";
        $self->do("ALTER TABLE fielddefs_fieldid_seq RENAME TO fielddefs_id_seq");
        $self->do("ALTER TABLE fielddefs ALTER COLUMN id
                    SET DEFAULT NEXTVAL('fielddefs_id_seq')");
    }
}

# Renames things that differ only in case.
sub _fix_case_differences {
    my ($table, $field) = @_;
    my $dbh = Bugzilla->dbh;

    my $duplicates = $dbh->selectcol_arrayref(
          "SELECT DISTINCT LOWER($field) FROM $table 
        GROUP BY LOWER($field) HAVING COUNT(LOWER($field)) > 1");

    foreach my $name (@$duplicates) {
        my $dups = $dbh->selectcol_arrayref(
            "SELECT $field FROM $table WHERE LOWER($field) = ?",
            undef, $name);
        my $primary = shift @$dups;
        foreach my $dup (@$dups) {
            my $new_name = "${dup}_";
            # Make sure the new name isn't *also* a duplicate.
            while (1) {
                last if (!$dbh->selectrow_array(
                             "SELECT 1 FROM $table WHERE LOWER($field) = ?",
                              undef, lc($new_name)));
                $new_name .= "_";
            }
            print "$table '$primary' and '$dup' have names that differ",
                  " only in case.\nRenaming '$dup' to '$new_name'...\n";
            $dbh->do("UPDATE $table SET $field = ? WHERE $field = ?",
                     undef, $new_name, $dup);
        }
    }
}

#####################################################################
# Custom Schema Information Functions
#####################################################################

# Pg includes the PostgreSQL system tables in table_list_real, so 
# we need to remove those.
sub bz_table_list_real {
    my $self = shift;

    my @full_table_list = $self->SUPER::bz_table_list_real(@_);
    # All PostgreSQL system tables start with "pg_" or "sql_"
    my @table_list = grep(!/(^pg_)|(^sql_)/, @full_table_list);
    return @table_list;
}

1;
