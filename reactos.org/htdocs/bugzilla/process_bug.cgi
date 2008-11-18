#!/usr/bin/perl -wT
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
#                 Dave Miller <justdave@syndicomm.com>
#                 Christopher Aillon <christopher@aillon.com>
#                 Myk Melez <myk@mozilla.org>
#                 Jeff Hedlund <jeff.hedlund@matrixsi.com>
#                 Frédéric Buclin <LpSolit@gmail.com>
#                 Lance Larsh <lance.larsh@oracle.com>
#                 Akamai Technologies <bugzilla-dev@akamai.com>

# Implementation notes for this file:
#
# 1) the 'id' form parameter is validated early on, and if it is not a valid
# bugid an error will be reported, so it is OK for later code to simply check
# for a defined form 'id' value, and it can assume a valid bugid.
#
# 2) If the 'id' form parameter is not defined (after the initial validation),
# then we are processing multiple bugs, and @idlist will contain the ids.
#
# 3) If we are processing just the one id, then it is stored in @idlist for
# later processing.

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Bug;
use Bugzilla::BugMail;
use Bugzilla::Mailer;
use Bugzilla::User;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Field;
use Bugzilla::Product;
use Bugzilla::Component;
use Bugzilla::Keyword;
use Bugzilla::Flag;

my $user = Bugzilla->login(LOGIN_REQUIRED);
local our $whoid = $user->id;
my $grouplist = $user->groups_as_string;

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
local our $vars = {};
$vars->{'use_keywords'} = 1 if Bugzilla::Keyword::keyword_count();

my @editable_bug_fields = editable_bug_fields();

my $requiremilestone = 0;
local our $PrivilegesRequired = 0;

######################################################################
# Subroutines
######################################################################

sub BugInGroupId {
    my ($bug_id, $group_id) = @_;
    detaint_natural($bug_id);
    detaint_natural($group_id);
    my ($in_group) = Bugzilla->dbh->selectrow_array(
        "SELECT CASE WHEN bug_id != 0 THEN 1 ELSE 0 END
           FROM bug_group_map
          WHERE bug_id = ? AND group_id = ?", undef, ($bug_id, $group_id));
    return $in_group;
}

# This function checks if there are any default groups defined.
# If so, then groups may have to be changed when bugs move from
# one bug to another.
sub AnyDefaultGroups {
    my $product_id = shift;
    my $dbh = Bugzilla->dbh;
    my $grouplist = Bugzilla->user->groups_as_string;

    my $and_product = $product_id ? ' AND product_id = ? ' : '';
    my @args = (CONTROLMAPDEFAULT);
    unshift(@args, $product_id) if $product_id;

    my $any_default =
        $dbh->selectrow_array("SELECT 1
                                 FROM group_control_map
                           INNER JOIN groups
                                   ON groups.id = group_control_map.group_id
                                WHERE isactive != 0
                                 $and_product
                                  AND membercontrol = ?
                                  AND group_id IN ($grouplist) " .
                                 $dbh->sql_limit(1),
                                undef, @args);
    return $any_default;
}

# Used to send email when an update is done.
sub send_results {
    my ($bug_id, $vars) = @_;
    my $template = Bugzilla->template;
    if (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
         Bugzilla::BugMail::Send($bug_id, $vars->{'mailrecipients'});
    }
    else {
        $template->process("bug/process/results.html.tmpl", $vars)
            || ThrowTemplateError($template->error());
    }
    $vars->{'header_done'} = 1;
}

######################################################################
# Begin Data/Security Validation
######################################################################

# Create a list of IDs of all bugs being modified in this request.
# This list will either consist of a single bug number from the "id"
# form/URL field or a series of numbers from multiple form/URL fields
# named "id_x" where "x" is the bug number.
# For each bug being modified, make sure its ID is a valid bug number 
# representing an existing bug that the user is authorized to access.
my @idlist;
if (defined $cgi->param('id')) {
  my $id = $cgi->param('id');
  ValidateBugID($id);

  # Store the validated, and detainted id back in the cgi data, as
  # lots of later code will need it, and will obtain it from there
  $cgi->param('id', $id);
  push @idlist, $id;
} else {
    foreach my $i ($cgi->param()) {
        if ($i =~ /^id_([1-9][0-9]*)/) {
            my $id = $1;
            ValidateBugID($id);
            push @idlist, $id;
        }
    }
}

# Make sure there are bugs to process.
scalar(@idlist) || ThrowUserError("no_bugs_chosen");

# Build a bug object using $cgi->param('id') as ID.
# If there are more than one bug changed at once, the bug object will be
# empty, which doesn't matter.
my $bug = new Bugzilla::Bug(scalar $cgi->param('id'));

# Make sure form param 'dontchange' is defined so it can be compared to easily.
$cgi->param('dontchange','') unless defined $cgi->param('dontchange');

# Make sure the 'knob' param is defined; else set it to 'none'.
$cgi->param('knob', 'none') unless defined $cgi->param('knob');

# Validate all timetracking fields
foreach my $field ("estimated_time", "work_time", "remaining_time") {
    if (defined $cgi->param($field)) {
        my $er_time = trim($cgi->param($field));
        if ($er_time ne $cgi->param('dontchange')) {
            Bugzilla::Bug::ValidateTime($er_time, $field);
        }
    }
}

if (Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'})) {
    my $wk_time = $cgi->param('work_time');
    if ($cgi->param('comment') =~ /^\s*$/ && $wk_time && $wk_time != 0) {
        ThrowUserError('comment_required');
    }
}

ValidateComment(scalar $cgi->param('comment'));

# If the bug(s) being modified have dependencies, validate them
# and rebuild the list with the validated values.  This is important
# because there are situations where validation changes the value
# instead of throwing an error, f.e. when one or more of the values
# is a bug alias that gets converted to its corresponding bug ID
# during validation.
foreach my $field ("dependson", "blocked") {
    if ($cgi->param('id')) {
        my @old = @{$bug->$field};
        my @new;
        foreach my $id (split(/[\s,]+/, $cgi->param($field))) {
            next unless $id;
            ValidateBugID($id, $field);
            push @new, $id;
        }
        $cgi->param($field, join(",", @new));
        my ($removed, $added) = diff_arrays(\@old, \@new);
        foreach my $id (@$added , @$removed) {
            # ValidateBugID is called without $field here so that it will
            # throw an error if any of the changed bugs are not visible.
            ValidateBugID($id);
            if (Bugzilla->params->{"strict_isolation"}) {
                my $deltabug = new Bugzilla::Bug($id);
                if (!$user->can_edit_product($deltabug->{'product_id'})) {
                    $vars->{'field'} = $field;
                    ThrowUserError("illegal_change_deps", $vars);
                }
            }
        }
        if ((@$added || @$removed)
            && !$bug->check_can_change_field($field, 0, 1, \$PrivilegesRequired))
        {
            $vars->{'privs'} = $PrivilegesRequired;
            $vars->{'field'} = $field;
            ThrowUserError("illegal_change", $vars);
        }
    } else {
        # Bugzilla does not support mass-change of dependencies so they
        # are not validated.  To prevent a URL-hacking risk, the dependencies
        # are deleted for mass-changes.
        $cgi->delete($field);
    }
}

# do a match on the fields if applicable

# The order of these function calls is important, as Flag::validate
# assumes User::match_field has ensured that the values
# in the requestee fields are legitimate user email addresses.
&Bugzilla::User::match_field($cgi, {
    'qa_contact'                => { 'type' => 'single' },
    'newcc'                     => { 'type' => 'multi'  },
    'masscc'                    => { 'type' => 'multi'  },
    'assigned_to'               => { 'type' => 'single' },
    '^requestee(_type)?-(\d+)$' => { 'type' => 'multi'  },
});

# Validate flags in all cases. validate() should not detect any
# reference to flags if $cgi->param('id') is undefined.
Bugzilla::Flag::validate($cgi, $cgi->param('id'));

######################################################################
# End Data/Security Validation
######################################################################

print $cgi->header() unless Bugzilla->usage_mode == USAGE_MODE_EMAIL;
$vars->{'title_tag'} = "bug_processed";

# Set the title if we can see a mid-air coming. This test may have false
# negatives, but never false positives, and should catch the majority of cases.
# It only works at all in the single bug case.
if (defined $cgi->param('id')) {
    my $delta_ts = $dbh->selectrow_array(
        q{SELECT delta_ts FROM bugs WHERE bug_id = ?},
        undef, $cgi->param('id'));
    
    if (defined $cgi->param('delta_ts') && $cgi->param('delta_ts') ne $delta_ts)
    {
        $vars->{'title_tag'} = "mid_air";
    }
}

# Set up the vars for navigational <link> elements
my @bug_list;
if ($cgi->cookie("BUGLIST") && defined $cgi->param('id')) {
    @bug_list = split(/:/, $cgi->cookie("BUGLIST"));
    $vars->{'bug_list'} = \@bug_list;
}

# This function checks if there is a comment required for a specific
# function and tests, if the comment was given.
# If comments are required for functions is defined by params.
#
sub CheckonComment {
    my ($function) = (@_);
    my $cgi = Bugzilla->cgi;
    
    # Param is 1 if comment should be added !
    my $ret = Bugzilla->params->{ "commenton" . $function };

    # Allow without comment in case of undefined Params.
    $ret = 0 unless ( defined( $ret ));

    if( $ret ) {
        if (!defined $cgi->param('comment')
            || $cgi->param('comment') =~ /^\s*$/) {
            # No comment - sorry, action not allowed !
            ThrowUserError("comment_required");
        } else {
            $ret = 0;
        }
    }
    return( ! $ret ); # Return val has to be inverted
}

# Figure out whether or not the user is trying to change the product
# (either the "product" variable is not set to "don't change" or the
# user is changing a single bug and has changed the bug's product),
# and make the user verify the version, component, target milestone,
# and bug groups if so.
my $oldproduct = '';
if (defined $cgi->param('id')) {
    $oldproduct = $dbh->selectrow_array(
        q{SELECT name FROM products INNER JOIN bugs
        ON products.id = bugs.product_id WHERE bug_id = ?},
        undef, $cgi->param('id'));
}

# At this point, the product must be defined, even if set to "dontchange".
defined($cgi->param('product'))
  || ThrowCodeError('undefined_field', { field => 'product' });

if (((defined $cgi->param('id') && $cgi->param('product') ne $oldproduct) 
     || (!$cgi->param('id')
         && $cgi->param('product') ne $cgi->param('dontchange')))
    && CheckonComment( "reassignbycomponent" ))
{
    # Check to make sure they actually have the right to change the product
    if (!$bug->check_can_change_field('product', $oldproduct, $cgi->param('product'),
                                      \$PrivilegesRequired))
    {
        $vars->{'oldvalue'} = $oldproduct;
        $vars->{'newvalue'} = $cgi->param('product');
        $vars->{'field'} = 'product';
        $vars->{'privs'} = $PrivilegesRequired;
        ThrowUserError("illegal_change", $vars);
    }

    my $prod = $cgi->param('product');
    my $prod_obj = new Bugzilla::Product({name => $prod});
    trick_taint($prod);

    # If at least one bug does not belong to the product we are
    # moving to, we have to check whether or not the user is
    # allowed to enter bugs into that product.
    # Note that this check must be done early to avoid the leakage
    # of component, version and target milestone names.
    my $check_can_enter =
        $dbh->selectrow_array("SELECT 1 FROM bugs
                               INNER JOIN products
                               ON bugs.product_id = products.id
                               WHERE products.name != ?
                               AND bugs.bug_id IN
                               (" . join(',', @idlist) . ") " .
                               $dbh->sql_limit(1),
                               undef, $prod);

    if ($check_can_enter) { $user->can_enter_product($prod, 1) }

    # note that when this script is called from buglist.cgi (rather
    # than show_bug.cgi), it's possible that the product will be changed
    # but that the version and/or component will be set to 
    # "--dont_change--" but still happen to be correct.  in this case,
    # the if statement will incorrectly trigger anyway.  this is a 
    # pretty weird case, and not terribly unreasonable behavior, but 
    # worthy of a comment, perhaps.
    #
    my @version_names = map($_->name, @{$prod_obj->versions});
    my @component_names = map($_->name, @{$prod_obj->components});
    my $vok = 0;
    if (defined $cgi->param('version')) {
        $vok = lsearch(\@version_names, $cgi->param('version')) >= 0;
    }
    my $cok = 0;
    if (defined $cgi->param('component')) {
        $cok = lsearch(\@component_names, $cgi->param('component')) >= 0;
    }

    my $mok = 1;   # so it won't affect the 'if' statement if milestones aren't used
    my @milestone_names = ();
    if ( Bugzilla->params->{"usetargetmilestone"} ) {
       @milestone_names = map($_->name, @{$prod_obj->milestones});
       $mok = 0;
       if (defined $cgi->param('target_milestone')) {
           $mok = lsearch(\@milestone_names, $cgi->param('target_milestone')) >= 0;
       }
    }

    # We cannot be sure if the component is the same by only checking $cok; the
    # current component name could exist in the new product. So always display
    # the form and use the confirm_product_change param to check if that was
    # shown. Also show the verification form if the product-specific fields
    # somehow still need to be verified, or if we need to verify whether or not
    # to add the bugs to their new product's group.
    my $has_default_groups = AnyDefaultGroups($prod_obj->id);

    if (!$vok || !$cok || !$mok || !defined $cgi->param('confirm_product_change')
        || ($has_default_groups && !defined $cgi->param('addtonewgroup'))) {

        if (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
            if (!$vok) {
                ThrowUserError('version_not_valid', {
                    version => $cgi->param('version'),
                    product => $cgi->param('product')});
            }
            if (!$cok) {
                ThrowUserError('component_not_valid', {
                    product => $cgi->param('product'),
                    name    => $cgi->param('component')});
            }
            if (!$mok) {
                ThrowUserError('milestone_not_valid', {
                    product   => $cgi->param('product'),
                    milestone => $cgi->param('target_milestone')});
            }
        }
        
        if (!$vok || !$cok || !$mok
            || !defined $cgi->param('confirm_product_change'))
        {
            $vars->{'verify_fields'} = 1;
            my %defaults;
            # We set the defaults to these fields to the old value,
            # if it's a valid option, otherwise we use the default where
            # that's appropriate
            $vars->{'versions'} = \@version_names;
            if ($vok) {
                $defaults{'version'} = $cgi->param('version');
            }
            elsif (scalar(@version_names) == 1) {
                $defaults{'version'} = $version_names[0];
            }

            $vars->{'components'} = \@component_names;
            if ($cok) {
                $defaults{'component'} = $cgi->param('component');
            }
            elsif (scalar(@component_names) == 1) {
                $defaults{'component'} = $component_names[0];
            }

            if (Bugzilla->params->{"usetargetmilestone"}) {
                $vars->{'use_target_milestone'} = 1;
                $vars->{'milestones'} = \@milestone_names;
                if ($mok) {
                    $defaults{'target_milestone'} = $cgi->param('target_milestone');
                } else {
                    $defaults{'target_milestone'} = $dbh->selectrow_array(
                        q{SELECT defaultmilestone FROM products 
                        WHERE name = ?}, undef, $prod);
                }
            }
            else {
                $vars->{'use_target_milestone'} = 0;
            }
            $vars->{'defaults'} = \%defaults;
        }
        else {
            $vars->{'verify_fields'} = 0;
        }

        $vars->{'verify_bug_group'} = ($has_default_groups
                                       && !defined $cgi->param('addtonewgroup'));

        # Get the ID of groups which are no longer valid in the new product.
        # If the bug was restricted to some group which still exists in the new
        # product, leave it alone, independently of your privileges.
        my $gids =
          $dbh->selectcol_arrayref("SELECT bgm.group_id
                                      FROM bug_group_map AS bgm
                                     WHERE bgm.bug_id IN (" . join(', ', @idlist) . ")
                                       AND bgm.group_id NOT IN
                                           (SELECT gcm.group_id
                                              FROM group_control_map AS gcm
                                             WHERE gcm.product_id = ?
                                               AND gcm.membercontrol IN (?, ?, ?))",
                                     undef, ($prod_obj->id, CONTROLMAPSHOWN,
                                             CONTROLMAPDEFAULT, CONTROLMAPMANDATORY));

        $vars->{'old_groups'} = Bugzilla::Group->new_from_list($gids);

        $template->process("bug/process/verify-new-product.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }
}

# At this point, the component must be defined, even if set to "dontchange".
defined($cgi->param('component'))
  || ThrowCodeError('undefined_field', { field => 'component' });

# Confirm that the reporter of the current bug can access the bug we are duping to.
sub DuplicateUserConfirm {
    my $cgi = Bugzilla->cgi;
    my $dbh = Bugzilla->dbh;
    my $template = Bugzilla->template;

    # if we've already been through here, then exit
    if (defined $cgi->param('confirm_add_duplicate')) {
        return;
    }

    # Remember that we validated both these ids earlier, so we know
    # they are both valid bug ids
    my $dupe = $cgi->param('id');
    my $original = $cgi->param('dup_id');
    
    my $reporter = $dbh->selectrow_array(
        q{SELECT reporter FROM bugs WHERE bug_id = ?}, undef, $dupe);
    my $rep_user = Bugzilla::User->new($reporter);

    if ($rep_user->can_see_bug($original)) {
        $cgi->param('confirm_add_duplicate', '1');
        return;
    }
    elsif (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
        # The email interface defaults to the safe alternative, which is
        # not CC'ing the user.
        $cgi->param('confirm_add_duplicate', 0);
        return;
    }

    $vars->{'cclist_accessible'} = $dbh->selectrow_array(
        q{SELECT cclist_accessible FROM bugs WHERE bug_id = ?},
        undef, $original);
    
    # Once in this part of the subroutine, the user has not been auto-validated
    # and the duper has not chosen whether or not to add to CC list, so let's
    # ask the duper what he/she wants to do.
    
    $vars->{'original_bug_id'} = $original;
    $vars->{'duplicate_bug_id'} = $dupe;
    
    # Confirm whether or not to add the reporter to the cc: list
    # of the original bug (the one this bug is being duped against).
    print Bugzilla->cgi->header();
    $template->process("bug/process/confirm-duplicate.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

if (defined $cgi->param('id')) {
    # since this means that we were called from show_bug.cgi, now is a good
    # time to do a whole bunch of error checking that can't easily happen when
    # we've been called from buglist.cgi, because buglist.cgi only tweaks
    # values that have been changed instead of submitting all the new values.
    # (XXX those error checks need to happen too, but implementing them 
    # is more work in the current architecture of this script...)
    my $prod_obj = Bugzilla::Product::check_product($cgi->param('product'));
    check_field('component', scalar $cgi->param('component'), 
                [map($_->name, @{$prod_obj->components})]);
    check_field('version', scalar $cgi->param('version'),
                [map($_->name, @{$prod_obj->versions})]);
    if ( Bugzilla->params->{"usetargetmilestone"} ) {
        check_field('target_milestone', scalar $cgi->param('target_milestone'), 
                    [map($_->name, @{$prod_obj->milestones})]);
    }
    check_field('rep_platform', scalar $cgi->param('rep_platform'));
    check_field('op_sys',       scalar $cgi->param('op_sys'));
    check_field('priority',     scalar $cgi->param('priority'));
    check_field('bug_severity', scalar $cgi->param('bug_severity'));

    # Those fields only have to exist. We don't validate their value here.
    foreach my $field_name ('bug_file_loc', 'short_desc', 'longdesclength') {
        defined($cgi->param($field_name))
          || ThrowCodeError('undefined_field', { field => $field_name });
    }
    $cgi->param('short_desc', clean_text($cgi->param('short_desc')));

    if (trim($cgi->param('short_desc')) eq "") {
        ThrowUserError("require_summary");
    }
}

my $action = trim($cgi->param('action') || '');

if ($action eq Bugzilla->params->{'move-button-text'}) {
    Bugzilla->params->{'move-enabled'} || ThrowUserError("move_bugs_disabled");

    $user->is_mover || ThrowUserError("auth_failure", {action => 'move',
                                                       object => 'bugs'});

    # Moved bugs are marked as RESOLVED MOVED.
    my $sth = $dbh->prepare("UPDATE bugs
                                SET bug_status = 'RESOLVED',
                                    resolution = 'MOVED',
                                    delta_ts = ?
                              WHERE bug_id = ?");
    # Bugs cannot be a dupe and moved at the same time.
    my $sth2 = $dbh->prepare("DELETE FROM duplicates WHERE dupe = ?");

    my $comment = "";
    if (defined $cgi->param('comment') && $cgi->param('comment') !~ /^\s*$/) {
        $comment = $cgi->param('comment');
    }

    $dbh->bz_lock_tables('bugs WRITE', 'bugs_activity WRITE', 'duplicates WRITE',
                         'longdescs WRITE', 'profiles READ', 'groups READ',
                         'bug_group_map READ', 'group_group_map READ',
                         'user_group_map READ', 'classifications READ',
                         'products READ', 'components READ', 'votes READ',
                         'cc READ', 'fielddefs READ');

    my $timestamp = $dbh->selectrow_array("SELECT NOW()");
    my @bugs;
    # First update all moved bugs.
    foreach my $id (@idlist) {
        my $bug = new Bugzilla::Bug($id);
        push(@bugs, $bug);

        $sth->execute($timestamp, $id);
        $sth2->execute($id);

        AppendComment($id, $whoid, $comment, 0, $timestamp, 0, CMT_MOVED_TO, $user->login);

        if ($bug->bug_status ne 'RESOLVED') {
            LogActivityEntry($id, 'bug_status', $bug->bug_status,
                             'RESOLVED', $whoid, $timestamp);
        }
        if ($bug->resolution ne 'MOVED') {
            LogActivityEntry($id, 'resolution', $bug->resolution,
                             'MOVED', $whoid, $timestamp);
        }
    }
    $dbh->bz_unlock_tables();

    # Now send emails.
    foreach my $id (@idlist) {
        $vars->{'mailrecipients'} = { 'changer' => $user->login };
        $vars->{'id'} = $id;
        $vars->{'type'} = "move";
        send_results($id, $vars);
    }
    # Prepare and send all data about these bugs to the new database
    my $to = Bugzilla->params->{'move-to-address'};
    $to =~ s/@/\@/;
    my $from = Bugzilla->params->{'moved-from-address'};
    $from =~ s/@/\@/;
    my $msg = "To: $to\n";
    $msg .= "From: Bugzilla <" . $from . ">\n";
    $msg .= "Subject: Moving bug(s) " . join(', ', @idlist) . "\n\n";

    my @fieldlist = (Bugzilla::Bug->fields, 'group', 'long_desc',
                     'attachment', 'attachmentdata');
    my %displayfields;
    foreach (@fieldlist) {
        $displayfields{$_} = 1;
    }

    $template->process("bug/show.xml.tmpl", { bugs => \@bugs,
                                              displayfields => \%displayfields,
                                            }, \$msg)
      || ThrowTemplateError($template->error());

    $msg .= "\n";
    MessageToMTA($msg);

    # End the response page.
    unless (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
        $template->process("bug/navigate.html.tmpl", $vars)
            || ThrowTemplateError($template->error());
        $template->process("global/footer.html.tmpl", $vars)
            || ThrowTemplateError($template->error());
    }
    exit;
}


$::query = "UPDATE bugs SET";
$::comma = "";
local our @values;
umask(0);

sub _remove_remaining_time {
    my $cgi = Bugzilla->cgi;
    if (Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'})) {
        if ( defined $cgi->param('remaining_time') 
             && $cgi->param('remaining_time') > 0 )
        {
            $cgi->param('remaining_time', 0);
            $vars->{'message'} = "remaining_time_zeroed";
        }
    }
    else {
        DoComma();
        $::query .= "remaining_time = 0";
    }
}

sub DoComma {
    $::query .= "$::comma\n    ";
    $::comma = ",";
}

# $everconfirmed is used by ChangeStatus() to determine whether we are
# confirming the bug or not.
local our $everconfirmed;
sub DoConfirm {
    my $bug = shift;
    if ($bug->check_can_change_field("canconfirm", 0, 1, 
                                     \$PrivilegesRequired)) 
    {
        DoComma();
        $::query .= "everconfirmed = 1";
        $everconfirmed = 1;
    }
}

sub ChangeStatus {
    my ($str) = (@_);
    my $cgi = Bugzilla->cgi;
    my $dbh = Bugzilla->dbh;

    if (!$cgi->param('dontchange')
        || $str ne $cgi->param('dontchange')) {
        DoComma();
        if ($cgi->param('knob') eq 'reopen') {
            # When reopening, we need to check whether the bug was ever
            # confirmed or not
            $::query .= "bug_status = CASE WHEN everconfirmed = 1 THEN " .
                        $dbh->quote($str) . " ELSE 'UNCONFIRMED' END";
        } elsif (is_open_state($str)) {
            # Note that we cannot combine this with the above branch - here we
            # need to check if bugs.bug_status is open, (since we don't want to
            # reopen closed bugs when reassigning), while above the whole point
            # is to reopen a closed bug.
            # Currently, the UI doesn't permit a user to reassign a closed bug
            # from the single bug page (only during a mass change), but they
            # could still hack the submit, so don't restrict this extended
            # check to the mass change page for safety/sanity/consistency
            # purposes.

            # The logic for this block is:
            # If the new state is open:
            #   If the old state was open
            #     If the bug was confirmed
            #       - move it to the new state
            #     Else
            #       - Set the state to unconfirmed
            #   Else
            #     - leave it as it was

            # This is valid only because 'reopen' is the only thing which moves
            # from closed to open, and it's handled above
            # This also relies on the fact that confirming and accepting have
            # already called DoConfirm before this is called

            my @open_state = map($dbh->quote($_), BUG_STATE_OPEN);
            my $open_state = join(", ", @open_state);

            # If we are changing everconfirmed to 1, we have to take this change
            # into account and the new bug status is given by $str.
            my $cond = $dbh->quote($str);
            # If we are not setting everconfirmed, the new bug status depends on
            # the actual value of everconfirmed, which is bug-specific.
            unless ($everconfirmed) {
                $cond = "(CASE WHEN everconfirmed = 1 THEN " . $cond .
                        " ELSE 'UNCONFIRMED' END)";
            }
            $::query .= "bug_status = CASE WHEN bug_status IN($open_state) THEN " .
                                      $cond . " ELSE bug_status END";
        } else {
            $::query .= "bug_status = ?";
            push(@values, $str);
        }
        # If bugs are reassigned and their status is "UNCONFIRMED", they
        # should keep this status instead of "NEW" as suggested here.
        # This point is checked for each bug later in the code.
        $cgi->param('bug_status', $str);
    }
}

sub ChangeResolution {
    my ($bug, $str) = (@_);
    my $dbh = Bugzilla->dbh;
    my $cgi = Bugzilla->cgi;

    if (!$cgi->param('dontchange')
        || $str ne $cgi->param('dontchange'))
    {
        # Make sure the user is allowed to change the resolution.
        # If the user is changing several bugs at once using the UI,
        # then he has enough privs to do so. In the case he is hacking
        # the URL, we don't care if he reads --UNKNOWN-- as a resolution
        # in the error message.
        my $old_resolution = '-- UNKNOWN --';
        my $bug_id = $cgi->param('id');
        if ($bug_id) {
            $old_resolution =
                $dbh->selectrow_array('SELECT resolution FROM bugs WHERE bug_id = ?',
                                       undef, $bug_id);
        }
        unless ($bug->check_can_change_field('resolution', $old_resolution, $str,
                                             \$PrivilegesRequired))
        {
            $vars->{'oldvalue'} = $old_resolution;
            $vars->{'newvalue'} = $str;
            $vars->{'field'} = 'resolution';
            $vars->{'privs'} = $PrivilegesRequired;
            ThrowUserError("illegal_change", $vars);
        }

        DoComma();
        $::query .= "resolution = ?";
        trick_taint($str);
        push(@values, $str);
        # We define this variable here so that customized installations
        # may set rules based on the resolution in Bug::check_can_change_field().
        $cgi->param('resolution', $str);
    }
}

# Changing this so that it will process groups from checkboxes instead of
# select lists.  This means that instead of looking for the bit-X values in
# the form, we need to loop through all the bug groups this user has access
# to, and for each one, see if it's selected.
# If the form element isn't present, or the user isn't in the group, leave
# it as-is

my @groupAdd = ();
my @groupDel = ();

my $groups = $dbh->selectall_arrayref(
    qq{SELECT groups.id, isactive FROM groups
        WHERE id IN($grouplist) AND isbuggroup = 1});
foreach my $group (@$groups) {
    my ($b, $isactive) = @$group;
    # The multiple change page may not show all groups a bug is in
    # (eg product groups when listing more than one product)
    # Only consider groups which were present on the form. We can't do this
    # for single bug changes because non-checked checkboxes aren't present.
    # All the checkboxes should be shown in that case, though, so it isn't
    # an issue there
    #
    # For bug updates that come from email_in.pl there will not be any
    # bit-X fields defined unless the user is explicitly changing the
    # state of the specified group (by including lines such as @bit-XX = 1
    # or @bit-XX = 0 in the body of the email).  Therefore if we are in
    # USAGE_MODE_EMAIL we will only change the group setting if bit-XX
    # is defined.
     if ((defined $cgi->param('id') && Bugzilla->usage_mode != USAGE_MODE_EMAIL)
         || defined $cgi->param("bit-$b"))
     {
        if (!$cgi->param("bit-$b")) {
            push(@groupDel, $b);
        } elsif ($cgi->param("bit-$b") == 1 && $isactive) {
            push(@groupAdd, $b);
        }
    }
}

foreach my $field ("rep_platform", "priority", "bug_severity",
                   "bug_file_loc", "short_desc", "version", "op_sys",
                   "target_milestone", "status_whiteboard") {
    if (defined $cgi->param($field)) {
        if (!$cgi->param('dontchange')
            || $cgi->param($field) ne $cgi->param('dontchange')) {
            DoComma();
            $::query .= "$field = ?";
            my $value = trim($cgi->param($field));
            trick_taint($value);
            push(@values, $value);
        }
    }
}

# Add custom fields data to the query that will update the database.
foreach my $field (Bugzilla->get_fields({custom => 1, obsolete => 0})) {
    my $fname = $field->name;
    if (defined $cgi->param($fname)
        && (!$cgi->param('dontchange')
            || $cgi->param($fname) ne $cgi->param('dontchange')))
    {
        DoComma();
        $::query .= "$fname = ?";
        my $value = $cgi->param($fname);
        check_field($fname, $value) if ($field->type == FIELD_TYPE_SINGLE_SELECT);
        $value = Bugzilla::Bug->_check_freetext_field($value)
          if ($field->type == FIELD_TYPE_FREETEXT);
        trick_taint($value);
        push(@values, $value);
    }
}

my $product;
my $prod_changed = 0;
my @newprod_ids;
if ($cgi->param('product') ne $cgi->param('dontchange')) {
    $product = Bugzilla::Product::check_product(scalar $cgi->param('product'));

    DoComma();
    $::query .= "product_id = ?";
    push(@values, $product->id);
    @newprod_ids = ($product->id);
    # If the bug remains in the same product, leave $prod_changed set to 0.
    # Even with 'strict_isolation' turned on, we ignore users who already
    # play a role for the bug; else you would never be able to edit it.
    # If you want to move the bug to another product, then you first have to
    # remove these users from the bug.
    unless (defined $cgi->param('id') && $bug->product_id == $product->id) {
        $prod_changed = 1;
    }
} else {
    @newprod_ids = @{$dbh->selectcol_arrayref("SELECT DISTINCT product_id
                                               FROM bugs 
                                               WHERE bug_id IN (" .
                                                   join(',', @idlist) . 
                                               ")")};
    if (scalar(@newprod_ids) == 1) {
        $product = new Bugzilla::Product($newprod_ids[0]);
    }
}

my $component;
if ($cgi->param('component') ne $cgi->param('dontchange')) {
    if (scalar(@newprod_ids) > 1) {
        ThrowUserError("no_component_change_for_multiple_products");
    }
    $component =
        Bugzilla::Component::check_component($product, scalar $cgi->param('component'));

    # This parameter is required later when checking fields the user can change.
    $cgi->param('component_id', $component->id);
    DoComma();
    $::query .= "component_id = ?";
    push(@values, $component->id);
}

# If this installation uses bug aliases, and the user is changing the alias,
# add this change to the query.
if (Bugzilla->params->{"usebugaliases"} && defined $cgi->param('alias')) {
    my $alias = trim($cgi->param('alias'));
    
    # Since aliases are unique (like bug numbers), they can only be changed
    # for one bug at a time, so ignore the alias change unless only a single
    # bug is being changed.
    if (scalar(@idlist) == 1) {
        # Add the alias change to the query.  If the field contains the blank 
        # value, make the field be NULL to indicate that the bug has no alias.
        # Otherwise, if the field contains a value, update the record 
        # with that value.
        DoComma();
        if ($alias ne "") {
            ValidateBugAlias($alias, $idlist[0]);
            $::query .= "alias = ?";
            push(@values, $alias);
        } else {
            $::query .= "alias = NULL";
        }
    }
}

# If the user is submitting changes from show_bug.cgi for a single bug,
# and that bug is restricted to a group, process the checkboxes that
# allowed the user to set whether or not the reporter
# and cc list can see the bug even if they are not members of all groups 
# to which the bug is restricted.
if (defined $cgi->param('id')) {
    my ($havegroup) = $dbh->selectrow_array(
        q{SELECT group_id FROM bug_group_map WHERE bug_id = ?},
        undef, $cgi->param('id'));
    if ( $havegroup ) {
        foreach my $field ('reporter_accessible', 'cclist_accessible') {
            if ($bug->check_can_change_field($field, 0, 1, \$PrivilegesRequired)) {
                DoComma();
                $cgi->param($field, $cgi->param($field) ? '1' : '0');
                $::query .= " $field = ?";
                push(@values, $cgi->param($field));
            }
            else {
                $cgi->delete($field);
            }
        }
    }
}

if ( defined $cgi->param('id') &&
     (Bugzilla->params->{"insidergroup"} 
      && Bugzilla->user->in_group(Bugzilla->params->{"insidergroup"})) ) 
{

    my $sth = $dbh->prepare('UPDATE longdescs SET isprivate = ?
                             WHERE bug_id = ? AND bug_when = ?');

    foreach my $field ($cgi->param()) {
        if ($field =~ /when-([0-9]+)/) {
            my $sequence = $1;
            my $private = $cgi->param("isprivate-$sequence") ? 1 : 0 ;
            if ($private != $cgi->param("oisprivate-$sequence")) {
                my $field_data = $cgi->param("$field");
                # Make sure a valid date is given.
                $field_data = format_time($field_data, '%Y-%m-%d %T');
                $sth->execute($private, $cgi->param('id'), $field_data);
            }
        }

    }
}

my $duplicate;

# We need to check the addresses involved in a CC change before we touch any bugs.
# What we'll do here is formulate the CC data into two hashes of ID's involved
# in this CC change.  Then those hashes can be used later on for the actual change.
my (%cc_add, %cc_remove);
if (defined $cgi->param('newcc')
    || defined $cgi->param('addselfcc')
    || defined $cgi->param('removecc')
    || defined $cgi->param('masscc')) {
    # If masscc is defined, then we came from buglist and need to either add or
    # remove cc's... otherwise, we came from bugform and may need to do both.
    my ($cc_add, $cc_remove) = "";
    if (defined $cgi->param('masscc')) {
        if ($cgi->param('ccaction') eq 'add') {
            $cc_add = join(' ',$cgi->param('masscc'));
        } elsif ($cgi->param('ccaction') eq 'remove') {
            $cc_remove = join(' ',$cgi->param('masscc'));
        }
    } else {
        $cc_add = join(' ',$cgi->param('newcc'));
        # We came from bug_form which uses a select box to determine what cc's
        # need to be removed...
        if (defined $cgi->param('removecc') && $cgi->param('cc')) {
            $cc_remove = join (",", $cgi->param('cc'));
        }
    }

    if ($cc_add) {
        $cc_add =~ s/[\s,]+/ /g; # Change all delimiters to a single space
        foreach my $person ( split(" ", $cc_add) ) {
            my $pid = login_to_id($person, THROW_ERROR);
            $cc_add{$pid} = $person;
        }
    }
    if ($cgi->param('addselfcc')) {
        $cc_add{$whoid} = $user->login;
    }
    if ($cc_remove) {
        $cc_remove =~ s/[\s,]+/ /g; # Change all delimiters to a single space
        foreach my $person ( split(" ", $cc_remove) ) {
            my $pid = login_to_id($person, THROW_ERROR);
            $cc_remove{$pid} = $person;
        }
    }
}

# Store the new assignee and QA contact IDs (if any). This is the
# only way to keep these informations when bugs are reassigned by
# component as $cgi->param('assigned_to') and $cgi->param('qa_contact')
# are not the right fields to look at.
# If the assignee or qacontact is changed, the new one is checked when
# changed information is validated.  If not, then the unchanged assignee
# or qacontact may have to be validated later.

my $assignee;
my $qacontact;
my $qacontact_checked = 0;
my $assignee_checked = 0;

my %usercache = ();

if (defined $cgi->param('qa_contact')
    && $cgi->param('knob') ne "reassignbycomponent")
{
    my $name = trim($cgi->param('qa_contact'));
    # The QA contact cannot be deleted from show_bug.cgi for a single bug!
    if ($name ne $cgi->param('dontchange')) {
        $qacontact = login_to_id($name, THROW_ERROR) if ($name ne "");
        if ($qacontact && Bugzilla->params->{"strict_isolation"}
            && !(defined $cgi->param('id') && $bug->qa_contact
                 && $qacontact == $bug->qa_contact->id))
        {
                $usercache{$qacontact} ||= Bugzilla::User->new($qacontact);
                my $qa_user = $usercache{$qacontact};
                foreach my $product_id (@newprod_ids) {
                    if (!$qa_user->can_edit_product($product_id)) {
                        my $product_name = Bugzilla::Product->new($product_id)->name;
                        ThrowUserError('invalid_user_group',
                                          {'users'   => $qa_user->login,
                                           'product' => $product_name,
                                           'bug_id' => (scalar(@idlist) > 1)
                                                         ? undef : $idlist[0]
                                          });
                    }
                }
        }
        $qacontact_checked = 1;
        DoComma();
        if($qacontact) {
            $::query .= "qa_contact = ?";
            push(@values, $qacontact);
        }
        else {
            $::query .= "qa_contact = NULL";
        }
    }
}

# By default, makes sure that all bugs are in a closed state.
# If $all_open is true, makes sure that all bugs are open.
sub check_bugs_resolution {
    my ($idlist, $all_open) = @_;
    my $dbh = Bugzilla->dbh;

    my $open_states = join(',', map {$dbh->quote($_)} BUG_STATE_OPEN);
    # The list has already been validated.
    $idlist = join(',', @$idlist);
    my $sql_cond = $all_open ? 'NOT' : '';
    my $has_unwanted_bugs =
      $dbh->selectrow_array("SELECT 1 FROM bugs WHERE bug_id IN ($idlist)
                             AND bug_status $sql_cond IN ($open_states)");

    # If there are unwanted bugs, then the test fails.
    return !$has_unwanted_bugs;
}

SWITCH: for ($cgi->param('knob')) {
    /^none$/ && do {
        last SWITCH;
    };
    /^confirm$/ && CheckonComment( "confirm" ) && do {
        DoConfirm($bug);
        ChangeStatus('NEW');
        last SWITCH;
    };
    /^accept$/ && CheckonComment( "accept" ) && do {
        DoConfirm($bug);
        ChangeStatus('ASSIGNED');
        if (Bugzilla->params->{"usetargetmilestone"} 
            && Bugzilla->params->{"musthavemilestoneonaccept"}) 
        {
            $requiremilestone = 1;
        }
        last SWITCH;
    };
    /^clearresolution$/ && CheckonComment( "clearresolution" ) && do {
        # All bugs must already be open.
        check_bugs_resolution(\@idlist, 'all_open')
          || ThrowUserError('resolution_deletion_not_allowed');
        ChangeResolution($bug, '');
        last SWITCH;
    };
    /^(resolve|change_resolution)$/ && CheckonComment( "resolve" ) && do {
        # Check here, because it's the only place we require the resolution
        check_field('resolution', scalar $cgi->param('resolution'),
                    Bugzilla::Bug->settable_resolutions);

        # don't resolve as fixed while still unresolved blocking bugs
        if (Bugzilla->params->{"noresolveonopenblockers"}
            && $cgi->param('resolution') eq 'FIXED')
        {
            my @dependencies = Bugzilla::Bug::CountOpenDependencies(@idlist);
            if (scalar @dependencies > 0) {
                ThrowUserError("still_unresolved_bugs",
                               { dependencies     => \@dependencies,
                                 dependency_count => scalar @dependencies });
            }
        }

        if ($cgi->param('knob') eq 'resolve') {
            # RESOLVED bugs should have no time remaining;
            # more time can be added for the VERIFY step, if needed.
            _remove_remaining_time();

            ChangeStatus('RESOLVED');
        }
        else {
            # You cannot use change_resolution if there is at least
            # one open bug.
            check_bugs_resolution(\@idlist) || ThrowUserError('resolution_not_allowed');
        }

        ChangeResolution($bug, $cgi->param('resolution'));
        last SWITCH;
    };
    /^reassign$/ && CheckonComment( "reassign" ) && do {
        if ($cgi->param('andconfirm')) {
            DoConfirm($bug);
        }
        ChangeStatus('NEW');
        DoComma();
        if (defined $cgi->param('assigned_to')
            && trim($cgi->param('assigned_to')) ne "") { 
            $assignee = login_to_id(trim($cgi->param('assigned_to')), THROW_ERROR);
            if (Bugzilla->params->{"strict_isolation"}) {
                $usercache{$assignee} ||= Bugzilla::User->new($assignee);
                my $assign_user = $usercache{$assignee};
                foreach my $product_id (@newprod_ids) {
                    if (!$assign_user->can_edit_product($product_id)) {
                        my $product_name = Bugzilla::Product->new($product_id)->name;
                        ThrowUserError('invalid_user_group',
                                          {'users'   => $assign_user->login,
                                           'product' => $product_name,
                                           'bug_id' => (scalar(@idlist) > 1)
                                                         ? undef : $idlist[0]
                                          });
                    }
                }
            }
        } else {
            ThrowUserError("reassign_to_empty");
        }
        $::query .= "assigned_to = ?";
        push(@values, $assignee);
        $assignee_checked = 1;
        last SWITCH;
    };
    /^reassignbycomponent$/  && CheckonComment( "reassignbycomponent" ) && do {
        if ($cgi->param('compconfirm')) {
            DoConfirm($bug);
        }
        ChangeStatus('NEW');
        last SWITCH;
    };
    /^reopen$/  && CheckonComment( "reopen" ) && do {
        ChangeStatus('REOPENED');
        ChangeResolution($bug, '');
        last SWITCH;
    };
    /^verify$/ && CheckonComment( "verify" ) && do {
        check_bugs_resolution(\@idlist)
          || ThrowUserError('bug_status_not_allowed', {status => 'VERIFIED'});

        ChangeStatus('VERIFIED');
        last SWITCH;
    };
    /^close$/ && CheckonComment( "close" ) && do {
        check_bugs_resolution(\@idlist)
          || ThrowUserError('bug_status_not_allowed', {status => 'CLOSED'});

        # CLOSED bugs should have no time remaining.
        _remove_remaining_time();

        ChangeStatus('CLOSED');
        last SWITCH;
    };
    /^duplicate$/ && CheckonComment( "duplicate" ) && do {
        # You cannot mark bugs as duplicates when changing
        # several bugs at once.
        unless (defined $cgi->param('id')) {
            ThrowUserError('dupe_not_allowed');
        }

        # Make sure we can change the original bug (issue A on bug 96085)
        defined($cgi->param('dup_id'))
          || ThrowCodeError('undefined_field', { field => 'dup_id' });

        $duplicate = $cgi->param('dup_id');
        ValidateBugID($duplicate, 'dup_id');
        $cgi->param('dup_id', $duplicate);

        # Make sure a loop isn't created when marking this bug
        # as duplicate.
        my %dupes;
        my $dupe_of = $duplicate;
        my $sth = $dbh->prepare('SELECT dupe_of FROM duplicates
                                 WHERE dupe = ?');

        while ($dupe_of) {
            if ($dupe_of == $cgi->param('id')) {
                ThrowUserError('dupe_loop_detected', { bug_id  => $cgi->param('id'),
                                                       dupe_of => $duplicate });
            }
            # If $dupes{$dupe_of} is already set to 1, then a loop
            # already exists which does not involve this bug.
            # As the user is not responsible for this loop, do not
            # prevent him from marking this bug as a duplicate.
            last if exists $dupes{"$dupe_of"};
            $dupes{"$dupe_of"} = 1;
            $sth->execute($dupe_of);
            $dupe_of = $sth->fetchrow_array;
        }

        # Also, let's see if the reporter has authorization to see
        # the bug to which we are duping. If not we need to prompt.
        DuplicateUserConfirm();

        # DUPLICATE bugs should have no time remaining.
        _remove_remaining_time();

        ChangeStatus('RESOLVED');
        ChangeResolution($bug, 'DUPLICATE');
        last SWITCH;
    };

    ThrowCodeError("unknown_action", { action => $cgi->param('knob') });
}

my @keywordlist;
my %keywordseen;

if (defined $cgi->param('keywords')) {
    foreach my $keyword (split(/[\s,]+/, $cgi->param('keywords'))) {
        if ($keyword eq '') {
            next;
        }
        my $keyword_obj = new Bugzilla::Keyword({name => $keyword});
        if (!$keyword_obj) {
            ThrowUserError("unknown_keyword",
                           { keyword => $keyword });
        }
        if (!$keywordseen{$keyword_obj->id}) {
            push(@keywordlist, $keyword_obj->id);
            $keywordseen{$keyword_obj->id} = 1;
        }
    }
}

my $keywordaction = $cgi->param('keywordaction') || "makeexact";
if (!grep($keywordaction eq $_, qw(add delete makeexact))) {
    $keywordaction = "makeexact";
}

if ($::comma eq ""
    && (! @groupAdd) && (! @groupDel)
    && (!Bugzilla::Keyword::keyword_count() 
        || (0 == @keywordlist && $keywordaction ne "makeexact"))
    && defined $cgi->param('masscc') && ! $cgi->param('masscc')
    ) {
    if (!defined $cgi->param('comment') || $cgi->param('comment') =~ /^\s*$/) {
        ThrowUserError("bugs_not_changed");
    }
}

# Process data for Time Tracking fields
if (Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'})) {
    foreach my $field ("estimated_time", "remaining_time") {
        if (defined $cgi->param($field)) {
            my $er_time = trim($cgi->param($field));
            if ($er_time ne $cgi->param('dontchange')) {
                DoComma();
                $::query .= "$field = ?";
                trick_taint($er_time);
                push(@values, $er_time);
            }
        }
    }

    if (defined $cgi->param('deadline')) {
        DoComma();
        if ($cgi->param('deadline')) {
            validate_date($cgi->param('deadline'))
              || ThrowUserError('illegal_date', {date => $cgi->param('deadline'),
                                                 format => 'YYYY-MM-DD'});
            $::query .= "deadline = ?";
            my $deadline = $cgi->param('deadline');
            trick_taint($deadline);
            push(@values, $deadline);
        } else {
            $::query .= "deadline = NULL";
        }
    }
}

my $basequery = $::query;

local our $delta_ts;
sub SnapShotBug {
    my ($id) = (@_);
    my $dbh = Bugzilla->dbh;
    my @row = $dbh->selectrow_array(q{SELECT delta_ts, } .
                join(',', editable_bug_fields()).q{ FROM bugs WHERE bug_id = ?},
                undef, $id);
    $delta_ts = shift @row;

    return @row;
}


sub SnapShotDeps {
    my ($bug_id, $target, $me) = (@_);
    my $list = Bugzilla::Bug::EmitDependList($me, $target, $bug_id);
    return join(',', @$list);
}


my $timestamp;

local our $bug_changed;
sub LogDependencyActivity {
    my ($i, $oldstr, $target, $me, $timestamp) = (@_);
    my $dbh = Bugzilla->dbh;
    my $newstr = SnapShotDeps($i, $target, $me);
    if ($oldstr ne $newstr) {
        # Figure out what's really different...
        my ($removed, $added) = diff_strings($oldstr, $newstr);
        LogActivityEntry($i,$target,$removed,$added,$whoid,$timestamp);
        # update timestamp on target bug so midairs will be triggered
        $dbh->do(q{UPDATE bugs SET delta_ts = ? WHERE bug_id = ?},
                 undef, $timestamp, $i);
        $bug_changed = 1;
        return 1;
    }
    return 0;
}

if (Bugzilla->params->{"strict_isolation"}) {
    my @blocked_cc = ();
    foreach my $pid (keys %cc_add) {
        $usercache{$pid} ||= Bugzilla::User->new($pid);
        my $cc_user = $usercache{$pid};
        foreach my $product_id (@newprod_ids) {
            if (!$cc_user->can_edit_product($product_id)) {
                push (@blocked_cc, $cc_user->login);
                last;
            }
        }
    }
    if (scalar(@blocked_cc)) {
        ThrowUserError("invalid_user_group", 
            {'users' => \@blocked_cc,
             'bug_id' => (scalar(@idlist) > 1) ? undef : $idlist[0]});
    }
}

if ($prod_changed && Bugzilla->params->{"strict_isolation"}) {
    my $sth_cc = $dbh->prepare("SELECT who
                                FROM cc
                                WHERE bug_id = ?");
    my $sth_bug = $dbh->prepare("SELECT assigned_to, qa_contact
                                 FROM bugs
                                 WHERE bug_id = ?");

    foreach my $id (@idlist) {
        $sth_cc->execute($id);
        my @blocked_cc = ();
        while (my ($pid) = $sth_cc->fetchrow_array) {
            # Ignore deleted accounts. They will never get notification.
            $usercache{$pid} ||= Bugzilla::User->new($pid) || next;
            my $cc_user = $usercache{$pid};
            if (!$cc_user->can_edit_product($product->id)) {
                push (@blocked_cc, $cc_user->login);
            }
        }
        if (scalar(@blocked_cc)) {
            ThrowUserError('invalid_user_group',
                              {'users'   => \@blocked_cc,
                               'bug_id' => $id,
                               'product' => $product->name});
        }
        $sth_bug->execute($id);
        my ($assignee, $qacontact) = $sth_bug->fetchrow_array;
        if (!$assignee_checked) {
            $usercache{$assignee} ||= Bugzilla::User->new($assignee) || next;
            my $assign_user = $usercache{$assignee};
            if (!$assign_user->can_edit_product($product->id)) {
                    ThrowUserError('invalid_user_group',
                                      {'users'   => $assign_user->login,
                                       'bug_id' => $id,
                                       'product' => $product->name});
            }
        }
        if (!$qacontact_checked && $qacontact) {
            $usercache{$qacontact} ||= Bugzilla::User->new($qacontact) || next;
            my $qa_user = $usercache{$qacontact};
            if (!$qa_user->can_edit_product($product->id)) {
                    ThrowUserError('invalid_user_group',
                                      {'users'   => $qa_user->login,
                                       'bug_id' => $id,
                                       'product' => $product->name});
            }
        }
    }
}


# This loop iterates once for each bug to be processed (i.e. all the
# bugs selected when this script is called with multiple bugs selected
# from buglist.cgi, or just the one bug when called from
# show_bug.cgi).
#
foreach my $id (@idlist) {
    my $query = $basequery;
    my @bug_values = @values;
    my $old_bug_obj = new Bugzilla::Bug($id);

    if ($cgi->param('knob') eq 'reassignbycomponent') {
        # We have to check whether the bug is moved to another product
        # and/or component before reassigning. If $component is defined,
        # use it; else use the product/component the bug is already in.
        my $new_comp_id = $component ? $component->id : $old_bug_obj->{'component_id'};
        $assignee = $dbh->selectrow_array('SELECT initialowner
                                           FROM components
                                           WHERE components.id = ?',
                                           undef, $new_comp_id);
        $query .= ", assigned_to = ?";
        push(@bug_values, $assignee);
        if (Bugzilla->params->{"useqacontact"}) {
            $qacontact = $dbh->selectrow_array('SELECT initialqacontact
                                                FROM components
                                                WHERE components.id = ?',
                                                undef, $new_comp_id);
            if ($qacontact) {
                $query .= ", qa_contact = ?";
                push(@bug_values, $qacontact);
            }
            else {
                $query .= ", qa_contact = NULL";
            }
        }

        

        # And add in the Default CC for the Component.
        my $comp_obj = $component || new Bugzilla::Component($new_comp_id);
        my @new_init_cc = @{$comp_obj->initial_cc};
        foreach my $cc (@new_init_cc) {
            # NewCC must be defined or the code below won't insert
            # any CCs.
            $cgi->param('newcc') || $cgi->param('newcc', []);
            $cc_add{$cc->id} = $cc->login;
        }
    }

    my %dependencychanged;
    $bug_changed = 0;
    my $write = "WRITE";        # Might want to make a param to control
                                # whether we do LOW_PRIORITY ...
    $dbh->bz_lock_tables("bugs $write", "bugs_activity $write", "cc $write",
            "profiles READ", "dependencies $write", "votes $write",
            "products READ", "components READ", "milestones READ",
            "keywords $write", "longdescs $write", "fielddefs READ",
            "bug_group_map $write", "flags $write", "duplicates $write",
            "user_group_map READ", "group_group_map READ", "flagtypes READ",
            "flaginclusions AS i READ", "flagexclusions AS e READ",
            "keyworddefs READ", "groups READ", "attachments READ",
            "group_control_map AS oldcontrolmap READ",
            "group_control_map AS newcontrolmap READ",
            "group_control_map READ", "email_setting READ", "classifications READ",
            "setting READ", "profile_setting READ");

    # It may sound crazy to set %formhash for each bug as $cgi->param()
    # will not change, but %formhash is modified below and we prefer
    # to set it again.
    my $i = 0;
    my @oldvalues = SnapShotBug($id);
    my %oldhash;
    my %formhash;
    foreach my $col (@editable_bug_fields) {
        # Consider NULL db entries to be equivalent to the empty string
        $oldvalues[$i] = defined($oldvalues[$i]) ? $oldvalues[$i] : '';
        # Convert the deadline taken from the DB into the YYYY-MM-DD format
        # for consistency with the deadline provided by the user, if any.
        # Else Bug::check_can_change_field() would see them as different
        # in all cases.
        if ($col eq 'deadline') {
            $oldvalues[$i] = format_time($oldvalues[$i], "%Y-%m-%d");
        }
        $oldhash{$col} = $oldvalues[$i];
        $formhash{$col} = $cgi->param($col) if defined $cgi->param($col);
        $i++;
    }
    # If the user is reassigning bugs, we need to:
    # - convert $newhash{'assigned_to'} and $newhash{'qa_contact'}
    #   email addresses into their corresponding IDs;
    # - update $newhash{'bug_status'} to its real state if the bug
    #   is in the unconfirmed state.
    $formhash{'qa_contact'} = $qacontact if Bugzilla->params->{'useqacontact'};
    if ($cgi->param('knob') eq 'reassignbycomponent'
        || $cgi->param('knob') eq 'reassign') {
        $formhash{'assigned_to'} = $assignee;
        if ($oldhash{'bug_status'} eq 'UNCONFIRMED') {
            $formhash{'bug_status'} = $oldhash{'bug_status'};
        }
    }
    # This hash is required by Bug::check_can_change_field().
    my $cgi_hash = {
        'dontchange' => scalar $cgi->param('dontchange'),
        'knob'       => scalar $cgi->param('knob')
    };
    foreach my $col (@editable_bug_fields) {
        # The 'resolution' field is checked by ChangeResolution(),
        # i.e. only if we effectively use it.
        next if ($col eq 'resolution');
        if (exists $formhash{$col}
            && !$old_bug_obj->check_can_change_field($col, $oldhash{$col}, $formhash{$col},
                                                     \$PrivilegesRequired, $cgi_hash))
        {
            my $vars;
            if ($col eq 'component_id') {
                # Display the component name
                $vars->{'oldvalue'} = $old_bug_obj->component;
                $vars->{'newvalue'} = $cgi->param('component');
                $vars->{'field'} = 'component';
            } elsif ($col eq 'assigned_to' || $col eq 'qa_contact') {
                # Display the assignee or QA contact email address
                $vars->{'oldvalue'} = user_id_to_login($oldhash{$col});
                $vars->{'newvalue'} = user_id_to_login($formhash{$col});
                $vars->{'field'} = $col;
            } else {
                $vars->{'oldvalue'} = $oldhash{$col};
                $vars->{'newvalue'} = $formhash{$col};
                $vars->{'field'} = $col;
            }
            $vars->{'privs'} = $PrivilegesRequired;
            ThrowUserError("illegal_change", $vars);
        }
    }
    
    # When editing multiple bugs, users can specify a list of keywords to delete
    # from bugs.  If the list matches the current set of keywords on those bugs,
    # Bug::check_can_change_field will fail to check permissions because it thinks
    # the list hasn't changed. To fix that, we have to call Bug::check_can_change_field
    # again with old!=new if the keyword action is "delete" and old=new.
    if ($keywordaction eq "delete"
        && defined $cgi->param('keywords')
        && length(@keywordlist) > 0
        && $cgi->param('keywords') eq $oldhash{keywords}
        && !$old_bug_obj->check_can_change_field("keywords", "old is not", "equal to new",
                                                 \$PrivilegesRequired))
    {
        $vars->{'oldvalue'} = $oldhash{keywords};
        $vars->{'newvalue'} = "no keywords";
        $vars->{'field'} = "keywords";
        $vars->{'privs'} = $PrivilegesRequired;
        ThrowUserError("illegal_change", $vars);
    }

    $oldhash{'product'} = $old_bug_obj->product;
    if (!Bugzilla->user->can_edit_product($oldhash{'product_id'})) {
        ThrowUserError("product_edit_denied",
                      { product => $oldhash{'product'} });
    }

    if ($requiremilestone) {
        # musthavemilestoneonaccept applies only if at least two
        # target milestones are defined for the current product.
        my $prod_obj = new Bugzilla::Product({'name' => $oldhash{'product'}});
        my $nb_milestones = scalar(@{$prod_obj->milestones});
        if ($nb_milestones > 1) {
            my $value = $cgi->param('target_milestone');
            if (!defined $value || $value eq $cgi->param('dontchange')) {
                $value = $oldhash{'target_milestone'};
            }
            my $defaultmilestone =
                $dbh->selectrow_array("SELECT defaultmilestone
                                       FROM products WHERE id = ?",
                                       undef, $oldhash{'product_id'});
            # if musthavemilestoneonaccept == 1, then the target
            # milestone must be different from the default one.
            if ($value eq $defaultmilestone) {
                ThrowUserError("milestone_required", { bug_id => $id });
            }
        }
    }   
    if (defined $cgi->param('delta_ts') && $cgi->param('delta_ts') ne $delta_ts)
    {
        ($vars->{'operations'}) =
            Bugzilla::Bug::GetBugActivity($id, $cgi->param('delta_ts'));

        $vars->{'start_at'} = $cgi->param('longdesclength');

        # Always sort midair collision comments oldest to newest,
        # regardless of the user's personal preference.
        $vars->{'comments'} = Bugzilla::Bug::GetComments($id, "oldest_to_newest");

        $cgi->param('delta_ts', $delta_ts);
        
        $vars->{'bug_id'} = $id;
        
        $dbh->bz_unlock_tables(UNLOCK_ABORT);
        
        # Warn the user about the mid-air collision and ask them what to do.
        $template->process("bug/process/midair.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }

    # Gather the dependency list, and make sure there are no circular refs
    my %deps = Bugzilla::Bug::ValidateDependencies(scalar($cgi->param('dependson')),
                                                   scalar($cgi->param('blocked')),
                                                   $id);

    #
    # Start updating the relevant database entries
    #

    $timestamp = $dbh->selectrow_array(q{SELECT NOW()});

    my $work_time;
    if (Bugzilla->user->in_group(Bugzilla->params->{'timetrackinggroup'})) {
        $work_time = $cgi->param('work_time');
        if ($work_time) {
            # AppendComment (called below) can in theory raise an error,
            # but because we've already validated work_time here it's
            # safe to log the entry before adding the comment.
            LogActivityEntry($id, "work_time", "", $work_time,
                             $whoid, $timestamp);
        }
    }

    if ($cgi->param('comment') || $work_time || $duplicate) {
        my $type = $duplicate ? CMT_DUPE_OF : CMT_NORMAL;

        AppendComment($id, $whoid, scalar($cgi->param('comment')),
                      scalar($cgi->param('commentprivacy')), $timestamp,
                      $work_time, $type, $duplicate);
        $bug_changed = 1;
    }

    if (Bugzilla::Keyword::keyword_count() 
        && defined $cgi->param('keywords')) 
    {
        # There are three kinds of "keywordsaction": makeexact, add, delete.
        # For makeexact, we delete everything, and then add our things.
        # For add, we delete things we're adding (to make sure we don't
        # end up having them twice), and then we add them.
        # For delete, we just delete things on the list.
        my $changed = 0;
        if ($keywordaction eq "makeexact") {
            $dbh->do(q{DELETE FROM keywords WHERE bug_id = ?},
                     undef, $id);
            $changed = 1;
        }
        my $sth_delete = $dbh->prepare(q{DELETE FROM keywords
                                               WHERE bug_id = ?
                                                 AND keywordid = ?});
        my $sth_insert =
            $dbh->prepare(q{INSERT INTO keywords (bug_id, keywordid)
                                 VALUES (?, ?)});
        foreach my $keyword (@keywordlist) {
            if ($keywordaction ne "makeexact") {
                $sth_delete->execute($id, $keyword);
                $changed = 1;
            }
            if ($keywordaction ne "delete") {
                $sth_insert->execute($id, $keyword);
                $changed = 1;
            }
        }
        if ($changed) {
            my $list = $dbh->selectcol_arrayref(
                q{SELECT keyworddefs.name
                    FROM keyworddefs
              INNER JOIN keywords 
                      ON keyworddefs.id = keywords.keywordid
                   WHERE keywords.bug_id = ?
                ORDER BY keyworddefs.name},
                undef, $id);
            $dbh->do("UPDATE bugs SET keywords = ? WHERE bug_id = ?",
                     undef, join(', ', @$list), $id);
        }
    }
    $query .= " WHERE bug_id = ?";
    push(@bug_values, $id);
    
    if ($::comma ne "") {
        $dbh->do($query, undef, @bug_values);
    }

    # Check for duplicates if the bug is [re]open or its resolution is changed.
    my $resolution = $dbh->selectrow_array(
        q{SELECT resolution FROM bugs WHERE bug_id = ?}, undef, $id);
    if ($resolution ne 'DUPLICATE') {
        $dbh->do(q{DELETE FROM duplicates WHERE dupe = ?}, undef, $id);
    }

    my %groupsrequired = ();
    my %groupsforbidden = ();
    my $group_controls =
        $dbh->selectall_arrayref(q{SELECT id, membercontrol
                                     FROM groups
                                LEFT JOIN group_control_map
                                       ON id = group_id
                                      AND product_id = ?
                                    WHERE isactive != 0},
        undef, $oldhash{'product_id'});
    foreach my $group_control (@$group_controls) {
        my ($group, $control) = @$group_control;
        $control ||= 0;
        unless ($control > CONTROLMAPNA)  {
            $groupsforbidden{$group} = 1;
        }
        if ($control == CONTROLMAPMANDATORY) {
            $groupsrequired{$group} = 1;
        }
    }

    my @groupAddNames = ();
    my @groupAddNamesAll = ();
    my $sth = $dbh->prepare(q{INSERT INTO bug_group_map (bug_id, group_id)
                                   VALUES (?, ?)});
    foreach my $grouptoadd (@groupAdd, keys %groupsrequired) {
        next if $groupsforbidden{$grouptoadd};
        my $group_obj = new Bugzilla::Group($grouptoadd);
        push(@groupAddNamesAll, $group_obj->name);
        if (!BugInGroupId($id, $grouptoadd)) {
            push(@groupAddNames, $group_obj->name);
            $sth->execute($id, $grouptoadd);
        }
    }
    my @groupDelNames = ();
    my @groupDelNamesAll = ();
    $sth = $dbh->prepare(q{DELETE FROM bug_group_map
                                 WHERE bug_id = ? AND group_id = ?});
    foreach my $grouptodel (@groupDel, keys %groupsforbidden) {
        my $group_obj = new Bugzilla::Group($grouptodel);
        push(@groupDelNamesAll, $group_obj->name);
        next if $groupsrequired{$grouptodel};
        if (BugInGroupId($id, $grouptodel)) {
            push(@groupDelNames, $group_obj->name);
        }
        $sth->execute($id, $grouptodel);
    }

    my $groupDelNames = join(',', @groupDelNames);
    my $groupAddNames = join(',', @groupAddNames);

    if ($groupDelNames ne $groupAddNames) {
        LogActivityEntry($id, "bug_group", $groupDelNames, $groupAddNames,
                         $whoid, $timestamp); 
        $bug_changed = 1;
    }

    my @ccRemoved = (); 
    if (defined $cgi->param('newcc')
        || defined $cgi->param('addselfcc')
        || defined $cgi->param('removecc')
        || defined $cgi->param('masscc')) {
        # Get the current CC list for this bug
        my %oncc;
        my $cc_list = $dbh->selectcol_arrayref(
            q{SELECT who FROM cc WHERE bug_id = ?}, undef, $id);
        foreach my $who (@$cc_list) {
            $oncc{$who} = 1;
        }

        my (@added, @removed) = ();

        my $sth_insert = $dbh->prepare(q{INSERT INTO cc (bug_id, who)
                                              VALUES (?, ?)});
        foreach my $pid (keys %cc_add) {
            # If this person isn't already on the cc list, add them
            if (! $oncc{$pid}) {
                $sth_insert->execute($id, $pid);
                push (@added, $cc_add{$pid});
                $oncc{$pid} = 1;
            }
        }
        my $sth_delete = $dbh->prepare(q{DELETE FROM cc
                                          WHERE bug_id = ? AND who = ?});
        foreach my $pid (keys %cc_remove) {
            # If the person is on the cc list, remove them
            if ($oncc{$pid}) {
                $sth_delete->execute($id, $pid);
                push (@removed, $cc_remove{$pid});
                $oncc{$pid} = 0;
            }
        }

        # If any changes were found, record it in the activity log
        if (scalar(@removed) || scalar(@added)) {
            my $removed = join(", ", @removed);
            my $added = join(", ", @added);
            LogActivityEntry($id,"cc",$removed,$added,$whoid,$timestamp);
            $bug_changed = 1;
        }
        @ccRemoved = @removed;
    }

    # We need to send mail for dependson/blocked bugs if the dependencies
    # change or the status or resolution change. This var keeps track of that.
    my $check_dep_bugs = 0;

    foreach my $pair ("blocked/dependson", "dependson/blocked") {
        my ($me, $target) = split("/", $pair);

        my @oldlist = @{$dbh->selectcol_arrayref("SELECT $target FROM dependencies
                                                  WHERE $me = ? ORDER BY $target",
                                                  undef, $id)};

        # Only bugs depending on the current one should get notification.
        # Bugs blocking the current one never get notification, unless they
        # are added or removed from the dependency list. This case is treated
        # below.
        @dependencychanged{@oldlist} = 1 if ($me eq 'dependson');

        if (defined $cgi->param($target)) {
            my %snapshot;
            my @newlist = sort {$a <=> $b} @{$deps{$target}};

            while (0 < @oldlist || 0 < @newlist) {
                if (@oldlist == 0 || (@newlist > 0 &&
                                      $oldlist[0] > $newlist[0])) {
                    $snapshot{$newlist[0]} = SnapShotDeps($newlist[0], $me,
                                                          $target);
                    shift @newlist;
                } elsif (@newlist == 0 || (@oldlist > 0 &&
                                           $newlist[0] > $oldlist[0])) {
                    $snapshot{$oldlist[0]} = SnapShotDeps($oldlist[0], $me,
                                                          $target);
                    shift @oldlist;
                } else {
                    if ($oldlist[0] != $newlist[0]) {
                        ThrowCodeError('list_comparison_error');
                    }
                    shift @oldlist;
                    shift @newlist;
                }
            }
            my @keys = keys(%snapshot);
            if (@keys) {
                my $oldsnap = SnapShotDeps($id, $target, $me);
                $dbh->do(qq{DELETE FROM dependencies WHERE $me = ?},
                         undef, $id);
                my $sth =
                    $dbh->prepare(qq{INSERT INTO dependencies ($me, $target)
                                          VALUES (?, ?)});
                foreach my $i (@{$deps{$target}}) {
                    $sth->execute($id, $i);
                }
                foreach my $k (@keys) {
                    LogDependencyActivity($k, $snapshot{$k}, $me, $target, $timestamp);
                }
                LogDependencyActivity($id, $oldsnap, $target, $me, $timestamp);
                $check_dep_bugs = 1;
                # All bugs added or removed from the dependency list
                # must be notified.
                @dependencychanged{@keys} = 1;
            }
        }
    }

    # When a bug changes products and the old or new product is associated
    # with a bug group, it may be necessary to remove the bug from the old
    # group or add it to the new one.  There are a very specific series of
    # conditions under which these activities take place, more information
    # about which can be found in comments within the conditionals below.
    # Check if the user has changed the product to which the bug belongs;
    if ($cgi->param('product') ne $cgi->param('dontchange')
        && $cgi->param('product') ne $oldhash{'product'})
    {
        # Depending on the "addtonewgroup" variable, groups with
        # defaults will change.
        #
        # For each group, determine
        # - The group id and if it is active
        # - The control map value for the old product and this group
        # - The control map value for the new product and this group
        # - Is the user in this group?
        # - Is the bug in this group?
        my $groups = $dbh->selectall_arrayref(
            qq{SELECT DISTINCT groups.id, isactive,
                               oldcontrolmap.membercontrol,
                               newcontrolmap.membercontrol,
                      CASE WHEN groups.id IN ($grouplist) THEN 1 ELSE 0 END,
                      CASE WHEN bug_group_map.group_id IS NOT NULL
                                THEN 1 ELSE 0 END
                 FROM groups
            LEFT JOIN group_control_map AS oldcontrolmap
                   ON oldcontrolmap.group_id = groups.id
                  AND oldcontrolmap.product_id = ?
            LEFT JOIN group_control_map AS newcontrolmap
                   ON newcontrolmap.group_id = groups.id
                  AND newcontrolmap.product_id = ?
            LEFT JOIN bug_group_map
                   ON bug_group_map.group_id = groups.id
                  AND bug_group_map.bug_id = ?},
            undef, $oldhash{'product_id'}, $product->id, $id);
        my @groupstoremove = ();
        my @groupstoadd = ();
        my @defaultstoadd = ();
        my @allgroups = ();
        my $buginanydefault = 0;
        foreach my $group (@$groups) {
            my ($groupid, $isactive, $oldcontrol, $newcontrol,
                   $useringroup, $bugingroup) = @$group;
            # An undefined newcontrol is none.
            $newcontrol = CONTROLMAPNA unless $newcontrol;
            $oldcontrol = CONTROLMAPNA unless $oldcontrol;
            push(@allgroups, $groupid);
            if (($bugingroup) && ($isactive)
                && ($oldcontrol == CONTROLMAPDEFAULT)) {
                # Bug was in a default group.
                $buginanydefault = 1;
            }
            if (($isactive) && (!$bugingroup)
                && ($newcontrol == CONTROLMAPDEFAULT)
                && ($useringroup)) {
                push (@defaultstoadd, $groupid);
            }
            if (($bugingroup) && ($isactive) && ($newcontrol == CONTROLMAPNA)) {
                # Group is no longer permitted.
                push(@groupstoremove, $groupid);
            }
            if ((!$bugingroup) && ($isactive) 
                && ($newcontrol == CONTROLMAPMANDATORY)) {
                # Group is now required.
                push(@groupstoadd, $groupid);
            }
        }
        # If addtonewgroups = "yes", new default groups will be added.
        # If addtonewgroups = "yesifinold", new default groups will be
        # added only if the bug was in ANY of the old default groups.
        # If addtonewgroups = "no", old default groups are left alone
        # and no new default group will be added.
        if (AnyDefaultGroups()
            && (($cgi->param('addtonewgroup') eq 'yes')
            || (($cgi->param('addtonewgroup') eq 'yesifinold')
            && ($buginanydefault)))) {
            push(@groupstoadd, @defaultstoadd);
        }

        # Now actually update the bug_group_map.
        my @DefGroupsAdded = ();
        my @DefGroupsRemoved = ();
        my $sth_insert =
            $dbh->prepare(q{INSERT INTO bug_group_map (bug_id, group_id)
                                 VALUES (?, ?)});
        my $sth_delete = $dbh->prepare(q{DELETE FROM bug_group_map
                                               WHERE bug_id = ?
                                                 AND group_id = ?});
        foreach my $groupid (@allgroups) {
            my $thisadd = grep( ($_ == $groupid), @groupstoadd);
            my $thisdel = grep( ($_ == $groupid), @groupstoremove);
            if ($thisadd) {
                my $group_obj = new Bugzilla::Group($groupid);
                push(@DefGroupsAdded, $group_obj->name);
                $sth_insert->execute($id, $groupid);
            } elsif ($thisdel) {
                my $group_obj = new Bugzilla::Group($groupid);
                push(@DefGroupsRemoved, $group_obj->name);
                $sth_delete->execute($id, $groupid);
            }
        }
        if ((@DefGroupsAdded) || (@DefGroupsRemoved)) {
            LogActivityEntry($id, "bug_group",
                join(', ', @DefGroupsRemoved),
                join(', ', @DefGroupsAdded),
                     $whoid, $timestamp); 
        }
    }
  
    # get a snapshot of the newly set values out of the database, 
    # and then generate any necessary bug activity entries by seeing 
    # what has changed since before we wrote out the new values.
    #
    my $new_bug_obj = new Bugzilla::Bug($id);
    my @newvalues = SnapShotBug($id);
    my %newhash;
    $i = 0;
    foreach my $col (@editable_bug_fields) {
        # Consider NULL db entries to be equivalent to the empty string
        $newvalues[$i] = defined($newvalues[$i]) ? $newvalues[$i] : '';
        # Convert the deadline to the YYYY-MM-DD format.
        if ($col eq 'deadline') {
            $newvalues[$i] = format_time($newvalues[$i], "%Y-%m-%d");
        }
        $newhash{$col} = $newvalues[$i];
        $i++;
    }
    # for passing to Bugzilla::BugMail to ensure that when someone is removed
    # from one of these fields, they get notified of that fact (if desired)
    #
    my $origOwner = "";
    my $origQaContact = "";

    # $msgs will store emails which have to be sent to voters, if any.
    my $msgs;

    foreach my $c (@editable_bug_fields) {
        my $col = $c;           # We modify it, don't want to modify array
                                # values in place.
        my $old = shift @oldvalues;
        my $new = shift @newvalues;
        if ($old ne $new) {

            # Products and components are now stored in the DB using ID's
            # We need to translate this to English before logging it
            if ($col eq 'product_id') {
                $old = $old_bug_obj->product;
                $new = $new_bug_obj->product;
                $col = 'product';
            }
            if ($col eq 'component_id') {
                $old = $old_bug_obj->component;
                $new = $new_bug_obj->component;
                $col = 'component';
            }

            # save off the old value for passing to Bugzilla::BugMail so
            # the old assignee can be notified
            #
            if ($col eq 'assigned_to') {
                $old = ($old) ? user_id_to_login($old) : "";
                $new = ($new) ? user_id_to_login($new) : "";
                $origOwner = $old;
            }

            # ditto for the old qa contact
            #
            if ($col eq 'qa_contact') {
                $old = ($old) ? user_id_to_login($old) : "";
                $new = ($new) ? user_id_to_login($new) : "";
                $origQaContact = $old;
            }

            # If this is the keyword field, only record the changes, not everything.
            if ($col eq 'keywords') {
                ($old, $new) = diff_strings($old, $new);
            }

            if ($col eq 'product') {
                # If some votes have been removed, RemoveVotes() returns
                # a list of messages to send to voters.
                # We delay the sending of these messages till tables are unlocked.
                $msgs = RemoveVotes($id, 0, 'votes_bug_moved');
                CheckIfVotedConfirmed($id, $whoid);
            }

            if ($col eq 'bug_status' 
                && is_open_state($old) ne is_open_state($new))
            {
                $check_dep_bugs = 1;
            }

            LogActivityEntry($id,$col,$old,$new,$whoid,$timestamp);
            $bug_changed = 1;
        }
    }
    # Set and update flags.
    Bugzilla::Flag::process($new_bug_obj, undef, $timestamp, $cgi);

    if ($bug_changed) {
        $dbh->do(q{UPDATE bugs SET delta_ts = ? WHERE bug_id = ?},
                 undef, $timestamp, $id);
    }
    $dbh->bz_unlock_tables();

    # Now is a good time to send email to voters.
    foreach my $msg (@$msgs) {
        MessageToMTA($msg);
    }

    if ($duplicate) {
        # If the bug was already marked as a duplicate, remove
        # the existing entry.
        $dbh->do('DELETE FROM duplicates WHERE dupe = ?',
                  undef, $cgi->param('id'));

        # Check to see if Reporter of this bug is reporter of Dupe 
        my $reporter = $dbh->selectrow_array(
            q{SELECT reporter FROM bugs WHERE bug_id = ?}, undef, $id);
        my $isreporter = $dbh->selectrow_array(
            q{SELECT reporter FROM bugs WHERE bug_id = ? AND reporter = ?},
            undef, $duplicate, $reporter);
        my $isoncc = $dbh->selectrow_array(q{SELECT who FROM cc
                                           WHERE bug_id = ? AND who = ?},
                                           undef, $duplicate, $reporter);
        unless ($isreporter || $isoncc
                || !$cgi->param('confirm_add_duplicate')) {
            # The reporter is oblivious to the existence of the new bug and is permitted access
            # ... add 'em to the cc (and record activity)
            LogActivityEntry($duplicate,"cc","",user_id_to_login($reporter),
                             $whoid,$timestamp);
            $dbh->do(q{INSERT INTO cc (who, bug_id) VALUES (?, ?)},
                     undef, $reporter, $duplicate);
        }
        # Bug 171639 - Duplicate notifications do not need to be private.
        AppendComment($duplicate, $whoid, "", 0, $timestamp, 0,
                      CMT_HAS_DUPE, scalar $cgi->param('id'));

        $dbh->do(q{INSERT INTO duplicates VALUES (?, ?)}, undef,
                 $duplicate, $cgi->param('id'));
    }

    # Now all changes to the DB have been made. It's time to email
    # all concerned users, including the bug itself, but also the
    # duplicated bug and dependent bugs, if any.

    $vars->{'mailrecipients'} = { 'cc' => \@ccRemoved,
                                  'owner' => $origOwner,
                                  'qacontact' => $origQaContact,
                                  'changer' => Bugzilla->user->login };

    $vars->{'id'} = $id;
    $vars->{'type'} = "bug";
    
    # Let the user know the bug was changed and who did and didn't
    # receive email about the change.
    send_results($id, $vars);
 
    if ($duplicate) {
        $vars->{'mailrecipients'} = { 'changer' => Bugzilla->user->login }; 

        $vars->{'id'} = $duplicate;
        $vars->{'type'} = "dupe";
        
        # Let the user know a duplication notation was added to the 
        # original bug.
        send_results($duplicate, $vars);
    }

    if ($check_dep_bugs) {
        foreach my $k (keys(%dependencychanged)) {
            $vars->{'mailrecipients'} = { 'changer' => Bugzilla->user->login }; 
            $vars->{'id'} = $k;
            $vars->{'type'} = "dep";

            # Let the user (if he is able to see the bug) know we checked to 
            # see if we should email notice of this change to users with a 
            # relationship to the dependent bug and who did and didn't 
            # receive email about it.
            send_results($k, $vars);
        }
    }
}

# Determine if Patch Viewer is installed, for Diff link
# (NB: Duplicate code with show_bug.cgi.)
eval {
    require PatchReader;
    $vars->{'patchviewerinstalled'} = 1;
};

if (defined $cgi->param('id')) {
    $action = Bugzilla->user->settings->{'post_bug_submit_action'}->{'value'};
} else {
    # param('id') is not defined when changing multiple bugs
    $action = 'nothing';
}

if (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
    # Do nothing.
}
elsif ($action eq 'next_bug') {
    my $next_bug;
    my $cur = lsearch(\@bug_list, $cgi->param("id"));
    if ($cur >= 0 && $cur < $#bug_list) {
        $next_bug = $bug_list[$cur + 1];
    }
    if ($next_bug) {
        if (detaint_natural($next_bug) && Bugzilla->user->can_see_bug($next_bug)) {
            my $bug = new Bugzilla::Bug($next_bug);
            ThrowCodeError("bug_error", { bug => $bug }) if $bug->error;

            $vars->{'bugs'} = [$bug];
            $vars->{'nextbug'} = $bug->bug_id;

            $template->process("bug/show.html.tmpl", $vars)
              || ThrowTemplateError($template->error());

            exit;
        }
    }
} elsif ($action eq 'same_bug') {
    if (Bugzilla->user->can_see_bug($cgi->param('id'))) {
        my $bug = new Bugzilla::Bug($cgi->param('id'));
        ThrowCodeError("bug_error", { bug => $bug }) if $bug->error;

        $vars->{'bugs'} = [$bug];

        $template->process("bug/show.html.tmpl", $vars)
          || ThrowTemplateError($template->error());

        exit;
    }
} elsif ($action ne 'nothing') {
    ThrowCodeError("invalid_post_bug_submit_action");
}

# End the response page.
unless (Bugzilla->usage_mode == USAGE_MODE_EMAIL) {
    # The user pref is 'Do nothing', so all we need is the current bug ID.
    $vars->{'bug'} = {bug_id => scalar $cgi->param('id')};

    $template->process("bug/navigate.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
    $template->process("global/footer.html.tmpl", $vars)
        || ThrowTemplateError($template->error());
}

1;
