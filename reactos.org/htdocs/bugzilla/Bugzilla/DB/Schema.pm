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
#                 Lance Larsh <lance.larsh@oracle.com>
#                 Dennis Melentyev <dennis.melentyev@infopulse.com.ua>
#                 Akamai Technologies <bugzilla-dev@akamai.com>

package Bugzilla::DB::Schema;

###########################################################################
#
# Purpose: Object-oriented, DBMS-independent database schema for Bugzilla
#
# This is the base class implementing common methods and abstract schema.
#
###########################################################################

use strict;
use Bugzilla::Error;
use Bugzilla::Hook;
use Bugzilla::Util;
use Bugzilla::Constants;

use Hash::Util qw(lock_value unlock_hash lock_keys unlock_keys);
use Safe;
# Historical, needed for SCHEMA_VERSION = '1.00'
use Storable qw(dclone freeze thaw);

# New SCHEMA_VERSION (2.00) use this
use Data::Dumper;

=head1 NAME

Bugzilla::DB::Schema - Abstract database schema for Bugzilla

=head1 SYNOPSIS

  # Obtain MySQL database schema.
  # Do not do this. Use Bugzilla::DB instead.
  use Bugzilla::DB::Schema;
  my $mysql_schema = new Bugzilla::DB::Schema('Mysql');

  # Recommended way to obtain database schema.
  use Bugzilla::DB;
  my $dbh = Bugzilla->dbh;
  my $schema = $dbh->_bz_schema();

  # Get the list of tables in the Bugzilla database.
  my @tables = $schema->get_table_list();

  # Get the SQL statements need to create the bugs table.
  my @statements = $schema->get_table_ddl('bugs');

  # Get the database-specific SQL data type used to implement
  # the abstract data type INT1.
  my $db_specific_type = $schema->sql_type('INT1');

=head1 DESCRIPTION

This module implements an object-oriented, abstract database schema.
It should be considered package-private to the Bugzilla::DB module.
That means that CGI scripts should never call any function in this
module directly, but should instead rely on methods provided by
Bugzilla::DB.

=head1 NEW TO SCHEMA.PM?

If this is your first time looking at Schema.pm, especially if
you are making changes to the database, please take a look at
L<http://www.bugzilla.org/docs/developer.html#sql-schema> to learn
more about how this integrates into the rest of Bugzilla.

=cut

#--------------------------------------------------------------------------
# Define the Bugzilla abstract database schema and version as constants.

=head1 CONSTANTS

=over 4

=item C<SCHEMA_VERSION>

The 'version' of the internal schema structure. This version number
is incremented every time the the fundamental structure of Schema
internals changes.

This is NOT changed every time a table or a column is added. This 
number is incremented only if the internal structures of this 
Schema would be incompatible with the internal structures of a 
previous Schema version.

In general, unless you are messing around with serialization
and deserialization of the schema, you don't need to worry about
this constant.

=begin private

An example of the use of the version number:

Today, we store all individual columns like this:

column_name => { TYPE => 'SOMETYPE', NOTNULL => 1 }

Imagine that someday we decide that NOTNULL => 1 is bad, and we want
to change it so that the schema instead uses NULL => 0.

But we have a bunch of Bugzilla installations around the world with a
serialized schema that has NOTNULL in it! When we deserialize that 
structure, it just WILL NOT WORK properly inside of our new Schema object.
So, immediately after deserializing, we need to go through the hash 
and change all NOTNULLs to NULLs and so on.

We know that we need to do that on deserializing because we know that
version 1.00 used NOTNULL. Having made the change to NULL, we would now
be version 1.01.

=end private

=item C<ABSTRACT_SCHEMA>

The abstract database schema structure consists of a hash reference
in which each key is the name of a table in the Bugzilla database.

The value for each key is a hash reference containing the keys
C<FIELDS> and C<INDEXES> which in turn point to array references
containing information on the table's fields and indexes. 

A field hash reference should must contain the key C<TYPE>. Optional field
keys include C<PRIMARYKEY>, C<NOTNULL>, and C<DEFAULT>. 

The C<INDEXES> array reference contains index names and information 
regarding the index. If the index name points to an array reference,
then the index is a regular index and the array contains the indexed
columns. If the index name points to a hash reference, then the hash
must contain the key C<FIELDS>. It may also contain the key C<TYPE>,
which can be used to specify the type of index such as UNIQUE or FULLTEXT.

=back

=cut

use constant SCHEMA_VERSION  => '2.00';
use constant ABSTRACT_SCHEMA => {

    # BUG-RELATED TABLES
    # ------------------

    # General Bug Information
    # -----------------------
    bugs => {
        FIELDS => [
            bug_id              => {TYPE => 'MEDIUMSERIAL', NOTNULL => 1,
                                    PRIMARYKEY => 1},
            assigned_to         => {TYPE => 'INT3', NOTNULL => 1},
            bug_file_loc        => {TYPE => 'TEXT'},
            bug_severity        => {TYPE => 'varchar(64)', NOTNULL => 1},
            bug_status          => {TYPE => 'varchar(64)', NOTNULL => 1},
            creation_ts         => {TYPE => 'DATETIME'},
            delta_ts            => {TYPE => 'DATETIME', NOTNULL => 1},
            short_desc          => {TYPE => 'varchar(255)', NOTNULL => 1},
            op_sys              => {TYPE => 'varchar(64)', NOTNULL => 1},
            priority            => {TYPE => 'varchar(64)', NOTNULL => 1},
            product_id          => {TYPE => 'INT2', NOTNULL => 1},
            rep_platform        => {TYPE => 'varchar(64)', NOTNULL => 1},
            reporter            => {TYPE => 'INT3', NOTNULL => 1},
            version             => {TYPE => 'varchar(64)', NOTNULL => 1},
            component_id        => {TYPE => 'INT2', NOTNULL => 1},
            resolution          => {TYPE => 'varchar(64)',
                                    NOTNULL => 1, DEFAULT => "''"},
            target_milestone    => {TYPE => 'varchar(20)',
                                    NOTNULL => 1, DEFAULT => "'---'"},
            qa_contact          => {TYPE => 'INT3'},
            status_whiteboard   => {TYPE => 'MEDIUMTEXT', NOTNULL => 1,
                                    DEFAULT => "''"},
            votes               => {TYPE => 'INT3', NOTNULL => 1,
                                    DEFAULT => '0'},
            # Note: keywords field is only a cache; the real data
            # comes from the keywords table
            keywords            => {TYPE => 'MEDIUMTEXT', NOTNULL => 1,
                                    DEFAULT => "''"},
            lastdiffed          => {TYPE => 'DATETIME'},
            everconfirmed       => {TYPE => 'BOOLEAN', NOTNULL => 1},
            reporter_accessible => {TYPE => 'BOOLEAN',
                                    NOTNULL => 1, DEFAULT => 'TRUE'},
            cclist_accessible   => {TYPE => 'BOOLEAN',
                                    NOTNULL => 1, DEFAULT => 'TRUE'},
            estimated_time      => {TYPE => 'decimal(5,2)',
                                    NOTNULL => 1, DEFAULT => '0'},
            remaining_time      => {TYPE => 'decimal(5,2)',
                                    NOTNULL => 1, DEFAULT => '0'},
            deadline            => {TYPE => 'DATETIME'},
            alias               => {TYPE => 'varchar(20)'},
        ],
        INDEXES => [
            bugs_alias_idx            => {FIELDS => ['alias'],
                                          TYPE => 'UNIQUE'},
            bugs_assigned_to_idx      => ['assigned_to'],
            bugs_creation_ts_idx      => ['creation_ts'],
            bugs_delta_ts_idx         => ['delta_ts'],
            bugs_bug_severity_idx     => ['bug_severity'],
            bugs_bug_status_idx       => ['bug_status'],
            bugs_op_sys_idx           => ['op_sys'],
            bugs_priority_idx         => ['priority'],
            bugs_product_id_idx       => ['product_id'],
            bugs_reporter_idx         => ['reporter'],
            bugs_version_idx          => ['version'],
            bugs_component_id_idx     => ['component_id'],
            bugs_resolution_idx       => ['resolution'],
            bugs_target_milestone_idx => ['target_milestone'],
            bugs_qa_contact_idx       => ['qa_contact'],
            bugs_votes_idx            => ['votes'],
        ],
    },

    bugs_activity => {
        FIELDS => [
            bug_id    => {TYPE => 'INT3', NOTNULL => 1},
            attach_id => {TYPE => 'INT3'},
            who       => {TYPE => 'INT3', NOTNULL => 1},
            bug_when  => {TYPE => 'DATETIME', NOTNULL => 1},
            fieldid   => {TYPE => 'INT3', NOTNULL => 1},
            added     => {TYPE => 'TINYTEXT'},
            removed   => {TYPE => 'TINYTEXT'},
        ],
        INDEXES => [
            bugs_activity_bug_id_idx  => ['bug_id'],
            bugs_activity_who_idx     => ['who'],
            bugs_activity_bug_when_idx => ['bug_when'],
            bugs_activity_fieldid_idx => ['fieldid'],
        ],
    },

    cc => {
        FIELDS => [
            bug_id => {TYPE => 'INT3', NOTNULL => 1},
            who    => {TYPE => 'INT3', NOTNULL => 1},
        ],
        INDEXES => [
            cc_bug_id_idx => {FIELDS => [qw(bug_id who)],
                              TYPE => 'UNIQUE'},
            cc_who_idx    => ['who'],
        ],
    },

    longdescs => {
        FIELDS => [
            comment_id      => {TYPE => 'MEDIUMSERIAL',  NOTNULL => 1,
                                PRIMARYKEY => 1},
            bug_id          => {TYPE => 'INT3',  NOTNULL => 1},
            who             => {TYPE => 'INT3', NOTNULL => 1},
            bug_when        => {TYPE => 'DATETIME', NOTNULL => 1},
            work_time       => {TYPE => 'decimal(5,2)', NOTNULL => 1,
                                DEFAULT => '0'},
            thetext         => {TYPE => 'MEDIUMTEXT', NOTNULL => 1},
            isprivate       => {TYPE => 'BOOLEAN', NOTNULL => 1,
                                DEFAULT => 'FALSE'},
            already_wrapped => {TYPE => 'BOOLEAN', NOTNULL => 1,
                                DEFAULT => 'FALSE'},
            type            => {TYPE => 'INT2', NOTNULL => 1,
                                DEFAULT => '0'},
            extra_data      => {TYPE => 'varchar(255)'}
        ],
        INDEXES => [
            longdescs_bug_id_idx   => ['bug_id'],
            longdescs_who_idx     => [qw(who bug_id)],
            longdescs_bug_when_idx => ['bug_when'],
            longdescs_thetext_idx => {FIELDS => ['thetext'],
                                      TYPE => 'FULLTEXT'},
        ],
    },

    dependencies => {
        FIELDS => [
            blocked   => {TYPE => 'INT3', NOTNULL => 1},
            dependson => {TYPE => 'INT3', NOTNULL => 1},
        ],
        INDEXES => [
            dependencies_blocked_idx   => ['blocked'],
            dependencies_dependson_idx => ['dependson'],
        ],
    },

    votes => {
        FIELDS => [
            who        => {TYPE => 'INT3', NOTNULL => 1},
            bug_id     => {TYPE => 'INT3', NOTNULL => 1},
            vote_count => {TYPE => 'INT2', NOTNULL => 1},
        ],
        INDEXES => [
            votes_who_idx    => ['who'],
            votes_bug_id_idx => ['bug_id'],
        ],
    },

    attachments => {
        FIELDS => [
            attach_id    => {TYPE => 'MEDIUMSERIAL', NOTNULL => 1,
                             PRIMARYKEY => 1},
            bug_id       => {TYPE => 'INT3', NOTNULL => 1},
            creation_ts  => {TYPE => 'DATETIME', NOTNULL => 1},
            description  => {TYPE => 'MEDIUMTEXT', NOTNULL => 1},
            mimetype     => {TYPE => 'MEDIUMTEXT', NOTNULL => 1},
            ispatch      => {TYPE => 'BOOLEAN'},
            filename     => {TYPE => 'varchar(100)', NOTNULL => 1},
            submitter_id => {TYPE => 'INT3', NOTNULL => 1},
            isobsolete   => {TYPE => 'BOOLEAN', NOTNULL => 1,
                             DEFAULT => 'FALSE'},
            isprivate    => {TYPE => 'BOOLEAN', NOTNULL => 1,
                             DEFAULT => 'FALSE'},
            isurl        => {TYPE => 'BOOLEAN', NOTNULL => 1,
                             DEFAULT => 'FALSE'},
        ],
        INDEXES => [
            attachments_bug_id_idx => ['bug_id'],
            attachments_creation_ts_idx => ['creation_ts'],
            attachments_submitter_id_idx => ['submitter_id', 'bug_id'],
        ],
    },
    attach_data => {
        FIELDS => [
            id      => {TYPE => 'INT3', NOTNULL => 1,
                        PRIMARYKEY => 1},
            thedata => {TYPE => 'LONGBLOB', NOTNULL => 1},
        ],
    },

    duplicates => {
        FIELDS => [
            dupe_of => {TYPE => 'INT3', NOTNULL => 1},
            dupe    => {TYPE => 'INT3', NOTNULL => 1,
                       PRIMARYKEY => 1},
        ],
    },

    # Keywords
    # --------

    keyworddefs => {
        FIELDS => [
            id          => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                            PRIMARYKEY => 1},
            name        => {TYPE => 'varchar(64)', NOTNULL => 1},
            description => {TYPE => 'MEDIUMTEXT'},
        ],
        INDEXES => [
            keyworddefs_name_idx   => {FIELDS => ['name'],
                                       TYPE => 'UNIQUE'},
        ],
    },

    keywords => {
        FIELDS => [
            bug_id    => {TYPE => 'INT3', NOTNULL => 1},
            keywordid => {TYPE => 'INT2', NOTNULL => 1},
        ],
        INDEXES => [
            keywords_bug_id_idx    => {FIELDS => [qw(bug_id keywordid)],
                                       TYPE => 'UNIQUE'},
            keywords_keywordid_idx => ['keywordid'],
        ],
    },

    # Flags
    # -----

    # "flags" stores one record for each flag on each bug/attachment.
    flags => {
        FIELDS => [
            id                => {TYPE => 'MEDIUMSERIAL', NOTNULL => 1,
                                  PRIMARYKEY => 1},
            type_id           => {TYPE => 'INT2', NOTNULL => 1},
            status            => {TYPE => 'char(1)', NOTNULL => 1},
            bug_id            => {TYPE => 'INT3', NOTNULL => 1},
            attach_id         => {TYPE => 'INT3'},
            creation_date     => {TYPE => 'DATETIME', NOTNULL => 1},
            modification_date => {TYPE => 'DATETIME'},
            setter_id         => {TYPE => 'INT3'},
            requestee_id      => {TYPE => 'INT3'},
        ],
        INDEXES => [
            flags_bug_id_idx       => [qw(bug_id attach_id)],
            flags_setter_id_idx    => ['setter_id'],
            flags_requestee_id_idx => ['requestee_id'],
            flags_type_id_idx      => ['type_id'],
        ],
    },

    # "flagtypes" defines the types of flags that can be set.
    flagtypes => {
        FIELDS => [
            id               => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                                 PRIMARYKEY => 1},
            name             => {TYPE => 'varchar(50)', NOTNULL => 1},
            description      => {TYPE => 'TEXT'},
            cc_list          => {TYPE => 'varchar(200)'},
            target_type      => {TYPE => 'char(1)', NOTNULL => 1,
                                 DEFAULT => "'b'"},
            is_active        => {TYPE => 'BOOLEAN', NOTNULL => 1,
                                 DEFAULT => 'TRUE'},
            is_requestable   => {TYPE => 'BOOLEAN', NOTNULL => 1,
                                 DEFAULT => 'FALSE'},
            is_requesteeble  => {TYPE => 'BOOLEAN', NOTNULL => 1,
                                 DEFAULT => 'FALSE'},
            is_multiplicable => {TYPE => 'BOOLEAN', NOTNULL => 1,
                                 DEFAULT => 'FALSE'},
            sortkey          => {TYPE => 'INT2', NOTNULL => 1,
                                 DEFAULT => '0'},
            grant_group_id   => {TYPE => 'INT3'},
            request_group_id => {TYPE => 'INT3'},
        ],
    },

    # "flaginclusions" and "flagexclusions" specify the products/components
    #     a bug/attachment must belong to in order for flags of a given type
    #     to be set for them.
    flaginclusions => {
        FIELDS => [
            type_id      => {TYPE => 'INT2', NOTNULL => 1},
            product_id   => {TYPE => 'INT2'},
            component_id => {TYPE => 'INT2'},
        ],
        INDEXES => [
            flaginclusions_type_id_idx =>
                [qw(type_id product_id component_id)],
        ],
    },

    flagexclusions => {
        FIELDS => [
            type_id      => {TYPE => 'INT2', NOTNULL => 1},
            product_id   => {TYPE => 'INT2'},
            component_id => {TYPE => 'INT2'},
        ],
        INDEXES => [
            flagexclusions_type_id_idx =>
                [qw(type_id product_id component_id)],
        ],
    },

    # General Field Information
    # -------------------------

    fielddefs => {
        FIELDS => [
            id          => {TYPE => 'MEDIUMSERIAL', NOTNULL => 1,
                            PRIMARYKEY => 1},
            name        => {TYPE => 'varchar(64)', NOTNULL => 1},
            type        => {TYPE => 'INT2', NOTNULL => 1,
                            DEFAULT => FIELD_TYPE_UNKNOWN},
            custom      => {TYPE => 'BOOLEAN', NOTNULL => 1,
                            DEFAULT => 'FALSE'},
            description => {TYPE => 'MEDIUMTEXT', NOTNULL => 1},
            mailhead    => {TYPE => 'BOOLEAN', NOTNULL => 1,
                            DEFAULT => 'FALSE'},
            sortkey     => {TYPE => 'INT2', NOTNULL => 1},
            obsolete    => {TYPE => 'BOOLEAN', NOTNULL => 1,
                            DEFAULT => 'FALSE'},
            enter_bug   => {TYPE => 'BOOLEAN', NOTNULL => 1,
                            DEFAULT => 'FALSE'},
        ],
        INDEXES => [
            fielddefs_name_idx    => {FIELDS => ['name'],
                                      TYPE => 'UNIQUE'},
            fielddefs_sortkey_idx => ['sortkey'],
        ],
    },

    # Per-product Field Values
    # ------------------------

    versions => {
        FIELDS => [
            id         =>  {TYPE => 'MEDIUMSERIAL', NOTNULL => 1,
                            PRIMARYKEY => 1},
            value      =>  {TYPE => 'varchar(64)', NOTNULL => 1},
            product_id =>  {TYPE => 'INT2', NOTNULL => 1},
        ],
        INDEXES => [
            versions_product_id_idx => {FIELDS => [qw(product_id value)],
                                        TYPE => 'UNIQUE'},
        ],
    },

    milestones => {
        FIELDS => [
            id         => {TYPE => 'MEDIUMSERIAL', NOTNULL => 1, 
                           PRIMARYKEY => 1},
            product_id => {TYPE => 'INT2', NOTNULL => 1},
            value      => {TYPE => 'varchar(20)', NOTNULL => 1},
            sortkey    => {TYPE => 'INT2', NOTNULL => 1,
                           DEFAULT => 0},
        ],
        INDEXES => [
            milestones_product_id_idx => {FIELDS => [qw(product_id value)],
                                          TYPE => 'UNIQUE'},
        ],
    },

    # Global Field Values
    # -------------------

    bug_status => {
        FIELDS => [
            id       => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                         PRIMARYKEY => 1},
            value    => {TYPE => 'varchar(64)', NOTNULL => 1},
            sortkey  => {TYPE => 'INT2', NOTNULL => 1, DEFAULT => 0},
            isactive => {TYPE => 'BOOLEAN', NOTNULL => 1, 
                         DEFAULT => 'TRUE'},
        ],
        INDEXES => [
            bug_status_value_idx  => {FIELDS => ['value'],
                                       TYPE => 'UNIQUE'},
            bug_status_sortkey_idx => ['sortkey', 'value'],
        ],
    },

    resolution => {
        FIELDS => [
            id       => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                         PRIMARYKEY => 1},
            value    => {TYPE => 'varchar(64)', NOTNULL => 1},
            sortkey  => {TYPE => 'INT2', NOTNULL => 1, DEFAULT => 0},
            isactive => {TYPE => 'BOOLEAN', NOTNULL => 1, 
                         DEFAULT => 'TRUE'},
        ],
        INDEXES => [
            resolution_value_idx   => {FIELDS => ['value'],
                                       TYPE => 'UNIQUE'},
            resolution_sortkey_idx => ['sortkey', 'value'],
        ],
    },

    bug_severity => {
        FIELDS => [
            id       => {TYPE => 'SMALLSERIAL', NOTNULL => 1, 
                         PRIMARYKEY => 1},
            value    => {TYPE => 'varchar(64)', NOTNULL => 1},
            sortkey  => {TYPE => 'INT2', NOTNULL => 1, DEFAULT => 0},
            isactive => {TYPE => 'BOOLEAN', NOTNULL => 1, 
                         DEFAULT => 'TRUE'},
        ],
        INDEXES => [
            bug_severity_value_idx   => {FIELDS => ['value'],
                                         TYPE => 'UNIQUE'},
            bug_severity_sortkey_idx => ['sortkey', 'value'],
        ],
    },

    priority => {
        FIELDS => [
            id       => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                         PRIMARYKEY => 1},
            value    => {TYPE => 'varchar(64)', NOTNULL => 1},
            sortkey  => {TYPE => 'INT2', NOTNULL => 1, DEFAULT => 0},
            isactive => {TYPE => 'BOOLEAN', NOTNULL => 1, 
                         DEFAULT => 'TRUE'},
        ],
        INDEXES => [
            priority_value_idx   => {FIELDS => ['value'],
                                     TYPE => 'UNIQUE'},
            priority_sortkey_idx => ['sortkey', 'value'],
        ],
    },

    rep_platform => {
        FIELDS => [
            id       => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                         PRIMARYKEY => 1},
            value    => {TYPE => 'varchar(64)', NOTNULL => 1},
            sortkey  => {TYPE => 'INT2', NOTNULL => 1, DEFAULT => 0},
            isactive => {TYPE => 'BOOLEAN', NOTNULL => 1, 
                         DEFAULT => 'TRUE'},
        ],
        INDEXES => [
            rep_platform_value_idx   => {FIELDS => ['value'],
                                         TYPE => 'UNIQUE'},
            rep_platform_sortkey_idx => ['sortkey', 'value'],
        ],
    },

    op_sys => {
        FIELDS => [
            id       => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                         PRIMARYKEY => 1},
            value    => {TYPE => 'varchar(64)', NOTNULL => 1},
            sortkey  => {TYPE => 'INT2', NOTNULL => 1, DEFAULT => 0},
            isactive => {TYPE => 'BOOLEAN', NOTNULL => 1, 
                         DEFAULT => 'TRUE'},
        ],
        INDEXES => [
            op_sys_value_idx   => {FIELDS => ['value'],
                                   TYPE => 'UNIQUE'},
            op_sys_sortkey_idx => ['sortkey', 'value'],
        ],
    },

    # USER INFO
    # ---------

    # General User Information
    # ------------------------

    profiles => {
        FIELDS => [
            userid         => {TYPE => 'MEDIUMSERIAL', NOTNULL => 1,
                               PRIMARYKEY => 1},
            login_name     => {TYPE => 'varchar(255)', NOTNULL => 1},
            cryptpassword  => {TYPE => 'varchar(128)'},
            realname       => {TYPE => 'varchar(255)', NOTNULL => 1,
                               DEFAULT => "''"},
            disabledtext   => {TYPE => 'MEDIUMTEXT', NOTNULL => 1,
                               DEFAULT => "''"},
            disable_mail   => {TYPE => 'BOOLEAN', NOTNULL => 1,
                               DEFAULT => 'FALSE'},
            mybugslink     => {TYPE => 'BOOLEAN', NOTNULL => 1,
                               DEFAULT => 'TRUE'},
            extern_id      => {TYPE => 'varchar(64)'},
        ],
        INDEXES => [
            profiles_login_name_idx => {FIELDS => ['login_name'],
                                        TYPE => 'UNIQUE'},
        ],
    },

    profiles_activity => {
        FIELDS => [
            userid        => {TYPE => 'INT3', NOTNULL => 1},
            who           => {TYPE => 'INT3', NOTNULL => 1},
            profiles_when => {TYPE => 'DATETIME', NOTNULL => 1},
            fieldid       => {TYPE => 'INT3', NOTNULL => 1},
            oldvalue      => {TYPE => 'TINYTEXT'},
            newvalue      => {TYPE => 'TINYTEXT'},
        ],
        INDEXES => [
            profiles_activity_userid_idx  => ['userid'],
            profiles_activity_profiles_when_idx => ['profiles_when'],
            profiles_activity_fieldid_idx => ['fieldid'],
        ],
    },

    email_setting => {
        FIELDS => [
            user_id      => {TYPE => 'INT3', NOTNULL => 1},
            relationship => {TYPE => 'INT1', NOTNULL => 1},
            event        => {TYPE => 'INT1', NOTNULL => 1},
        ],
        INDEXES => [
            email_setting_user_id_idx  =>
                                    {FIELDS => [qw(user_id relationship event)],
                                     TYPE => 'UNIQUE'},
        ],
    },

    watch => {
        FIELDS => [
            watcher => {TYPE => 'INT3', NOTNULL => 1},
            watched => {TYPE => 'INT3', NOTNULL => 1},
        ],
        INDEXES => [
            watch_watcher_idx => {FIELDS => [qw(watcher watched)],
                                  TYPE => 'UNIQUE'},
            watch_watched_idx => ['watched'],
        ],
    },

    namedqueries => {
        FIELDS => [
            id           => {TYPE => 'MEDIUMSERIAL', NOTNULL => 1,
                             PRIMARYKEY => 1},
            userid       => {TYPE => 'INT3', NOTNULL => 1},
            name         => {TYPE => 'varchar(64)', NOTNULL => 1},
            query        => {TYPE => 'MEDIUMTEXT', NOTNULL => 1},
            query_type   => {TYPE => 'BOOLEAN', NOTNULL => 1},
        ],
        INDEXES => [
            namedqueries_userid_idx => {FIELDS => [qw(userid name)],
                                        TYPE => 'UNIQUE'},
        ],
    },

    namedqueries_link_in_footer => {
        FIELDS => [
            namedquery_id => {TYPE => 'INT3', NOTNULL => 1},
            user_id       => {TYPE => 'INT3', NOTNULL => 1},
        ],
        INDEXES => [
            namedqueries_link_in_footer_id_idx => {FIELDS => [qw(namedquery_id user_id)],
                                                   TYPE => 'UNIQUE'},
            namedqueries_link_in_footer_userid_idx => ['user_id'],
        ],
    },

    component_cc => {

        FIELDS => [
            user_id      => {TYPE => 'INT3', NOTNULL => 1},
            component_id => {TYPE => 'INT2', NOTNULL => 1},
        ],
        INDEXES => [
            component_cc_user_id_idx => {FIELDS => [qw(component_id user_id)],
                                         TYPE => 'UNIQUE'},
        ],
    },

    # Authentication
    # --------------

    logincookies => {
        FIELDS => [
            cookie   => {TYPE => 'varchar(16)', NOTNULL => 1,
                         PRIMARYKEY => 1},
            userid   => {TYPE => 'INT3', NOTNULL => 1},
            ipaddr   => {TYPE => 'varchar(40)', NOTNULL => 1},
            lastused => {TYPE => 'DATETIME', NOTNULL => 1},
        ],
        INDEXES => [
            logincookies_lastused_idx => ['lastused'],
        ],
    },

    # "tokens" stores the tokens users receive when a password or email
    #     change is requested.  Tokens provide an extra measure of security
    #     for these changes.
    tokens => {
        FIELDS => [
            userid    => {TYPE => 'INT3'},
            issuedate => {TYPE => 'DATETIME', NOTNULL => 1} ,
            token     => {TYPE => 'varchar(16)', NOTNULL => 1,
                          PRIMARYKEY => 1},
            tokentype => {TYPE => 'varchar(8)', NOTNULL => 1} ,
            eventdata => {TYPE => 'TINYTEXT'},
        ],
        INDEXES => [
            tokens_userid_idx => ['userid'],
        ],
    },

    # GROUPS
    # ------

    groups => {
        FIELDS => [
            id           => {TYPE => 'MEDIUMSERIAL', NOTNULL => 1,
                             PRIMARYKEY => 1},
            name         => {TYPE => 'varchar(255)', NOTNULL => 1},
            description  => {TYPE => 'TEXT', NOTNULL => 1},
            isbuggroup   => {TYPE => 'BOOLEAN', NOTNULL => 1},
            userregexp   => {TYPE => 'TINYTEXT', NOTNULL => 1,
                             DEFAULT => "''"},
            isactive     => {TYPE => 'BOOLEAN', NOTNULL => 1,
                             DEFAULT => 'TRUE'},
        ],
        INDEXES => [
            groups_name_idx => {FIELDS => ['name'], TYPE => 'UNIQUE'},
        ],
    },

    group_control_map => {
        FIELDS => [
            group_id      => {TYPE => 'INT3', NOTNULL => 1},
            product_id    => {TYPE => 'INT3', NOTNULL => 1},
            entry         => {TYPE => 'BOOLEAN', NOTNULL => 1},
            membercontrol => {TYPE => 'BOOLEAN', NOTNULL => 1},
            othercontrol  => {TYPE => 'BOOLEAN', NOTNULL => 1},
            canedit       => {TYPE => 'BOOLEAN', NOTNULL => 1},
            editcomponents => {TYPE => 'BOOLEAN', NOTNULL => 1,
                               DEFAULT => 'FALSE'},
            editbugs      => {TYPE => 'BOOLEAN', NOTNULL => 1,
                              DEFAULT => 'FALSE'},
            canconfirm    => {TYPE => 'BOOLEAN', NOTNULL => 1,
                              DEFAULT => 'FALSE'},
        ],
        INDEXES => [
            group_control_map_product_id_idx =>
            {FIELDS => [qw(product_id group_id)], TYPE => 'UNIQUE'},
            group_control_map_group_id_idx    => ['group_id'],
        ],
    },

    # "user_group_map" determines the groups that a user belongs to
    # directly or due to regexp and which groups can be blessed by a user.
    #
    # grant_type:
    # if GRANT_DIRECT - record was explicitly granted
    # if GRANT_DERIVED - record was derived from expanding a group hierarchy
    # if GRANT_REGEXP - record was created by evaluating a regexp
    user_group_map => {
        FIELDS => [
            user_id    => {TYPE => 'INT3', NOTNULL => 1},
            group_id   => {TYPE => 'INT3', NOTNULL => 1},
            isbless    => {TYPE => 'BOOLEAN', NOTNULL => 1,
                           DEFAULT => 'FALSE'},
            grant_type => {TYPE => 'INT1', NOTNULL => 1,
                           DEFAULT => GRANT_DIRECT},
        ],
        INDEXES => [
            user_group_map_user_id_idx =>
                {FIELDS => [qw(user_id group_id grant_type isbless)],
                 TYPE => 'UNIQUE'},
        ],
    },

    # This table determines which groups are made a member of another
    # group, given the ability to bless another group, or given
    # visibility to another groups existence and membership
    # grant_type:
    # if GROUP_MEMBERSHIP - member groups are made members of grantor
    # if GROUP_BLESS - member groups may grant membership in grantor
    # if GROUP_VISIBLE - member groups may see grantor group
    group_group_map => {
        FIELDS => [
            member_id  => {TYPE => 'INT3', NOTNULL => 1},
            grantor_id => {TYPE => 'INT3', NOTNULL => 1},
            grant_type => {TYPE => 'INT1', NOTNULL => 1,
                           DEFAULT => GROUP_MEMBERSHIP},
        ],
        INDEXES => [
            group_group_map_member_id_idx =>
                {FIELDS => [qw(member_id grantor_id grant_type)],
                 TYPE => 'UNIQUE'},
        ],
    },

    # This table determines which groups a user must be a member of
    # in order to see a bug.
    bug_group_map => {
        FIELDS => [
            bug_id   => {TYPE => 'INT3', NOTNULL => 1},
            group_id => {TYPE => 'INT3', NOTNULL => 1},
        ],
        INDEXES => [
            bug_group_map_bug_id_idx   =>
                {FIELDS => [qw(bug_id group_id)], TYPE => 'UNIQUE'},
            bug_group_map_group_id_idx => ['group_id'],
        ],
    },

    # This table determines which groups a user must be a member of
    # in order to see a named query somebody else shares.
    namedquery_group_map => {
        FIELDS => [
            namedquery_id => {TYPE => 'INT3', NOTNULL => 1},
            group_id      => {TYPE => 'INT3', NOTNULL => 1},
        ],
        INDEXES => [
            namedquery_group_map_namedquery_id_idx   =>
                {FIELDS => [qw(namedquery_id)], TYPE => 'UNIQUE'},
            namedquery_group_map_group_id_idx => ['group_id'],
        ],
    },

    category_group_map => {
        FIELDS => [
            category_id => {TYPE => 'INT2', NOTNULL => 1},
            group_id    => {TYPE => 'INT3', NOTNULL => 1},
        ],
        INDEXES => [
            category_group_map_category_id_idx =>
                {FIELDS => [qw(category_id group_id)], TYPE => 'UNIQUE'},
        ],
    },


    # PRODUCTS
    # --------

    classifications => {
        FIELDS => [
            id          => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                            PRIMARYKEY => 1},
            name        => {TYPE => 'varchar(64)', NOTNULL => 1},
            description => {TYPE => 'MEDIUMTEXT'},
            sortkey     => {TYPE => 'INT2', NOTNULL => 1, DEFAULT => '0'},
        ],
        INDEXES => [
            classifications_name_idx => {FIELDS => ['name'],
                                           TYPE => 'UNIQUE'},
        ],
    },

    products => {
        FIELDS => [
            id                => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                                  PRIMARYKEY => 1},
            name              => {TYPE => 'varchar(64)', NOTNULL => 1},
            classification_id => {TYPE => 'INT2', NOTNULL => 1,
                                  DEFAULT => '1'},
            description       => {TYPE => 'MEDIUMTEXT'},
            milestoneurl      => {TYPE => 'TINYTEXT', NOTNULL => 1,
                                  DEFAULT => "''"},
            disallownew       => {TYPE => 'BOOLEAN', NOTNULL => 1,
                                  DEFAULT => 0},
            votesperuser      => {TYPE => 'INT2', NOTNULL => 1,
                                  DEFAULT => 0},
            maxvotesperbug    => {TYPE => 'INT2', NOTNULL => 1,
                                  DEFAULT => '10000'},
            votestoconfirm    => {TYPE => 'INT2', NOTNULL => 1,
                                  DEFAULT => 0},
            defaultmilestone  => {TYPE => 'varchar(20)',
                                  NOTNULL => 1, DEFAULT => "'---'"},
        ],
        INDEXES => [
            products_name_idx   => {FIELDS => ['name'],
                                    TYPE => 'UNIQUE'},
        ],
    },

    components => {
        FIELDS => [
            id               => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                                 PRIMARYKEY => 1},
            name             => {TYPE => 'varchar(64)', NOTNULL => 1},
            product_id       => {TYPE => 'INT2', NOTNULL => 1},
            initialowner     => {TYPE => 'INT3', NOTNULL => 1},
            initialqacontact => {TYPE => 'INT3'},
            description      => {TYPE => 'MEDIUMTEXT', NOTNULL => 1},
        ],
        INDEXES => [
            components_product_id_idx => {FIELDS => [qw(product_id name)],
                                          TYPE => 'UNIQUE'},
            components_name_idx   => ['name'],
        ],
    },

    # CHARTS
    # ------

    series => {
        FIELDS => [
            series_id   => {TYPE => 'MEDIUMSERIAL', NOTNULL => 1,
                            PRIMARYKEY => 1},
            creator     => {TYPE => 'INT3'},
            category    => {TYPE => 'INT2', NOTNULL => 1},
            subcategory => {TYPE => 'INT2', NOTNULL => 1},
            name        => {TYPE => 'varchar(64)', NOTNULL => 1},
            frequency   => {TYPE => 'INT2', NOTNULL => 1},
            last_viewed => {TYPE => 'DATETIME'},
            query       => {TYPE => 'MEDIUMTEXT', NOTNULL => 1},
            is_public   => {TYPE => 'BOOLEAN', NOTNULL => 1,
                            DEFAULT => 'FALSE'},
        ],
        INDEXES => [
            series_creator_idx  =>
                {FIELDS => [qw(creator category subcategory name)],
                 TYPE => 'UNIQUE'},
        ],
    },

    series_data => {
        FIELDS => [
            series_id    => {TYPE => 'INT3', NOTNULL => 1},
            series_date  => {TYPE => 'DATETIME', NOTNULL => 1},
            series_value => {TYPE => 'INT3', NOTNULL => 1},
        ],
        INDEXES => [
            series_data_series_id_idx =>
                {FIELDS => [qw(series_id series_date)],
                 TYPE => 'UNIQUE'},
        ],
    },

    series_categories => {
        FIELDS => [
            id   => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                     PRIMARYKEY => 1},
            name => {TYPE => 'varchar(64)', NOTNULL => 1},
        ],
        INDEXES => [
            series_categories_name_idx => {FIELDS => ['name'],
                                           TYPE => 'UNIQUE'},
        ],
    },

    # WHINE SYSTEM
    # ------------

    whine_queries => {
        FIELDS => [
            id            => {TYPE => 'MEDIUMSERIAL', PRIMARYKEY => 1,
                              NOTNULL => 1},
            eventid       => {TYPE => 'INT3', NOTNULL => 1},
            query_name    => {TYPE => 'varchar(64)', NOTNULL => 1,
                              DEFAULT => "''"},
            sortkey       => {TYPE => 'INT2', NOTNULL => 1,
                              DEFAULT => '0'},
            onemailperbug => {TYPE => 'BOOLEAN', NOTNULL => 1,
                              DEFAULT => 'FALSE'},
            title         => {TYPE => 'varchar(128)', NOTNULL => 1,
                              DEFAULT => "''"},
        ],
        INDEXES => [
            whine_queries_eventid_idx => ['eventid'],
        ],
    },

    whine_schedules => {
        FIELDS => [
            id          => {TYPE => 'MEDIUMSERIAL', PRIMARYKEY => 1,
                            NOTNULL => 1},
            eventid     => {TYPE => 'INT3', NOTNULL => 1},
            run_day     => {TYPE => 'varchar(32)'},
            run_time    => {TYPE => 'varchar(32)'},
            run_next    => {TYPE => 'DATETIME'},
            mailto      => {TYPE => 'INT3', NOTNULL => 1},
            mailto_type => {TYPE => 'INT2', NOTNULL => 1, DEFAULT => '0'},
        ],
        INDEXES => [
            whine_schedules_run_next_idx => ['run_next'],
            whine_schedules_eventid_idx  => ['eventid'],
        ],
    },

    whine_events => {
        FIELDS => [
            id           => {TYPE => 'MEDIUMSERIAL', PRIMARYKEY => 1,
                             NOTNULL => 1},
            owner_userid => {TYPE => 'INT3', NOTNULL => 1},
            subject      => {TYPE => 'varchar(128)'},
            body         => {TYPE => 'MEDIUMTEXT'},
        ],
    },

    # QUIPS
    # -----

    quips => {
        FIELDS => [
            quipid   => {TYPE => 'MEDIUMSERIAL', NOTNULL => 1,
                         PRIMARYKEY => 1},
            userid   => {TYPE => 'INT3'},
            quip     => {TYPE => 'TEXT', NOTNULL => 1},
            approved => {TYPE => 'BOOLEAN', NOTNULL => 1,
                         DEFAULT => 'TRUE'},
        ],
    },

    # SETTINGS
    # --------
    # setting          - each global setting will have exactly one entry
    #                    in this table.
    # setting_value    - stores the list of acceptable values for each
    #                    setting, and a sort index that controls the order
    #                    in which the values are displayed.
    # profile_setting  - If a user has chosen to use a value other than the
    #                    global default for a given setting, it will be
    #                    stored in this table. Note: even if a setting is
    #                    later changed so is_enabled = false, the stored
    #                    value will remain in case it is ever enabled again.
    #
    setting => {
        FIELDS => [
            name          => {TYPE => 'varchar(32)', NOTNULL => 1,
                              PRIMARYKEY => 1}, 
            default_value => {TYPE => 'varchar(32)', NOTNULL => 1},
            is_enabled    => {TYPE => 'BOOLEAN', NOTNULL => 1,
                              DEFAULT => 'TRUE'},
            subclass      => {TYPE => 'varchar(32)'},
        ],
    },

    setting_value => {
        FIELDS => [
            name        => {TYPE => 'varchar(32)', NOTNULL => 1},
            value       => {TYPE => 'varchar(32)', NOTNULL => 1},
            sortindex   => {TYPE => 'INT2', NOTNULL => 1},
        ],
        INDEXES => [
            setting_value_nv_unique_idx  => {FIELDS => [qw(name value)],
                                             TYPE => 'UNIQUE'},
            setting_value_ns_unique_idx  => {FIELDS => [qw(name sortindex)],
                                             TYPE => 'UNIQUE'},
        ],
     },

    profile_setting => {
        FIELDS => [
            user_id       => {TYPE => 'INT3', NOTNULL => 1},
            setting_name  => {TYPE => 'varchar(32)', NOTNULL => 1},
            setting_value => {TYPE => 'varchar(32)', NOTNULL => 1},
        ],
        INDEXES => [
            profile_setting_value_unique_idx  => {FIELDS => [qw(user_id setting_name)],
                                                  TYPE => 'UNIQUE'},
        ],
     },

    # SCHEMA STORAGE
    # --------------

    bz_schema => {
        FIELDS => [
            schema_data => {TYPE => 'LONGBLOB', NOTNULL => 1},
            version     => {TYPE => 'decimal(3,2)', NOTNULL => 1},
        ],
    },

};

use constant FIELD_TABLE_SCHEMA => {
    FIELDS => [
        id       => {TYPE => 'SMALLSERIAL', NOTNULL => 1,
                     PRIMARYKEY => 1},
        value    => {TYPE => 'varchar(64)', NOTNULL => 1},
        sortkey  => {TYPE => 'INT2', NOTNULL => 1, DEFAULT => 0},
        isactive => {TYPE => 'BOOLEAN', NOTNULL => 1,
                     DEFAULT => 'TRUE'},
    ],
    # Note that bz_add_field_table should prepend the table name
    # to these index names.
    INDEXES => [
        value_idx   => {FIELDS => ['value'], TYPE => 'UNIQUE'},
        sortkey_idx => ['sortkey', 'value'],
    ],
};
#--------------------------------------------------------------------------

=head1 METHODS

Note: Methods which can be implemented generically for all DBs are
implemented in this module. If needed, they can be overridden with
DB-specific code in a subclass. Methods which are prefixed with C<_>
are considered protected. Subclasses may override these methods, but
other modules should not invoke these methods directly.

=cut

#--------------------------------------------------------------------------
sub new {

=over

=item C<new>

 Description: Public constructor method used to instantiate objects of this
              class. However, it also can be used as a factory method to
              instantiate database-specific subclasses when an optional
              driver argument is supplied.
 Parameters:  $driver (optional) - Used to specify the type of database.
              This routine C<die>s if no subclass is found for the specified
              driver.
              $schema (optional) - A reference to a hash. Callers external
                  to this package should never use this parameter.
 Returns:     new instance of the Schema class or a database-specific subclass

=cut

    my $this = shift;
    my $class = ref($this) || $this;
    my $driver = shift;

    if ($driver) {
        (my $subclass = $driver) =~ s/^(\S)/\U$1/;
        $class .= '::' . $subclass;
        eval "require $class;";
        die "The $class class could not be found ($subclass " .
            "not supported?): $@" if ($@);
    }
    die "$class is an abstract base class. Instantiate a subclass instead."
      if ($class eq __PACKAGE__);

    my $self = {};
    bless $self, $class;
    $self = $self->_initialize(@_);

    return($self);

} #eosub--new
#--------------------------------------------------------------------------
sub _initialize {

=item C<_initialize>

 Description: Protected method that initializes an object after
              instantiation with the abstract schema. All subclasses should
              override this method. The typical subclass implementation
              should first call the C<_initialize> method of the superclass,
              then do any database-specific initialization (especially
              define the database-specific implementation of the all
              abstract data types), and then call the C<_adjust_schema>
              method.
 Parameters:  $abstract_schema (optional) - A reference to a hash. If 
                  provided, this hash will be used as the internal
                  representation of the abstract schema instead of our
                  default abstract schema. This is intended for internal 
                  use only by deserialize_abstract.
 Returns:     the instance of the Schema class

=cut

    my $self = shift;
    my $abstract_schema = shift;

    if (!$abstract_schema) {
        # While ABSTRACT_SCHEMA cannot be modified, $abstract_schema can be.
        # So, we dclone it to prevent anything from mucking with the constant.
        $abstract_schema = dclone(ABSTRACT_SCHEMA);

        # Let extensions add tables, but make sure they can't modify existing
        # tables. If we don't lock/unlock keys, lock_value complains.
        lock_keys(%$abstract_schema);
        foreach my $table (keys %{ABSTRACT_SCHEMA()}) {
            lock_value(%$abstract_schema, $table) 
                if exists $abstract_schema->{$table};
        }
        unlock_keys(%$abstract_schema);
        Bugzilla::Hook::process('db_schema-abstract_schema', 
                                { schema => $abstract_schema });
        unlock_hash(%$abstract_schema);
    }

    $self->{schema} = dclone($abstract_schema);
    $self->{abstract_schema} = $abstract_schema;

    return $self;

} #eosub--_initialize
#--------------------------------------------------------------------------
sub _adjust_schema {

=item C<_adjust_schema>

 Description: Protected method that alters the abstract schema at
              instantiation-time to be database-specific. It is a generic
              enough routine that it can be defined here in the base class.
              It takes the abstract schema and replaces the abstract data
              types with database-specific data types.
 Parameters:  none
 Returns:     the instance of the Schema class

=cut

    my $self = shift;

    # The _initialize method has already set up the db_specific hash with
    # the information on how to implement the abstract data types for the
    # instantiated DBMS-specific subclass.
    my $db_specific = $self->{db_specific};

    # Loop over each table in the abstract database schema.
    foreach my $table (keys %{ $self->{schema} }) {
        my %fields = (@{ $self->{schema}{$table}{FIELDS} });
        # Loop over the field definitions in each table.
        foreach my $field_def (values %fields) {
            # If the field type is an abstract data type defined in the
            # $db_specific hash, replace it with the DBMS-specific data type
            # that implements it.
            if (exists($db_specific->{$field_def->{TYPE}})) {
                $field_def->{TYPE} = $db_specific->{$field_def->{TYPE}};
            }
            # Replace abstract default values (such as 'TRUE' and 'FALSE')
            # with their database-specific implementations.
            if (exists($field_def->{DEFAULT})
                && exists($db_specific->{$field_def->{DEFAULT}})) {
                $field_def->{DEFAULT} = $db_specific->{$field_def->{DEFAULT}};
            }
        }
    }

    return $self;

} #eosub--_adjust_schema
#--------------------------------------------------------------------------
sub get_type_ddl {

=item C<get_type_ddl>

 Description: Public method to convert abstract (database-generic) field
              specifiers to database-specific data types suitable for use
              in a C<CREATE TABLE> or C<ALTER TABLE> SQL statment. If no
              database-specific field type has been defined for the given
              field type, then it will just return the same field type.
 Parameters:  a hash or a reference to a hash of a field containing the
              following keys: C<TYPE> (required), C<NOTNULL> (optional),
              C<DEFAULT> (optional), C<PRIMARYKEY> (optional), C<REFERENCES>
              (optional)
 Returns:     a DDL string suitable for describing a field in a
              C<CREATE TABLE> or C<ALTER TABLE> SQL statement

=cut

    my $self = shift;
    my $finfo = (@_ == 1 && ref($_[0]) eq 'HASH') ? $_[0] : { @_ };

    my $type = $finfo->{TYPE};
    die "A valid TYPE was not specified for this column." unless ($type);

    my $default = $finfo->{DEFAULT};
    # Replace any abstract default value (such as 'TRUE' or 'FALSE')
    # with its database-specific implementation.
    if ( defined $default && exists($self->{db_specific}->{$default}) ) {
        $default = $self->{db_specific}->{$default};
    }

    my $fkref = $self->{enable_references} ? $finfo->{REFERENCES} : undef;
    my $type_ddl = $self->convert_type($type);
    # DEFAULT attribute must appear before any column constraints
    # (e.g., NOT NULL), for Oracle
    $type_ddl .= " DEFAULT $default" if (defined($default));
    $type_ddl .= " NOT NULL" if ($finfo->{NOTNULL});
    $type_ddl .= " PRIMARY KEY" if ($finfo->{PRIMARYKEY});
    $type_ddl .= "\n\t\t\t\tREFERENCES $fkref" if $fkref;

    return($type_ddl);

} #eosub--get_type_ddl

sub convert_type {

=item C<convert_type>

Converts a TYPE from the L</ABSTRACT_SCHEMA> format into the real SQL type.

=cut

    my ($self, $type) = @_;
    return $self->{db_specific}->{$type} || $type;
}

sub get_column {
=item C<get_column($table, $column)>

 Description: Public method to get the abstract definition of a column.
 Parameters:  $table - the table name
              $column - a column in the table
 Returns:     a hashref containing information about the column, including its
              type (C<TYPE>), whether or not it can be null (C<NOTNULL>),
              its default value if it has one (C<DEFAULT), etc.
              Returns undef if the table or column does not exist.

=cut

    my($self, $table, $column) = @_;

    # Prevent a possible dereferencing of an undef hash, if the
    # table doesn't exist.
    if (exists $self->{schema}->{$table}) {
        my %fields = (@{ $self->{schema}{$table}{FIELDS} });
        return $fields{$column};
    }
    return undef;
} #eosub--get_column

sub get_table_list {

=item C<get_table_list>

 Description: Public method for discovering what tables should exist in the
              Bugzilla database.
 Parameters:  none
 Returns:     an array of table names

=cut

    my $self = shift;

    return(sort(keys %{ $self->{schema} }));

} #eosub--get_table_list
#--------------------------------------------------------------------------
sub get_table_columns {

=item C<get_table_columns>

 Description: Public method for discovering what columns are in a given
              table in the Bugzilla database.
 Parameters:  $table - the table name
 Returns:     array of column names

=cut

    my($self, $table) = @_;
    my @ddl = ();

    my $thash = $self->{schema}{$table};
    die "Table $table does not exist in the database schema."
        unless (ref($thash));

    my @columns = ();
    my @fields = @{ $thash->{FIELDS} };
    while (@fields) {
        push(@columns, shift(@fields));
        shift(@fields);
    }

    return @columns;

} #eosub--get_table_columns

sub get_table_indexes_abstract {
    my ($self, $table) = @_;
    my $table_def = $self->get_table_abstract($table);
    my %indexes = @{$table_def->{INDEXES} || []};
    return \%indexes;
}

sub get_create_database_sql {
    my ($self, $name) = @_;
    return ("CREATE DATABASE $name");
}

sub get_table_ddl {

=item C<get_table_ddl>

 Description: Public method to generate the SQL statements needed to create
              the a given table and its indexes in the Bugzilla database.
              Subclasses may override or extend this method, if needed, but
              subclasses probably should override C<_get_create_table_ddl>
              or C<_get_create_index_ddl> instead.
 Parameters:  $table - the table name
 Returns:     an array of strings containing SQL statements

=cut

    my($self, $table) = @_;
    my @ddl = ();

    die "Table $table does not exist in the database schema."
        unless (ref($self->{schema}{$table}));

    my $create_table = $self->_get_create_table_ddl($table);
    push(@ddl, $create_table) if $create_table;

    my @indexes = @{ $self->{schema}{$table}{INDEXES} || [] };
    while (@indexes) {
        my $index_name = shift(@indexes);
        my $index_info = shift(@indexes);
        my $index_sql  = $self->get_add_index_ddl($table, $index_name, 
                                                  $index_info);
        push(@ddl, $index_sql) if $index_sql;
    }

    push(@ddl, @{ $self->{schema}{$table}{DB_EXTRAS} })
      if (ref($self->{schema}{$table}{DB_EXTRAS}));

    return @ddl;

} #eosub--get_table_ddl
#--------------------------------------------------------------------------
sub _get_create_table_ddl {

=item C<_get_create_table_ddl>

 Description: Protected method to generate the "create table" SQL statement
              for a given table.
 Parameters:  $table - the table name
 Returns:     a string containing the DDL statement for the specified table

=cut

    my($self, $table) = @_;

    my $thash = $self->{schema}{$table};
    die "Table $table does not exist in the database schema."
        unless (ref($thash));

    my $create_table = "CREATE TABLE $table \(\n";

    my @fields = @{ $thash->{FIELDS} };
    while (@fields) {
        my $field = shift(@fields);
        my $finfo = shift(@fields);
        $create_table .= "\t$field\t" . $self->get_type_ddl($finfo);
        $create_table .= "," if (@fields);
        $create_table .= "\n";
    }

    $create_table .= "\)";

    return($create_table)

} #eosub--_get_create_table_ddl
#--------------------------------------------------------------------------
sub _get_create_index_ddl {

=item C<_get_create_index_ddl>

 Description: Protected method to generate a "create index" SQL statement
              for a given table and index.
 Parameters:  $table_name - the name of the table
              $index_name - the name of the index
              $index_fields - a reference to an array of field names
              $index_type (optional) - specify type of index (e.g., UNIQUE)
 Returns:     a string containing the DDL statement

=cut

    my ($self, $table_name, $index_name, $index_fields, $index_type) = @_;

    my $sql = "CREATE ";
    $sql .= "$index_type " if ($index_type && $index_type eq 'UNIQUE');
    $sql .= "INDEX $index_name ON $table_name \(" .
      join(", ", @$index_fields) . "\)";

    return($sql);

} #eosub--_get_create_index_ddl
#--------------------------------------------------------------------------

sub get_add_column_ddl {

=item C<get_add_column_ddl($table, $column, \%definition, $init_value)>

 Description: Generate SQL to add a column to a table.
 Params:      $table - The table containing the column.
              $column - The name of the column being added.
              \%definition - The new definition for the column,
                  in standard C<ABSTRACT_SCHEMA> format.
              $init_value - (optional) An initial value to set 
                            the column to. Should already be SQL-quoted
                            if necessary.
 Returns:     An array of SQL statements.

=cut

    my ($self, $table, $column, $definition, $init_value) = @_;
    my @statements;
    push(@statements, "ALTER TABLE $table ADD COLUMN $column " .
        $self->get_type_ddl($definition));

    # XXX - Note that although this works for MySQL, most databases will fail
    # before this point, if we haven't set a default.
    (push(@statements, "UPDATE $table SET $column = $init_value"))
        if defined $init_value;

    return (@statements);
}

sub get_add_index_ddl {

=item C<get_add_index_ddl>

 Description: Gets SQL for creating an index.
              NOTE: Subclasses should not override this function. Instead,
              if they need to specify a custom CREATE INDEX statement, 
              they should override C<_get_create_index_ddl>
 Params:      $table - The name of the table the index will be on.
              $name  - The name of the new index.
              $definition - An index definition. Either a hashref 
                            with FIELDS and TYPE or an arrayref 
                            containing a list of columns.
 Returns:     An array of SQL statements that will create the 
              requested index.

=cut

    my ($self, $table, $name, $definition) = @_;

    my ($index_fields, $index_type);
    # Index defs can be arrays or hashes
    if (ref($definition) eq 'HASH') {
        $index_fields = $definition->{FIELDS};
        $index_type = $definition->{TYPE};
    } else {
        $index_fields = $definition;
        $index_type = '';
    }
    
    return $self->_get_create_index_ddl($table, $name, $index_fields, 
                                        $index_type);
}

sub get_alter_column_ddl {

=item C<get_alter_column_ddl($table, $column, \%definition)>

 Description: Generate SQL to alter a column in a table.
              The column that you are altering must exist,
              and the table that it lives in must exist.
 Params:      $table - The table containing the column.
              $column - The name of the column being changed.
              \%definition - The new definition for the column,
                  in standard C<ABSTRACT_SCHEMA> format.
              $set_nulls_to - A value to set NULL values to, if
                  your new definition is NOT NULL and contains
                  no DEFAULT, and when there is a possibility
                  that the column could contain NULLs. $set_nulls_to
                  should be already SQL-quoted if necessary.
 Returns:     An array of SQL statements.

=cut

    my ($self, $table, $column, $new_def, $set_nulls_to) = @_;

    my @statements;
    my $old_def = $self->get_column_abstract($table, $column);
    my $specific = $self->{db_specific};

    # If the types have changed, we have to deal with that.
    if (uc(trim($old_def->{TYPE})) ne uc(trim($new_def->{TYPE}))) {
        push(@statements, $self->_get_alter_type_sql($table, $column, 
                                                     $new_def, $old_def));
    }

    my $default = $new_def->{DEFAULT};
    my $default_old = $old_def->{DEFAULT};
    # This first condition prevents "uninitialized value" errors.
    if (!defined $default && !defined $default_old) {
        # Do Nothing
    }
    # If we went from having a default to not having one
    elsif (!defined $default && defined $default_old) {
        push(@statements, "ALTER TABLE $table ALTER COLUMN $column"
                        . " DROP DEFAULT");
    }
    # If we went from no default to a default, or we changed the default.
    elsif ( (defined $default && !defined $default_old) || 
            ($default ne $default_old) ) 
    {
        $default = $specific->{$default} if exists $specific->{$default};
        push(@statements, "ALTER TABLE $table ALTER COLUMN $column "
                         . " SET DEFAULT $default");
    }

    # If we went from NULL to NOT NULL.
    if (!$old_def->{NOTNULL} && $new_def->{NOTNULL}) {
        my $setdefault;
        # Handle any fields that were NULL before, if we have a default,
        $setdefault = $new_def->{DEFAULT} if exists $new_def->{DEFAULT};
        # But if we have a set_nulls_to, that overrides the DEFAULT 
        # (although nobody would usually specify both a default and 
        # a set_nulls_to.)
        $setdefault = $set_nulls_to if defined $set_nulls_to;
        if (defined $setdefault) {
            push(@statements, "UPDATE $table SET $column = $setdefault"
                            . "  WHERE $column IS NULL");
        }
        push(@statements, "ALTER TABLE $table ALTER COLUMN $column"
                        . " SET NOT NULL");
    }
    # If we went from NOT NULL to NULL
    elsif ($old_def->{NOTNULL} && !$new_def->{NOTNULL}) {
        push(@statements, "ALTER TABLE $table ALTER COLUMN $column"
                        . " DROP NOT NULL");
    }

    # If we went from not being a PRIMARY KEY to being a PRIMARY KEY.
    if (!$old_def->{PRIMARYKEY} && $new_def->{PRIMARYKEY}) {
        push(@statements, "ALTER TABLE $table ADD PRIMARY KEY ($column)");
    }
    # If we went from being a PK to not being a PK
    elsif ( $old_def->{PRIMARYKEY} && !$new_def->{PRIMARYKEY} ) {
        push(@statements, "ALTER TABLE $table DROP PRIMARY KEY");
    }

    return @statements;
}

sub get_drop_index_ddl {

=item C<get_drop_index_ddl($table, $name)>

 Description: Generates SQL statements to drop an index.
 Params:      $table - The table the index is on.
              $name  - The name of the index being dropped.
 Returns:     An array of SQL statements.

=cut

    my ($self, $table, $name) = @_;

    # Although ANSI SQL-92 doesn't specify a method of dropping an index,
    # many DBs support this syntax.
    return ("DROP INDEX $name");
}

sub get_drop_column_ddl {

=item C<get_drop_column_ddl($table, $column)>

 Description: Generate SQL to drop a column from a table.
 Params:      $table - The table containing the column.
              $column - The name of the column being dropped.
 Returns:     An array of SQL statements.

=cut

    my ($self, $table, $column) = @_;
    return ("ALTER TABLE $table DROP COLUMN $column");
}

=item C<get_drop_table_ddl($table)>

 Description: Generate SQL to drop a table from the database.
 Params:      $table - The name of the table to drop.
 Returns:     An array of SQL statements.

=cut

sub get_drop_table_ddl {
    my ($self, $table) = @_;
    return ("DROP TABLE $table");
}

sub get_rename_column_ddl {

=item C<get_rename_column_ddl($table, $old_name, $new_name)>

 Description: Generate SQL to change the name of a column in a table.
              NOTE: ANSI SQL contains no simple way to rename a column,
                    so this function is ABSTRACT and must be implemented
                    by subclasses.
 Params:      $table - The table containing the column to be renamed.
              $old_name - The name of the column being renamed.
              $new_name - The name the column is changing to.
 Returns:     An array of SQL statements.

=cut

    die "ANSI SQL has no way to rename a column, and your database driver\n"
        . " has not implemented a method.";
}


sub get_rename_table_sql {

=item C<get_rename_table_sql>

=over

=item B<Description>

Gets SQL to rename a table in the database.

=item B<Params>

=over

=item C<$old_name> - The current name of the table.

=item C<$new_name> - The new name of the table.

=back

=item B<Returns>: An array of SQL statements to rename a table.

=back

=cut

    my ($self, $old_name, $new_name) = @_;
    return ("ALTER TABLE $old_name RENAME TO $new_name");
}

=item C<delete_table($name)>

 Description: Deletes a table from this Schema object.
              Dies if you try to delete a table that doesn't exist.
 Params:      $name - The name of the table to delete.
 Returns:     nothing

=cut

sub delete_table {
    my ($self, $name) = @_;

    die "Attempted to delete nonexistent table '$name'." unless
        $self->get_table_abstract($name);

    delete $self->{abstract_schema}->{$name};
    delete $self->{schema}->{$name};
}

sub get_column_abstract {

=item C<get_column_abstract($table, $column)>

 Description: A column definition from the abstract internal schema.
              cross-database format.
 Params:      $table - The name of the table
              $column - The name of the column that you want
 Returns:     A hash reference. For the format, see the docs for
              C<ABSTRACT_SCHEMA>.
              Returns undef if the column or table does not exist.

=cut

    my ($self, $table, $column) = @_;

    # Prevent a possible dereferencing of an undef hash, if the
    # table doesn't exist.
    if ($self->get_table_abstract($table)) {
        my %fields = (@{ $self->{abstract_schema}{$table}{FIELDS} });
        return $fields{$column};
    }
    return undef;
}

=item C<get_indexes_on_column_abstract($table, $column)>

 Description: Gets a list of indexes that are on a given column.
 Params:      $table - The table the column is on.
              $column - The name of the column.
 Returns:     Indexes in the standard format of an INDEX
              entry on a table. That is, key-value pairs
              where the key is the index name and the value
              is the index definition.
              If there are no indexes on that column, we return
              undef.

=cut

sub get_indexes_on_column_abstract {
    my ($self, $table, $column) = @_;
    my %ret_hash;

    my $table_def = $self->get_table_abstract($table);
    if ($table_def && exists $table_def->{INDEXES}) {
        my %indexes = (@{ $table_def->{INDEXES} });
        foreach my $index_name (keys %indexes) {
            my $col_list;
            # Get the column list, depending on whether the index
            # is in hashref or arrayref format.
            if (ref($indexes{$index_name}) eq 'HASH') {
                $col_list = $indexes{$index_name}->{FIELDS};
            } else {
                $col_list = $indexes{$index_name};
            }

            if(grep($_ eq $column, @$col_list)) {
                $ret_hash{$index_name} = dclone($indexes{$index_name});
            }
        }
    }

    return %ret_hash;
}

sub get_index_abstract {

=item C<get_index_abstract($table, $index)>

 Description: Returns an index definition from the internal abstract schema.
 Params:      $table - The table the index is on.
              $index - The name of the index.
 Returns:     A hash reference representing an index definition.
              See the C<ABSTRACT_SCHEMA> docs for details.
              Returns undef if the index does not exist.

=cut

    my ($self, $table, $index) = @_;

    # Prevent a possible dereferencing of an undef hash, if the
    # table doesn't exist.
    my $index_table = $self->get_table_abstract($table);
    if ($index_table && exists $index_table->{INDEXES}) {
        my %indexes = (@{ $index_table->{INDEXES} });
        return $indexes{$index};
    }
    return undef;
}

=item C<get_table_abstract($table)>

 Description: Gets the abstract definition for a table in this Schema
              object.
 Params:      $table - The name of the table you want a definition for.
 Returns:     An abstract table definition, or undef if the table doesn't
              exist.

=cut

sub get_table_abstract {
    my ($self, $table) = @_;
    return $self->{abstract_schema}->{$table};
}

=item C<add_table($name, \%definition)>

 Description: Creates a new table in this Schema object.
              If you do not specify a definition, we will
              simply create an empty table.
 Params:      $name - The name for the new table.
              \%definition (optional) - An abstract definition for
                  the new table.
 Returns:     nothing

=cut

sub add_table {
    my ($self, $name, $definition) = @_;
    (die "Table already exists: $name")
        if exists $self->{abstract_schema}->{$name};
    if ($definition) {
        $self->{abstract_schema}->{$name} = dclone($definition);
        $self->{schema} = dclone($self->{abstract_schema});
        $self->_adjust_schema();
    }
    else {
        $self->{abstract_schema}->{$name} = {FIELDS => []};
        $self->{schema}->{$name}          = {FIELDS => []};
    }
}



sub rename_table {

=item C<rename_table>

Renames a table from C<$old_name> to C<$new_name> in this Schema object.

=cut


    my ($self, $old_name, $new_name) = @_;
    my $table = $self->get_table_abstract($old_name);
    $self->delete_table($old_name);
    $self->add_table($new_name, $table);
}

sub delete_column {

=item C<delete_column($table, $column)>

 Description: Deletes a column from this Schema object.
 Params:      $table - Name of the table that the column is in.
                       The table must exist, or we will fail.
              $column  - Name of the column to delete.
 Returns:     nothing

=cut

    my ($self, $table, $column) = @_;

    my $abstract_fields = $self->{abstract_schema}{$table}{FIELDS};
    my $name_position = lsearch($abstract_fields, $column);
    die "Attempted to delete nonexistent column ${table}.${column}" 
        if $name_position == -1;
    # Delete the key/value pair from the array.
    splice(@$abstract_fields, $name_position, 2);

    $self->{schema} = dclone($self->{abstract_schema});
    $self->_adjust_schema();
}

sub rename_column {

=item C<rename_column($table, $old_name, $new_name)>

 Description: Renames a column on a table in the Schema object.
              The column that you are renaming must exist.
 Params:      $table - The table the column is on.
              $old_name - The current name of the column.
              $new_name - The new name of hte column.
 Returns:     nothing

=cut

    my ($self, $table, $old_name, $new_name) = @_;
    my $def = $self->get_column_abstract($table, $old_name);
    die "Renaming a column that doesn't exist" if !$def;
    $self->delete_column($table, $old_name);
    $self->set_column($table, $new_name, $def);
}

sub set_column {

=item C<set_column($table, $column, \%new_def)>

 Description: Changes the definition of a column in this Schema object.
              If the column doesn't exist, it will be added.
              The table that you specify must already exist in the Schema.
              NOTE: This does not affect the database on the disk.
              Use the C<Bugzilla::DB> "Schema Modification Methods"
              if you want to do that.
 Params:      $table - The name of the table that the column is on.
              $column - The name of the column.
              \%new_def - The new definition for the column, in 
                  C<ABSTRACT_SCHEMA> format.
 Returns:     nothing

=cut

    my ($self, $table, $column, $new_def) = @_;

    my $fields = $self->{abstract_schema}{$table}{FIELDS};
    $self->_set_object($table, $column, $new_def, $fields);
}

sub set_index {

=item C<set_index($table, $name, $definition)>

 Description: Changes the definition of an index in this Schema object.
              If the index doesn't exist, it will be added.
              The table that you specify must already exist in the Schema.
              NOTE: This does not affect the database on the disk.
              Use the C<Bugzilla::DB> "Schema Modification Methods"
              if you want to do that.
 Params:      $table      - The table the index is on.
              $name       - The name of the index.
              $definition - A hashref or an arrayref. An index 
                            definition in C<ABSTRACT_SCHEMA> format.
 Returns:     nothing

=cut

    my ($self, $table, $name, $definition) = @_;

    if ( exists $self->{abstract_schema}{$table}
         && !exists $self->{abstract_schema}{$table}{INDEXES} ) {
        $self->{abstract_schema}{$table}{INDEXES} = [];
    }

    my $indexes = $self->{abstract_schema}{$table}{INDEXES};
    $self->_set_object($table, $name, $definition, $indexes);
}

# A private helper for set_index and set_column.
# This does the actual "work" of those two functions.
# $array_to_change is an arrayref.
sub _set_object {
    my ($self, $table, $name, $definition, $array_to_change) = @_;

    my $obj_position = lsearch($array_to_change, $name) + 1;
    # If the object doesn't exist, then add it.
    if (!$obj_position) {
        push(@$array_to_change, $name);
        push(@$array_to_change, $definition);
    }
    # We're modifying an existing object in the Schema.
    else {
        splice(@$array_to_change, $obj_position, 1, $definition);
    }

    $self->{schema} = dclone($self->{abstract_schema});
    $self->_adjust_schema();
}

=item C<delete_index($table, $name)>

 Description: Removes an index definition from this Schema object.
              If the index doesn't exist, we will fail.
              The table that you specify must exist in the Schema.
              NOTE: This does not affect the database on the disk.
              Use the C<Bugzilla::DB> "Schema Modification Methods"
              if you want to do that.
 Params:      $table - The table the index is on.
              $name  - The name of the index that we're removing.
 Returns:     nothing

=cut

sub delete_index {
    my ($self, $table, $name) = @_;

    my $indexes = $self->{abstract_schema}{$table}{INDEXES};
    my $name_position = lsearch($indexes, $name);
    die "Attempted to delete nonexistent index $name on the $table table" 
        if $name_position == -1;
    # Delete the key/value pair from the array.
    splice(@$indexes, $name_position, 2);
    $self->{schema} = dclone($self->{abstract_schema});
    $self->_adjust_schema();
}

sub columns_equal {

=item C<columns_equal($col_one, $col_two)>

 Description: Tells you if two columns have entirely identical definitions.
              The TYPE field's value will be compared case-insensitive.
              However, all other fields will be case-sensitive.
 Params:      $col_one, $col_two - The columns to compare. Hash 
                  references, in C<ABSTRACT_SCHEMA> format.
 Returns:     C<1> if the columns are identical, C<0> if they are not.

=back

=cut

    my $self = shift;
    my $col_one = dclone(shift);
    my $col_two = dclone(shift);

    $col_one->{TYPE} = uc($col_one->{TYPE});
    $col_two->{TYPE} = uc($col_two->{TYPE});

    my @col_one_array = %$col_one;
    my @col_two_array = %$col_two;

    my ($removed, $added) = diff_arrays(\@col_one_array, \@col_two_array);

    # If there are no differences between the arrays,
    # then they are equal.
    return !scalar(@$removed) && !scalar(@$added) ? 1 : 0;
}


=head1 SERIALIZATION/DESERIALIZATION

=over 4

=item C<serialize_abstract()>

 Description: Serializes the "abstract" schema into a format
              that deserialize_abstract() can read in. This is
              a method, called on a Schema instance.
 Parameters:  none
 Returns:     A scalar containing the serialized, abstract schema.
              Do not attempt to manipulate this data directly,
              as the format may change at any time in the future.
              The only thing you should do with the returned value
              is either store it somewhere (coupled with appropriate 
              SCHEMA_VERSION) or deserialize it.

=cut

sub serialize_abstract {
    my ($self) = @_;
    
    # Make it ok to eval
    local $Data::Dumper::Purity = 1;
    
    # Avoid cross-refs
    local $Data::Dumper::Deepcopy = 1;
    
    # Always sort keys to allow textual compare
    local $Data::Dumper::Sortkeys = 1;
    
    return Dumper($self->{abstract_schema});
}

=item C<deserialize_abstract($serialized, $version)>

 Description: Used for when you've read a serialized Schema off the disk,
              and you want a Schema object that represents that data.
 Params:      $serialized - scalar. The serialized data.
              $version - A number in the format X.YZ. The "version"
                  of the Schema that did the serialization.
                  See the docs for C<SCHEMA_VERSION> for more details.
 Returns:     A Schema object. It will have the methods of (and work 
              in the same fashion as) the current version of Schema. 
              However, it will represent the serialized data instead of
              ABSTRACT_SCHEMA.
=cut

sub deserialize_abstract {
    my ($class, $serialized, $version) = @_;

    my $thawed_hash;
    if (int($version) < 2) {
        $thawed_hash = thaw($serialized);
    }
    else {
        my $cpt = new Safe;
        $cpt->reval($serialized) ||
            die "Unable to restore cached schema: " . $@;
        $thawed_hash = ${$cpt->varglob('VAR1')};
    }

    return $class->new(undef, $thawed_hash);
}

#####################################################################
# Class Methods
#####################################################################

=back

=head1 CLASS METHODS

These methods are generally called on the class instead of on a specific
object.

=over

=item C<get_empty_schema()>

 Description: Returns a Schema that has no tables. In effect, this
              Schema is totally "empty."
 Params:      none
 Returns:     A "empty" Schema object.

=back

=cut

sub get_empty_schema {
    my ($class) = @_;
    return $class->deserialize_abstract(Dumper({}), SCHEMA_VERSION);
}

1;

__END__

=head1 ABSTRACT DATA TYPES

The size and range data provided here is only
intended as a guide.  See your database's Bugzilla
module (in this directory) for the most up-to-date
values for these data types.  The following
abstract data types are used:

=over 4

=item C<BOOLEAN>

Logical value 0 or 1 where 1 is true, 0 is false.

=item C<INT1>

Integer values (-128 - 127 or 0 - 255 unsigned).

=item C<INT2>

Integer values (-32,768 - 32767 or 0 - 65,535 unsigned).

=item C<INT3>

Integer values (-8,388,608 - 8,388,607 or 0 - 16,777,215 unsigned)

=item C<INT4>

Integer values (-2,147,483,648 - 2,147,483,647 or 0 - 4,294,967,295 
unsigned)

=item C<SMALLSERIAL>

An auto-increment L</INT1>

=item C<MEDIUMSERIAL>

An auto-increment L</INT3>

=item C<INTSERIAL>

An auto-increment L</INT4>

=item C<TINYTEXT>

Variable length string of characters up to 255 (2^8 - 1) characters wide 
or more depending on the character set used.

=item C<MEDIUMTEXT>

Variable length string of characters up to 16M (2^24 - 1) characters wide 
or more depending on the character set used.

=item C<TEXT>

Variable length string of characters up to 64K (2^16 - 1) characters wide 
or more depending on the character set used.

=item C<LONGBLOB>

Variable length string of binary data up to 4M (2^32 - 1) bytes wide

=item C<DATETIME>

DATETIME support varies from database to database, however, it's generally 
safe to say that DATETIME entries support all date/time combinations greater
than 1900-01-01 00:00:00.  Note that the format used is C<YYYY-MM-DD hh:mm:ss>
to be safe, though it's possible that your database may not require
leading zeros.  For greatest compatibility, however, please make sure dates 
are formatted as above for queries to guarantee consistent results.

=back

Database-specific subclasses should define the implementation for these data
types as a hash reference stored internally in the schema object as
C<db_specific>. This is typically done in overridden L<_initialize> method.

The following abstract boolean values should also be defined on a
database-specific basis:

=over 4

=item C<TRUE>

=item C<FALSE>

=back

=head1 SEE ALSO

L<Bugzilla::DB>

L<http://www.bugzilla.org/docs/developer.html#sql-schema>

=cut
