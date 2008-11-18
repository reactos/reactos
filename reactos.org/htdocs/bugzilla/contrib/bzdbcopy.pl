#!/usr/bin/perl -w
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
# The Initial Developer of the Original Code is Everything Solved.
# Portions created by Everything Solved are Copyright (C) 2006 
# Everything Solved. All Rights Reserved.
#
# Contributor(s): Max Kanat-Alexander <mkanat@bugzilla.org>

use strict;
use lib ".";
use Bugzilla;
use Bugzilla::DB;
use Bugzilla::Util;

#####################################################################
# User-Configurable Settings
#####################################################################

# Settings for the 'Source' DB that you are copying from.
use constant SOURCE_DB_TYPE => 'Mysql';
use constant SOURCE_DB_NAME => 'bugs';
use constant SOURCE_DB_USER => 'bugs';
use constant SOURCE_DB_PASSWORD => '';

# Settings for the 'Target' DB that you are copying to.
use constant TARGET_DB_TYPE => 'Pg';
use constant TARGET_DB_NAME => 'bugs';
use constant TARGET_DB_USER => 'bugs';
use constant TARGET_DB_PASSWORD => '';

#####################################################################
# MAIN SCRIPT
#####################################################################

print "Connecting to the '" . SOURCE_DB_NAME . "' source database on " 
      . SOURCE_DB_TYPE . "...\n";
my $source_db = Bugzilla::DB::_connect(SOURCE_DB_TYPE, 'localhost', 
    SOURCE_DB_NAME, undef, undef, SOURCE_DB_USER, SOURCE_DB_PASSWORD);

print "Connecting to the '" . TARGET_DB_NAME . "' target database on "
      . TARGET_DB_TYPE . "...\n";
my $target_db = Bugzilla::DB::_connect(TARGET_DB_TYPE, 'localhost', 
    TARGET_DB_NAME, undef, undef, TARGET_DB_USER, TARGET_DB_PASSWORD);
my $ident_char = $target_db->get_info( 29 ); # SQL_IDENTIFIER_QUOTE_CHAR

# We use the table list from the target DB, because if somebody
# has customized their source DB, we still want the script to work,
# and it may otherwise fail in that situation (that is, the tables
# may not exist in the target DB).
my @table_list = $target_db->bz_table_list_real();

# We don't want to copy over the bz_schema table's contents.
my $bz_schema_location = lsearch(\@table_list, 'bz_schema');
splice(@table_list, $bz_schema_location, 1) if $bz_schema_location > 0;

# We turn off autocommit on the target DB, because we're doing so
# much copying.
$target_db->{AutoCommit} = 0;
$target_db->{AutoCommit} == 0 
    || warn "Failed to disable autocommit on " . TARGET_DB_TYPE;
foreach my $table (@table_list) {
    my @serial_cols;
    print "Reading data from the source '$table' table on " 
          . SOURCE_DB_TYPE . "...\n";
    my @table_columns = $target_db->bz_table_columns_real($table);
    # The column names could be quoted using the quote identifier char
    # Remove these chars as different databases use different quote chars
    @table_columns = map { s/^\Q$ident_char\E?(.*?)\Q$ident_char\E?$/$1/; $_ }
                         @table_columns;

    my $select_query = "SELECT " . join(',', @table_columns) . " FROM $table";
    my $data_in = $source_db->selectall_arrayref($select_query);

    my $insert_query = "INSERT INTO $table ( " . join(',', @table_columns) 
                       . " ) VALUES (";
    $insert_query .= '?,' foreach (@table_columns);
    # Remove the last comma.
    chop($insert_query);
    $insert_query .= ")";
    my $insert_sth = $target_db->prepare($insert_query);

    print "Clearing out the target '$table' table on " 
          . TARGET_DB_TYPE . "...\n";
    $target_db->do("DELETE FROM $table");
    
    print "Writing data to the target '$table' table on " 
          . TARGET_DB_TYPE . "...";
    foreach my $row (@$data_in) {
        # Each column needs to be bound separately, because
        # many columns need to be dealt with specially.
        my $colnum = 0;
        foreach my $column (@table_columns) {
            # bind_param args start at 1, but arrays start at 0.
            my $param_num = $colnum + 1;
            my $already_bound;

            # Certain types of columns need special handling.
            my $col_info = $source_db->bz_column_info($table, $column);
            if ($col_info && $col_info->{TYPE} eq 'LONGBLOB') {
                $insert_sth->bind_param($param_num, 
                    $row->[$colnum], $target_db->BLOB_TYPE);
                $already_bound = 1;
            }
            elsif ($col_info && $col_info->{TYPE} =~ /decimal/) {
                # In MySQL, decimal cols can be too long.
                my $col_type = $col_info->{TYPE};
                $col_type =~ /decimal\((\d+),(\d+)\)/;
                my ($precision, $decimals) = ($1, $2);
                # If it's longer than precision + decimal point
                if ( length($row->[$colnum]) > ($precision + 1) ) {
                    # Truncate it to the highest allowed value.
                    my $orig_value = $row->[$colnum];
                    $row->[$colnum] = '';
                    my $non_decimal = $precision - $decimals;
                    $row->[$colnum] .= '9' while ($non_decimal--);
                    $row->[$colnum] .= '.';
                    $row->[$colnum] .= '9' while ($decimals--);
                    print "Truncated value $orig_value to " . $row->[$colnum] 
                         . " for $table.$column.\n";
                }
            }
            elsif ($col_info && $col_info->{TYPE} =~ /DATETIME/i) {
                my $date = $row->[$colnum];
                # MySQL can have strange invalid values for Datetimes.
                $row->[$colnum] = '1901-01-01 00:00:00'
                    if $date && $date eq '0000-00-00 00:00:00';
            }

            $insert_sth->bind_param($param_num, $row->[$colnum])
                unless $already_bound;
            $colnum++;
        }

        $insert_sth->execute();
    }

    # PostgreSQL doesn't like it when you insert values into
    # a serial field; it doesn't increment the counter 
    # automatically.
    if ($target_db->isa('Bugzilla::DB::Pg')) {
        foreach my $column (@table_columns) {
            my $col_info = $source_db->bz_column_info($table, $column);
            if ($col_info && $col_info->{TYPE} =~ /SERIAL/i) {
                # Set the sequence to the current max value + 1.
                my ($max_val) = $target_db->selectrow_array(
                    "SELECT MAX($column) FROM $table");
                $max_val = 0 if !defined $max_val;
                $max_val++;
                print "\nSetting the next value for $table.$column to $max_val.";
                $target_db->do("SELECT pg_catalog.setval 
                                ('${table}_${column}_seq', $max_val, false)");
            }
        }
    }

    print "\n\n";
}

print "Committing changes to the target database...\n";
$target_db->commit;

print "All done! Make sure to run checksetup on the new DB.\n";
$source_db->disconnect;
$target_db->disconnect;

1;

__END__

=head1 NAME

bzdbcopy.pl - Copies data from one Bugzilla database to another. 

=head1 DESCRIPTION

The intended use of this script is to copy data from an installation
running on one DB platform to an installation running on another
DB platform.

It must be run from the directory containing your Bugzilla installation.
That means if this script is in the contrib/ directory, you should
be running it as: C<./contrib/bzdbcopy.pl>

Note: Both schemas must already exist and be B<IDENTICAL>. (That is, 
they must have both been created/updated by the same version of 
checksetup.pl.) This script will B<DESTROY ALL CURRENT DATA> in the 
target database.

Both Schemas must be at least from Bugzilla 2.19.3, but if you're
running a Bugzilla from before 2.20rc2, you'll need the patch at:
L<http://bugzilla.mozilla.org/show_bug.cgi?id=300311> in order to
be able to run this script.

Before you using it, you have to correctly set all the variables
in the "User-Configurable Settings" section at the top of the script. 
The C<SOURCE> settings are for the database you're copying from, and 
the C<TARGET> settings are for the database you're copying to. The 
C<DB_TYPE> is the name of a DB driver from the F<Bugzilla/DB/> directory.

