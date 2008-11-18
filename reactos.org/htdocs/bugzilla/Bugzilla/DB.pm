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
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Jacob Steenhagen <jake@bugzilla.org>
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Christopher Aillon <christopher@aillon.com>
#                 Tomas Kopal <Tomas.Kopal@altap.cz>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Lance Larsh <lance.larsh@oracle.com>

package Bugzilla::DB;

use strict;

use DBI;

# Inherit the DB class from DBI::db.
use base qw(DBI::db);

use Bugzilla::Constants;
use Bugzilla::Install::Requirements;
use Bugzilla::Install::Localconfig;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::DB::Schema;

use List::Util qw(max);

#####################################################################
# Constants
#####################################################################

use constant BLOB_TYPE => DBI::SQL_BLOB;

# Set default values for what used to be the enum types.  These values
# are no longer stored in localconfig.  If we are upgrading from a
# Bugzilla with enums to a Bugzilla without enums, we use the
# enum values.
#
# The values that you see here are ONLY DEFAULTS. They are only used
# the FIRST time you run checksetup.pl, IF you are NOT upgrading from a
# Bugzilla with enums. After that, they are either controlled through
# the Bugzilla UI or through the DB.
use constant ENUM_DEFAULTS => {
    bug_severity  => ['blocker', 'critical', 'major', 'normal',
                      'minor', 'trivial', 'enhancement'],
    priority     => ["P1","P2","P3","P4","P5"],
    op_sys       => ["All","Windows","Mac OS","Linux","Other"],
    rep_platform => ["All","PC","Macintosh","Other"],
    bug_status   => ["UNCONFIRMED","NEW","ASSIGNED","REOPENED","RESOLVED",
                     "VERIFIED","CLOSED"],
    resolution   => ["","FIXED","INVALID","WONTFIX", "DUPLICATE","WORKSFORME",
                     "MOVED"],
};

#####################################################################
# Connection Methods
#####################################################################

sub connect_shadow {
    my $params = Bugzilla->params;
    die "Tried to connect to non-existent shadowdb" 
        unless $params->{'shadowdb'};

    my $lc = Bugzilla->localconfig;

    return _connect($lc->{db_driver}, $params->{"shadowdbhost"},
                    $params->{'shadowdb'}, $params->{"shadowdbport"},
                    $params->{"shadowdbsock"}, $lc->{db_user}, $lc->{db_pass});
}

sub connect_main {
    my $lc = Bugzilla->localconfig;
    return _connect($lc->{db_driver}, $lc->{db_host}, $lc->{db_name}, $lc->{db_port},
                    $lc->{db_sock}, $lc->{db_user}, $lc->{db_pass});
}

sub _connect {
    my ($driver, $host, $dbname, $port, $sock, $user, $pass) = @_;

    my $pkg_module = DB_MODULE->{lc($driver)}->{db};

    # do the actual import
    eval ("require $pkg_module")
        || die ("'$driver' is not a valid choice for \$db_driver in "
                . " localconfig: " . $@);

    # instantiate the correct DB specific module
    my $dbh = $pkg_module->new($user, $pass, $host, $dbname, $port, $sock);

    return $dbh;
}

sub _handle_error {
    require Carp;

    # Cut down the error string to a reasonable size
    $_[0] = substr($_[0], 0, 2000) . ' ... ' . substr($_[0], -2000)
        if length($_[0]) > 4000;
    $_[0] = Carp::longmess($_[0]);
    return 0; # Now let DBI handle raising the error
}

sub bz_check_requirements {
    my ($output) = @_;

    my $lc = Bugzilla->localconfig;
    my $db = DB_MODULE->{lc($lc->{db_driver})};
    # Only certain values are allowed for $db_driver.
    if (!defined $db) {
        die "$lc->{db_driver} is not a valid choice for \$db_driver in"
            . bz_locations()->{'localconfig'};
    }

    die("It is not safe to run Bugzilla inside the 'mysql' database.\n"
        . "Please pick a different value for \$db_name in localconfig.")
        if $lc->{db_name} eq 'mysql';

    # Check the existence and version of the DBD that we need.
    my $dbd        = $db->{dbd};
    my $sql_server = $db->{name};
    my $sql_want   = $db->{db_version};
    unless (have_vers($dbd, $output)) {
        my $command = install_command($dbd);
        my $root    = ROOT_USER;
        my $dbd_mod = $dbd->{module};
        my $dbd_ver = $dbd->{version};
        my $version = $dbd_ver ? " $dbd_ver or higher" : '';
        print <<EOT;

For $sql_server, Bugzilla requires that perl's $dbd_mod $dbd_ver or later be
installed. To install this module, run the following command (as $root):

    $command

EOT
        exit;
    }

    # We don't try to connect to the actual database if $db_check is
    # disabled.
    unless ($lc->{db_check}) {
        print "\n" if $output;
        return;
    }

    # And now check the version of the database server itself.
    my $dbh = _get_no_db_connection();

    printf("Checking for %15s %-9s ", $sql_server, "(v$sql_want)")
        if $output;
    my $sql_vers = $dbh->bz_server_version;
    $dbh->disconnect;

    # Check what version of the database server is installed and let
    # the user know if the version is too old to be used with Bugzilla.
    if ( vers_cmp($sql_vers,$sql_want) > -1 ) {
        print "ok: found v$sql_vers\n" if $output;
    } else {
        print <<EOT;

Your $sql_server v$sql_vers is too old. Bugzilla requires version
$sql_want or later of $sql_server. Please download and install a
newer version.

EOT
        exit;
    }

    print "\n" if $output;
}

# Note that this function requires that localconfig exist and
# be valid.
sub bz_create_database {
    my $dbh;
    # See if we can connect to the actual Bugzilla database.
    my $conn_success = eval { $dbh = connect_main(); };
    my $db_name = Bugzilla->localconfig->{db_name};

    if (!$conn_success) {
        $dbh = _get_no_db_connection();
        print "Creating database $db_name...\n";

        # Try to create the DB, and if we fail print a friendly error.
        my $success  = eval {
            my @sql = $dbh->_bz_schema->get_create_database_sql($db_name);
            # This ends with 1 because this particular do doesn't always
            # return something.
            $dbh->do($_) foreach @sql; 1;
        };
        if (!$success) {
            my $error = $dbh->errstr || $@;
            chomp($error);
            print STDERR  "The '$db_name' database could not be created.",
                          " The error returned was:\n\n    $error\n\n",
                          _bz_connect_error_reasons();
            exit;
        }
    }

    $dbh->disconnect;
}

# A helper for bz_create_database and bz_check_requirements.
sub _get_no_db_connection {
    my ($sql_server) = @_;
    my $dbh;
    my $lc = Bugzilla->localconfig;
    my $conn_success = eval {
        $dbh = _connect($lc->{db_driver}, $lc->{db_host}, '', $lc->{db_port},
                        $lc->{db_sock}, $lc->{db_user}, $lc->{db_pass});
    };
    if (!$conn_success) {
        my $sql_server = DB_MODULE->{lc($lc->{db_driver})}->{name};
        # Can't use $dbh->errstr because $dbh is undef.
        my $error = $DBI::errstr || $@;
        chomp($error);
        print STDERR "There was an error connecting to $sql_server:\n\n",
                     "    $error\n\n", _bz_connect_error_reasons();
        exit;
    }
    return $dbh;    
}

# Just a helper because we have to re-use this text.
# We don't use this in db_new because it gives away the database
# username, and db_new errors can show up on CGIs.
sub _bz_connect_error_reasons {
    my $lc_file = bz_locations()->{'localconfig'};
    my $lc      = Bugzilla->localconfig;
    my $db      = DB_MODULE->{lc($lc->{db_driver})};
    my $server  = $db->{name};

return <<EOT;
This might have several reasons:

* $server is not running.
* $server is running, but there is a problem either in the
  server configuration or the database access rights. Read the Bugzilla
  Guide in the doc directory. The section about database configuration
  should help.
* Your password for the '$lc->{db_user}' user, specified in \$db_pass, is 
  incorrect, in '$lc_file'.
* There is a subtle problem with Perl, DBI, or $server. Make
  sure all settings in '$lc_file' are correct. If all else fails, set
  '\$db_check' to 0.

EOT
}

# List of abstract methods we are checking the derived class implements
our @_abstract_methods = qw(REQUIRED_VERSION PROGRAM_NAME DBD_VERSION
                            new sql_regexp sql_not_regexp sql_limit sql_to_days
                            sql_date_format sql_interval
                            bz_lock_tables bz_unlock_tables);

# This overridden import method will check implementation of inherited classes
# for missing implementation of abstract methods
# See http://perlmonks.thepen.com/44265.html
sub import {
    my $pkg = shift;

    # do not check this module
    if ($pkg ne __PACKAGE__) {
        # make sure all abstract methods are implemented
        foreach my $meth (@_abstract_methods) {
            $pkg->can($meth)
                or croak("Class $pkg does not define method $meth");
        }
    }

    # Now we want to call our superclass implementation.
    # If our superclass is Exporter, which is using caller() to find
    # a namespace to populate, we need to adjust for this extra call.
    # All this can go when we stop using deprecated functions.
    my $is_exporter = $pkg->isa('Exporter');
    $Exporter::ExportLevel++ if $is_exporter;
    $pkg->SUPER::import(@_);
    $Exporter::ExportLevel-- if $is_exporter;
}

sub sql_istrcmp {
    my ($self, $left, $right, $op) = @_;
    $op ||= "=";

    return $self->sql_istring($left) . " $op " . $self->sql_istring($right);
}

sub sql_istring {
    my ($self, $string) = @_;

    return "LOWER($string)";
}

sub sql_position {
    my ($self, $fragment, $text) = @_;

    return "POSITION($fragment IN $text)";
}

sub sql_group_by {
    my ($self, $needed_columns, $optional_columns) = @_;

    my $expression = "GROUP BY $needed_columns";
    $expression .= ", " . $optional_columns if $optional_columns;
    
    return $expression;
}

sub sql_string_concat {
    my ($self, @params) = @_;
    
    return '(' . join(' || ', @params) . ')';
}

sub sql_fulltext_search {
    my ($self, $column, $text) = @_;

    # This is as close as we can get to doing full text search using
    # standard ANSI SQL, without real full text search support. DB specific
    # modules should override this, as this will be always much slower.

    # make the string lowercase to do case insensitive search
    my $lower_text = lc($text);

    # split the text we search for into separate words
    my @words = split(/\s+/, $lower_text);

    # surround the words with wildcards and SQL quotes so we can use them
    # in LIKE search clauses
    @words = map($self->quote("%$_%"), @words);

    # untaint words, since they are safe to use now that we've quoted them
    map(trick_taint($_), @words);

    # turn the words into a set of LIKE search clauses
    @words = map("LOWER($column) LIKE $_", @words);

    # search for occurrences of all specified words in the column
    return "CASE WHEN (" . join(" AND ", @words) . ") THEN 1 ELSE 0 END";
}

#####################################################################
# General Info Methods
#####################################################################

# XXX - Needs to be documented.
sub bz_server_version {
    my ($self) = @_;
    return $self->get_info(18); # SQL_DBMS_VER
}

sub bz_last_key {
    my ($self, $table, $column) = @_;

    return $self->last_insert_id(Bugzilla->localconfig->{db_name}, undef, 
                                 $table, $column);
}

sub bz_get_field_defs {
    my ($self) = @_;

    my $extra = "";
    if (!Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'})) {
        $extra = "AND name NOT IN ('estimated_time', 'remaining_time', " .
                 "'work_time', 'percentage_complete', 'deadline')";
    }

    my @fields;
    my $sth = $self->prepare("SELECT name, description FROM fielddefs
                              WHERE obsolete = 0 $extra
                              ORDER BY sortkey");
    $sth->execute();
    while (my $field_ref = $sth->fetchrow_hashref()) {
        push(@fields, $field_ref);
    }
    return(@fields);
}

#####################################################################
# Database Setup
#####################################################################

sub bz_setup_database {
    my ($self) = @_;

    # If we haven't ever stored a serialized schema,
    # set up the bz_schema table and store it.
    $self->_bz_init_schema_storage();
    
    my @desired_tables = $self->_bz_schema->get_table_list();

    foreach my $table_name (@desired_tables) {
        $self->bz_add_table($table_name);
    }
}

# This really just exists to get overridden in Bugzilla::DB::Mysql.
sub bz_enum_initial_values {
    return ENUM_DEFAULTS;
}

sub bz_populate_enum_tables {
    my ($self) = @_;

    my $enum_values = $self->bz_enum_initial_values();
    while (my ($table, $values) = each %$enum_values) {
        $self->_bz_populate_enum_table($table, $values);
    }
}

#####################################################################
# Schema Modification Methods
#####################################################################

sub bz_add_column {
    my ($self, $table, $name, $new_def, $init_value) = @_;

    # You can't add a NOT NULL column to a table with
    # no DEFAULT statement, unless you have an init_value.
    # SERIAL types are an exception, though, because they can
    # auto-populate.
    if ( $new_def->{NOTNULL} && !exists $new_def->{DEFAULT} 
         && !defined $init_value && $new_def->{TYPE} !~ /SERIAL/)
    {
        ThrowCodeError('column_not_null_without_default',
                       { name => "$table.$name" });
    }

    my $current_def = $self->bz_column_info($table, $name);

    if (!$current_def) {
        my @statements = $self->_bz_real_schema->get_add_column_ddl(
            $table, $name, $new_def, 
            defined $init_value ? $self->quote($init_value) : undef);
        print get_text('install_column_add',
                       { column => $name, table => $table }) . "\n"
            if Bugzilla->usage_mode == USAGE_MODE_CMDLINE;
        foreach my $sql (@statements) {
            $self->do($sql);
        }
        $self->_bz_real_schema->set_column($table, $name, $new_def);
        $self->_bz_store_real_schema;
    }
}

sub bz_alter_column {
    my ($self, $table, $name, $new_def, $set_nulls_to) = @_;

    my $current_def = $self->bz_column_info($table, $name);

    if (!$self->_bz_schema->columns_equal($current_def, $new_def)) {
        # You can't change a column to be NOT NULL if you have no DEFAULT
        # and no value for $set_nulls_to, if there are any NULL values 
        # in that column.
        if ($new_def->{NOTNULL} && 
            !exists $new_def->{DEFAULT} && !defined $set_nulls_to)
        {
            # Check for NULLs
            my $any_nulls = $self->selectrow_array(
                "SELECT 1 FROM $table WHERE $name IS NULL");
            ThrowCodeError('column_not_null_no_default_alter', 
                           { name => "$table.$name" }) if ($any_nulls);
        }
        $self->bz_alter_column_raw($table, $name, $new_def, $current_def,
                                   $set_nulls_to);
        $self->_bz_real_schema->set_column($table, $name, $new_def);
        $self->_bz_store_real_schema;
    }
}


# bz_alter_column_raw($table, $name, $new_def, $current_def)
#
# Description: A helper function for bz_alter_column.
#              Alters a column in the database
#              without updating any Schema object. Generally
#              should only be called by bz_alter_column.
#              Used when either: (1) You don't yet have a Schema
#              object but you need to alter a column, for some reason.
#              (2) You need to alter a column for some database-specific
#              reason.
# Params:      $table   - The name of the table the column is on.
#              $name    - The name of the column you're changing.
#              $new_def - The abstract definition that you are changing
#                         this column to.
#              $current_def - (optional) The current definition of the
#                             column. Will be used in the output message,
#                             if given.
#              $set_nulls_to - The same as the param of the same name
#                              from bz_alter_column.
# Returns:     nothing
#
sub bz_alter_column_raw {
    my ($self, $table, $name, $new_def, $current_def, $set_nulls_to) = @_;
    my @statements = $self->_bz_real_schema->get_alter_column_ddl(
        $table, $name, $new_def,
        defined $set_nulls_to ? $self->quote($set_nulls_to) : undef);
    my $new_ddl = $self->_bz_schema->get_type_ddl($new_def);
    print "Updating column $name in table $table ...\n";
    if (defined $current_def) {
        my $old_ddl = $self->_bz_schema->get_type_ddl($current_def);
        print "Old: $old_ddl\n";
    }
    print "New: $new_ddl\n";
    $self->do($_) foreach (@statements);
}

sub bz_add_index {
    my ($self, $table, $name, $definition) = @_;

    my $index_exists = $self->bz_index_info($table, $name);

    if (!$index_exists) {
        $self->bz_add_index_raw($table, $name, $definition);
        $self->_bz_real_schema->set_index($table, $name, $definition);
        $self->_bz_store_real_schema;
    }
}

# bz_add_index_raw($table, $name, $silent)
#
# Description: A helper function for bz_add_index.
#              Adds an index to the database
#              without updating any Schema object. Generally
#              should only be called by bz_add_index.
#              Used when you don't yet have a Schema
#              object but you need to add an index, for some reason.
# Params:      $table  - The name of the table the index is on.
#              $name   - The name of the index you're adding.
#              $definition - The abstract index definition, in hashref
#                            or arrayref format.
#              $silent - (optional) If specified and true, don't output
#                        any message about this change.
# Returns:     nothing
#
sub bz_add_index_raw {
    my ($self, $table, $name, $definition, $silent) = @_;
    my @statements = $self->_bz_schema->get_add_index_ddl(
        $table, $name, $definition);
    print "Adding new index '$name' to the $table table ...\n" unless $silent;
    $self->do($_) foreach (@statements);
}

sub bz_add_table {
    my ($self, $name) = @_;

    my $table_exists = $self->bz_table_info($name);

    if (!$table_exists) {
        $self->_bz_add_table_raw($name);
        $self->_bz_real_schema->add_table($name,
            $self->_bz_schema->get_table_abstract($name));
        $self->_bz_store_real_schema;
    }
}

# _bz_add_table_raw($name) - Private
#
# Description: A helper function for bz_add_table.
#              Creates a table in the database without
#              updating any Schema object. Generally
#              should only be called by bz_add_table and by
#              _bz_init_schema_storage. Used when you don't
#              yet have a Schema object but you need to
#              add a table, for some reason.
# Params:      $name - The name of the table you're creating. 
#                  The definition for the table is pulled from 
#                  _bz_schema.
# Returns:     nothing
#
sub _bz_add_table_raw {
    my ($self, $name) = @_;
    my @statements = $self->_bz_schema->get_table_ddl($name);
    print "Adding new table $name ...\n" unless i_am_cgi();
    $self->do($_) foreach (@statements);
}

sub bz_add_field_table {
    my ($self, $name) = @_;
    # We do nothing if the table already exists.
    return if $self->bz_table_info($name);

    # Copy this so that we're not modifying the constant.
    my %table_schema = %{ $self->_bz_schema->FIELD_TABLE_SCHEMA };
    my %indexes = @{ $table_schema{INDEXES} };
    my %fixed_indexes;
    foreach my $key (keys %indexes) {
        $fixed_indexes{$name . "_" . $key} = $indexes{$key};
    }
    # INDEXES is supposed to be an arrayref, so we have to convert back.
    my @indexes_array = %fixed_indexes;
    $table_schema{INDEXES} = \@indexes_array;
    # We add this to the abstract schema so that bz_add_table can find it.
    $self->_bz_schema->add_table($name, \%table_schema);
    $self->bz_add_table($name);
}

sub bz_drop_column {
    my ($self, $table, $column) = @_;

    my $current_def = $self->bz_column_info($table, $column);

    if ($current_def) {
        my @statements = $self->_bz_real_schema->get_drop_column_ddl(
            $table, $column);
        print get_text('install_column_drop', 
                       { table => $table, column => $column }) . "\n"
            if Bugzilla->usage_mode == USAGE_MODE_CMDLINE;
        foreach my $sql (@statements) {
            # Because this is a deletion, we don't want to die hard if
            # we fail because of some local customization. If something
            # is already gone, that's fine with us!
            eval { $self->do($sql); } or warn "Failed SQL: [$sql] Error: $@";
        }
        $self->_bz_real_schema->delete_column($table, $column);
        $self->_bz_store_real_schema;
    }
}

sub bz_drop_index {
    my ($self, $table, $name) = @_;

    my $index_exists = $self->bz_index_info($table, $name);

    if ($index_exists) {
        $self->bz_drop_index_raw($table, $name);
        $self->_bz_real_schema->delete_index($table, $name);
        $self->_bz_store_real_schema;        
    }
}

# bz_drop_index_raw($table, $name, $silent)
#
# Description: A helper function for bz_drop_index.
#              Drops an index from the database
#              without updating any Schema object. Generally
#              should only be called by bz_drop_index.
#              Used when either: (1) You don't yet have a Schema 
#              object but you need to drop an index, for some reason.
#              (2) You need to drop an index that somehow got into the
#              database but doesn't exist in Schema.
# Params:      $table  - The name of the table the index is on.
#              $name   - The name of the index you're dropping.
#              $silent - (optional) If specified and true, don't output
#                        any message about this change.
# Returns:     nothing
#
sub bz_drop_index_raw {
    my ($self, $table, $name, $silent) = @_;
    my @statements = $self->_bz_schema->get_drop_index_ddl(
        $table, $name);
    print "Removing index '$name' from the $table table...\n" unless $silent;
    foreach my $sql (@statements) {
        # Because this is a deletion, we don't want to die hard if
        # we fail because of some local customization. If something
        # is already gone, that's fine with us!
        eval { $self->do($sql) } or warn "Failed SQL: [$sql] Error: $@";
    }
}

sub bz_drop_table {
    my ($self, $name) = @_;

    my $table_exists = $self->bz_table_info($name);

    if ($table_exists) {
        my @statements = $self->_bz_schema->get_drop_table_ddl($name);
        print get_text('install_table_drop', { name => $name }) . "\n"
            if Bugzilla->usage_mode == USAGE_MODE_CMDLINE;
        foreach my $sql (@statements) {
            # Because this is a deletion, we don't want to die hard if
            # we fail because of some local customization. If something
            # is already gone, that's fine with us!
            eval { $self->do($sql); } or warn "Failed SQL: [$sql] Error: $@";
        }
        $self->_bz_real_schema->delete_table($name);
        $self->_bz_store_real_schema;
    }
}

sub bz_rename_column {
    my ($self, $table, $old_name, $new_name) = @_;

    my $old_col_exists  = $self->bz_column_info($table, $old_name);

    if ($old_col_exists) {
        my $already_renamed = $self->bz_column_info($table, $new_name);
            ThrowCodeError('db_rename_conflict',
                           { old => "$table.$old_name", 
                             new => "$table.$new_name" }) if $already_renamed;
        my @statements = $self->_bz_real_schema->get_rename_column_ddl(
            $table, $old_name, $new_name);

        print get_text('install_column_rename', 
                       { old => "$table.$old_name", new => "$table.$new_name" })
               . "\n" if Bugzilla->usage_mode == USAGE_MODE_CMDLINE;

        foreach my $sql (@statements) {
            $self->do($sql);
        }
        $self->_bz_real_schema->rename_column($table, $old_name, $new_name);
        $self->_bz_store_real_schema;
    }
}

sub bz_rename_table {
    my ($self, $old_name, $new_name) = @_;
    my $old_table = $self->bz_table_info($old_name);
    return if !$old_table;

    my $new = $self->bz_table_info($new_name);
    ThrowCodeError('db_rename_conflict', { old => $old_name,
                                           new => $new_name }) if $new;
    my @sql = $self->_bz_real_schema->get_rename_table_sql($old_name, $new_name);
    print get_text('install_table_rename', 
                   { old => $old_name, new => $new_name }) . "\n"
        if Bugzilla->usage_mode == USAGE_MODE_CMDLINE;
    $self->do($_) foreach @sql;
    $self->_bz_real_schema->rename_table($old_name, $new_name);
    $self->_bz_store_real_schema;
}

#####################################################################
# Schema Information Methods
#####################################################################

sub _bz_schema {
    my ($self) = @_;
    return $self->{private_bz_schema} if exists $self->{private_bz_schema};
    my @module_parts = split('::', ref $self);
    my $module_name  = pop @module_parts;
    $self->{private_bz_schema} = Bugzilla::DB::Schema->new($module_name);
    return $self->{private_bz_schema};
}

# _bz_get_initial_schema()
#
# Description: A protected method, intended for use only by Bugzilla::DB
#              and subclasses. Used to get the initial Schema that will
#              be written to disk for _bz_init_schema_storage. You probably
#              want to use _bz_schema or _bz_real_schema instead of this
#              method.
# Params:      none
# Returns:     A Schema object that can be serialized and written to disk
#              for _bz_init_schema_storage.
sub _bz_get_initial_schema {
    my ($self) = @_;
    return $self->_bz_schema->get_empty_schema();
}

sub bz_column_info {
    my ($self, $table, $column) = @_;
    return $self->_bz_real_schema->get_column_abstract($table, $column);
}

sub bz_index_info {
    my ($self, $table, $index) = @_;
    my $index_def =
        $self->_bz_real_schema->get_index_abstract($table, $index);
    if (ref($index_def) eq 'ARRAY') {
        $index_def = {FIELDS => $index_def, TYPE => ''};
    }
    return $index_def;
}

sub bz_table_info {
    my ($self, $table) = @_;
    return $self->_bz_real_schema->get_table_abstract($table);
}


sub bz_table_columns {
    my ($self, $table) = @_;
    return $self->_bz_real_schema->get_table_columns($table);
}

sub bz_table_indexes {
    my ($self, $table) = @_;
    my $indexes = $self->_bz_real_schema->get_table_indexes_abstract($table);
    my %return_indexes;
    # We do this so that they're always hashes.
    foreach my $name (keys %$indexes) {
        $return_indexes{$name} = $self->bz_index_info($table, $name);
    }
    return \%return_indexes;
}

#####################################################################
# Protected "Real Database" Schema Information Methods
#####################################################################

# Only Bugzilla::DB and subclasses should use these methods.
# If you need a method that does the same thing as one of these
# methods, use the version without _real on the end.

# bz_table_columns_real($table)
#
# Description: Returns a list of columns on a given table
#              as the table actually is, on the disk.
# Params:      $table - Name of the table.
# Returns:     An array of column names.
#
sub bz_table_columns_real {
    my ($self, $table) = @_;
    my $sth = $self->column_info(undef, undef, $table, '%');
    return @{ $self->selectcol_arrayref($sth, {Columns => [4]}) };
}

# bz_table_list_real()
#
# Description: Gets a list of tables in the current
#              database, directly from the disk.
# Params:      none
# Returns:     An array containing table names.
sub bz_table_list_real {
    my ($self) = @_;
    my $table_sth = $self->table_info(undef, undef, undef, "TABLE");
    return @{$self->selectcol_arrayref($table_sth, { Columns => [3] })};
}

#####################################################################
# Transaction Methods
#####################################################################

sub bz_start_transaction {
    my ($self) = @_;

    if ($self->{private_bz_in_transaction}) {
        ThrowCodeError("nested_transaction");
    } else {
        # Turn AutoCommit off and start a new transaction
        $self->begin_work();
        $self->{private_bz_in_transaction} = 1;
    }
}

sub bz_commit_transaction {
    my ($self) = @_;

    if (!$self->{private_bz_in_transaction}) {
        ThrowCodeError("not_in_transaction");
    } else {
        $self->commit();

        $self->{private_bz_in_transaction} = 0;
    }
}

sub bz_rollback_transaction {
    my ($self) = @_;

    if (!$self->{private_bz_in_transaction}) {
        ThrowCodeError("not_in_transaction");
    } else {
        $self->rollback();

        $self->{private_bz_in_transaction} = 0;
    }
}

#####################################################################
# Subclass Helpers
#####################################################################

sub db_new {
    my ($class, $dsn, $user, $pass, $attributes) = @_;

    # set up default attributes used to connect to the database
    # (if not defined by DB specific implementation)
    $attributes = { RaiseError => 0,
                    AutoCommit => 1,
                    PrintError => 0,
                    ShowErrorStatement => 1,
                    HandleError => \&_handle_error,
                    TaintIn => 1,
                    FetchHashKeyName => 'NAME',  
                    # Note: NAME_lc causes crash on ActiveState Perl
                    # 5.8.4 (see Bug 253696)
                    # XXX - This will likely cause problems in DB
                    # back ends that twiddle column case (Oracle?)
                  } if (!defined($attributes));

    # connect using our known info to the specified db
    my $self = DBI->connect($dsn, $user, $pass, $attributes)
        or die "\nCan't connect to the database.\nError: $DBI::errstr\n"
        . "  Is your database installed and up and running?\n  Do you have"
        . " the correct username and password selected in localconfig?\n\n";

    # RaiseError was only set to 0 so that we could catch the 
    # above "die" condition.
    $self->{RaiseError} = 1;

    # class variables
    $self->{private_bz_in_transaction} = 0;

    bless ($self, $class);

    return $self;
}

#####################################################################
# Private Methods
#####################################################################

=begin private

=head1 PRIVATE METHODS

These methods really are private. Do not override them in subclasses.

=over 4

=item C<_init_bz_schema_storage>

 Description: Initializes the bz_schema table if it contains nothing.
 Params:      none
 Returns:     nothing

=cut

sub _bz_init_schema_storage {
    my ($self) = @_;

    my $table_size;
    eval {
        $table_size = 
            $self->selectrow_array("SELECT COUNT(*) FROM bz_schema");
    };

    if (!$table_size) {
        my $init_schema = $self->_bz_get_initial_schema;
        my $store_me = $init_schema->serialize_abstract();
        my $schema_version = $init_schema->SCHEMA_VERSION;

        # If table_size is not defined, then we hit an error reading the
        # bz_schema table, which means it probably doesn't exist yet. So,
        # we have to create it. If we failed above for some other reason,
        # we'll see the failure here.
        # However, we must create the table after we do get_initial_schema,
        # because some versions of get_initial_schema read that the table
        # exists and then add it to the Schema, where other versions don't.
        if (!defined $table_size) {
            $self->_bz_add_table_raw('bz_schema');
        }

        print "Initializing the new Schema storage...\n";
        my $sth = $self->prepare("INSERT INTO bz_schema "
                                 ." (schema_data, version) VALUES (?,?)");
        $sth->bind_param(1, $store_me, $self->BLOB_TYPE);
        $sth->bind_param(2, $schema_version);
        $sth->execute();

        # And now we have to update the on-disk schema to hold the bz_schema
        # table, if the bz_schema table didn't exist when we were called.
        if (!defined $table_size) {
            $self->_bz_real_schema->add_table('bz_schema',
                $self->_bz_schema->get_table_abstract('bz_schema'));
            $self->_bz_store_real_schema;
        }
    } 
    # Sanity check
    elsif ($table_size > 1) {
        # We tell them to delete the newer one. Better to have checksetup
        # run migration code too many times than to have it not run the
        # correct migration code at all.
        die "Attempted to initialize the schema but there are already "
            . " $table_size copies of it stored.\nThis should never happen.\n"
            . " Compare the rows of the bz_schema table and delete the "
            . "newer one(s).";
    }
}

=item C<_bz_real_schema()>

 Description: Returns a Schema object representing the database
              that is being used in the current installation.
 Params:      none
 Returns:     A C<Bugzilla::DB::Schema> object representing the database
              as it exists on the disk.

=cut

sub _bz_real_schema {
    my ($self) = @_;
    return $self->{private_real_schema} if exists $self->{private_real_schema};

    my ($data, $version) = $self->selectrow_array(
        "SELECT schema_data, version FROM bz_schema");

    (die "_bz_real_schema tried to read the bz_schema table but it's empty!")
        if !$data;

    $self->{private_real_schema} = 
        $self->_bz_schema->deserialize_abstract($data, $version);

    return $self->{private_real_schema};
}

=item C<_bz_store_real_schema()>

 Description: Stores the _bz_real_schema structures in the database
              for later recovery. Call this function whenever you make
              a change to the _bz_real_schema.
 Params:      none
 Returns:     nothing

 Precondition: $self->{_bz_real_schema} must exist.

=back

=end private

=cut

sub _bz_store_real_schema {
    my ($self) = @_;

    # Make sure that there's a schema to update
    my $table_size = $self->selectrow_array("SELECT COUNT(*) FROM bz_schema");

    die "Attempted to update the bz_schema table but there's nothing "
        . "there to update. Run checksetup." unless $table_size;

    # We want to store the current object, not one
    # that we read from the database. So we use the actual hash
    # member instead of the subroutine call. If the hash
    # member is not defined, we will (and should) fail.
    my $update_schema = $self->{private_real_schema};
    my $store_me = $update_schema->serialize_abstract();
    my $schema_version = $update_schema->SCHEMA_VERSION;
    my $sth = $self->prepare("UPDATE bz_schema 
                                 SET schema_data = ?, version = ?");
    $sth->bind_param(1, $store_me, $self->BLOB_TYPE);
    $sth->bind_param(2, $schema_version);
    $sth->execute();
}

# For bz_populate_enum_tables
sub _bz_populate_enum_table {
    my ($self, $table, $valuelist) = @_;

    my $sql_table = $self->quote_identifier($table);

    # Check if there are any table entries
    my $table_size = $self->selectrow_array("SELECT COUNT(*) FROM $sql_table");

    # If the table is empty...
    if (!$table_size) {
        my $insert = $self->prepare(
            "INSERT INTO $sql_table (value,sortkey) VALUES (?,?)");
        print "Inserting values into the '$table' table:\n";
        my $sortorder = 0;
        my $maxlen    = max(map(length($_), @$valuelist)) + 2;
        foreach my $value (@$valuelist) {
            $sortorder += 100;
            printf "%-${maxlen}s sortkey: $sortorder\n", "'$value'";
            $insert->execute($value, $sortorder);
        }
    }
}

1;

__END__

=head1 NAME

Bugzilla::DB - Database access routines, using L<DBI>

=head1 SYNOPSIS

  # Obtain db handle
  use Bugzilla::DB;
  my $dbh = Bugzilla->dbh;

  # prepare a query using DB methods
  my $sth = $dbh->prepare("SELECT " .
                          $dbh->sql_date_format("creation_ts", "%Y%m%d") .
                          " FROM bugs WHERE bug_status != 'RESOLVED' " .
                          $dbh->sql_limit(1));

  # Execute the query
  $sth->execute;

  # Get the results
  my @result = $sth->fetchrow_array;

  # Schema Modification
  $dbh->bz_add_column($table, $name, \%definition, $init_value);
  $dbh->bz_add_index($table, $name, $definition);
  $dbh->bz_add_table($name);
  $dbh->bz_drop_index($table, $name);
  $dbh->bz_drop_table($name);
  $dbh->bz_alter_column($table, $name, \%new_def, $set_nulls_to);
  $dbh->bz_drop_column($table, $column);
  $dbh->bz_rename_column($table, $old_name, $new_name);

  # Schema Information
  my $column = $dbh->bz_column_info($table, $column);
  my $index  = $dbh->bz_index_info($table, $index);

  # General Information
  my @fields    = $dbh->bz_get_field_defs();

=head1 DESCRIPTION

Functions in this module allows creation of a database handle to connect
to the Bugzilla database. This should never be done directly; all users
should use the L<Bugzilla> module to access the current C<dbh> instead.

This module also contains methods extending the returned handle with
functionality which is different between databases allowing for easy
customization for particular database via inheritance. These methods
should be always preffered over hard-coding SQL commands.

=head1 CONSTANTS

Subclasses of Bugzilla::DB are required to define certain constants. These
constants are required to be subroutines or "use constant" variables.

=over 4

=item C<BLOB_TYPE>

The C<\%attr> argument that must be passed to bind_param in order to 
correctly escape a C<LONGBLOB> type.

=back


=head1 CONNECTION

A new database handle to the required database can be created using this
module. This is normally done by the L<Bugzilla> module, and so these routines
should not be called from anywhere else.

=head2 Functions

=over

=item C<connect_main>

=over

=item B<Description>

Function to connect to the main database, returning a new database handle.

=item B<Params>

=over

=item C<$no_db_name> (optional) - If true, connect to the database
server, but don't connect to a specific database. This is only used 
when creating a database. After you create the database, you should 
re-create a new Bugzilla::DB object without using this parameter. 

=back

=item B<Returns>

New instance of the DB class

=back

=item C<connect_shadow>

=over

=item B<Description>

Function to connect to the shadow database, returning a new database handle.
This routine C<die>s if no shadow database is configured.

=item B<Params> (none)

=item B<Returns>

A new instance of the DB class

=back

=item C<bz_check_requirements>

=over

=item B<Description>

Checks to make sure that you have the correct DBD and database version 
installed for the database that Bugzilla will be using. Prints a message 
and exits if you don't pass the requirements.

If C<$db_check> is false (from F<localconfig>), we won't check the 
database version.

=item B<Params>

=over

=item C<$output> - C<true> if the function should display informational 
output about what it's doing, such as versions found.

=back

=item B<Returns> (nothing)

=back


=item C<bz_create_database>

=over

=item B<Description>

Creates an empty database with the name C<$db_name>, if that database 
doesn't already exist. Prints an error message and exits if we can't 
create the database.

=item B<Params> (none)

=item B<Returns> (nothing)

=back

=item C<_connect>

=over

=item B<Description>

Internal function, creates and returns a new, connected instance of the 
correct DB class.  This routine C<die>s if no driver is specified.

=item B<Params>

=over

=item C<$driver> - name of the database driver to use

=item C<$host> - host running the database we are connecting to

=item C<$dbname> - name of the database to connect to

=item C<$port> - port the database is listening on

=item C<$sock> - socket the database is listening on

=item C<$user> - username used to log in to the database

=item C<$pass> - password used to log in to the database

=back

=item B<Returns>

A new instance of the DB class

=back

=item C<_handle_error>

Function passed to the DBI::connect call for error handling. It shortens the 
error for printing.

=item C<import>

Overrides the standard import method to check that derived class
implements all required abstract methods. Also calls original implementation 
in its super class.

=back

=head1 ABSTRACT METHODS

Note: Methods which can be implemented generically for all DBs are implemented in
this module. If needed, they can be overridden with DB specific code.
Methods which do not have standard implementation are abstract and must
be implemented for all supported databases separately.
To avoid confusion with standard DBI methods, all methods returning string with
formatted SQL command have prefix C<sql_>. All other methods have prefix C<bz_>.

=head2 Constructor

=over

=item C<new>

=over

=item B<Description>

Constructor.  Abstract method, should be overridden by database specific 
code.

=item B<Params>

=over 

=item C<$user> - username used to log in to the database

=item C<$pass> - password used to log in to the database

=item C<$host> - host running the database we are connecting to

=item C<$dbname> - name of the database to connect to

=item C<$port> - port the database is listening on

=item C<$sock> - socket the database is listening on

=back

=item B<Returns>

A new instance of the DB class

=item B<Note>

The constructor should create a DSN from the parameters provided and
then call C<db_new()> method of its super class to create a new
class instance. See L<db_new> description in this module. As per
DBI documentation, all class variables must be prefixed with
"private_". See L<DBI>.

=back

=back

=head2 SQL Generation

=over

=item C<sql_regexp>

=over

=item B<Description>

Outputs SQL regular expression operator for POSIX regex
searches (case insensitive) in format suitable for a given
database.

Abstract method, should be overridden by database specific code.

=item B<Params>

=over

=item C<$expr> - SQL expression for the text to be searched (scalar)

=item C<$pattern> - the regular expression to search for (scalar)

=back

=item B<Returns>

Formatted SQL for regular expression search (e.g. REGEXP) (scalar)

=back

=item C<sql_not_regexp>

=over

=item B<Description>

Outputs SQL regular expression operator for negative POSIX
regex searches (case insensitive) in format suitable for a given
database.

Abstract method, should be overridden by database specific code.

=item B<Params>

=over

=item C<$expr> - SQL expression for the text to be searched (scalar)

=item C<$pattern> - the regular expression to search for (scalar)

=back

=item B<Returns>

Formatted SQL for negative regular expression search (e.g. NOT REGEXP) 
(scalar)

=back

=item C<sql_limit>

=over

=item B<Description>

Returns SQL syntax for limiting results to some number of rows
with optional offset if not starting from the begining.

Abstract method, should be overridden by database specific code.

=item B<Params>

=over

=item C<$limit> - number of rows to return from query (scalar)

=item C<$offset> - number of rows to skip prior counting (scalar)

=back

=item B<Returns>

Formatted SQL for limiting number of rows returned from query
with optional offset (e.g. LIMIT 1, 1) (scalar)

=back

=item C<sql_from_days>

=over

=item B<Description>

Outputs SQL syntax for converting Julian days to date.

Abstract method, should be overridden by database specific code.

=item B<Params>

=over

=item C<$days> - days to convert to date

=back

=item B<Returns>

Formatted SQL for returning Julian days in dates. (scalar)

=back

=item C<sql_to_days>

=over

=item B<Description>

Outputs SQL syntax for converting date to Julian days.

Abstract method, should be overridden by database specific code.

=item B<Params>

=over

=item C<$date> - date to convert to days

=back

=item B<Returns>

Formatted SQL for returning date fields in Julian days. (scalar)

=back

=item C<sql_date_format>

=over

=item B<Description>

Outputs SQL syntax for formatting dates.

Abstract method, should be overridden by database specific code.

=item B<Params>

=over

=item C<$date> - date or name of date type column (scalar)

=item C<$format> - format string for date output (scalar)
(C<%Y> = year, four digits, C<%y> = year, two digits, C<%m> = month,
C<%d> = day, C<%a> = weekday name, 3 letters, C<%H> = hour 00-23,
C<%i> = minute, C<%s> = second)

=back

=item B<Returns>

Formatted SQL for date formatting (scalar)

=back

=item C<sql_interval>

=over

=item B<Description>

Outputs proper SQL syntax for a time interval function.

Abstract method, should be overridden by database specific code.

=item B<Params>

=over

=item C<$interval> - the time interval requested (e.g. '30') (integer)

=item C<$units> - the units the interval is in (e.g. 'MINUTE') (string)

=back

=item B<Returns>

Formatted SQL for interval function (scalar)

=back

=item C<sql_position>

=over

=item B<Description>

Outputs proper SQL syntax determinig position of a substring
(fragment) withing a string (text). Note: if the substring or
text are string constants, they must be properly quoted (e.g. "'pattern'").

=item B<Params>

=over

=item C<$fragment> - the string fragment we are searching for (scalar)

=item C<$text> - the text to search (scalar)

=back

=item B<Returns>

Formatted SQL for substring search (scalar)

=back

=item C<sql_group_by>

=over

=item B<Description>

Outputs proper SQL syntax for grouping the result of a query.

For ANSI SQL databases, we need to group by all columns we are
querying for (except for columns used in aggregate functions).
Some databases require (or even allow) to specify only one
or few columns if the result is uniquely defined. For those
databases, the default implementation needs to be overloaded.

=item B<Params>

=over

=item C<$needed_columns> - string with comma separated list of columns
we need to group by to get expected result (scalar)

=item C<$optional_columns> - string with comma separated list of all
other columns we are querying for, but which are not in the required list.

=back

=item B<Returns>

Formatted SQL for row grouping (scalar)

=back

=item C<sql_string_concat>

=over

=item B<Description>

Returns SQL syntax for concatenating multiple strings (constants
or values from table columns) together.

=item B<Params>

=over

=item C<@params> - array of column names or strings to concatenate

=back

=item B<Returns>

Formatted SQL for concatenating specified strings

=back

=item C<sql_fulltext_search>

=over

=item B<Description>

Returns SQL syntax for performing a full text search for specified text 
on a given column.

There is a ANSI SQL version of this method implemented using LIKE operator,
but it's not a real full text search. DB specific modules should override 
this, as this generic implementation will be always much slower. This 
generic implementation returns 'relevance' as 0 for no match, or 1 for a 
match.

=item B<Params>

=over

=item C<$column> - name of column to search (scalar)

=item C<$text> - text to search for (scalar)

=back

=item B<Returns>

Formatted SQL for full text search

=back

=item C<sql_istrcmp>

=over

=item B<Description>

Returns SQL for a case-insensitive string comparison.

=item B<Params>

=over

=item C<$left> - What should be on the left-hand-side of the operation.

=item C<$right> - What should be on the right-hand-side of the operation.

=item C<$op> (optional) - What the operation is. Should be a  valid ANSI 
SQL comparison operator, such as C<=>, C<E<lt>>, C<LIKE>, etc. Defaults 
to C<=> if not specified.

=back

=item B<Returns>

A SQL statement that will run the comparison in a case-insensitive fashion.

=item B<Note>

Uses L</sql_istring>, so it has the same performance concerns.
Try to avoid using this function unless absolutely necessary.

Subclass Implementors: Override sql_istring instead of this
function, most of the time (this function uses sql_istring).

=back

=item C<sql_istring>

=over

=item B<Description>

Returns SQL syntax "preparing" a string or text column for case-insensitive 
comparison.

=item B<Params>

=over

=item C<$string> - string to convert (scalar)

=back

=item B<Returns>

Formatted SQL making the string case insensitive.

=item B<Note>

The default implementation simply calls LOWER on the parameter.
If this is used to search on a text column with index, the index
will not be usually used unless it was created as LOWER(column).

=back

=item C<bz_lock_tables>

=over

=item B<Description>

Performs a table lock operation on specified tables. If the underlying 
database supports transactions, it should also implicitly start a new 
transaction.

Abstract method, should be overridden by database specific code.

=item B<Params>

=over

=item C<@tables> - list of names of tables to lock in MySQL
notation (ex. 'bugs AS bugs2 READ', 'logincookies WRITE')

=back

=item B<Returns> (nothing)

=back

=item C<bz_unlock_tables>

=over

=item B<Description>

Performs a table unlock operation.

If the underlying database supports transactions, it should also implicitly 
commit or rollback the transaction.

Also, this function should allow to be called with the abort flag
set even without locking tables first without raising an error
to simplify error handling.

Abstract method, should be overridden by database specific code.

=item B<Params>

=over

=item C<$abort> - C<UNLOCK_ABORT> if the operation on locked tables
failed (if transactions are supported, the action will be rolled
back). No param if the operation succeeded. This is only used by
L<Bugzilla::Error/throw_error>.

=back

=item B<Returns> (none)

=back

=back


=head1 IMPLEMENTED METHODS

These methods are implemented in Bugzilla::DB, and only need
to be implemented in subclasses if you need to override them for 
database-compatibility reasons.

=head2 General Information Methods

These methods return information about data in the database.

=over

=item C<bz_last_key>

=over

=item B<Description>

Returns the last serial number, usually from a previous INSERT.

Must be executed directly following the relevant INSERT.
This base implementation uses L<DBI/last_insert_id>. If the
DBD supports it, it is the preffered way to obtain the last
serial index. If it is not supported, the DB-specific code
needs to override this function.

=item B<Params>

=over

=item C<$table> - name of table containing serial column (scalar)

=item C<$column> - name of column containing serial data type (scalar)

=back

=item B<Returns>

Last inserted ID (scalar)

=back

=item C<bz_get_field_defs>

=over

=item B<Description>

Returns a list of all the "bug" fields in Bugzilla. The list
contains hashes, with a C<name> key and a C<description> key.

=item B<Params> (none)

=item B<Returns>

List of all the "bug" fields

=back

=back

=head2 Database Setup Methods

These methods are used by the Bugzilla installation programs to set up
the database.

=over

=item C<bz_populate_enum_tables>

=over

=item B<Description>

For an upgrade or an initial installation, populates the tables that hold 
the legal values for the old "enum" fields: C<bug_severity>, 
C<resolution>, etc. Prints out information if it inserts anything into the
DB.

=item B<Params> (none)

=item B<Returns> (nothing)

=back

=back


=head2 Schema Modification Methods

These methods modify the current Bugzilla Schema.

Where a parameter says "Abstract index/column definition", it returns/takes
information in the formats defined for indexes and columns in
C<Bugzilla::DB::Schema::ABSTRACT_SCHEMA>.

=over

=item C<bz_add_column>

=over

=item B<Description>

Adds a new column to a table in the database. Prints out a brief statement 
that it did so, to stdout. Note that you cannot add a NOT NULL column that 
has no default -- the database won't know what to set all the NULL
values to.

=item B<Params>

=over

=item C<$table> - the table where the column is being added

=item C<$name> - the name of the new column

=item C<\%definition> - Abstract column definition for the new column

=item C<$init_value> (optional) - An initial value to set the column
to. Required if your column is NOT NULL and has no DEFAULT set.

=back

=item B<Returns> (nothing)

=back

=item C<bz_add_index>

=over

=item B<Description>

Adds a new index to a table in the database. Prints out a brief statement 
that it did so, to stdout. If the index already exists, we will do nothing.

=item B<Params>

=over

=item C<$table> - The table the new index is on.

=item C<$name>  - A name for the new index.

=item C<$definition> - An abstract index definition. Either a hashref 
or an arrayref.

=back

=item B<Returns> (nothing)

=back

=item C<bz_add_table>

=over

=item B<Description>

Creates a new table in the database, based on the definition for that 
table in the abstract schema.

Note that unlike the other 'add' functions, this does not take a 
definition, but always creates the table as it exists in
L<Bugzilla::DB::Schema/ABSTRACT_SCHEMA>.

If a table with that name already exists, then this function returns 
silently.

=item B<Params>

=over

=item C<$name> - The name of the table you want to create.

=back

=item B<Returns> (nothing)

=back

=item C<bz_drop_index>

=over

=item B<Description>

Removes an index from the database. Prints out a brief statement that it 
did so, to stdout. If the index doesn't exist, we do nothing.

=item B<Params>

=over

=item C<$table> - The table that the index is on.

=item C<$name> - The name of the index that you want to drop.

=back

=item B<Returns> (nothing)

=back

=item C<bz_drop_table>

=over

=item B<Description>

Drops a table from the database. If the table doesn't exist, we just 
return silently.

=item B<Params>

=over

=item C<$name> - The name of the table to drop.

=back

=item B<Returns> (nothing)

=back

=item C<bz_alter_column>

=over

=item B<Description>

Changes the data type of a column in a table. Prints out the changes 
being made to stdout. If the new type is the same as the old type, 
the function returns without changing anything.

=item B<Params>

=over

=item C<$table> - the table where the column is

=item C<$name> - the name of the column you want to change

=item C<\%new_def> - An abstract column definition for the new 
data type of the columm

=item C<$set_nulls_to> (Optional) - If you are changing the column
to be NOT NULL, you probably also want to set any existing NULL columns 
to a particular value. Specify that value here. B<NOTE>: The value should 
not already be SQL-quoted.

=back

=item B<Returns> (nothing)

=back

=item C<bz_drop_column>

=over

=item B<Description>

Removes a column from a database table. If the column doesn't exist, we 
return without doing anything. If we do anything, we print a short 
message to C<stdout> about the change.

=item B<Params>

=over

=item C<$table> - The table where the column is

=item C<$column> - The name of the column you want to drop

=back

=item B<Returns> (nothing)

=back

=item C<bz_rename_column>

=over

=item B<Description>

Renames a column in a database table. If the C<$old_name> column 
doesn't exist, we return without doing anything. If C<$old_name> 
and C<$new_name> both already exist in the table specified, we fail.

=item B<Params>

=over

=item C<$table> - The name of the table containing the column 
that you want to rename

=item C<$old_name> - The current name of the column that you want to rename

=item C<$new_name> - The new name of the column

=back

=item B<Returns> (nothing)

=back

=item C<bz_rename_table>

=over

=item B<Description>

Renames a table in the database. Does nothing if the table doesn't exist.

Throws an error if the old table exists and there is already a table 
with the new name.

=item B<Params>

=over

=item C<$old_name> - The current name of the table.

=item C<$new_name> - What you're renaming the table to.

=back

=item B<Returns> (nothing)

=back

=back

=head2 Schema Information Methods

These methods return information about the current Bugzilla database
schema, as it currently exists on the disk. 

Where a parameter says "Abstract index/column definition", it returns/takes
information in the formats defined for indexes and columns for
L<Bugzilla::DB::Schema/ABSTRACT_SCHEMA>.

=over

=item C<bz_column_info>

=over

=item B<Description>

Get abstract column definition.

=item B<Params>

=over

=item C<$table> - The name of the table the column is in.

=item C<$column> - The name of the column.

=back

=item B<Returns>

An abstract column definition for that column. If the table or column 
does not exist, we return C<undef>.

=back

=item C<bz_index_info>

=over

=item B<Description>

Get abstract index definition.

=item B<Params>

=over

=item C<$table> - The table the index is on.

=item C<$index> - The name of the index.

=back

=item B<Returns>

An abstract index definition for that index, always in hashref format. 
The hashref will always contain the C<TYPE> element, but it will
be an empty string if it's just a normal index.

If the index does not exist, we return C<undef>.

=back

=back


=head2 Transaction Methods

These methods deal with the starting and stopping of transactions 
in the database.

=over

=item C<bz_start_transaction>

Starts a transaction if supported by the database being used. Returns nothing
and takes no parameters.

=item C<bz_commit_transaction>

Ends a transaction, commiting all changes, if supported by the database 
being used. Returns nothing and takes no parameters.

=item C<bz_rollback_transaction>

Ends a transaction, rolling back all changes, if supported by the database 
being used. Returns nothing and takes no parameters.

=back


=head1 SUBCLASS HELPERS

Methods in this class are intended to be used by subclasses to help them
with their functions.

=over

=item C<db_new>

=over

=item B<Description>

Constructor

=item B<Params>

=over

=item C<$dsn> - database connection string

=item C<$user> - username used to log in to the database

=item C<$pass> - password used to log in to the database

=item C<\%attributes> - set of attributes for DB connection (optional)

=back

=item B<Returns>

A new instance of the DB class

=item B<Note>

The name of this constructor is not C<new>, as that would make
our check for implementation of C<new> by derived class useless.

=back

=back


=head1 SEE ALSO

L<DBI>

L<Bugzilla::Constants/DB_MODULE>
