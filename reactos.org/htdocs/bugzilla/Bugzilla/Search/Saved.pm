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
# The Initial Developer of the Original Code is Everything Solved.
# Portions created by Everything Solved are Copyright (C) 2006 
# Everything Solved. All Rights Reserved.
#
# Contributor(s): Max Kanat-Alexander <mkanat@bugzilla.org>

use strict;

package Bugzilla::Search::Saved;

use base qw(Bugzilla::Object);

use Bugzilla::CGI;
use Bugzilla::Constants;
use Bugzilla::Group;
use Bugzilla::Error;
use Bugzilla::Search qw(IsValidQueryType);
use Bugzilla::User;
use Bugzilla::Util;

#############
# Constants #
#############

use constant DB_TABLE => 'namedqueries';

use constant DB_COLUMNS => qw(
    id
    userid
    name
    query
    query_type
);

use constant REQUIRED_CREATE_FIELDS => qw(name query);

use constant VALIDATORS => {
    name       => \&_check_name,
    query      => \&_check_query,
    query_type => \&_check_query_type,
    link_in_footer => \&_check_link_in_footer,
};

use constant UPDATE_COLUMNS => qw(query query_type);

##############
# Validators #
##############

sub _check_link_in_footer { return $_[1] ? 1 : 0; }

sub _check_name {
    my ($invocant, $name) = @_;
    $name = trim($name);
    $name || ThrowUserError("query_name_missing");
    $name !~ /[<>&]/ || ThrowUserError("illegal_query_name");
    if (length($name) > MAX_LEN_QUERY_NAME) {
        ThrowUserError("query_name_too_long");
    }
    return $name;
}

sub _check_query {
    my ($invocant, $query) = @_;
    $query || ThrowUserError("buglist_parameters_required");
    my $cgi = new Bugzilla::CGI($query);
    $cgi->clean_search_url;
    return $cgi->query_string;
}

sub _check_query_type {
    my ($invocant, $type) = @_;
    # Right now the only query type is LIST_OF_BUGS.
    return $type ? LIST_OF_BUGS : QUERY_LIST;
}

#########################
# Database Manipulation #
#########################

sub create {
    my $class = shift;
    Bugzilla->login(LOGIN_REQUIRED);
    my $dbh = Bugzilla->dbh;
    $class->check_required_create_fields(@_);
    my $params = $class->run_create_validators(@_);

    # Right now you can only create a Saved Search for the current user.
    $params->{userid} = Bugzilla->user->id;

    $dbh->bz_lock_tables('namedqueries WRITE',
                         'namedqueries_link_in_footer WRITE');
    my $lif = delete $params->{link_in_footer};
    my $obj = $class->insert_create_data($params);
    if ($lif) {
        $dbh->do('INSERT INTO namedqueries_link_in_footer 
                  (user_id, namedquery_id) VALUES (?,?)',
                 undef, $params->{userid}, $obj->id);
    }
    $dbh->bz_unlock_tables();

    return $obj;
}

#####################
# Complex Accessors #
#####################

sub edit_link {
    my ($self) = @_;
    return $self->{edit_link} if defined $self->{edit_link};
    my $cgi = new Bugzilla::CGI($self->url);
    if (!$cgi->param('query_type') 
        || !IsValidQueryType($cgi->param('query_type')))
    {
        $cgi->param('query_type', 'advanced');
    }
    $self->{edit_link} = $cgi->canonicalise_query;
    return $self->{edit_link};
}

sub used_in_whine {
    my ($self) = @_;
    return $self->{used_in_whine} if exists $self->{used_in_whine};
    ($self->{used_in_whine}) = Bugzilla->dbh->selectrow_array(
        'SELECT 1 FROM whine_events INNER JOIN whine_queries
                       ON whine_events.id = whine_queries.eventid
          WHERE whine_events.owner_userid = ? AND query_name = ?', undef, 
          $self->{userid}, $self->name) || 0;
    return $self->{used_in_whine};
}

sub link_in_footer {
    my ($self, $user) = @_;
    # We only cache link_in_footer for the current Bugzilla->user.
    return $self->{link_in_footer} if exists $self->{link_in_footer} && !$user;
    my $user_id = $user ? $user->id : Bugzilla->user->id;
    my $link_in_footer = Bugzilla->dbh->selectrow_array(
        'SELECT 1 FROM namedqueries_link_in_footer
          WHERE namedquery_id = ? AND user_id = ?', 
        undef, $self->id, $user_id) || 0;
    $self->{link_in_footer} = $link_in_footer if !$user;
    return $link_in_footer;
}

sub shared_with_group {
    my ($self) = @_;
    return $self->{shared_with_group} if exists $self->{shared_with_group};
    # Bugzilla only currently supports sharing with one group, even
    # though the database backend allows for an infinite number.
    my ($group_id) = Bugzilla->dbh->selectrow_array(
        'SELECT group_id FROM namedquery_group_map WHERE namedquery_id = ?',
        undef, $self->id);
    $self->{shared_with_group} = $group_id ? new Bugzilla::Group($group_id) 
                                 : undef;
    return $self->{shared_with_group};
}

####################
# Simple Accessors #
####################

sub bug_ids_only { return ($_[0]->{'query_type'} == LIST_OF_BUGS) ? 1 : 0; }
sub url          { return $_[0]->{'query'}; }

sub user {
    my ($self) = @_;
    return $self->{user} if defined $self->{user};
    $self->{user} = new Bugzilla::User($self->{userid});
    return $self->{user};
}

############
# Mutators #
############

sub set_url        { $_[0]->set('query',      $_[1]); }
sub set_query_type { $_[0]->set('query_type', $_[1]); }

1;

__END__

=head1 NAME

Bugzilla::Search::Saved - A saved search

=head1 SYNOPSIS

 use Bugzilla::Search::Saved;

 my $query = new Bugzilla::Search::Saved($query_id);

 my $edit_link  = $query->edit_link;
 my $search_url = $query->url;
 my $owner      = $query->user;

=head1 DESCRIPTION

This module exists to represent a L<Bugzilla::Search> that has been
saved to the database.

This is an implementation of L<Bugzilla::Object>, and so has all the
same methods available as L<Bugzilla::Object>, in addition to what is
documented below.

=head1 METHODS

=head2 Constructors and Database Manipulation

=over

=item C<new>

Does not accept a bare C<name> argument. Instead, accepts only an id.

See also: L<Bugzilla::Object/new>.

=back


=head2 Accessors

These return data about the object, without modifying the object.

=over

=item C<edit_link>

A url with which you can edit the search.

=item C<url>

The CGI parameters for the search, as a string.

=item C<link_in_footer>

Whether or not this search should be displayed in the footer for the
I<current user> (not the owner of the search, but the person actually
using Bugzilla right now).

=item C<bug_ids_only>

True if the search contains only a list of Bug IDs.

=item C<shared_with_group>

The L<Bugzilla::Group> that this search is shared with. C<undef> if
this search isn't shared.

=back
