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
#                 Dawn Endico <endico@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Joe Robins <jmrobins@tgix.com>
#                 Jake <jake@bugzilla.org>
#                 J. Paul Reed <preed@sigkill.com>
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Christopher Aillon <christopher@aillon.com>
#                 Shane H. W. Travis <travis@sedsystems.ca>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Marc Schumann <wurblzap@gmail.com>

package Bugzilla::Constants;
use strict;
use base qw(Exporter);

# For bz_locations
use File::Basename;

@Bugzilla::Constants::EXPORT = qw(
    BUGZILLA_VERSION

    bz_locations
    CONTROLMAPNA
    CONTROLMAPSHOWN
    CONTROLMAPDEFAULT
    CONTROLMAPMANDATORY

    AUTH_OK
    AUTH_NODATA
    AUTH_ERROR
    AUTH_LOGINFAILED
    AUTH_DISABLED
    AUTH_NO_SUCH_USER

    USER_PASSWORD_MIN_LENGTH
    USER_PASSWORD_MAX_LENGTH

    LOGIN_OPTIONAL
    LOGIN_NORMAL
    LOGIN_REQUIRED

    LOGOUT_ALL
    LOGOUT_CURRENT
    LOGOUT_KEEP_CURRENT

    GRANT_DIRECT
    GRANT_REGEXP

    GROUP_MEMBERSHIP
    GROUP_BLESS
    GROUP_VISIBLE

    MAILTO_USER
    MAILTO_GROUP

    DEFAULT_COLUMN_LIST
    DEFAULT_QUERY_NAME

    QUERY_LIST
    LIST_OF_BUGS

    COMMENT_COLS

    CMT_NORMAL
    CMT_DUPE_OF
    CMT_HAS_DUPE
    CMT_POPULAR_VOTES
    CMT_MOVED_TO

    UNLOCK_ABORT
    THROW_ERROR
    
    RELATIONSHIPS
    REL_ASSIGNEE REL_QA REL_REPORTER REL_CC REL_VOTER REL_GLOBAL_WATCHER
    REL_ANY
    
    POS_EVENTS
    EVT_OTHER EVT_ADDED_REMOVED EVT_COMMENT EVT_ATTACHMENT EVT_ATTACHMENT_DATA
    EVT_PROJ_MANAGEMENT EVT_OPENED_CLOSED EVT_KEYWORD EVT_CC EVT_DEPEND_BLOCK
    
    NEG_EVENTS
    EVT_UNCONFIRMED EVT_CHANGED_BY_ME 
        
    GLOBAL_EVENTS
    EVT_FLAG_REQUESTED EVT_REQUESTED_FLAG

    FULLTEXT_BUGLIST_LIMIT

    ADMIN_GROUP_NAME
    PER_PRODUCT_PRIVILEGES

    SENDMAIL_EXE
    SENDMAIL_PATH

    FIELD_TYPE_UNKNOWN
    FIELD_TYPE_FREETEXT
    FIELD_TYPE_SINGLE_SELECT

    BUG_STATE_OPEN

    USAGE_MODE_BROWSER
    USAGE_MODE_CMDLINE
    USAGE_MODE_WEBSERVICE
    USAGE_MODE_EMAIL

    ERROR_MODE_WEBPAGE
    ERROR_MODE_DIE
    ERROR_MODE_DIE_SOAP_FAULT

    INSTALLATION_MODE_INTERACTIVE
    INSTALLATION_MODE_NON_INTERACTIVE

    DB_MODULE
    ROOT_USER
    ON_WINDOWS

    MAX_TOKEN_AGE

    SAFE_PROTOCOLS

    MAX_LEN_QUERY_NAME
    MAX_FREETEXT_LENGTH
);

@Bugzilla::Constants::EXPORT_OK = qw(contenttypes);

# CONSTANTS
#
# Bugzilla version
use constant BUGZILLA_VERSION => "3.0.4";

#
# ControlMap constants for group_control_map.
# membercontol:othercontrol => meaning
# Na:Na               => Bugs in this product may not be restricted to this 
#                        group.
# Shown:Na            => Members of the group may restrict bugs 
#                        in this product to this group.
# Shown:Shown         => Members of the group may restrict bugs
#                        in this product to this group.
#                        Anyone who can enter bugs in this product may initially
#                        restrict bugs in this product to this group.
# Shown:Mandatory     => Members of the group may restrict bugs
#                        in this product to this group.
#                        Non-members who can enter bug in this product
#                        will be forced to restrict it.
# Default:Na          => Members of the group may restrict bugs in this
#                        product to this group and do so by default.
# Default:Default     => Members of the group may restrict bugs in this
#                        product to this group and do so by default and
#                        nonmembers have this option on entry.
# Default:Mandatory   => Members of the group may restrict bugs in this
#                        product to this group and do so by default.
#                        Non-members who can enter bug in this product
#                        will be forced to restrict it.
# Mandatory:Mandatory => Bug will be forced into this group regardless.
# All other combinations are illegal.

use constant CONTROLMAPNA => 0;
use constant CONTROLMAPSHOWN => 1;
use constant CONTROLMAPDEFAULT => 2;
use constant CONTROLMAPMANDATORY => 3;

# See Bugzilla::Auth for docs on AUTH_*, LOGIN_* and LOGOUT_*

use constant AUTH_OK => 0;
use constant AUTH_NODATA => 1;
use constant AUTH_ERROR => 2;
use constant AUTH_LOGINFAILED => 3;
use constant AUTH_DISABLED => 4;
use constant AUTH_NO_SUCH_USER  => 5;

# The minimum and maximum lengths a password must have.
use constant USER_PASSWORD_MIN_LENGTH => 3;
use constant USER_PASSWORD_MAX_LENGTH => 16;

use constant LOGIN_OPTIONAL => 0;
use constant LOGIN_NORMAL => 1;
use constant LOGIN_REQUIRED => 2;

use constant LOGOUT_ALL => 0;
use constant LOGOUT_CURRENT => 1;
use constant LOGOUT_KEEP_CURRENT => 2;

use constant contenttypes =>
  {
   "html"=> "text/html" , 
   "rdf" => "application/rdf+xml" , 
   "atom"=> "application/atom+xml" ,
   "xml" => "application/xml" , 
   "js"  => "application/x-javascript" , 
   "csv" => "text/csv" ,
   "png" => "image/png" ,
   "ics" => "text/calendar" ,
  };

use constant GRANT_DIRECT => 0;
use constant GRANT_REGEXP => 2;

use constant GROUP_MEMBERSHIP => 0;
use constant GROUP_BLESS => 1;
use constant GROUP_VISIBLE => 2;

use constant MAILTO_USER => 0;
use constant MAILTO_GROUP => 1;

# The default list of columns for buglist.cgi
use constant DEFAULT_COLUMN_LIST => (
    "bug_severity", "priority", "op_sys","assigned_to",
    "bug_status", "resolution", "short_desc"
);

# Used by query.cgi and buglist.cgi as the named-query name
# for the default settings.
use constant DEFAULT_QUERY_NAME => '(Default query)';

# The possible types for saved searches.
use constant QUERY_LIST => 0;
use constant LIST_OF_BUGS => 1;

# The column length for displayed (and wrapped) bug comments.
use constant COMMENT_COLS => 80;

# The type of bug comments.
use constant CMT_NORMAL => 0;
use constant CMT_DUPE_OF => 1;
use constant CMT_HAS_DUPE => 2;
use constant CMT_POPULAR_VOTES => 3;
use constant CMT_MOVED_TO => 4;

# used by Bugzilla::DB to indicate that tables are being unlocked
# because of error
use constant UNLOCK_ABORT => 1;

# Determine whether a validation routine should return 0 or throw
# an error when the validation fails.
use constant THROW_ERROR => 1;

use constant REL_ASSIGNEE           => 0;
use constant REL_QA                 => 1;
use constant REL_REPORTER           => 2;
use constant REL_CC                 => 3;
use constant REL_VOTER              => 4;
use constant REL_GLOBAL_WATCHER     => 5;

use constant RELATIONSHIPS => REL_ASSIGNEE, REL_QA, REL_REPORTER, REL_CC, 
                              REL_VOTER, REL_GLOBAL_WATCHER;
                              
# Used for global events like EVT_FLAG_REQUESTED
use constant REL_ANY                => 100;

# There are two sorts of event - positive and negative. Positive events are
# those for which the user says "I want mail if this happens." Negative events
# are those for which the user says "I don't want mail if this happens."
#
# Exactly when each event fires is defined in wants_bug_mail() in User.pm; I'm
# not commenting them here in case the comments and the code get out of sync.
use constant EVT_OTHER              => 0;
use constant EVT_ADDED_REMOVED      => 1;
use constant EVT_COMMENT            => 2;
use constant EVT_ATTACHMENT         => 3;
use constant EVT_ATTACHMENT_DATA    => 4;
use constant EVT_PROJ_MANAGEMENT    => 5;
use constant EVT_OPENED_CLOSED      => 6;
use constant EVT_KEYWORD            => 7;
use constant EVT_CC                 => 8;
use constant EVT_DEPEND_BLOCK       => 9;

use constant POS_EVENTS => EVT_OTHER, EVT_ADDED_REMOVED, EVT_COMMENT, 
                           EVT_ATTACHMENT, EVT_ATTACHMENT_DATA, 
                           EVT_PROJ_MANAGEMENT, EVT_OPENED_CLOSED, EVT_KEYWORD,
                           EVT_CC, EVT_DEPEND_BLOCK;

use constant EVT_UNCONFIRMED        => 50;
use constant EVT_CHANGED_BY_ME      => 51;

use constant NEG_EVENTS => EVT_UNCONFIRMED, EVT_CHANGED_BY_ME;

# These are the "global" flags, which aren't tied to a particular relationship.
# and so use REL_ANY.
use constant EVT_FLAG_REQUESTED     => 100; # Flag has been requested of me
use constant EVT_REQUESTED_FLAG     => 101; # I have requested a flag

use constant GLOBAL_EVENTS => EVT_FLAG_REQUESTED, EVT_REQUESTED_FLAG;

#  Number of bugs to return in a buglist when performing
#  a fulltext search.
use constant FULLTEXT_BUGLIST_LIMIT => 200;

# Default administration group name.
use constant ADMIN_GROUP_NAME => 'admin';

# Privileges which can be per-product.
use constant PER_PRODUCT_PRIVILEGES => ('editcomponents', 'editbugs', 'canconfirm');

# Path to sendmail.exe (Windows only)
use constant SENDMAIL_EXE => '/usr/lib/sendmail.exe';
# Paths to search for the sendmail binary (non-Windows)
use constant SENDMAIL_PATH => '/usr/lib:/usr/sbin:/usr/ucblib';

# Field types.  Match values in fielddefs.type column.  These are purposely
# not named after database column types, since Bugzilla fields comprise not
# only storage but also logic.  For example, we might add a "user" field type
# whose values are stored in an integer column in the database but for which
# we do more than we would do for a standard integer type (f.e. we might
# display a user picker).

use constant FIELD_TYPE_UNKNOWN   => 0;
use constant FIELD_TYPE_FREETEXT  => 1;
use constant FIELD_TYPE_SINGLE_SELECT => 2;

# The maximum number of days a token will remain valid.
use constant MAX_TOKEN_AGE => 3;

# Protocols which are considered as safe.
use constant SAFE_PROTOCOLS => ('afs', 'cid', 'ftp', 'gopher', 'http', 'https',
                                'irc', 'mid', 'news', 'nntp', 'prospero', 'telnet',
                                'view-source', 'wais');

# States that are considered to be "open" for bugs.
use constant BUG_STATE_OPEN => ('NEW', 'REOPENED', 'ASSIGNED', 
                                'UNCONFIRMED');

# Usage modes. Default USAGE_MODE_BROWSER. Use with Bugzilla->usage_mode.
use constant USAGE_MODE_BROWSER    => 0;
use constant USAGE_MODE_CMDLINE    => 1;
use constant USAGE_MODE_WEBSERVICE => 2;
use constant USAGE_MODE_EMAIL      => 3;

# Error modes. Default set by Bugzilla->usage_mode (so ERROR_MODE_WEBPAGE
# usually). Use with Bugzilla->error_mode.
use constant ERROR_MODE_WEBPAGE        => 0;
use constant ERROR_MODE_DIE            => 1;
use constant ERROR_MODE_DIE_SOAP_FAULT => 2;

# The various modes that checksetup.pl can run in.
use constant INSTALLATION_MODE_INTERACTIVE => 0;
use constant INSTALLATION_MODE_NON_INTERACTIVE => 1;

# Data about what we require for different databases.
use constant DB_MODULE => {
    'mysql' => {db => 'Bugzilla::DB::Mysql', db_version => '4.1.2',
                dbd => { 
                    package => 'DBD-mysql',
                    module  => 'DBD::mysql',
                    version => '2.9003',
                    # Certain versions are broken, development versions are
                    # always disallowed.
                    blacklist => ['^3\.000[3-6]', '_'],
                },
                name => 'MySQL'},
    'pg'    => {db => 'Bugzilla::DB::Pg', db_version => '8.00.0000',
                dbd => {
                    package => 'DBD-Pg',
                    module  => 'DBD::Pg',
                    version => '1.45',
                },
                name => 'PostgreSQL'},
};

# The user who should be considered "root" when we're giving
# instructions to Bugzilla administrators.
use constant ROOT_USER => $^O =~ /MSWin32/i ? 'Administrator' : 'root';

# True if we're on Win32.
use constant ON_WINDOWS => ($^O =~ /MSWin32/i);

# The longest that a saved search name can be.
use constant MAX_LEN_QUERY_NAME => 64;

# Maximum length allowed for free text fields.
use constant MAX_FREETEXT_LENGTH => 255;

sub bz_locations {
    # We know that Bugzilla/Constants.pm must be in %INC at this point.
    # So the only question is, what's the name of the directory
    # above it? This is the most reliable way to get our current working
    # directory under both mod_cgi and mod_perl. We call dirname twice
    # to get the name of the directory above the "Bugzilla/" directory.
    #
    # Calling dirname twice like that won't work on VMS or AmigaOS
    # but I doubt anybody runs Bugzilla on those.
    #
    # On mod_cgi this will be a relative path. On mod_perl it will be an
    # absolute path.
    my $libpath = dirname(dirname($INC{'Bugzilla/Constants.pm'}));
    # We have to detaint $libpath, but we can't use Bugzilla::Util here.
    $libpath =~ /(.*)/;
    $libpath = $1;

    my ($project, $localconfig, $datadir);
    if ($ENV{'PROJECT'} && $ENV{'PROJECT'} =~ /^(\w+)$/) {
        $project = $1;
        $datadir = "data/$project";
    } else {
        $datadir = "data";
    }

    # The "localconfig" file has another path on the ReactOS Web Server
    $localconfig = "../../www.reactos.org_config/bugzilla-config";
    
    # We have to return absolute paths for mod_perl. 
    # That means that if you modify these paths, they must be absolute paths.
    return {
        'libpath'     => $libpath,
        # If you put the libraries in a different location than the CGIs,
        # make sure this still points to the CGIs.
        'cgi_path'    => $libpath,
        'templatedir' => "$libpath/template",
        'project'     => $project,
        'localconfig' => "$libpath/$localconfig",
        'datadir'     => "$libpath/$datadir",
        'attachdir'   => "$libpath/$datadir/attachments",
        'skinsdir'    => "$libpath/skins",
        # $webdotdir must be in the webtree somewhere. Even if you use a 
        # local dot, we output images to there. Also, if $webdotdir is 
        # not relative to the bugzilla root directory, you'll need to 
        # change showdependencygraph.cgi to set image_url to the correct 
        # location.
        # The script should really generate these graphs directly...
        'webdotdir'   => "$libpath/$datadir/webdot",
        'extensionsdir' => "$libpath/extensions",
    };
}

1;
