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
# Contributor(s): Dawn Endico    <endico@mozilla.org>
#                 Terry Weissman <terry@mozilla.org>
#                 Chris Yeh      <cyeh@bluemartini.com>
#                 Bradley Baetz  <bbaetz@acm.org>
#                 Dave Miller    <justdave@bugzilla.org>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Frédéric Buclin <LpSolit@gmail.com>
#                 Lance Larsh <lance.larsh@oracle.com>

package Bugzilla::Bug;

use strict;

use Bugzilla::Attachment;
use Bugzilla::Constants;
use Bugzilla::Field;
use Bugzilla::Flag;
use Bugzilla::FlagType;
use Bugzilla::Keyword;
use Bugzilla::User;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Product;
use Bugzilla::Component;
use Bugzilla::Group;

use List::Util qw(min);

use base qw(Bugzilla::Object Exporter);
@Bugzilla::Bug::EXPORT = qw(
    AppendComment ValidateComment
    bug_alias_to_id ValidateBugAlias ValidateBugID
    RemoveVotes CheckIfVotedConfirmed
    LogActivityEntry
    is_open_state
    editable_bug_fields
);

#####################################################################
# Constants
#####################################################################

use constant DB_TABLE   => 'bugs';
use constant ID_FIELD   => 'bug_id';
use constant NAME_FIELD => 'alias';
use constant LIST_ORDER => ID_FIELD;

# This is a sub because it needs to call other subroutines.
sub DB_COLUMNS {
    my $dbh = Bugzilla->dbh;
    return qw(
        alias
        bug_file_loc
        bug_id
        bug_severity
        bug_status
        cclist_accessible
        component_id
        delta_ts
        estimated_time
        everconfirmed
        op_sys
        priority
        product_id
        remaining_time
        rep_platform
        reporter_accessible
        resolution
        short_desc
        status_whiteboard
        target_milestone
        version
    ),
    'assigned_to AS assigned_to_id',
    'reporter    AS reporter_id',
    'qa_contact  AS qa_contact_id',
    $dbh->sql_date_format('creation_ts', '%Y.%m.%d %H:%i') . ' AS creation_ts',
    $dbh->sql_date_format('deadline', '%Y-%m-%d') . ' AS deadline',
    Bugzilla->custom_field_names;
}

use constant REQUIRED_CREATE_FIELDS => qw(
    component
    product
    short_desc
    version
);

# There are also other, more complex validators that are called
# from run_create_validators.
sub VALIDATORS {
    my $validators = {
        alias          => \&_check_alias,
        bug_file_loc   => \&_check_bug_file_loc,
        bug_severity   => \&_check_bug_severity,
        comment        => \&_check_comment,
        commentprivacy => \&_check_commentprivacy,
        deadline       => \&_check_deadline,
        estimated_time => \&_check_estimated_time,
        op_sys         => \&_check_op_sys,
        priority       => \&_check_priority,
        product        => \&_check_product,
        remaining_time => \&_check_remaining_time,
        rep_platform   => \&_check_rep_platform,
        short_desc     => \&_check_short_desc,
        status_whiteboard => \&_check_status_whiteboard,
    };

    my @custom_fields = Bugzilla->get_fields({custom => 1, obsolete => 0});

    foreach my $field (@custom_fields) {
        my $validator;
        if ($field->type == FIELD_TYPE_SINGLE_SELECT) {
            $validator = \&_check_select_field;
        }
        elsif ($field->type == FIELD_TYPE_FREETEXT) {
            $validator = \&_check_freetext_field;
        }
        $validators->{$field->name} = $validator if $validator;
    }
    return $validators;
};

# Used in LogActivityEntry(). Gives the max length of lines in the
# activity table.
use constant MAX_LINE_LENGTH => 254;

# Used in ValidateComment(). Gives the max length allowed for a comment.
use constant MAX_COMMENT_LENGTH => 65535;

# The statuses that are valid on enter_bug.cgi and post_bug.cgi.
# The order is important--see _check_bug_status
use constant VALID_ENTRY_STATUS => qw(
    UNCONFIRMED
    NEW
    ASSIGNED
);

#####################################################################

sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
    my $param = shift;

    # If we get something that looks like a word (not a number),
    # make it the "name" param.
    if (!defined $param || (!ref($param) && $param !~ /^\d+$/)) {
        # But only if aliases are enabled.
        if (Bugzilla->params->{'usebugaliases'} && $param) {
            $param = { name => $param };
        }
        else {
            # Aliases are off, and we got something that's not a number.
            my $error_self = {};
            bless $error_self, $class;
            $error_self->{'bug_id'} = $param;
            $error_self->{'error'}  = 'InvalidBugId';
            return $error_self;
        }
    }

    unshift @_, $param;
    my $self = $class->SUPER::new(@_);

    # Bugzilla::Bug->new always returns something, but sets $self->{error}
    # if the bug wasn't found in the database.
    if (!$self) {
        my $error_self = {};
        bless $error_self, $class;
        $error_self->{'bug_id'} = ref($param) ? $param->{name} : $param;
        $error_self->{'error'}  = 'NotFound';
        return $error_self;
    }

    # XXX At some point these should be moved into accessors.
    # They only are here because this is how Bugzilla::Bug
    # originally did things, before it was a Bugzilla::Object.
    $self->{'isunconfirmed'} = ($self->{bug_status} eq 'UNCONFIRMED');
    $self->{'isopened'}      = is_open_state($self->{bug_status});

    return $self;
}

# Docs for create() (there's no POD in this file yet, but we very
# much need this documented right now):
#
# The same as Bugzilla::Object->create. Parameters are only required
# if they say so below.
#
# Params:
#
# C<product>     - B<Required> The name of the product this bug is being
#                  filed against.
# C<component>   - B<Required> The name of the component this bug is being
#                  filed against.
#
# C<bug_severity> - B<Required> The severity for the bug, a string.
# C<creation_ts>  - B<Required> A SQL timestamp for when the bug was created.
# C<short_desc>   - B<Required> A summary for the bug.
# C<op_sys>       - B<Required> The OS the bug was found against.
# C<priority>     - B<Required> The initial priority for the bug.
# C<rep_platform> - B<Required> The platform the bug was found against.
# C<version>      - B<Required> The version of the product the bug was found in.
#
# C<alias>        - An alias for this bug. Will be ignored if C<usebugaliases>
#                   is off.
# C<target_milestone> - When this bug is expected to be fixed.
# C<status_whiteboard> - A string.
# C<bug_status>   - The initial status of the bug, a string.
# C<bug_file_loc> - The URL field.
#
# C<assigned_to> - The full login name of the user who the bug is
#                  initially assigned to.
# C<qa_contact>  - The full login name of the QA Contact for this bug. 
#                  Will be ignored if C<useqacontact> is off.
#
# C<estimated_time> - For time-tracking. Will be ignored if 
#                     C<timetrackinggroup> is not set, or if the current
#                     user is not a member of the timetrackinggroup.
# C<deadline>       - For time-tracking. Will be ignored for the same
#                     reasons as C<estimated_time>.
sub create {
    my ($class, $params) = @_;
    my $dbh = Bugzilla->dbh;

    # These fields have default values which we can use if they are undefined.
    $params->{bug_severity} = Bugzilla->params->{defaultseverity}
      unless defined $params->{bug_severity};
    $params->{priority} = Bugzilla->params->{defaultpriority}
      unless defined $params->{priority};
    $params->{op_sys} = Bugzilla->params->{defaultopsys}
      unless defined $params->{op_sys};
    $params->{rep_platform} = Bugzilla->params->{defaultplatform}
      unless defined $params->{rep_platform};
    # Make sure a comment is always defined.
    $params->{comment} = '' unless defined $params->{comment};

    $class->check_required_create_fields($params);
    $params = $class->run_create_validators($params);

    # These are not a fields in the bugs table, so we don't pass them to
    # insert_create_data.
    my $cc_ids = $params->{cc};
    delete $params->{cc};
    my $groups = $params->{groups};
    delete $params->{groups};
    my $depends_on = $params->{dependson};
    delete $params->{dependson};
    my $blocked = $params->{blocked};
    delete $params->{blocked};
    my ($comment, $privacy) = ($params->{comment}, $params->{commentprivacy});
    delete $params->{comment};
    delete $params->{commentprivacy};

    # Set up the keyword cache for bug creation.
    my $keywords = $params->{keywords};
    $params->{keywords} = join(', ', sort {lc($a) cmp lc($b)} 
                                          map($_->name, @$keywords));

    # We don't want the bug to appear in the system until it's correctly
    # protected by groups.
    my $timestamp = $params->{creation_ts}; 
    delete $params->{creation_ts};

    $dbh->bz_lock_tables('bugs WRITE', 'bug_group_map WRITE', 
        'longdescs WRITE', 'cc WRITE', 'keywords WRITE', 'dependencies WRITE',
        'bugs_activity WRITE', 'fielddefs READ');

    my $bug = $class->insert_create_data($params);

    # Add the group restrictions
    my $sth_group = $dbh->prepare(
        'INSERT INTO bug_group_map (bug_id, group_id) VALUES (?, ?)');
    foreach my $group_id (@$groups) {
        $sth_group->execute($bug->bug_id, $group_id);
    }

    $dbh->do('UPDATE bugs SET creation_ts = ? WHERE bug_id = ?', undef,
             $timestamp, $bug->bug_id);
    # Update the bug instance as well
    $bug->{creation_ts} = $timestamp;

    # Add the CCs
    my $sth_cc = $dbh->prepare('INSERT INTO cc (bug_id, who) VALUES (?,?)');
    foreach my $user_id (@$cc_ids) {
        $sth_cc->execute($bug->bug_id, $user_id);
    }

    # Add in keywords
    my $sth_keyword = $dbh->prepare(
        'INSERT INTO keywords (bug_id, keywordid) VALUES (?, ?)');
    foreach my $keyword_id (map($_->id, @$keywords)) {
        $sth_keyword->execute($bug->bug_id, $keyword_id);
    }

    # Set up dependencies (blocked/dependson)
    my $sth_deps = $dbh->prepare(
        'INSERT INTO dependencies (blocked, dependson) VALUES (?, ?)');
    my $sth_bug_time = $dbh->prepare('UPDATE bugs SET delta_ts = ? WHERE bug_id = ?');

    foreach my $depends_on_id (@$depends_on) {
        $sth_deps->execute($bug->bug_id, $depends_on_id);
        # Log the reverse action on the other bug.
        LogActivityEntry($depends_on_id, 'blocked', '', $bug->bug_id,
                         $bug->{reporter_id}, $timestamp);
        $sth_bug_time->execute($timestamp, $depends_on_id);
    }
    foreach my $blocked_id (@$blocked) {
        $sth_deps->execute($blocked_id, $bug->bug_id);
        # Log the reverse action on the other bug.
        LogActivityEntry($blocked_id, 'dependson', '', $bug->bug_id,
                         $bug->{reporter_id}, $timestamp);
        $sth_bug_time->execute($timestamp, $blocked_id);
    }

    # And insert the comment. We always insert a comment on bug creation,
    # but sometimes it's blank.
    my @columns = qw(bug_id who bug_when thetext);
    my @values  = ($bug->bug_id, $bug->{reporter_id}, $timestamp, $comment);
    # We don't include the "isprivate" column unless it was specified. 
    # This allows it to fall back to its database default.
    if (defined $privacy) {
        push(@columns, 'isprivate');
        push(@values, $privacy);
    }
    my $qmarks = "?," x @columns;
    chop($qmarks);
    $dbh->do('INSERT INTO longdescs (' . join(',', @columns)  . ")
                   VALUES ($qmarks)", undef, @values);

    $dbh->bz_unlock_tables();

    return $bug;
}


sub run_create_validators {
    my $class  = shift;
    my $params = $class->SUPER::run_create_validators(@_);

    my $product = $params->{product};
    $params->{product_id} = $product->id;
    delete $params->{product};

    ($params->{bug_status}, $params->{everconfirmed})
        = $class->_check_bug_status($product, $params->{bug_status});

    $params->{target_milestone} = $class->_check_target_milestone($product,
        $params->{target_milestone});

    $params->{version} = $class->_check_version($product, $params->{version});

    $params->{keywords} = $class->_check_keywords($product, $params->{keywords});

    $params->{groups} = $class->_check_groups($product,
        $params->{groups});

    my $component = $class->_check_component($product, $params->{component});
    $params->{component_id} = $component->id;
    delete $params->{component};

    $params->{assigned_to} = 
        $class->_check_assigned_to($component, $params->{assigned_to});
    $params->{qa_contact} =
        $class->_check_qa_contact($component, $params->{qa_contact});
    $params->{cc} = $class->_check_cc($component, $params->{cc});

    # Callers cannot set Reporter, currently.
    $params->{reporter} = Bugzilla->user->id;

    $params->{creation_ts} ||= Bugzilla->dbh->selectrow_array('SELECT NOW()');
    $params->{delta_ts} = $params->{creation_ts};

    if ($params->{estimated_time}) {
        $params->{remaining_time} = $params->{estimated_time};
    }

    $class->_check_strict_isolation($product, $params->{cc},
                                    $params->{assigned_to}, $params->{qa_contact});

    ($params->{dependson}, $params->{blocked}) = 
        $class->_check_dependencies($product, $params->{dependson}, $params->{blocked});

    # You can't set these fields on bug creation (or sometimes ever).
    delete $params->{resolution};
    delete $params->{votes};
    delete $params->{lastdiffed};
    delete $params->{bug_id};

    return $params;
}

# This is the correct way to delete bugs from the DB.
# No bug should be deleted from anywhere else except from here.
#
sub remove_from_db {
    my ($self) = @_;
    my $dbh = Bugzilla->dbh;

    if ($self->{'error'}) {
        ThrowCodeError("bug_error", { bug => $self });
    }

    my $bug_id = $self->{'bug_id'};

    # tables having 'bugs.bug_id' as a foreign key:
    # - attachments
    # - bug_group_map
    # - bugs
    # - bugs_activity
    # - cc
    # - dependencies
    # - duplicates
    # - flags
    # - keywords
    # - longdescs
    # - votes

    # Also, the attach_data table uses attachments.attach_id as a foreign
    # key, and so indirectly depends on a bug deletion too.

    $dbh->bz_lock_tables('attachments WRITE', 'bug_group_map WRITE',
                         'bugs WRITE', 'bugs_activity WRITE', 'cc WRITE',
                         'dependencies WRITE', 'duplicates WRITE',
                         'flags WRITE', 'keywords WRITE',
                         'longdescs WRITE', 'votes WRITE',
                         'attach_data WRITE');

    $dbh->do("DELETE FROM bug_group_map WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM bugs_activity WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM cc WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM dependencies WHERE blocked = ? OR dependson = ?",
             undef, ($bug_id, $bug_id));
    $dbh->do("DELETE FROM duplicates WHERE dupe = ? OR dupe_of = ?",
             undef, ($bug_id, $bug_id));
    $dbh->do("DELETE FROM flags WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM keywords WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM longdescs WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM votes WHERE bug_id = ?", undef, $bug_id);

    # The attach_data table doesn't depend on bugs.bug_id directly.
    my $attach_ids =
        $dbh->selectcol_arrayref("SELECT attach_id FROM attachments
                                  WHERE bug_id = ?", undef, $bug_id);

    if (scalar(@$attach_ids)) {
        $dbh->do("DELETE FROM attach_data WHERE id IN (" .
                 join(",", @$attach_ids) . ")");
    }

    # Several of the previous tables also depend on attach_id.
    $dbh->do("DELETE FROM attachments WHERE bug_id = ?", undef, $bug_id);
    $dbh->do("DELETE FROM bugs WHERE bug_id = ?", undef, $bug_id);

    $dbh->bz_unlock_tables();

    # Now this bug no longer exists
    $self->DESTROY;
    return $self;
}

#####################################################################
# Validators
#####################################################################

sub _check_alias {
   my ($invocant, $alias) = @_;
   $alias = trim($alias);
   return undef if (!Bugzilla->params->{'usebugaliases'} || !$alias);
   ValidateBugAlias($alias);
   return $alias;
}

sub _check_assigned_to {
    my ($invocant, $component, $name) = @_;
    my $user = Bugzilla->user;

    $name = trim($name);
    # Default assignee is the component owner.
    my $id;
    if (!$user->in_group('editbugs', $component->product_id) || !$name) {
        $id = $component->default_assignee->id;
    } else {
        $id = login_to_id($name, THROW_ERROR);
    }
    return $id;
}

sub _check_bug_file_loc {
    my ($invocant, $url) = @_;
    # If bug_file_loc is "http://", the default, use an empty value instead.
    $url = '' if (!defined($url) || $url eq 'http://');
    return $url;
}

sub _check_bug_severity {
    my ($invocant, $severity) = @_;
    $severity = trim($severity);
    check_field('bug_severity', $severity);
    return $severity;
}

sub _check_bug_status {
    my ($invocant, $product, $status) = @_;
    my $user = Bugzilla->user;

    my @valid_statuses = VALID_ENTRY_STATUS;

    if ($user->in_group('editbugs', $product->id)
        || $user->in_group('canconfirm', $product->id)) {
       # Default to NEW if the user with privs hasn't selected another status.
       $status ||= 'NEW';
    }
    elsif (!$product->votes_to_confirm) {
        # Without privs, products that don't support UNCONFIRMED default to
        # NEW.
        $status = 'NEW';
    }
    else {
        $status = 'UNCONFIRMED';
    }

    # UNCONFIRMED becomes an invalid status if votes_to_confirm is 0,
    # even if you are in editbugs.
    shift @valid_statuses if !$product->votes_to_confirm;

    check_field('bug_status', $status, \@valid_statuses);
    return ($status, $status eq 'UNCONFIRMED' ? 0 : 1);
}

sub _check_cc {
    my ($invocant, $component, $ccs) = @_;
    return [map {$_->id} @{$component->initial_cc}] unless $ccs;

    my %cc_ids;
    foreach my $person (@$ccs) {
        next unless $person;
        my $id = login_to_id($person, THROW_ERROR);
        $cc_ids{$id} = 1;
    }

    # Enforce Default CC
    $cc_ids{$_->id} = 1 foreach (@{$component->initial_cc});

    return [keys %cc_ids];
}

sub _check_comment {
    my ($invocant, $comment) = @_;

    $comment = '' unless defined $comment;

    # Remove any trailing whitespace. Leading whitespace could be
    # a valid part of the comment.
    $comment =~ s/\s*$//s;
    $comment =~ s/\r\n?/\n/g; # Get rid of \r.

    ValidateComment($comment);

    if (Bugzilla->params->{"commentoncreate"} && !$comment) {
        ThrowUserError("description_required");
    }

    # On creation only, there must be a single-space comment, or
    # email will be supressed.
    $comment = ' ' if $comment eq '' && !ref($invocant);

    return $comment;
}

sub _check_commentprivacy {
    my ($invocant, $comment_privacy) = @_;
    my $insider_group = Bugzilla->params->{"insidergroup"};
    return ($insider_group && Bugzilla->user->in_group($insider_group) 
            && $comment_privacy) ? 1 : 0;
}

sub _check_component {
    my ($invocant, $product, $name) = @_;
    $name = trim($name);
    $name || ThrowUserError("require_component");
    my $obj = Bugzilla::Component::check_component($product, $name);
    return $obj;
}

sub _check_deadline {
    my ($invocant, $date) = @_;
    $date = trim($date);
    my $tt_group = Bugzilla->params->{"timetrackinggroup"};
    return undef unless $date && $tt_group 
                        && Bugzilla->user->in_group($tt_group);
    validate_date($date)
        || ThrowUserError('illegal_date', { date   => $date,
                                            format => 'YYYY-MM-DD' });
    return $date;
}

# Takes two comma/space-separated strings and returns arrayrefs
# of valid bug IDs.
sub _check_dependencies {
    my ($invocant, $product, $depends_on, $blocks) = @_;

    # Only editbugs users can set dependencies on bug entry.
    return ([], []) unless Bugzilla->user->in_group('editbugs', $product->id);

    $depends_on ||= '';
    $blocks     ||= '';

    # Make sure all the bug_ids are valid.
    my @results;
    foreach my $string ($depends_on, $blocks) {
        my @array = split(/[\s,]+/, $string);
        # Eliminate nulls
        @array = grep($_, @array);
        # $field is not passed to ValidateBugID to prevent adding new
        # dependencies on inaccessible bugs.
        ValidateBugID($_) foreach (@array);
        push(@results, \@array);
    }

    #                               dependson    blocks
    my %deps = ValidateDependencies($results[0], $results[1]);

    return ($deps{'dependson'}, $deps{'blocked'});
}

sub _check_estimated_time {
    return $_[0]->_check_time($_[1], 'estimated_time');
}

sub _check_groups {
    my ($invocant, $product, $group_ids) = @_;

    my $user = Bugzilla->user;

    my %add_groups;
    my $controls = $product->group_controls;

    foreach my $id (@$group_ids) {
        my $group = new Bugzilla::Group($id)
            || ThrowUserError("invalid_group_ID");

        # This can only happen if somebody hacked the enter_bug form.
        ThrowCodeError("inactive_group", { name => $group->name })
            unless $group->is_active;

        my $membercontrol = $controls->{$id}
                            && $controls->{$id}->{membercontrol};
        my $othercontrol  = $controls->{$id} 
                            && $controls->{$id}->{othercontrol};
        
        my $permit = ($membercontrol && $user->in_group($group->name))
                     || $othercontrol;

        $add_groups{$id} = 1 if $permit;
    }

    foreach my $id (keys %$controls) {
        next unless $controls->{$id}->{'group'}->is_active;
        my $membercontrol = $controls->{$id}->{membercontrol} || 0;
        my $othercontrol  = $controls->{$id}->{othercontrol}  || 0;

        # Add groups required
        if ($membercontrol == CONTROLMAPMANDATORY
            || ($othercontrol == CONTROLMAPMANDATORY
                && !$user->in_group_id($id))) 
        {
            # User had no option, bug needs to be in this group.
            $add_groups{$id} = 1;
        }
    }

    my @add_groups = keys %add_groups;
    return \@add_groups;
}

sub _check_keywords {
    my ($invocant, $product, $keyword_string) = @_;
    $keyword_string = trim($keyword_string);
    return [] if (!$keyword_string
                  || !Bugzilla->user->in_group('editbugs', $product->id));

    my %keywords;
    foreach my $keyword (split(/[\s,]+/, $keyword_string)) {
        next unless $keyword;
        my $obj = new Bugzilla::Keyword({ name => $keyword });
        ThrowUserError("unknown_keyword", { keyword => $keyword }) if !$obj;
        $keywords{$obj->id} = $obj;
    }
    return [values %keywords];
}

sub _check_product {
    my ($invocant, $name) = @_;
    # Check that the product exists and that the user
    # is allowed to enter bugs into this product.
    Bugzilla->user->can_enter_product($name, THROW_ERROR);
    # can_enter_product already does everything that check_product
    # would do for us, so we don't need to use it.
    my $obj = new Bugzilla::Product({ name => $name });
    return $obj;
}

sub _check_op_sys {
    my ($invocant, $op_sys) = @_;
    $op_sys = trim($op_sys);
    check_field('op_sys', $op_sys);
    return $op_sys;
}

sub _check_priority {
    my ($invocant, $priority) = @_;
    if (!Bugzilla->params->{'letsubmitterchoosepriority'}) {
        $priority = Bugzilla->params->{'defaultpriority'};
    }
    $priority = trim($priority);
    check_field('priority', $priority);

    return $priority;
}

sub _check_remaining_time {
    return $_[0]->_check_time($_[1], 'remaining_time');
}

sub _check_rep_platform {
    my ($invocant, $platform) = @_;
    $platform = trim($platform);
    check_field('rep_platform', $platform);
    return $platform;
}

sub _check_short_desc {
    my ($invocant, $short_desc) = @_;
    # Set the parameter to itself, but cleaned up
    $short_desc = clean_text($short_desc) if $short_desc;

    if (!defined $short_desc || $short_desc eq '') {
        ThrowUserError("require_summary");
    }
    return $short_desc;
}

sub _check_status_whiteboard { return defined $_[1] ? $_[1] : ''; }

# Unlike other checkers, this one doesn't return anything.
sub _check_strict_isolation {
    my ($invocant, $product, $cc_ids, $assignee_id, $qa_contact_id) = @_;

    return unless Bugzilla->params->{'strict_isolation'};

    my @related_users = @$cc_ids;
    push(@related_users, $assignee_id);

    if (Bugzilla->params->{'useqacontact'} && $qa_contact_id) {
        push(@related_users, $qa_contact_id);
    }

    # For each unique user in @related_users...(assignee and qa_contact
    # could be duplicates of users in the CC list)
    my %unique_users = map {$_ => 1} @related_users;
    my @blocked_users;
    foreach my $pid (keys %unique_users) {
        my $related_user = Bugzilla::User->new($pid);
        if (!$related_user->can_edit_product($product->id)) {
            push (@blocked_users, $related_user->login);
        }
    }
    if (scalar(@blocked_users)) {
        ThrowUserError("invalid_user_group",
            {'users' => \@blocked_users,
             'new' => 1,
             'product' => $product->name});
    }
}

sub _check_target_milestone {
    my ($invocant, $product, $target) = @_;
    $target = trim($target);
    $target = $product->default_milestone if !defined $target;
    check_field('target_milestone', $target,
            [map($_->name, @{$product->milestones})]);
    return $target;
}

sub _check_time {
    my ($invocant, $time, $field) = @_;
    my $tt_group = Bugzilla->params->{"timetrackinggroup"};
    return 0 unless $tt_group && Bugzilla->user->in_group($tt_group);
    $time = trim($time) || 0;
    ValidateTime($time, $field);
    return $time;
}

sub _check_qa_contact {
    my ($invocant, $component, $name) = @_;
    my $user = Bugzilla->user;

    return undef unless Bugzilla->params->{'useqacontact'};

    $name = trim($name);

    my $id;
    if (!$user->in_group('editbugs', $component->product_id) || !$name) {
        # We want to insert NULL into the database if we get a 0.
        $id = $component->default_qa_contact->id || undef;
    } else {
        $id = login_to_id($name, THROW_ERROR);
    }

    return $id;
}

sub _check_version {
    my ($invocant, $product, $version) = @_;
    $version = trim($version);
    check_field('version', $version, [map($_->name, @{$product->versions})]);
    return $version;
}

# Custom Field Validators

sub _check_freetext_field {
    my ($invocant, $text) = @_;

    $text = (defined $text) ? trim($text) : '';
    if (length($text) > MAX_FREETEXT_LENGTH) {
        ThrowUserError('freetext_too_long', { text => $text });
    }
    return $text;
}

sub _check_select_field {
    my ($invocant, $value, $field) = @_;
    $value = trim($value);
    check_field($field, $value);
    return $value;
}

#####################################################################
# Class Accessors
#####################################################################

sub fields {
    my $class = shift;

    return (
        # Standard Fields
        # Keep this ordering in sync with bugzilla.dtd.
        qw(bug_id alias creation_ts short_desc delta_ts
           reporter_accessible cclist_accessible
           classification_id classification
           product component version rep_platform op_sys
           bug_status resolution dup_id
           bug_file_loc status_whiteboard keywords
           priority bug_severity target_milestone
           dependson blocked votes everconfirmed
           reporter assigned_to cc),
    
        # Conditional Fields
        Bugzilla->params->{'useqacontact'} ? "qa_contact" : (),
        Bugzilla->params->{'timetrackinggroup'} ? 
            qw(estimated_time remaining_time actual_time deadline) : (),
    
        # Custom Fields
        Bugzilla->custom_field_names
    );
}


#####################################################################
# Instance Accessors
#####################################################################

# These subs are in alphabetical order, as much as possible.
# If you add a new sub, please try to keep it in alphabetical order
# with the other ones.

# Note: If you add a new method, remember that you must check the error
# state of the bug before returning any data. If $self->{error} is
# defined, then return something empty. Otherwise you risk potential
# security holes.

sub dup_id {
    my ($self) = @_;
    return $self->{'dup_id'} if exists $self->{'dup_id'};

    $self->{'dup_id'} = undef;
    return if $self->{'error'};

    if ($self->{'resolution'} eq 'DUPLICATE') { 
        my $dbh = Bugzilla->dbh;
        $self->{'dup_id'} =
          $dbh->selectrow_array(q{SELECT dupe_of 
                                  FROM duplicates
                                  WHERE dupe = ?},
                                undef,
                                $self->{'bug_id'});
    }
    return $self->{'dup_id'};
}

sub actual_time {
    my ($self) = @_;
    return $self->{'actual_time'} if exists $self->{'actual_time'};

    if ( $self->{'error'} || 
         !Bugzilla->user->in_group(Bugzilla->params->{"timetrackinggroup"}) ) {
        $self->{'actual_time'} = undef;
        return $self->{'actual_time'};
    }

    my $sth = Bugzilla->dbh->prepare("SELECT SUM(work_time)
                                      FROM longdescs 
                                      WHERE longdescs.bug_id=?");
    $sth->execute($self->{bug_id});
    $self->{'actual_time'} = $sth->fetchrow_array();
    return $self->{'actual_time'};
}

sub any_flags_requesteeble {
    my ($self) = @_;
    return $self->{'any_flags_requesteeble'} 
        if exists $self->{'any_flags_requesteeble'};
    return 0 if $self->{'error'};

    $self->{'any_flags_requesteeble'} = 
        grep($_->{'is_requesteeble'}, @{$self->flag_types});

    return $self->{'any_flags_requesteeble'};
}

sub attachments {
    my ($self) = @_;
    return $self->{'attachments'} if exists $self->{'attachments'};
    return [] if $self->{'error'};

    $self->{'attachments'} =
        Bugzilla::Attachment->get_attachments_by_bug($self->bug_id);
    return $self->{'attachments'};
}

sub assigned_to {
    my ($self) = @_;
    return $self->{'assigned_to'} if exists $self->{'assigned_to'};
    $self->{'assigned_to_id'} = 0 if $self->{'error'};
    $self->{'assigned_to'} = new Bugzilla::User($self->{'assigned_to_id'});
    return $self->{'assigned_to'};
}

sub blocked {
    my ($self) = @_;
    return $self->{'blocked'} if exists $self->{'blocked'};
    return [] if $self->{'error'};
    $self->{'blocked'} = EmitDependList("dependson", "blocked", $self->bug_id);
    return $self->{'blocked'};
}

# Even bugs in an error state always have a bug_id.
sub bug_id { $_[0]->{'bug_id'}; }

sub cc {
    my ($self) = @_;
    return $self->{'cc'} if exists $self->{'cc'};
    return [] if $self->{'error'};

    my $dbh = Bugzilla->dbh;
    $self->{'cc'} = $dbh->selectcol_arrayref(
        q{SELECT profiles.login_name FROM cc, profiles
           WHERE bug_id = ?
             AND cc.who = profiles.userid
        ORDER BY profiles.login_name},
      undef, $self->bug_id);

    $self->{'cc'} = undef if !scalar(@{$self->{'cc'}});

    return $self->{'cc'};
}

sub component {
    my ($self) = @_;
    return $self->{component} if exists $self->{component};
    return '' if $self->{error};
    ($self->{component}) = Bugzilla->dbh->selectrow_array(
        'SELECT name FROM components WHERE id = ?',
        undef, $self->{component_id});
    return $self->{component};
}

sub classification_id {
    my ($self) = @_;
    return $self->{classification_id} if exists $self->{classification_id};
    return 0 if $self->{error};
    ($self->{classification_id}) = Bugzilla->dbh->selectrow_array(
        'SELECT classification_id FROM products WHERE id = ?',
        undef, $self->{product_id});
    return $self->{classification_id};
}

sub classification {
    my ($self) = @_;
    return $self->{classification} if exists $self->{classification};
    return '' if $self->{error};
    ($self->{classification}) = Bugzilla->dbh->selectrow_array(
        'SELECT name FROM classifications WHERE id = ?',
        undef, $self->classification_id);
    return $self->{classification};
}

sub dependson {
    my ($self) = @_;
    return $self->{'dependson'} if exists $self->{'dependson'};
    return [] if $self->{'error'};
    $self->{'dependson'} = 
        EmitDependList("blocked", "dependson", $self->bug_id);
    return $self->{'dependson'};
}

sub flag_types {
    my ($self) = @_;
    return $self->{'flag_types'} if exists $self->{'flag_types'};
    return [] if $self->{'error'};

    # The types of flags that can be set on this bug.
    # If none, no UI for setting flags will be displayed.
    my $flag_types = Bugzilla::FlagType::match(
        {'target_type'  => 'bug',
         'product_id'   => $self->{'product_id'}, 
         'component_id' => $self->{'component_id'} });

    foreach my $flag_type (@$flag_types) {
        $flag_type->{'flags'} = Bugzilla::Flag::match(
            { 'bug_id'      => $self->bug_id,
              'type_id'     => $flag_type->{'id'},
              'target_type' => 'bug' });
    }

    $self->{'flag_types'} = $flag_types;

    return $self->{'flag_types'};
}

sub keywords {
    my ($self) = @_;
    return $self->{'keywords'} if exists $self->{'keywords'};
    return () if $self->{'error'};

    my $dbh = Bugzilla->dbh;
    my $list_ref = $dbh->selectcol_arrayref(
         "SELECT keyworddefs.name
            FROM keyworddefs, keywords
           WHERE keywords.bug_id = ?
             AND keyworddefs.id = keywords.keywordid
        ORDER BY keyworddefs.name",
        undef, ($self->bug_id));

    $self->{'keywords'} = join(', ', @$list_ref);
    return $self->{'keywords'};
}

sub longdescs {
    my ($self) = @_;
    return $self->{'longdescs'} if exists $self->{'longdescs'};
    return [] if $self->{'error'};
    $self->{'longdescs'} = GetComments($self->{bug_id});
    return $self->{'longdescs'};
}

sub milestoneurl {
    my ($self) = @_;
    return $self->{'milestoneurl'} if exists $self->{'milestoneurl'};
    return '' if $self->{'error'};

    $self->{'prod_obj'} ||= new Bugzilla::Product({name => $self->product});
    $self->{'milestoneurl'} = $self->{'prod_obj'}->milestone_url;
    return $self->{'milestoneurl'};
}

sub product {
    my ($self) = @_;
    return $self->{product} if exists $self->{product};
    return '' if $self->{error};
    ($self->{product}) = Bugzilla->dbh->selectrow_array(
        'SELECT name FROM products WHERE id = ?',
        undef, $self->{product_id});
    return $self->{product};
}

sub qa_contact {
    my ($self) = @_;
    return $self->{'qa_contact'} if exists $self->{'qa_contact'};
    return undef if $self->{'error'};

    if (Bugzilla->params->{'useqacontact'} && $self->{'qa_contact_id'}) {
        $self->{'qa_contact'} = new Bugzilla::User($self->{'qa_contact_id'});
    } else {
        # XXX - This is somewhat inconsistent with the assignee/reporter 
        # methods, which will return an empty User if they get a 0. 
        # However, we're keeping it this way now, for backwards-compatibility.
        $self->{'qa_contact'} = undef;
    }
    return $self->{'qa_contact'};
}

sub reporter {
    my ($self) = @_;
    return $self->{'reporter'} if exists $self->{'reporter'};
    $self->{'reporter_id'} = 0 if $self->{'error'};
    $self->{'reporter'} = new Bugzilla::User($self->{'reporter_id'});
    return $self->{'reporter'};
}


sub show_attachment_flags {
    my ($self) = @_;
    return $self->{'show_attachment_flags'} 
        if exists $self->{'show_attachment_flags'};
    return 0 if $self->{'error'};

    # The number of types of flags that can be set on attachments to this bug
    # and the number of flags on those attachments.  One of these counts must be
    # greater than zero in order for the "flags" column to appear in the table
    # of attachments.
    my $num_attachment_flag_types = Bugzilla::FlagType::count(
        { 'target_type'  => 'attachment',
          'product_id'   => $self->{'product_id'},
          'component_id' => $self->{'component_id'} });
    my $num_attachment_flags = Bugzilla::Flag::count(
        { 'target_type'  => 'attachment',
          'bug_id'       => $self->bug_id });

    $self->{'show_attachment_flags'} =
        ($num_attachment_flag_types || $num_attachment_flags);

    return $self->{'show_attachment_flags'};
}

sub use_votes {
    my ($self) = @_;
    return 0 if $self->{'error'};

    $self->{'prod_obj'} ||= new Bugzilla::Product({name => $self->product});

    return Bugzilla->params->{'usevotes'} 
           && $self->{'prod_obj'}->votes_per_user > 0;
}

sub groups {
    my $self = shift;
    return $self->{'groups'} if exists $self->{'groups'};
    return [] if $self->{'error'};

    my $dbh = Bugzilla->dbh;
    my @groups;

    # Some of this stuff needs to go into Bugzilla::User

    # For every group, we need to know if there is ANY bug_group_map
    # record putting the current bug in that group and if there is ANY
    # user_group_map record putting the user in that group.
    # The LEFT JOINs are checking for record existence.
    #
    my $grouplist = Bugzilla->user->groups_as_string;
    my $sth = $dbh->prepare(
             "SELECT DISTINCT groups.id, name, description," .
             " CASE WHEN bug_group_map.group_id IS NOT NULL" .
             " THEN 1 ELSE 0 END," .
             " CASE WHEN groups.id IN($grouplist) THEN 1 ELSE 0 END," .
             " isactive, membercontrol, othercontrol" .
             " FROM groups" . 
             " LEFT JOIN bug_group_map" .
             " ON bug_group_map.group_id = groups.id" .
             " AND bug_id = ?" .
             " LEFT JOIN group_control_map" .
             " ON group_control_map.group_id = groups.id" .
             " AND group_control_map.product_id = ? " .
             " WHERE isbuggroup = 1" .
             " ORDER BY description");
    $sth->execute($self->{'bug_id'},
                  $self->{'product_id'});

    while (my ($groupid, $name, $description, $ison, $ingroup, $isactive,
            $membercontrol, $othercontrol) = $sth->fetchrow_array()) {

        $membercontrol ||= 0;

        # For product groups, we only want to use the group if either
        # (1) The bit is set and not required, or
        # (2) The group is Shown or Default for members and
        #     the user is a member of the group.
        if ($ison ||
            ($isactive && $ingroup
                       && (($membercontrol == CONTROLMAPDEFAULT)
                           || ($membercontrol == CONTROLMAPSHOWN))
            ))
        {
            my $ismandatory = $isactive
              && ($membercontrol == CONTROLMAPMANDATORY);

            push (@groups, { "bit" => $groupid,
                             "name" => $name,
                             "ison" => $ison,
                             "ingroup" => $ingroup,
                             "mandatory" => $ismandatory,
                             "description" => $description });
        }
    }

    $self->{'groups'} = \@groups;

    return $self->{'groups'};
}

sub user {
    my $self = shift;
    return $self->{'user'} if exists $self->{'user'};
    return {} if $self->{'error'};

    my $user = Bugzilla->user;
    my $canmove = Bugzilla->params->{'move-enabled'} && $user->is_mover;

    my $prod_id = $self->{'product_id'};

    my $unknown_privileges = $user->in_group('editbugs', $prod_id);
    my $canedit = $unknown_privileges
                  || $user->id == $self->{assigned_to_id}
                  || (Bugzilla->params->{'useqacontact'}
                      && $self->{'qa_contact_id'}
                      && $user->id == $self->{qa_contact_id});
    my $canconfirm = $unknown_privileges
                     || $user->in_group('canconfirm', $prod_id);
    my $isreporter = $user->id
                     && $user->id == $self->{reporter_id};

    $self->{'user'} = {canmove    => $canmove,
                       canconfirm => $canconfirm,
                       canedit    => $canedit,
                       isreporter => $isreporter};
    return $self->{'user'};
}

sub choices {
    my $self = shift;
    return $self->{'choices'} if exists $self->{'choices'};
    return {} if $self->{'error'};

    $self->{'choices'} = {};
    $self->{prod_obj} ||= new Bugzilla::Product({name => $self->product});

    my @prodlist = map {$_->name} @{Bugzilla->user->get_enterable_products};
    # The current product is part of the popup, even if new bugs are no longer
    # allowed for that product
    if (lsearch(\@prodlist, $self->product) < 0) {
        push(@prodlist, $self->product);
        @prodlist = sort @prodlist;
    }

    # Hack - this array contains "". See bug 106589.
    my @res = grep ($_, @{settable_resolutions()});

    $self->{'choices'} =
      {
       'product' => \@prodlist,
       'rep_platform' => get_legal_field_values('rep_platform'),
       'priority'     => get_legal_field_values('priority'),
       'bug_severity' => get_legal_field_values('bug_severity'),
       'op_sys'       => get_legal_field_values('op_sys'),
       'bug_status'   => get_legal_field_values('bug_status'),
       'resolution'   => \@res,
       'component'    => [map($_->name, @{$self->{prod_obj}->components})],
       'version'      => [map($_->name, @{$self->{prod_obj}->versions})],
       'target_milestone' => [map($_->name, @{$self->{prod_obj}->milestones})],
      };

    return $self->{'choices'};
}

# List of resolutions that may be set directly by hand in the bug form.
# 'MOVED' and 'DUPLICATE' are excluded from the list because setting
# bugs to those resolutions requires a special process.
sub settable_resolutions {
    my $resolutions = get_legal_field_values('resolution');
    my $pos = lsearch($resolutions, 'DUPLICATE');
    if ($pos >= 0) {
        splice(@$resolutions, $pos, 1);
    }
    $pos = lsearch($resolutions, 'MOVED');
    if ($pos >= 0) {
        splice(@$resolutions, $pos, 1);
    }
    return $resolutions;
}

sub votes {
    my ($self) = @_;
    return 0 if $self->{error};
    return $self->{votes} if defined $self->{votes};

    my $dbh = Bugzilla->dbh;
    $self->{votes} = $dbh->selectrow_array(
        'SELECT SUM(vote_count) FROM votes
          WHERE bug_id = ? ' . $dbh->sql_group_by('bug_id'),
        undef, $self->bug_id);
    $self->{votes} ||= 0;
    return $self->{votes};
}

# Convenience Function. If you need speed, use this. If you need
# other Bug fields in addition to this, just create a new Bug with
# the alias.
# Queries the database for the bug with a given alias, and returns
# the ID of the bug if it exists or the undefined value if it doesn't.
sub bug_alias_to_id {
    my ($alias) = @_;
    return undef unless Bugzilla->params->{"usebugaliases"};
    my $dbh = Bugzilla->dbh;
    trick_taint($alias);
    return $dbh->selectrow_array(
        "SELECT bug_id FROM bugs WHERE alias = ?", undef, $alias);
}

#####################################################################
# Subroutines
#####################################################################

sub AppendComment {
    my ($bugid, $whoid, $comment, $isprivate, $timestamp, $work_time,
        $type, $extra_data) = @_;
    $work_time ||= 0;
    $type ||= CMT_NORMAL;
    my $dbh = Bugzilla->dbh;

    ValidateTime($work_time, "work_time") if $work_time;
    trick_taint($work_time);
    detaint_natural($type)
      || ThrowCodeError('bad_arg', {argument => 'type', function => 'AppendComment'});

    # Use the date/time we were given if possible (allowing calling code
    # to synchronize the comment's timestamp with those of other records).
    $timestamp ||= $dbh->selectrow_array('SELECT NOW()');

    $comment =~ s/\r\n/\n/g;     # Handle Windows-style line endings.
    $comment =~ s/\r/\n/g;       # Handle Mac-style line endings.

    if ($comment =~ /^\s*$/ && !$type) {  # Nothin' but whitespace
        return;
    }

    # Comments are always safe, because we always display their raw contents,
    # and we use them in a placeholder below.
    trick_taint($comment); 
    my $privacyval = $isprivate ? 1 : 0 ;
    $dbh->do(q{INSERT INTO longdescs
                      (bug_id, who, bug_when, thetext, isprivate, work_time,
                       type, extra_data)
               VALUES (?, ?, ?, ?, ?, ?, ?, ?)}, undef,
             ($bugid, $whoid, $timestamp, $comment, $privacyval, $work_time,
              $type, $extra_data));
    $dbh->do("UPDATE bugs SET delta_ts = ? WHERE bug_id = ?",
             undef, $timestamp, $bugid);
}

sub update_comment {
    my ($self, $comment_id, $new_comment) = @_;

    # Some validation checks.
    if ($self->{'error'}) {
        ThrowCodeError("bug_error", { bug => $self });
    }
    detaint_natural($comment_id)
      || ThrowCodeError('bad_arg', {argument => 'comment_id', function => 'update_comment'});

    # The comment ID must belong to this bug.
    my @current_comment_obj = grep {$_->{'id'} == $comment_id} @{$self->longdescs};
    scalar(@current_comment_obj)
      || ThrowCodeError('bad_arg', {argument => 'comment_id', function => 'update_comment'});

    # If the new comment is undefined, then there is nothing to update.
    # To delete a comment, an empty string should be passed.
    return unless defined $new_comment;
    $new_comment =~ s/\s*$//s;    # Remove trailing whitespaces.
    $new_comment =~ s/\r\n?/\n/g; # Handle Windows and Mac-style line endings.
    trick_taint($new_comment);

    # We assume ValidateComment() has already been called earlier.
    Bugzilla->dbh->do('UPDATE longdescs SET thetext = ? WHERE comment_id = ?',
                       undef, ($new_comment, $comment_id));

    # Update the comment object with this new text.
    $current_comment_obj[0]->{'body'} = $new_comment;
}

# Represents which fields from the bugs table are handled by process_bug.cgi.
sub editable_bug_fields {
    my @fields = Bugzilla->dbh->bz_table_columns('bugs');
    # Obsolete custom fields are not editable.
    my @obsolete_fields = Bugzilla->get_fields({obsolete => 1, custom => 1});
    @obsolete_fields = map { $_->name } @obsolete_fields;
    foreach my $remove ("bug_id", "reporter", "creation_ts", "delta_ts", "lastdiffed", @obsolete_fields) {
        my $location = lsearch(\@fields, $remove);
        splice(@fields, $location, 1);
    }
    # Sorted because the old @::log_columns variable, which this replaces,
    # was sorted.
    return sort(@fields);
}

# XXX - When Bug::update() will be implemented, we should make this routine
#       a private method.
sub EmitDependList {
    my ($myfield, $targetfield, $bug_id) = (@_);
    my $dbh = Bugzilla->dbh;
    my $list_ref = $dbh->selectcol_arrayref(
          "SELECT $targetfield FROM dependencies
            WHERE $myfield = ? ORDER BY $targetfield",
            undef, $bug_id);
    return $list_ref;
}

# Tells you whether or not the argument is a valid "open" state.
sub is_open_state {
    my ($state) = @_;
    return (grep($_ eq $state, BUG_STATE_OPEN) ? 1 : 0);
}

sub ValidateTime {
    my ($time, $field) = @_;

    # regexp verifies one or more digits, optionally followed by a period and
    # zero or more digits, OR we have a period followed by one or more digits
    # (allow negatives, though, so people can back out errors in time reporting)
    if ($time !~ /^-?(?:\d+(?:\.\d*)?|\.\d+)$/) {
        ThrowUserError("number_not_numeric",
                       {field => "$field", num => "$time"});
    }

    # Only the "work_time" field is allowed to contain a negative value.
    if ( ($time < 0) && ($field ne "work_time") ) {
        ThrowUserError("number_too_small",
                       {field => "$field", num => "$time", min_num => "0"});
    }

    if ($time > 99999.99) {
        ThrowUserError("number_too_large",
                       {field => "$field", num => "$time", max_num => "99999.99"});
    }
}

sub GetComments {
    my ($id, $comment_sort_order, $start, $end, $raw) = @_;
    my $dbh = Bugzilla->dbh;

    $comment_sort_order = $comment_sort_order ||
        Bugzilla->user->settings->{'comment_sort_order'}->{'value'};

    my $sort_order = ($comment_sort_order eq "oldest_to_newest") ? 'asc' : 'desc';

    my @comments;
    my @args = ($id);

    my $query = 'SELECT longdescs.comment_id AS id, profiles.realname AS name,
                        profiles.login_name AS email, ' .
                        $dbh->sql_date_format('longdescs.bug_when', '%Y.%m.%d %H:%i:%s') .
                      ' AS time, longdescs.thetext AS body, longdescs.work_time,
                        isprivate, already_wrapped, type, extra_data
                   FROM longdescs
             INNER JOIN profiles
                     ON profiles.userid = longdescs.who
                  WHERE longdescs.bug_id = ?';
    if ($start) {
        $query .= ' AND longdescs.bug_when > ?
                    AND longdescs.bug_when <= ?';
        push(@args, ($start, $end));
    }
    $query .= " ORDER BY longdescs.bug_when $sort_order";
    my $sth = $dbh->prepare($query);
    $sth->execute(@args);

    while (my $comment_ref = $sth->fetchrow_hashref()) {
        my %comment = %$comment_ref;

        $comment{'email'} .= Bugzilla->params->{'emailsuffix'};
        $comment{'name'} = $comment{'name'} || $comment{'email'};

        # If raw data is requested, do not format 'special' comments.
        $comment{'body'} = format_comment(\%comment) unless $raw;

        push (@comments, \%comment);
    }
   
    if ($comment_sort_order eq "newest_to_oldest_desc_first") {
        unshift(@comments, pop @comments);
    }

    return \@comments;
}

# Format language specific comments. This routine must not update
# $comment{'body'} itself, see BugMail::prepare_comments().
sub format_comment {
    my $comment = shift;
    my $body;

    if ($comment->{'type'} == CMT_DUPE_OF) {
        $body = $comment->{'body'} . "\n\n" .
                get_text('bug_duplicate_of', { dupe_of => $comment->{'extra_data'} });
    }
    elsif ($comment->{'type'} == CMT_HAS_DUPE) {
        $body = get_text('bug_has_duplicate', { dupe => $comment->{'extra_data'} });
    }
    elsif ($comment->{'type'} == CMT_POPULAR_VOTES) {
        $body = get_text('bug_confirmed_by_votes');
    }
    elsif ($comment->{'type'} == CMT_MOVED_TO) {
        $body = $comment->{'body'} . "\n\n" .
                get_text('bug_moved_to', { login => $comment->{'extra_data'} });
    }
    else {
        $body = $comment->{'body'};
    }
    return $body;
}

# Get the activity of a bug, starting from $starttime (if given).
# This routine assumes ValidateBugID has been previously called.
sub GetBugActivity {
    my ($id, $starttime) = @_;
    my $dbh = Bugzilla->dbh;

    # Arguments passed to the SQL query.
    my @args = ($id);

    # Only consider changes since $starttime, if given.
    my $datepart = "";
    if (defined $starttime) {
        trick_taint($starttime);
        push (@args, $starttime);
        $datepart = "AND bugs_activity.bug_when > ?";
    }

    # Only includes attachments the user is allowed to see.
    my $suppjoins = "";
    my $suppwhere = "";
    if (Bugzilla->params->{"insidergroup"} 
        && !Bugzilla->user->in_group(Bugzilla->params->{'insidergroup'})) 
    {
        $suppjoins = "LEFT JOIN attachments 
                   ON attachments.attach_id = bugs_activity.attach_id";
        $suppwhere = "AND COALESCE(attachments.isprivate, 0) = 0";
    }

    my $query = "
        SELECT COALESCE(fielddefs.description, " 
               # This is a hack - PostgreSQL requires both COALESCE
               # arguments to be of the same type, and this is the only
               # way supported by both MySQL 3 and PostgreSQL to convert
               # an integer to a string. MySQL 4 supports CAST.
               . $dbh->sql_string_concat('bugs_activity.fieldid', q{''}) .
               "), fielddefs.name, bugs_activity.attach_id, " .
        $dbh->sql_date_format('bugs_activity.bug_when', '%Y.%m.%d %H:%i:%s') .
            ", bugs_activity.removed, bugs_activity.added, profiles.login_name
          FROM bugs_activity
               $suppjoins
     LEFT JOIN fielddefs
            ON bugs_activity.fieldid = fielddefs.id
    INNER JOIN profiles
            ON profiles.userid = bugs_activity.who
         WHERE bugs_activity.bug_id = ?
               $datepart
               $suppwhere
      ORDER BY bugs_activity.bug_when";

    my $list = $dbh->selectall_arrayref($query, undef, @args);

    my @operations;
    my $operation = {};
    my $changes = [];
    my $incomplete_data = 0;

    foreach my $entry (@$list) {
        my ($field, $fieldname, $attachid, $when, $removed, $added, $who) = @$entry;
        my %change;
        my $activity_visible = 1;

        # check if the user should see this field's activity
        if ($fieldname eq 'remaining_time'
            || $fieldname eq 'estimated_time'
            || $fieldname eq 'work_time'
            || $fieldname eq 'deadline')
        {
            $activity_visible = 
                Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'}) ? 1 : 0;
        } else {
            $activity_visible = 1;
        }

        if ($activity_visible) {
            # This gets replaced with a hyperlink in the template.
            $field =~ s/^Attachment\s*// if $attachid;

            # Check for the results of an old Bugzilla data corruption bug
            $incomplete_data = 1 if ($added =~ /^\?/ || $removed =~ /^\?/);

            # An operation, done by 'who' at time 'when', has a number of
            # 'changes' associated with it.
            # If this is the start of a new operation, store the data from the
            # previous one, and set up the new one.
            if ($operation->{'who'}
                && ($who ne $operation->{'who'}
                    || $when ne $operation->{'when'}))
            {
                $operation->{'changes'} = $changes;
                push (@operations, $operation);

                # Create new empty anonymous data structures.
                $operation = {};
                $changes = [];
            }

            $operation->{'who'} = $who;
            $operation->{'when'} = $when;

            $change{'field'} = $field;
            $change{'fieldname'} = $fieldname;
            $change{'attachid'} = $attachid;
            $change{'removed'} = $removed;
            $change{'added'} = $added;
            push (@$changes, \%change);
        }
    }

    if ($operation->{'who'}) {
        $operation->{'changes'} = $changes;
        push (@operations, $operation);
    }

    return(\@operations, $incomplete_data);
}

# Update the bugs_activity table to reflect changes made in bugs.
sub LogActivityEntry {
    my ($i, $col, $removed, $added, $whoid, $timestamp) = @_;
    my $dbh = Bugzilla->dbh;
    # in the case of CCs, deps, and keywords, there's a possibility that someone
    # might try to add or remove a lot of them at once, which might take more
    # space than the activity table allows.  We'll solve this by splitting it
    # into multiple entries if it's too long.
    while ($removed || $added) {
        my ($removestr, $addstr) = ($removed, $added);
        if (length($removestr) > MAX_LINE_LENGTH) {
            my $commaposition = find_wrap_point($removed, MAX_LINE_LENGTH);
            $removestr = substr($removed, 0, $commaposition);
            $removed = substr($removed, $commaposition);
            $removed =~ s/^[,\s]+//; # remove any comma or space
        } else {
            $removed = ""; # no more entries
        }
        if (length($addstr) > MAX_LINE_LENGTH) {
            my $commaposition = find_wrap_point($added, MAX_LINE_LENGTH);
            $addstr = substr($added, 0, $commaposition);
            $added = substr($added, $commaposition);
            $added =~ s/^[,\s]+//; # remove any comma or space
        } else {
            $added = ""; # no more entries
        }
        trick_taint($addstr);
        trick_taint($removestr);
        my $fieldid = get_field_id($col);
        $dbh->do("INSERT INTO bugs_activity
                  (bug_id, who, bug_when, fieldid, removed, added)
                  VALUES (?, ?, ?, ?, ?, ?)",
                  undef, ($i, $whoid, $timestamp, $fieldid, $removestr, $addstr));
    }
}

# CountOpenDependencies counts the number of open dependent bugs for a
# list of bugs and returns a list of bug_id's and their dependency count
# It takes one parameter:
#  - A list of bug numbers whose dependencies are to be checked
sub CountOpenDependencies {
    my (@bug_list) = @_;
    my @dependencies;
    my $dbh = Bugzilla->dbh;

    my $sth = $dbh->prepare(
          "SELECT blocked, COUNT(bug_status) " .
            "FROM bugs, dependencies " .
           "WHERE blocked IN (" . (join "," , @bug_list) . ") " .
             "AND bug_id = dependson " .
             "AND bug_status IN ('" . (join "','", BUG_STATE_OPEN)  . "') " .
          $dbh->sql_group_by('blocked'));
    $sth->execute();

    while (my ($bug_id, $dependencies) = $sth->fetchrow_array()) {
        push(@dependencies, { bug_id       => $bug_id,
                              dependencies => $dependencies });
    }

    return @dependencies;
}

sub ValidateComment {
    my ($comment) = @_;

    if (defined($comment) && length($comment) > MAX_COMMENT_LENGTH) {
        ThrowUserError("comment_too_long");
    }
}

# If a bug is moved to a product which allows less votes per bug
# compared to the previous product, extra votes need to be removed.
sub RemoveVotes {
    my ($id, $who, $reason) = (@_);
    my $dbh = Bugzilla->dbh;

    my $whopart = ($who) ? " AND votes.who = $who" : "";

    my $sth = $dbh->prepare("SELECT profiles.login_name, " .
                            "profiles.userid, votes.vote_count, " .
                            "products.votesperuser, products.maxvotesperbug " .
                            "FROM profiles " . 
                            "LEFT JOIN votes ON profiles.userid = votes.who " .
                            "LEFT JOIN bugs ON votes.bug_id = bugs.bug_id " .
                            "LEFT JOIN products ON products.id = bugs.product_id " .
                            "WHERE votes.bug_id = ? " . $whopart);
    $sth->execute($id);
    my @list;
    while (my ($name, $userid, $oldvotes, $votesperuser, $maxvotesperbug) = $sth->fetchrow_array()) {
        push(@list, [$name, $userid, $oldvotes, $votesperuser, $maxvotesperbug]);
    }

    # @messages stores all emails which have to be sent, if any.
    # This array is passed to the caller which will send these emails itself.
    my @messages = ();

    if (scalar(@list)) {
        foreach my $ref (@list) {
            my ($name, $userid, $oldvotes, $votesperuser, $maxvotesperbug) = (@$ref);

            $maxvotesperbug = min($votesperuser, $maxvotesperbug);

            # If this product allows voting and the user's votes are in
            # the acceptable range, then don't do anything.
            next if $votesperuser && $oldvotes <= $maxvotesperbug;

            # If the user has more votes on this bug than this product
            # allows, then reduce the number of votes so it fits
            my $newvotes = $maxvotesperbug;

            my $removedvotes = $oldvotes - $newvotes;

            if ($newvotes) {
                $dbh->do("UPDATE votes SET vote_count = ? " .
                         "WHERE bug_id = ? AND who = ?",
                         undef, ($newvotes, $id, $userid));
            } else {
                $dbh->do("DELETE FROM votes WHERE bug_id = ? AND who = ?",
                         undef, ($id, $userid));
            }

            # Notice that we did not make sure that the user fit within the $votesperuser
            # range.  This is considered to be an acceptable alternative to losing votes
            # during product moves.  Then next time the user attempts to change their votes,
            # they will be forced to fit within the $votesperuser limit.

            # Now lets send the e-mail to alert the user to the fact that their votes have
            # been reduced or removed.
            my $vars = {
                'to' => $name . Bugzilla->params->{'emailsuffix'},
                'bugid' => $id,
                'reason' => $reason,

                'votesremoved' => $removedvotes,
                'votesold' => $oldvotes,
                'votesnew' => $newvotes,
            };

            my $voter = new Bugzilla::User($userid);
            my $template = Bugzilla->template_inner($voter->settings->{'lang'}->{'value'});

            my $msg;
            $template->process("email/votes-removed.txt.tmpl", $vars, \$msg);
            push(@messages, $msg);
        }
        Bugzilla->template_inner("");

        my $votes = $dbh->selectrow_array("SELECT SUM(vote_count) " .
                                          "FROM votes WHERE bug_id = ?",
                                          undef, $id) || 0;
        $dbh->do("UPDATE bugs SET votes = ? WHERE bug_id = ?",
                 undef, ($votes, $id));
    }
    # Now return the array containing emails to be sent.
    return \@messages;
}

# If a user votes for a bug, or the number of votes required to
# confirm a bug has been reduced, check if the bug is now confirmed.
sub CheckIfVotedConfirmed {
    my ($id, $who) = (@_);
    my $dbh = Bugzilla->dbh;

    my ($votes, $status, $everconfirmed, $votestoconfirm, $timestamp) =
        $dbh->selectrow_array("SELECT votes, bug_status, everconfirmed, " .
                              "       votestoconfirm, NOW() " .
                              "FROM bugs INNER JOIN products " .
                              "                  ON products.id = bugs.product_id " .
                              "WHERE bugs.bug_id = ?",
                              undef, $id);

    my $ret = 0;
    if ($votes >= $votestoconfirm && !$everconfirmed) {
        if ($status eq 'UNCONFIRMED') {
            my $fieldid = get_field_id("bug_status");
            $dbh->do("UPDATE bugs SET bug_status = 'NEW', everconfirmed = 1, " .
                     "delta_ts = ? WHERE bug_id = ?",
                     undef, ($timestamp, $id));
            $dbh->do("INSERT INTO bugs_activity " .
                     "(bug_id, who, bug_when, fieldid, removed, added) " .
                     "VALUES (?, ?, ?, ?, ?, ?)",
                     undef, ($id, $who, $timestamp, $fieldid, 'UNCONFIRMED', 'NEW'));
        }
        else {
            $dbh->do("UPDATE bugs SET everconfirmed = 1, delta_ts = ? " .
                     "WHERE bug_id = ?", undef, ($timestamp, $id));
        }

        my $fieldid = get_field_id("everconfirmed");
        $dbh->do("INSERT INTO bugs_activity " .
                 "(bug_id, who, bug_when, fieldid, removed, added) " .
                 "VALUES (?, ?, ?, ?, ?, ?)",
                 undef, ($id, $who, $timestamp, $fieldid, '0', '1'));

        AppendComment($id, $who, "", 0, $timestamp, 0, CMT_POPULAR_VOTES);

        $ret = 1;
    }
    return $ret;
}

################################################################################
# check_can_change_field() defines what users are allowed to change. You
# can add code here for site-specific policy changes, according to the
# instructions given in the Bugzilla Guide and below. Note that you may also
# have to update the Bugzilla::Bug::user() function to give people access to the
# options that they are permitted to change.
#
# check_can_change_field() returns true if the user is allowed to change this
# field, and false if they are not.
#
# The parameters to this method are as follows:
# $field    - name of the field in the bugs table the user is trying to change
# $oldvalue - what they are changing it from
# $newvalue - what they are changing it to
# $PrivilegesRequired - return the reason of the failure, if any
# $data     - hash containing relevant parameters, e.g. from the CGI object
################################################################################
sub check_can_change_field {
    my $self = shift;
    my ($field, $oldvalue, $newvalue, $PrivilegesRequired, $data) = (@_);
    my $user = Bugzilla->user;

    $oldvalue = defined($oldvalue) ? $oldvalue : '';
    $newvalue = defined($newvalue) ? $newvalue : '';

    # Return true if they haven't changed this field at all.
    if ($oldvalue eq $newvalue) {
        return 1;
    } elsif (trim($oldvalue) eq trim($newvalue)) {
        return 1;
    # numeric fields need to be compared using ==
    } elsif (($field eq 'estimated_time' || $field eq 'remaining_time')
             && $newvalue ne $data->{'dontchange'}
             && $oldvalue == $newvalue)
    {
        return 1;
    }

    # Allow anyone to change comments.
    if ($field =~ /^longdesc/) {
        return 1;
    }

    # Ignore the assigned_to field if the bug is not being reassigned
    if ($field eq 'assigned_to'
        && $data->{'knob'} ne 'reassignbycomponent'
        && $data->{'knob'} ne 'reassign')
    {
        return 1;
    }

    # If the user isn't allowed to change a field, we must tell him who can.
    # We store the required permission set into the $PrivilegesRequired
    # variable which gets passed to the error template.
    #
    # $PrivilegesRequired = 0 : no privileges required;
    # $PrivilegesRequired = 1 : the reporter, assignee or an empowered user;
    # $PrivilegesRequired = 2 : the assignee or an empowered user;
    # $PrivilegesRequired = 3 : an empowered user.

    # Allow anyone with (product-specific) "editbugs" privs to change anything.
    if ($user->in_group('editbugs', $self->{'product_id'})) {
        return 1;
    }

    # *Only* users with (product-specific) "canconfirm" privs can confirm bugs.
    if ($field eq 'canconfirm'
        || ($field eq 'bug_status'
            && $oldvalue eq 'UNCONFIRMED'
            && is_open_state($newvalue)))
    {
        $$PrivilegesRequired = 3;
        return $user->in_group('canconfirm', $self->{'product_id'});
    }

    # Make sure that a valid bug ID has been given.
    if (!$self->{'error'}) {
        # Allow the assignee to change anything else.
        if ($self->{'assigned_to_id'} == $user->id) {
            return 1;
        }

        # Allow the QA contact to change anything else.
        if (Bugzilla->params->{'useqacontact'}
            && $self->{'qa_contact_id'}
            && ($self->{'qa_contact_id'} == $user->id))
        {
            return 1;
        }
    }

    # At this point, the user is either the reporter or an
    # unprivileged user. We first check for fields the reporter
    # is not allowed to change.

    # The reporter may not:
    # - reassign bugs, unless the bugs are assigned to him;
    #   in that case we will have already returned 1 above
    #   when checking for the assignee of the bug.
    if ($field eq 'assigned_to') {
        $$PrivilegesRequired = 2;
        return 0;
    }
    # - change the QA contact
    if ($field eq 'qa_contact') {
        $$PrivilegesRequired = 2;
        return 0;
    }
    # - change the target milestone
    if ($field eq 'target_milestone') {
        $$PrivilegesRequired = 2;
        return 0;
    }
    # - change the priority (unless he could have set it originally)
    if ($field eq 'priority'
        && !Bugzilla->params->{'letsubmitterchoosepriority'})
    {
        $$PrivilegesRequired = 2;
        return 0;
    }

    # The reporter is allowed to change anything else.
    if (!$self->{'error'} && $self->{'reporter_id'} == $user->id) {
        return 1;
    }

    # If we haven't returned by this point, then the user doesn't
    # have the necessary permissions to change this field.
    $$PrivilegesRequired = 1;
    return 0;
}

#
# Field Validation
#

# Validates and verifies a bug ID, making sure the number is a 
# positive integer, that it represents an existing bug in the
# database, and that the user is authorized to access that bug.
# We detaint the number here, too.
sub ValidateBugID {
    my ($id, $field) = @_;
    my $dbh = Bugzilla->dbh;
    my $user = Bugzilla->user;

    # Get rid of leading '#' (number) mark, if present.
    $id =~ s/^\s*#//;
    # Remove whitespace
    $id = trim($id);

    # If the ID isn't a number, it might be an alias, so try to convert it.
    my $alias = $id;
    if (!detaint_natural($id)) {
        $id = bug_alias_to_id($alias);
        $id || ThrowUserError("invalid_bug_id_or_alias",
                              {'bug_id' => $alias,
                               'field'  => $field });
    }
    
    # Modify the calling code's original variable to contain the trimmed,
    # converted-from-alias ID.
    $_[0] = $id;
    
    # First check that the bug exists
    $dbh->selectrow_array("SELECT bug_id FROM bugs WHERE bug_id = ?", undef, $id)
      || ThrowUserError("invalid_bug_id_non_existent", {'bug_id' => $id});

    return if (defined $field && ($field eq "dependson" || $field eq "blocked"));
    
    return if $user->can_see_bug($id);

    # The user did not pass any of the authorization tests, which means they
    # are not authorized to see the bug.  Display an error and stop execution.
    # The error the user sees depends on whether or not they are logged in
    # (i.e. $user->id contains the user's positive integer ID).
    if ($user->id) {
        ThrowUserError("bug_access_denied", {'bug_id' => $id});
    } else {
        ThrowUserError("bug_access_query", {'bug_id' => $id});
    }
}

# ValidateBugAlias:
#   Check that the bug alias is valid and not used by another bug.  If 
#   curr_id is specified, verify the alias is not used for any other
#   bug id.  
sub ValidateBugAlias {
    my ($alias, $curr_id) = @_;
    my $dbh = Bugzilla->dbh;

    $alias = trim($alias || "");
    trick_taint($alias);

    if ($alias eq "") {
        ThrowUserError("alias_not_defined");
    }

    # Make sure the alias isn't too long.
    if (length($alias) > 20) {
        ThrowUserError("alias_too_long");
    }

    # Make sure the alias is unique.
    my $query = "SELECT bug_id FROM bugs WHERE alias = ?";
    if ($curr_id && detaint_natural($curr_id)) {
        $query .= " AND bug_id != $curr_id";
    }
    my $id = $dbh->selectrow_array($query, undef, $alias); 

    my $vars = {};
    $vars->{'alias'} = $alias;
    if ($id) {
        $vars->{'bug_id'} = $id;
        ThrowUserError("alias_in_use", $vars);
    }

    # Make sure the alias isn't just a number.
    if ($alias =~ /^\d+$/) {
        ThrowUserError("alias_is_numeric", $vars);
    }

    # Make sure the alias has no commas or spaces.
    if ($alias =~ /[, ]/) {
        ThrowUserError("alias_has_comma_or_space", $vars);
    }

    $_[0] = $alias;
}

# Validate and return a hash of dependencies
sub ValidateDependencies {
    my $fields = {};
    # These can be arrayrefs or they can be strings.
    $fields->{'dependson'} = shift;
    $fields->{'blocked'} = shift;
    my $id = shift || 0;

    unless (defined($fields->{'dependson'})
            || defined($fields->{'blocked'}))
    {
        return;
    }

    my $dbh = Bugzilla->dbh;
    my %deps;
    my %deptree;
    foreach my $pair (["blocked", "dependson"], ["dependson", "blocked"]) {
        my ($me, $target) = @{$pair};
        $deptree{$target} = [];
        $deps{$target} = [];
        next unless $fields->{$target};

        my %seen;
        my $target_array = ref($fields->{$target}) ? $fields->{$target}
                           : [split(/[\s,]+/, $fields->{$target})];
        foreach my $i (@$target_array) {
            if ($id == $i) {
                ThrowUserError("dependency_loop_single");
            }
            if (!exists $seen{$i}) {
                push(@{$deptree{$target}}, $i);
                $seen{$i} = 1;
            }
        }
        # populate $deps{$target} as first-level deps only.
        # and find remainder of dependency tree in $deptree{$target}
        @{$deps{$target}} = @{$deptree{$target}};
        my @stack = @{$deps{$target}};
        while (@stack) {
            my $i = shift @stack;
            my $dep_list =
                $dbh->selectcol_arrayref("SELECT $target
                                          FROM dependencies
                                          WHERE $me = ?", undef, $i);
            foreach my $t (@$dep_list) {
                # ignore any _current_ dependencies involving this bug,
                # as they will be overwritten with data from the form.
                if ($t != $id && !exists $seen{$t}) {
                    push(@{$deptree{$target}}, $t);
                    push @stack, $t;
                    $seen{$t} = 1;
                }
            }
        }
    }

    my @deps   = @{$deptree{'dependson'}};
    my @blocks = @{$deptree{'blocked'}};
    my %union = ();
    my %isect = ();
    foreach my $b (@deps, @blocks) { $union{$b}++ && $isect{$b}++ }
    my @isect = keys %isect;
    if (scalar(@isect) > 0) {
        ThrowUserError("dependency_loop_multi", {'deps' => \@isect});
    }
    return %deps;
}


#####################################################################
# Autoloaded Accessors
#####################################################################

# Determines whether an attribute access trapped by the AUTOLOAD function
# is for a valid bug attribute.  Bug attributes are properties and methods
# predefined by this module as well as bug fields for which an accessor
# can be defined by AUTOLOAD at runtime when the accessor is first accessed.
#
# XXX Strangely, some predefined attributes are on the list, but others aren't,
# and the original code didn't specify why that is.  Presumably the only
# attributes that need to be on this list are those that aren't predefined;
# we should verify that and update the list accordingly.
#
sub _validate_attribute {
    my ($attribute) = @_;

    my @valid_attributes = (
        # Miscellaneous properties and methods.
        qw(error groups product_id component_id
           longdescs milestoneurl attachments
           isopened isunconfirmed
           flag_types num_attachment_flag_types
           show_attachment_flags any_flags_requesteeble),

        # Bug fields.
        Bugzilla::Bug->fields
    );

    return grep($attribute eq $_, @valid_attributes) ? 1 : 0;
}

sub AUTOLOAD {
  use vars qw($AUTOLOAD);
  my $attr = $AUTOLOAD;

  $attr =~ s/.*:://;
  return unless $attr=~ /[^A-Z]/;
  if (!_validate_attribute($attr)) {
      require Carp;
      Carp::confess("invalid bug attribute $attr");
  }

  no strict 'refs';
  *$AUTOLOAD = sub {
      my $self = shift;
      if (defined $self->{$attr}) {
          return $self->{$attr};
      } else {
          return '';
      }
  };

  goto &$AUTOLOAD;
}

1;
