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
# Contributor(s): Andrew Dunstan <andrew@dunslane.net>,
#                 Edward J. Sabol <edwardjsabol@iname.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::DB::Schema::Mysql;

###############################################################################
#
# DB::Schema implementation for MySQL
#
###############################################################################

use strict;
use Bugzilla::Error;

use base qw(Bugzilla::DB::Schema);

# This is for column_info_to_column, to know when a tinyint is a 
# boolean and when it's really a tinyint. This only has to be accurate
# up to and through 2.19.3, because that's the only time we need
# column_info_to_column.
#
# This is basically a hash of tables/columns, with one entry for each column
# that should be interpreted as a BOOLEAN instead of as an INT1 when
# reading in the Schema from the disk. The values are discarded; I just
# used "1" for simplicity.
use constant BOOLEAN_MAP => {
    bugs           => {everconfirmed => 1, reporter_accessible => 1,
                       cclist_accessible => 1, qacontact_accessible => 1,
                       assignee_accessible => 1},
    longdescs      => {isprivate => 1, already_wrapped => 1},
    attachments    => {ispatch => 1, isobsolete => 1, isprivate => 1},
    flags          => {is_active => 1},
    flagtypes      => {is_active => 1, is_requestable => 1, 
                       is_requesteeble => 1, is_multiplicable => 1},
    fielddefs      => {mailhead => 1, obsolete => 1},
    bug_status     => {isactive => 1},
    resolution     => {isactive => 1},
    bug_severity   => {isactive => 1},
    priority       => {isactive => 1},
    rep_platform   => {isactive => 1},
    op_sys         => {isactive => 1},
    profiles       => {mybugslink => 1, newemailtech => 1},
    namedqueries   => {linkinfooter => 1, watchfordiffs => 1},
    groups         => {isbuggroup => 1, isactive => 1},
    group_control_map => {entry => 1, membercontrol => 1, othercontrol => 1,
                          canedit => 1},
    group_group_map => {isbless => 1},
    user_group_map => {isbless => 1, isderived => 1},
    products       => {disallownew => 1},
    series         => {public => 1},
    whine_queries  => {onemailperbug => 1},
    quips          => {approved => 1},
    setting        => {is_enabled => 1}
};

# Maps the db_specific hash backwards, for use in column_info_to_column.
use constant REVERSE_MAPPING => {
    # Boolean and the SERIAL fields are handled in column_info_to_column,
    # and so don't have an entry here.
    TINYINT   => 'INT1',
    SMALLINT  => 'INT2',
    MEDIUMINT => 'INT3',
    INTEGER   => 'INT4',
    # All the other types have the same name in their abstract version
    # as in their db-specific version, so no reverse mapping is needed.
};

#------------------------------------------------------------------------------
sub _initialize {

    my $self = shift;

    $self = $self->SUPER::_initialize(@_);

    $self->{db_specific} = {

        BOOLEAN =>      'tinyint',
        FALSE =>        '0', 
        TRUE =>         '1',

        INT1 =>         'tinyint',
        INT2 =>         'smallint',
        INT3 =>         'mediumint',
        INT4 =>         'integer',

        SMALLSERIAL =>  'smallint auto_increment',
        MEDIUMSERIAL => 'mediumint auto_increment',
        INTSERIAL =>    'integer auto_increment',

        TINYTEXT =>     'tinytext',
        MEDIUMTEXT =>   'mediumtext',
        TEXT =>         'text',

        LONGBLOB =>     'longblob',

        DATETIME =>     'datetime',

    };

    $self->_adjust_schema;

    return $self;

} #eosub--_initialize
#------------------------------------------------------------------------------
sub _get_create_table_ddl {
    # Extend superclass method to specify the MYISAM storage engine.
    # Returns a "create table" SQL statement.

    my($self, $table) = @_;

    my $charset = Bugzilla->dbh->bz_db_is_utf8 ? "CHARACTER SET utf8" : '';
    return($self->SUPER::_get_create_table_ddl($table) 
           . " ENGINE = MYISAM $charset");

} #eosub--_get_create_table_ddl
#------------------------------------------------------------------------------
sub _get_create_index_ddl {
    # Extend superclass method to create FULLTEXT indexes on text fields.
    # Returns a "create index" SQL statement.

    my($self, $table_name, $index_name, $index_fields, $index_type) = @_;

    my $sql = "CREATE ";
    $sql .= "$index_type " if ($index_type eq 'UNIQUE'
                               || $index_type eq 'FULLTEXT');
    $sql .= "INDEX \`$index_name\` ON $table_name \(" .
      join(", ", @$index_fields) . "\)";

    return($sql);

} #eosub--_get_create_index_ddl
#--------------------------------------------------------------------

sub get_create_database_sql {
    my ($self, $name) = @_;
    # We only create as utf8 if we have no params (meaning we're doing
    # a new installation) or if the utf8 param is on.
    my $create_utf8 = Bugzilla->params->{'utf8'} 
                      || !defined Bugzilla->params->{'utf8'};
    my $charset = $create_utf8 ? "CHARACTER SET utf8" : '';
    return ("CREATE DATABASE $name $charset");
}

# MySQL has a simpler ALTER TABLE syntax than ANSI.
sub get_alter_column_ddl {
    my ($self, $table, $column, $new_def, $set_nulls_to) = @_;
    my $old_def = $self->get_column($table, $column);
    my %new_def_copy = %$new_def;
    if ($old_def->{PRIMARYKEY} && $new_def->{PRIMARYKEY}) {
        # If a column stays a primary key do NOT specify PRIMARY KEY in the
        # ALTER TABLE statement. This avoids a MySQL error that two primary
        # keys are not allowed.
        delete $new_def_copy{PRIMARYKEY};
    }

    my $new_ddl = $self->get_type_ddl(\%new_def_copy);
    my @statements;
    push(@statements, "UPDATE $table SET $column = $set_nulls_to
                        WHERE $column IS NULL") if defined $set_nulls_to;
    push(@statements, "ALTER TABLE $table CHANGE COLUMN 
                       $column $column $new_ddl");
    if ($old_def->{PRIMARYKEY} && !$new_def->{PRIMARYKEY}) {
        # Dropping a PRIMARY KEY needs an explicit DROP PRIMARY KEY
        push(@statements, "ALTER TABLE $table DROP PRIMARY KEY");
    }
    return @statements;
}

sub get_drop_index_ddl {
    my ($self, $table, $name) = @_;
    return ("DROP INDEX \`$name\` ON $table");
}

# A special function for MySQL, for renaming a lot of indexes.
# Index renames is a hash, where the key is a string - the 
# old names of the index, and the value is a hash - the index
# definition that we're renaming to, with an extra key of "NAME"
# that contains the new index name.
# The indexes in %indexes must be in hashref format.
sub get_rename_indexes_ddl {
    my ($self, $table, %indexes) = @_;
    my @keys = keys %indexes or return ();

    my $sql = "ALTER TABLE $table ";

    foreach my $old_name (@keys) {
        my $name = $indexes{$old_name}->{NAME};
        my $type = $indexes{$old_name}->{TYPE};
        $type ||= 'INDEX';
        my $fields = join(',', @{$indexes{$old_name}->{FIELDS}});
        # $old_name needs to be escaped, sometimes, because it was
        # a reserved word.
        $old_name = '`' . $old_name . '`';
        $sql .= " ADD $type $name ($fields), DROP INDEX $old_name,";
    }
    # Remove the last comma.
    chop($sql);
    return ($sql);
}

# Converts a DBI column_info output to an abstract column definition.
# Expects to only be called by Bugzila::DB::Mysql::_bz_build_schema_from_disk,
# although there's a chance that it will also work properly if called
# elsewhere.
sub column_info_to_column {
    my ($self, $column_info) = @_;

    # Unfortunately, we have to break Schema's normal "no database"
    # barrier a few times in this function.
    my $dbh = Bugzilla->dbh;

    my $table = $column_info->{TABLE_NAME};
    my $col_name = $column_info->{COLUMN_NAME};

    my $column = {};

    ($column->{NOTNULL} = 1) if $column_info->{NULLABLE} == 0;

    if ($column_info->{mysql_is_pri_key}) {
        # In MySQL, if a table has no PK, but it has a UNIQUE index,
        # that index will show up as the PK. So we have to eliminate
        # that possibility.
        # Unfortunately, the only way to definitely solve this is
        # to break Schema's standard of not touching the live database
        # and check if the index called PRIMARY is on that field.
        my $pri_index = $dbh->bz_index_info_real($table, 'PRIMARY');
        if ( $pri_index && grep($_ eq $col_name, @{$pri_index->{FIELDS}}) ) {
            $column->{PRIMARYKEY} = 1;
        }
    }

    # MySQL frequently defines a default for a field even when we
    # didn't explicitly set one. So we have to have some special
    # hacks to determine whether or not we should actually put
    # a default in the abstract schema for this field.
    if (defined $column_info->{COLUMN_DEF}) {
        # The defaults that MySQL inputs automatically are usually
        # something that would be considered "false" by perl, either
        # a 0 or an empty string. (Except for datetime and decimal
        # fields, which have their own special auto-defaults.)
        #
        # Here's how we handle this: If it exists in the schema
        # without a default, then we don't use the default. If it
        # doesn't exist in the schema, then we're either going to
        # be dropping it soon, or it's a custom end-user column, in which
        # case having a bogus default won't harm anything.
        my $schema_column = $self->get_column($table, $col_name);
        unless ( (!$column_info->{COLUMN_DEF} 
                  || $column_info->{COLUMN_DEF} eq '0000-00-00 00:00:00'
                  || $column_info->{COLUMN_DEF} eq '0.00')
                && $schema_column 
                && !exists $schema_column->{DEFAULT}) {
            
            my $default = $column_info->{COLUMN_DEF};
            # Schema uses '0' for the defaults for decimal fields. 
            $default = 0 if $default =~ /^0\.0+$/;
            # If we're not a number, we're a string and need to be
            # quoted.
            $default = $dbh->quote($default) if !($default =~ /^(-)?(\d+)(.\d+)?$/);
            $column->{DEFAULT} = $default;
        }
    }

    my $type = $column_info->{TYPE_NAME};

    # Certain types of columns need the size/precision appended.
    if ($type =~ /CHAR$/ || $type eq 'DECIMAL') {
        # This is nicely lowercase and has the size/precision appended.
        $type = $column_info->{mysql_type_name};
    }

    # If we're a tinyint, we could be either a BOOLEAN or an INT1.
    # Only the BOOLEAN_MAP knows the difference.
    elsif ($type eq 'TINYINT' && exists BOOLEAN_MAP->{$table}
           && exists BOOLEAN_MAP->{$table}->{$col_name}) {
        $type = 'BOOLEAN';
        if (exists $column->{DEFAULT}) {
            $column->{DEFAULT} = $column->{DEFAULT} ? 'TRUE' : 'FALSE';
        }
    }

    # We also need to check if we're an auto_increment field.
    elsif ($type =~ /INT/) {
        # Unfortunately, the only way to do this in DBI is to query the
        # database, so we have to break the rule here that Schema normally
        # doesn't touch the live DB.
        my $ref_sth = $dbh->prepare(
            "SELECT $col_name FROM $table LIMIT 1");
        $ref_sth->execute;
        if ($ref_sth->{mysql_is_auto_increment}->[0]) {
            if ($type eq 'MEDIUMINT') {
                $type = 'MEDIUMSERIAL';
            }
            elsif ($type eq 'SMALLINT') {
                $type = 'SMALLSERIAL';
            } 
            else {
                $type = 'INTSERIAL';
            }
        }
        $ref_sth->finish;

    }

    # For all other db-specific types, check if they exist in 
    # REVERSE_MAPPING and use the type found there.
    if (exists REVERSE_MAPPING->{$type}) {
        $type = REVERSE_MAPPING->{$type};
    }

    $column->{TYPE} = $type;

    #print "$table.$col_name: " . Data::Dumper->Dump([$column]) . "\n";

    return $column;
}

sub get_rename_column_ddl {
    my ($self, $table, $old_name, $new_name) = @_;
    my $def = $self->get_type_ddl($self->get_column($table, $old_name));
    # MySQL doesn't like having the PRIMARY KEY statement in a rename.
    $def =~ s/PRIMARY KEY//i;
    return ("ALTER TABLE $table CHANGE COLUMN $old_name $new_name $def");
}

1;
