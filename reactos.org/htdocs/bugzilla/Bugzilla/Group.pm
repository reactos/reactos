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
# Contributor(s): Joel Peshkin <bugreport@peshkin.net>
#                 Erik Stambaugh <erik@dasbistro.com>
#                 Tiago R. Mello <timello@async.com.br>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

use strict;

package Bugzilla::Group;

use base qw(Bugzilla::Object);

use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;

###############################
##### Module Initialization ###
###############################

use constant DB_COLUMNS => qw(
    groups.id
    groups.name
    groups.description
    groups.isbuggroup
    groups.userregexp
    groups.isactive
);

use constant DB_TABLE => 'groups';

use constant LIST_ORDER => 'isbuggroup, name';

use constant VALIDATORS => {
    name        => \&_check_name,
    description => \&_check_description,
    userregexp  => \&_check_user_regexp,
    isbuggroup  => \&_check_is_bug_group,
};

use constant REQUIRED_CREATE_FIELDS => qw(name description isbuggroup);

###############################
####      Accessors      ######
###############################

sub description  { return $_[0]->{'description'};  }
sub is_bug_group { return $_[0]->{'isbuggroup'};   }
sub user_regexp  { return $_[0]->{'userregexp'};   }
sub is_active    { return $_[0]->{'isactive'};     }

###############################
####        Methods        ####
###############################

sub is_active_bug_group {
    my $self = shift;
    return $self->is_active && $self->is_bug_group;
}

sub _rederive_regexp {
    my ($self) = @_;
    RederiveRegexp($self->user_regexp, $self->id);
}

sub members_non_inherited {
    my ($self) = @_;
    return $self->{members_non_inherited} 
           if exists $self->{members_non_inherited};

    my $member_ids = Bugzilla->dbh->selectcol_arrayref(
        'SELECT DISTINCT user_id FROM user_group_map 
          WHERE isbless = 0 AND group_id = ?',
        undef, $self->id) || [];
    require Bugzilla::User;
    $self->{members_non_inherited} = Bugzilla::User->new_from_list($member_ids);
    return $self->{members_non_inherited};
}

################################
#####  Module Subroutines    ###
################################

sub create {
    my $class = shift;
    my ($params) = @_;
    my $dbh = Bugzilla->dbh;

    print get_text('install_group_create', { name => $params->{name} }) . "\n" 
        if Bugzilla->usage_mode == USAGE_MODE_CMDLINE;

    my $group = $class->SUPER::create(@_);

    # Since we created a new group, give the "admin" group all privileges
    # initially.
    my $admin = new Bugzilla::Group({name => 'admin'});
    # This function is also used to create the "admin" group itself,
    # so there's a chance it won't exist yet.
    if ($admin) {
        my $sth = $dbh->prepare('INSERT INTO group_group_map
                                 (member_id, grantor_id, grant_type)
                                 VALUES (?, ?, ?)');
        $sth->execute($admin->id, $group->id, GROUP_MEMBERSHIP);
        $sth->execute($admin->id, $group->id, GROUP_BLESS);
        $sth->execute($admin->id, $group->id, GROUP_VISIBLE);
    }

    $group->_rederive_regexp() if $group->user_regexp;
    return $group;
}

sub ValidateGroupName {
    my ($name, @users) = (@_);
    my $dbh = Bugzilla->dbh;
    my $query = "SELECT id FROM groups " .
                "WHERE name = ?";
    if (Bugzilla->params->{'usevisibilitygroups'}) {
        my @visible = (-1);
        foreach my $user (@users) {
            $user && push @visible, @{$user->visible_groups_direct};
        }
        my $visible = join(', ', @visible);
        $query .= " AND id IN($visible)";
    }
    my $sth = $dbh->prepare($query);
    $sth->execute($name);
    my ($ret) = $sth->fetchrow_array();
    return $ret;
}

# This sub is not perldoc'ed because we expect it to go away and
# just become the _rederive_regexp private method.
sub RederiveRegexp {
    my ($regexp, $gid) = @_;
    my $dbh = Bugzilla->dbh;
    my $sth = $dbh->prepare("SELECT userid, login_name, group_id
                               FROM profiles
                          LEFT JOIN user_group_map
                                 ON user_group_map.user_id = profiles.userid
                                AND group_id = ?
                                AND grant_type = ?
                                AND isbless = 0");
    my $sthadd = $dbh->prepare("INSERT INTO user_group_map
                                 (user_id, group_id, grant_type, isbless)
                                 VALUES (?, ?, ?, 0)");
    my $sthdel = $dbh->prepare("DELETE FROM user_group_map
                                 WHERE user_id = ? AND group_id = ?
                                 AND grant_type = ? and isbless = 0");
    $sth->execute($gid, GRANT_REGEXP);
    while (my ($uid, $login, $present) = $sth->fetchrow_array()) {
        if (($regexp =~ /\S+/) && ($login =~ m/$regexp/i))
        {
            $sthadd->execute($uid, $gid, GRANT_REGEXP) unless $present;
        } else {
            $sthdel->execute($uid, $gid, GRANT_REGEXP) if $present;
        }
    }
}

###############################
###       Validators        ###
###############################

sub _check_name {
    my ($invocant, $name) = @_;
    $name = trim($name);
    $name || ThrowUserError("empty_group_name");
    my $exists = new Bugzilla::Group({name => $name });
    ThrowUserError("group_exists", { name => $name }) if $exists;
    return $name;
}

sub _check_description {
    my ($invocant, $desc) = @_;
    $desc = trim($desc);
    $desc || ThrowUserError("empty_group_description");
    return $desc;
}

sub _check_user_regexp {
    my ($invocant, $regex) = @_;
    $regex = trim($regex) || '';
    ThrowUserError("invalid_regexp") unless (eval {qr/$regex/});
    return $regex;
}

sub _check_is_bug_group {
    return $_[1] ? 1 : 0;
}
1;

__END__

=head1 NAME

Bugzilla::Group - Bugzilla group class.

=head1 SYNOPSIS

    use Bugzilla::Group;

    my $group = new Bugzilla::Group(1);
    my $group = new Bugzilla::Group({name => 'AcmeGroup'});

    my $id           = $group->id;
    my $name         = $group->name;
    my $description  = $group->description;
    my $user_reg_exp = $group->user_reg_exp;
    my $is_active    = $group->is_active;
    my $is_active_bug_group = $group->is_active_bug_group;

    my $group_id = Bugzilla::Group::ValidateGroupName('admin', @users);
    my @groups   = Bugzilla::Group->get_all;

=head1 DESCRIPTION

Group.pm represents a Bugzilla Group object. It is an implementation
of L<Bugzilla::Object>, and thus has all the methods that L<Bugzilla::Object>
provides, in addition to any methods documented below.

=head1 SUBROUTINES

=over

=item C<create>

Note that in addition to what L<Bugzilla::Object/create($params)>
normally does, this function also makes the new group be inherited
by the C<admin> group. That is, the C<admin> group will automatically
be a member of this group.

=item C<ValidateGroupName($name, @users)>

 Description: ValidateGroupName checks to see if ANY of the users
              in the provided list of user objects can see the
              named group.

 Params:      $name - String with the group name.
              @users - An array with Bugzilla::User objects.

 Returns:     It returns the group id if successful
              and undef otherwise.

=back

=head1 METHODS

=over

=item C<members_non_inherited>

Returns an arrayref of L<Bugzilla::User> objects representing people who are
"directly" in this group, meaning that they're in it because they match
the group regular expression, or they have been actually added to the
group manually.

=back
