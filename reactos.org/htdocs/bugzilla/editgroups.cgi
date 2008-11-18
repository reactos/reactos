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
# Contributor(s): Dave Miller <justdave@syndicomm.com>
#                 Joel Peshkin <bugreport@peshkin.net>
#                 Jacob Steenhagen <jake@bugzilla.org>
#                 Vlad Dascalu <jocuri@softhome.net>
#                 Frédéric Buclin <LpSolit@gmail.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Config qw(:admin);
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Group;
use Bugzilla::Product;
use Bugzilla::User;
use Bugzilla::Token;

use constant SPECIAL_GROUPS => ('chartgroup', 'insidergroup',
                                'timetrackinggroup', 'querysharegroup');

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
my $vars = {};

my $user = Bugzilla->login(LOGIN_REQUIRED);

print $cgi->header();

$user->in_group('creategroups')
  || ThrowUserError("auth_failure", {group  => "creategroups",
                                     action => "edit",
                                     object => "groups"});

my $action = trim($cgi->param('action') || '');
my $token  = $cgi->param('token');

# Add missing entries in bug_group_map for bugs created while
# a mandatory group was disabled and which is now enabled again.
sub fix_bug_permissions {
    my $gid = shift;
    my $dbh = Bugzilla->dbh;

    detaint_natural($gid);
    return unless $gid;

    my $bug_ids =
      $dbh->selectcol_arrayref('SELECT bugs.bug_id
                                  FROM bugs
                            INNER JOIN group_control_map
                                    ON group_control_map.product_id = bugs.product_id
                             LEFT JOIN bug_group_map
                                    ON bug_group_map.bug_id = bugs.bug_id
                                   AND bug_group_map.group_id = group_control_map.group_id
                                 WHERE group_control_map.group_id = ?
                                   AND group_control_map.membercontrol = ?
                                   AND bug_group_map.group_id IS NULL',
                                 undef, ($gid, CONTROLMAPMANDATORY));

    my $sth = $dbh->prepare('INSERT INTO bug_group_map (bug_id, group_id) VALUES (?, ?)');
    foreach my $bug_id (@$bug_ids) {
        $sth->execute($bug_id, $gid);
    }
}

# CheckGroupID checks that a positive integer is given and is
# actually a valid group ID. If all tests are successful, the
# trimmed group ID is returned.

sub CheckGroupID {
    my ($group_id) = @_;
    $group_id = trim($group_id || 0);
    ThrowUserError("group_not_specified") unless $group_id;
    (detaint_natural($group_id)
      && Bugzilla->dbh->selectrow_array("SELECT id FROM groups WHERE id = ?",
                                        undef, $group_id))
      || ThrowUserError("invalid_group_ID");
    return $group_id;
}

# This subroutine is called when:
# - a new group is created. CheckGroupName checks that its name
#   is not empty and is not already used by any existing group.
# - an existing group is edited. CheckGroupName checks that its
#   name has not been deleted or renamed to another existing
#   group name (whose group ID is different from $group_id).
# In both cases, an error message is returned to the user if any
# test fails! Else, the trimmed group name is returned.

sub CheckGroupName {
    my ($name, $group_id) = @_;
    $name = trim($name || '');
    trick_taint($name);
    ThrowUserError("empty_group_name") unless $name;
    my $excludeself = (defined $group_id) ? " AND id != $group_id" : "";
    my $name_exists = Bugzilla->dbh->selectrow_array("SELECT name FROM groups " .
                                                     "WHERE name = ? $excludeself",
                                                     undef, $name);
    if ($name_exists) {
        ThrowUserError("group_exists", { name => $name });
    }
    return $name;
}

# CheckGroupDesc checks that a non empty description is given. The
# trimmed description is returned.

sub CheckGroupDesc {
    my ($desc) = @_;
    $desc = trim($desc || '');
    trick_taint($desc);
    ThrowUserError("empty_group_description") unless $desc;
    return $desc;
}

# CheckGroupRegexp checks that the regular expression is valid
# (the regular expression being optional, the test is successful
# if none is given, as expected). The trimmed regular expression
# is returned.

sub CheckGroupRegexp {
    my ($regexp) = @_;
    $regexp = trim($regexp || '');
    trick_taint($regexp);
    ThrowUserError("invalid_regexp") unless (eval {qr/$regexp/});
    return $regexp;
}

# If no action is specified, get a list of all groups available.

unless ($action) {
    my @groups = Bugzilla::Group->get_all;
    $vars->{'groups'} = \@groups;
    
    print $cgi->header();
    $template->process("admin/groups/list.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

#
# action='changeform' -> present form for altering an existing group
#
# (next action will be 'postchanges')
#

if ($action eq 'changeform') {
    # Check that an existing group ID is given
    my $group_id = CheckGroupID($cgi->param('group'));
    my ($name, $description, $regexp, $isactive, $isbuggroup) =
        $dbh->selectrow_array("SELECT name, description, userregexp, " .
                              "isactive, isbuggroup " .
                              "FROM groups WHERE id = ?", undef, $group_id);

    # For each group, we use left joins to establish the existence of
    # a record making that group a member of this group
    # and the existence of a record permitting that group to bless
    # this one

    my @groups;
    my $group_list =
      $dbh->selectall_arrayref('SELECT groups.id, groups.name, groups.description,
                                       CASE WHEN group_group_map.member_id IS NOT NULL
                                            THEN 1 ELSE 0 END,
                                       CASE WHEN B.member_id IS NOT NULL
                                            THEN 1 ELSE 0 END,
                                       CASE WHEN C.member_id IS NOT NULL
                                            THEN 1 ELSE 0 END
                                  FROM groups
                                  LEFT JOIN group_group_map
                                    ON group_group_map.member_id = groups.id
                                   AND group_group_map.grantor_id = ?
                                   AND group_group_map.grant_type = ?
                                  LEFT JOIN group_group_map as B
                                    ON B.member_id = groups.id
                                   AND B.grantor_id = ?
                                   AND B.grant_type = ?
                                  LEFT JOIN group_group_map as C
                                    ON C.member_id = groups.id
                                   AND C.grantor_id = ?
                                   AND C.grant_type = ?
                                 ORDER by name',
                                undef, ($group_id, GROUP_MEMBERSHIP,
                                        $group_id, GROUP_BLESS,
                                        $group_id, GROUP_VISIBLE));

    foreach (@$group_list) {
        my ($grpid, $grpnam, $grpdesc, $grpmember, $blessmember, $membercansee) = @$_;
        my $group = {};
        $group->{'grpid'}       = $grpid;
        $group->{'grpnam'}      = $grpnam;
        $group->{'grpdesc'}     = $grpdesc;
        $group->{'grpmember'}   = $grpmember;
        $group->{'blessmember'} = $blessmember;
        $group->{'membercansee'}= $membercansee;
        push(@groups, $group);
    }

    $vars->{'group_id'}    = $group_id;
    $vars->{'name'}        = $name;
    $vars->{'description'} = $description;
    $vars->{'regexp'}      = $regexp;
    $vars->{'isactive'}    = $isactive;
    $vars->{'isbuggroup'}  = $isbuggroup;
    $vars->{'groups'}      = \@groups;
    $vars->{'token'}       = issue_session_token('edit_group');

    print $cgi->header();
    $template->process("admin/groups/edit.html.tmpl", $vars)
      || ThrowTemplateError($template->error());

    exit;
}

#
# action='add' -> present form for parameters for new group
#
# (next action will be 'new')
#

if ($action eq 'add') {
    $vars->{'token'} = issue_session_token('add_group');
    print $cgi->header();
    $template->process("admin/groups/create.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    
    exit;
}



#
# action='new' -> add group entered in the 'action=add' screen
#

if ($action eq 'new') {
    check_token_data($token, 'add_group');
    # Check that a not already used group name is given, that
    # a description is also given and check if the regular
    # expression is valid (if any).
    my $name = CheckGroupName($cgi->param('name'));
    my $desc = CheckGroupDesc($cgi->param('desc'));
    my $regexp = CheckGroupRegexp($cgi->param('regexp'));
    my $isactive = $cgi->param('isactive') ? 1 : 0;

    # Add the new group
    $dbh->do('INSERT INTO groups
              (name, description, isbuggroup, userregexp, isactive)
              VALUES (?, ?, 1, ?, ?)',
              undef, ($name, $desc, $regexp, $isactive));

    my $gid = $dbh->bz_last_key('groups', 'id');
    my $admin = Bugzilla::Group->new({name => 'admin'})->id();
    # Since we created a new group, give the "admin" group all privileges
    # initially.
    my $sth = $dbh->prepare('INSERT INTO group_group_map
                             (member_id, grantor_id, grant_type)
                             VALUES (?, ?, ?)');

    $sth->execute($admin, $gid, GROUP_MEMBERSHIP);
    $sth->execute($admin, $gid, GROUP_BLESS);
    $sth->execute($admin, $gid, GROUP_VISIBLE);

    # Permit all existing products to use the new group if makeproductgroups.
    if ($cgi->param('insertnew')) {
        $dbh->do('INSERT INTO group_control_map
                  (group_id, product_id, entry, membercontrol,
                   othercontrol, canedit)
                  SELECT ?, products.id, 0, ?, ?, 0 FROM products',
                  undef, ($gid, CONTROLMAPSHOWN, CONTROLMAPNA));
    }
    Bugzilla::Group::RederiveRegexp($regexp, $gid);
    delete_token($token);

    print $cgi->header();
    $template->process("admin/groups/created.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

#
# action='del' -> ask if user really wants to delete
#
# (next action would be 'delete')
#

if ($action eq 'del') {
    # Check that an existing group ID is given
    my $gid = CheckGroupID($cgi->param('group'));
    my ($name, $desc, $isbuggroup) =
        $dbh->selectrow_array("SELECT name, description, isbuggroup " .
                              "FROM groups WHERE id = ?", undef, $gid);

    # System groups cannot be deleted!
    if (!$isbuggroup) {
        ThrowUserError("system_group_not_deletable", { name => $name });
    }
    # Groups having a special role cannot be deleted.
    my @special_groups;
    foreach my $special_group (SPECIAL_GROUPS) {
        if ($name eq Bugzilla->params->{$special_group}) {
            push(@special_groups, $special_group);
        }
    }
    if (scalar(@special_groups)) {
        ThrowUserError('group_has_special_role', {'name'  => $name,
                                                  'groups' => \@special_groups});
    }

    # Group inheritance no longer appears in user_group_map.
    my $grouplist = join(',', @{Bugzilla::User->flatten_group_membership($gid)});
    my $hasusers =
        $dbh->selectrow_array("SELECT 1 FROM user_group_map
                               WHERE group_id IN ($grouplist) AND isbless = 0 " .
                               $dbh->sql_limit(1)) || 0;

    my ($shared_queries) =
        $dbh->selectrow_array('SELECT COUNT(*)
                                 FROM namedquery_group_map
                                WHERE group_id = ?',
                              undef, $gid);

    my $bug_ids = $dbh->selectcol_arrayref('SELECT bug_id FROM bug_group_map
                                            WHERE group_id = ?', undef, $gid);

    my $hasbugs = scalar(@$bug_ids) ? 1 : 0;
    my $buglist = join(',', @$bug_ids);

    my $hasproduct = Bugzilla::Product->new({'name' => $name}) ? 1 : 0;

    my $hasflags = $dbh->selectrow_array('SELECT 1 FROM flagtypes 
                                           WHERE grant_group_id = ?
                                              OR request_group_id = ? ' .
                                          $dbh->sql_limit(1),
                                          undef, ($gid, $gid)) || 0;

    $vars->{'gid'}            = $gid;
    $vars->{'name'}           = $name;
    $vars->{'description'}    = $desc;
    $vars->{'hasusers'}       = $hasusers;
    $vars->{'hasbugs'}        = $hasbugs;
    $vars->{'hasproduct'}     = $hasproduct;
    $vars->{'hasflags'}       = $hasflags;
    $vars->{'shared_queries'} = $shared_queries;
    $vars->{'buglist'}        = $buglist;
    $vars->{'token'}          = issue_session_token('delete_group');

    print $cgi->header();
    $template->process("admin/groups/delete.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    
    exit;
}

#
# action='delete' -> really delete the group
#

if ($action eq 'delete') {
    check_token_data($token, 'delete_group');
    # Check that an existing group ID is given
    my $gid = CheckGroupID($cgi->param('group'));
    my ($name, $isbuggroup) =
        $dbh->selectrow_array("SELECT name, isbuggroup FROM groups " .
                              "WHERE id = ?", undef, $gid);

    # System groups cannot be deleted!
    if (!$isbuggroup) {
        ThrowUserError("system_group_not_deletable", { name => $name });
    }
    # Groups having a special role cannot be deleted.
    my @special_groups;
    foreach my $special_group (SPECIAL_GROUPS) {
        if ($name eq Bugzilla->params->{$special_group}) {
            push(@special_groups, $special_group);
        }
    }
    if (scalar(@special_groups)) {
        ThrowUserError('group_has_special_role', {'name'  => $name,
                                                  'groups' => \@special_groups});
    }

    my $cantdelete = 0;

    # Group inheritance no longer appears in user_group_map.
    my $grouplist = join(',', @{Bugzilla::User->flatten_group_membership($gid)});
    my $hasusers =
        $dbh->selectrow_array("SELECT 1 FROM user_group_map
                               WHERE group_id IN ($grouplist) AND isbless = 0 " .
                               $dbh->sql_limit(1)) || 0;

    if ($hasusers && !defined $cgi->param('removeusers')) {
        $cantdelete = 1;
    }

    my $hasbugs = $dbh->selectrow_array('SELECT 1 FROM bug_group_map
                                         WHERE group_id = ? ' .
                                         $dbh->sql_limit(1),
                                         undef, $gid) || 0;
    if ($hasbugs && !defined $cgi->param('removebugs')) {
        $cantdelete = 1;
    }

    if (Bugzilla::Product->new({'name' => $name})
        && !defined $cgi->param('unbind'))
    {
        $cantdelete = 1;
    }

    my $hasflags = $dbh->selectrow_array('SELECT 1 FROM flagtypes 
                                           WHERE grant_group_id = ?
                                              OR request_group_id = ? ' .
                                          $dbh->sql_limit(1),
                                          undef, ($gid, $gid)) || 0;
    if ($hasflags && !defined $cgi->param('removeflags')) {
        $cantdelete = 1;
    }

    $vars->{'gid'}        = $gid;
    $vars->{'name'}       = $name;

    ThrowUserError('group_cannot_delete', $vars) if $cantdelete;

    $dbh->do('UPDATE flagtypes SET grant_group_id = ?
               WHERE grant_group_id = ?',
              undef, (undef, $gid));
    $dbh->do('UPDATE flagtypes SET request_group_id = ?
               WHERE request_group_id = ?',
              undef, (undef, $gid));
    $dbh->do('DELETE FROM namedquery_group_map WHERE group_id = ?',
              undef, $gid);
    $dbh->do('DELETE FROM user_group_map WHERE group_id = ?',
              undef, $gid);
    $dbh->do('DELETE FROM group_group_map 
               WHERE grantor_id = ? OR member_id = ?',
              undef, ($gid, $gid));
    $dbh->do('DELETE FROM bug_group_map WHERE group_id = ?',
              undef, $gid);
    $dbh->do('DELETE FROM group_control_map WHERE group_id = ?',
              undef, $gid);
    $dbh->do('DELETE FROM whine_schedules
               WHERE mailto_type = ? AND mailto = ?',
              undef, (MAILTO_GROUP, $gid));
    $dbh->do('DELETE FROM groups WHERE id = ?',
              undef, $gid);

    delete_token($token);

    print $cgi->header();
    $template->process("admin/groups/deleted.html.tmpl", $vars)
      || ThrowTemplateError($template->error());

    exit;
}

#
# action='postchanges' -> update the groups
#

if ($action eq 'postchanges') {
    check_token_data($token, 'edit_group');
    # ZLL: Bug 181589: we need to have something to remove explicitly listed users from
    # groups in order for the conversion to 2.18 groups to work
    my $action;

    if ($cgi->param('remove_explicit_members')) {
        $action = 1;
    } elsif ($cgi->param('remove_explicit_members_regexp')) {
        $action = 2;
    } else {
        $action = 3;
    }
    
    my ($gid, $chgs, $name, $regexp) = doGroupChanges();
    
    $vars->{'action'}  = $action;
    $vars->{'changes'} = $chgs;
    $vars->{'gid'}     = $gid;
    $vars->{'name'}    = $name;
    if ($action == 2) {
        $vars->{'regexp'} = $regexp;
    }
    delete_token($token);

    print $cgi->header();
    $template->process("admin/groups/change.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

if (($action eq 'remove_all_regexp') || ($action eq 'remove_all')) {
    # remove all explicit users from the group with
    # gid = $cgi->param('group') that match the regular expression
    # stored in the DB for that group or all of them period

    my $gid = CheckGroupID($cgi->param('group'));

    my ($name, $regexp) =
      $dbh->selectrow_array('SELECT name, userregexp FROM groups
                             WHERE id = ?', undef, $gid);

    $dbh->bz_lock_tables('groups WRITE', 'profiles READ',
                         'user_group_map WRITE');

    my $sth = $dbh->prepare("SELECT user_group_map.user_id, profiles.login_name
                               FROM user_group_map
                         INNER JOIN profiles
                                 ON user_group_map.user_id = profiles.userid
                              WHERE user_group_map.group_id = ?
                                AND grant_type = ?
                                AND isbless = 0");
    $sth->execute($gid, GRANT_DIRECT);

    my @users;
    my $sth2 = $dbh->prepare("DELETE FROM user_group_map
                              WHERE user_id = ?
                              AND isbless = 0
                              AND group_id = ?");

    while ( my ($userid, $userlogin) = $sth->fetchrow_array() ) {
        if ((($regexp =~ /\S/) && ($userlogin =~ m/$regexp/i))
            || ($action eq 'remove_all'))
        {
            $sth2->execute($userid, $gid);

            my $user = {};
            $user->{'login'} = $userlogin;
            push(@users, $user);
        }
    }
    $dbh->bz_unlock_tables();

    $vars->{'users'}      = \@users;
    $vars->{'name'}       = $name;
    $vars->{'regexp'}     = $regexp;
    $vars->{'remove_all'} = ($action eq 'remove_all');
    $vars->{'gid'}        = $gid;
    
    print $cgi->header();
    $template->process("admin/groups/remove.html.tmpl", $vars)
      || ThrowTemplateError($template->error());

    exit;
}


#
# No valid action found
#

ThrowCodeError("action_unrecognized", $vars);


# Helper sub to handle the making of changes to a group
sub doGroupChanges {
    my $cgi = Bugzilla->cgi;
    my $dbh = Bugzilla->dbh;

    $dbh->bz_lock_tables('groups WRITE', 'group_group_map WRITE',
                         'bug_group_map WRITE', 'user_group_map WRITE',
                         'group_control_map READ', 'bugs READ', 'profiles READ',
                         # Due to the way Bugzilla::Config::BugFields::get_param_list()
                         # works, we need to lock these tables too.
                         'priority READ', 'bug_severity READ', 'rep_platform READ',
                         'op_sys READ');

    # Check that the given group ID and regular expression are valid.
    # If tests are successful, trimmed values are returned by CheckGroup*.
    my $gid = CheckGroupID($cgi->param('group'));
    my $regexp = CheckGroupRegexp($cgi->param('regexp'));

    # The name and the description of system groups cannot be edited.
    # We then need to know if the group being edited is a system group.
    my $isbuggroup = $dbh->selectrow_array('SELECT isbuggroup FROM groups
                                            WHERE id = ?', undef, $gid);
    my $name;
    my $desc;
    my $isactive;
    my $chgs = 0;

    # We trust old values given by the template. If they are hacked
    # in a way that some of the tests below become negative, the
    # corresponding attributes are not updated in the DB, which does
    # not hurt.
    if ($isbuggroup) {
        # Check that the group name and its description are valid
        # and return trimmed values if tests are successful.
        $name = CheckGroupName($cgi->param('name'), $gid);
        $desc = CheckGroupDesc($cgi->param('desc'));
        $isactive = $cgi->param('isactive') ? 1 : 0;

        if ($name ne $cgi->param('oldname')) {
            $chgs = 1;
            $dbh->do('UPDATE groups SET name = ? WHERE id = ?',
                      undef, ($name, $gid));
            # If the group is used by some parameters, we have to update
            # these parameters too.
            my $update_params = 0;
            foreach my $group (SPECIAL_GROUPS) {
                if ($cgi->param('oldname') eq Bugzilla->params->{$group}) {
                    SetParam($group, $name);
                    $update_params = 1;
                }
            }
            write_params() if $update_params;
        }
        if ($desc ne $cgi->param('olddesc')) {
            $chgs = 1;
            $dbh->do('UPDATE groups SET description = ? WHERE id = ?',
                      undef, ($desc, $gid));
        }
        if ($isactive ne $cgi->param('oldisactive')) {
            $chgs = 1;
            $dbh->do('UPDATE groups SET isactive = ? WHERE id = ?',
                      undef, ($isactive, $gid));
            # If the group was mandatory for some products before
            # we deactivated it and we now activate this group again,
            # we have to add all bugs created while this group was
            # disabled in bug_group_map to correctly protect them.
            if ($isactive) { fix_bug_permissions($gid); }
        }
    }
    if ($regexp ne $cgi->param('oldregexp')) {
        $chgs = 1;
        $dbh->do('UPDATE groups SET userregexp = ? WHERE id = ?',
                  undef, ($regexp, $gid));
        Bugzilla::Group::RederiveRegexp($regexp, $gid);
    }

    my $sthInsert = $dbh->prepare('INSERT INTO group_group_map
                                   (member_id, grantor_id, grant_type)
                                   VALUES (?, ?, ?)');

    my $sthDelete = $dbh->prepare('DELETE FROM group_group_map
                                    WHERE member_id = ?
                                      AND grantor_id = ?
                                      AND grant_type = ?');

    foreach my $b (grep {/^oldgrp-\d*$/} $cgi->param()) {
        if (defined($cgi->param($b))) {
            $b =~ /^oldgrp-(\d+)$/;
            my $v = $1;
            my $grp = $cgi->param("grp-$v") || 0;
            if (($v != $gid) && ($cgi->param("oldgrp-$v") != $grp)) {
                $chgs = 1;
                if ($grp != 0) {
                    $sthInsert->execute($v, $gid, GROUP_MEMBERSHIP);
                } else {
                    $sthDelete->execute($v, $gid, GROUP_MEMBERSHIP);
                }
            }

            my $bless = $cgi->param("bless-$v") || 0;
            my $oldbless = $cgi->param("oldbless-$v");
            if ((defined $oldbless) and ($oldbless != $bless)) {
                $chgs = 1;
                if ($bless != 0) {
                    $sthInsert->execute($v, $gid, GROUP_BLESS);
                } else {
                    $sthDelete->execute($v, $gid, GROUP_BLESS);
                }
            }

            my $cansee = $cgi->param("cansee-$v") || 0;
            if (Bugzilla->params->{"usevisibilitygroups"} 
               && ($cgi->param("oldcansee-$v") != $cansee)) {
                $chgs = 1;
                if ($cansee != 0) {
                    $sthInsert->execute($v, $gid, GROUP_VISIBLE);
                } else {
                    $sthDelete->execute($v, $gid, GROUP_VISIBLE);
                }
            }

        }
    }
    $dbh->bz_unlock_tables();
    return $gid, $chgs, $name, $regexp;
}
