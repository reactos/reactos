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
# Contributor(s): Myk Melez <myk@mozilla.org>
#                 Jouni Heikniemi <jouni@heikniemi.net>
#                 Frédéric Buclin <LpSolit@gmail.com>

use strict;

package Bugzilla::Flag;

=head1 NAME

Bugzilla::Flag - A module to deal with Bugzilla flag values.

=head1 SYNOPSIS

Flag.pm provides an interface to flags as stored in Bugzilla.
See below for more information.

=head1 NOTES

=over

=item *

Import relevant functions from that script.

=item *

Use of private functions / variables outside this module may lead to
unexpected results after an upgrade.  Please avoid using private
functions in other files/modules.  Private functions are functions
whose names start with _ or a re specifically noted as being private.

=back

=cut

use Bugzilla::FlagType;
use Bugzilla::User;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Mailer;
use Bugzilla::Constants;
use Bugzilla::Field;

use base qw(Bugzilla::Object Exporter);
@Bugzilla::Flag::EXPORT = qw(SKIP_REQUESTEE_ON_ERROR);

###############################
####    Initialization     ####
###############################

use constant DB_COLUMNS => qw(
    flags.id
    flags.type_id
    flags.bug_id
    flags.attach_id
    flags.requestee_id
    flags.setter_id
    flags.status
);

use constant DB_TABLE => 'flags';
use constant LIST_ORDER => 'id';

use constant SKIP_REQUESTEE_ON_ERROR => 1;

###############################
####      Accessors      ######
###############################

=head2 METHODS

=over

=item C<id>

Returns the ID of the flag.

=item C<name>

Returns the name of the flagtype the flag belongs to.

=item C<bug_id>

Returns the ID of the bug this flag belongs to.

=item C<attach_id>

Returns the ID of the attachment this flag belongs to, if any.

=item C<status>

Returns the status '+', '-', '?' of the flag.

=back

=cut

sub id     { return $_[0]->{'id'};     }
sub name   { return $_[0]->type->name; }
sub bug_id { return $_[0]->{'bug_id'}; }
sub attach_id { return $_[0]->{'attach_id'}; }
sub status { return $_[0]->{'status'}; }

###############################
####       Methods         ####
###############################

=pod

=over

=item C<type>

Returns the type of the flag, as a Bugzilla::FlagType object.

=item C<setter>

Returns the user who set the flag, as a Bugzilla::User object.

=item C<requestee>

Returns the user who has been requested to set the flag, as a
Bugzilla::User object.

=item C<attachment>

Returns the attachment object the flag belongs to if the flag
is an attachment flag, else undefined.

=back

=cut

sub type {
    my $self = shift;

    $self->{'type'} ||= new Bugzilla::FlagType($self->{'type_id'});
    return $self->{'type'};
}

sub setter {
    my $self = shift;

    $self->{'setter'} ||= new Bugzilla::User($self->{'setter_id'});
    return $self->{'setter'};
}

sub requestee {
    my $self = shift;

    if (!defined $self->{'requestee'} && $self->{'requestee_id'}) {
        $self->{'requestee'} = new Bugzilla::User($self->{'requestee_id'});
    }
    return $self->{'requestee'};
}

sub attachment {
    my $self = shift;
    return undef unless $self->attach_id;

    require Bugzilla::Attachment;
    $self->{'attachment'} ||= Bugzilla::Attachment->get($self->attach_id);
    return $self->{'attachment'};
}

################################
## Searching/Retrieving Flags ##
################################

=pod

=over

=item C<match($criteria)>

Queries the database for flags matching the given criteria
(specified as a hash of field names and their matching values)
and returns an array of matching records.

=back

=cut

sub match {
    my ($criteria) = @_;
    my $dbh = Bugzilla->dbh;

    my @criteria = sqlify_criteria($criteria);
    $criteria = join(' AND ', @criteria);

    my $flag_ids = $dbh->selectcol_arrayref("SELECT id FROM flags
                                             WHERE $criteria");

    return Bugzilla::Flag->new_from_list($flag_ids);
}

=pod

=over

=item C<count($criteria)>

Queries the database for flags matching the given criteria
(specified as a hash of field names and their matching values)
and returns an array of matching records.

=back

=cut

sub count {
    my ($criteria) = @_;
    my $dbh = Bugzilla->dbh;

    my @criteria = sqlify_criteria($criteria);
    $criteria = join(' AND ', @criteria);

    my $count = $dbh->selectrow_array("SELECT COUNT(*) FROM flags WHERE $criteria");

    return $count;
}

######################################################################
# Creating and Modifying
######################################################################

=pod

=over

=item C<validate($cgi, $bug_id, $attach_id, $skip_requestee_on_error)>

Validates fields containing flag modifications.

If the attachment is new, it has no ID yet and $attach_id is set
to -1 to force its check anyway.

=back

=cut

sub validate {
    my ($cgi, $bug_id, $attach_id, $skip_requestee_on_error) = @_;

    my $dbh = Bugzilla->dbh;

    # Get a list of flags to validate.  Uses the "map" function
    # to extract flag IDs from form field names by matching fields
    # whose name looks like "flag_type-nnn" (new flags) or "flag-nnn"
    # (existing flags), where "nnn" is the ID, and returning just
    # the ID portion of matching field names.
    my @flagtype_ids = map(/^flag_type-(\d+)$/ ? $1 : (), $cgi->param());
    my @flag_ids = map(/^flag-(\d+)$/ ? $1 : (), $cgi->param());

    return unless (scalar(@flagtype_ids) || scalar(@flag_ids));

    # No flag reference should exist when changing several bugs at once.
    ThrowCodeError("flags_not_available", { type => 'b' }) unless $bug_id;

    # We don't check that these new flags are valid for this bug/attachment,
    # because the bug may be moved into another product meanwhile.
    # This check will be done later when creating new flags, see FormToNewFlags().

    if (scalar(@flag_ids)) {
        # No reference to existing flags should exist when creating a new
        # attachment.
        if ($attach_id && ($attach_id < 0)) {
            ThrowCodeError('flags_not_available', { type => 'a' });
        }

        # Make sure all existing flags belong to the bug/attachment
        # they pretend to be.
        my $field = ($attach_id) ? "attach_id" : "bug_id";
        my $field_id = $attach_id || $bug_id;
        my $not = ($attach_id) ? "" : "NOT";

        my $invalid_data =
            $dbh->selectrow_array("SELECT 1 FROM flags
                                   WHERE id IN (" . join(',', @flag_ids) . ")
                                   AND ($field != ? OR attach_id IS $not NULL) " .
                                   $dbh->sql_limit(1),
                                   undef, $field_id);

        if ($invalid_data) {
            ThrowCodeError('invalid_flag_association',
                           { bug_id    => $bug_id,
                             attach_id => $attach_id });
        }
    }

    # Validate new flags.
    foreach my $id (@flagtype_ids) {
        my $status = $cgi->param("flag_type-$id");
        my @requestees = $cgi->param("requestee_type-$id");
        my $private_attachment = $cgi->param('isprivate') ? 1 : 0;

        # Don't bother validating types the user didn't touch.
        next if $status eq 'X';

        # Make sure the flag type exists. If it doesn't, FormToNewFlags()
        # will ignore it, so it's safe to ignore it here.
        my $flag_type = new Bugzilla::FlagType($id);
        next unless $flag_type;

        # Make sure the flag type is active.
        unless ($flag_type->is_active) {
            ThrowCodeError('flag_type_inactive', {'type' => $flag_type->name});
        }

        _validate(undef, $flag_type, $status, undef, \@requestees, $private_attachment,
                  $bug_id, $attach_id, $skip_requestee_on_error);
    }

    # Validate existing flags.
    foreach my $id (@flag_ids) {
        my $status = $cgi->param("flag-$id");
        my @requestees = $cgi->param("requestee-$id");
        my $private_attachment = $cgi->param('isprivate') ? 1 : 0;

        # Make sure the flag exists. If it doesn't, process() will ignore it,
        # so it's safe to ignore it here.
        my $flag = new Bugzilla::Flag($id);
        next unless $flag;

        _validate($flag, $flag->type, $status, undef, \@requestees, $private_attachment,
                  undef, undef, $skip_requestee_on_error);
    }
}

sub _validate {
    my ($flag, $flag_type, $status, $setter, $requestees, $private_attachment,
        $bug_id, $attach_id, $skip_requestee_on_error) = @_;

    # By default, the flag setter (or requester) is the current user.
    $setter ||= Bugzilla->user;

    my $id = $flag ? $flag->id : $flag_type->id; # Used in the error messages below.
    $bug_id ||= $flag->bug_id;
    $attach_id ||= $flag->attach_id if $flag; # Maybe it's a bug flag.

    # Make sure the user chose a valid status.
    grep($status eq $_, qw(X + - ?))
      || ThrowCodeError('flag_status_invalid',
                        { id => $id, status => $status });

    # Make sure the user didn't request the flag unless it's requestable.
    # If the flag existed and was requested before it became unrequestable,
    # leave it as is.
    if ($status eq '?'
        && (!$flag || $flag->status ne '?')
        && !$flag_type->is_requestable)
    {
        ThrowCodeError('flag_status_invalid',
                       { id => $id, status => $status });
    }

    # Make sure the user didn't specify a requestee unless the flag
    # is specifically requestable. For existing flags, if the requestee
    # was set before the flag became specifically unrequestable, don't
    # let the user change the requestee, but let the user remove it by
    # entering an empty string for the requestee.
    if ($status eq '?' && !$flag_type->is_requesteeble) {
        my $old_requestee = ($flag && $flag->requestee) ?
                                $flag->requestee->login : '';
        my $new_requestee = join('', @$requestees);
        if ($new_requestee && $new_requestee ne $old_requestee) {
            ThrowCodeError('flag_requestee_disabled',
                           { type => $flag_type });
        }
    }

    # Make sure the user didn't enter multiple requestees for a flag
    # that can't be requested from more than one person at a time.
    if ($status eq '?'
        && !$flag_type->is_multiplicable
        && scalar(@$requestees) > 1)
    {
        ThrowUserError('flag_not_multiplicable', { type => $flag_type });
    }

    # Make sure the requestees are authorized to access the bug
    # (and attachment, if this installation is using the "insider group"
    # feature and the attachment is marked private).
    if ($status eq '?' && $flag_type->is_requesteeble) {
        my $old_requestee = ($flag && $flag->requestee) ?
                                $flag->requestee->login : '';

        my @legal_requestees;
        foreach my $login (@$requestees) {
            if ($login eq $old_requestee) {
                # This requestee was already set. Leave him alone.
                push(@legal_requestees, $login);
                next;
            }

            # We know the requestee exists because we ran
            # Bugzilla::User::match_field before getting here.
            my $requestee = new Bugzilla::User({ name => $login });

            # Throw an error if the user can't see the bug.
            # Note that if permissions on this bug are changed,
            # can_see_bug() will refer to old settings.
            if (!$requestee->can_see_bug($bug_id)) {
                next if $skip_requestee_on_error;
                ThrowUserError('flag_requestee_unauthorized',
                               { flag_type  => $flag_type,
                                 requestee  => $requestee,
                                 bug_id     => $bug_id,
                                 attach_id  => $attach_id });
            }

            # Throw an error if the target is a private attachment and
            # the requestee isn't in the group of insiders who can see it.
            if ($attach_id
                && $private_attachment
                && Bugzilla->params->{'insidergroup'}
                && !$requestee->in_group(Bugzilla->params->{'insidergroup'}))
            {
                next if $skip_requestee_on_error;
                ThrowUserError('flag_requestee_unauthorized_attachment',
                               { flag_type  => $flag_type,
                                 requestee  => $requestee,
                                 bug_id     => $bug_id,
                                 attach_id  => $attach_id });
            }

            # Throw an error if the user won't be allowed to set the flag.
            if (!$requestee->can_set_flag($flag_type)) {
                next if $skip_requestee_on_error;
                ThrowUserError('flag_requestee_needs_privs',
                               {'requestee' => $requestee,
                                'flagtype'  => $flag_type});
            }

            # This requestee can be set.
            push(@legal_requestees, $login);
        }

        # Update the requestee list for this flag.
        if (scalar(@legal_requestees) < scalar(@$requestees)) {
            my $field_name = 'requestee_type-' . $flag_type->id;
            Bugzilla->cgi->delete($field_name);
            Bugzilla->cgi->param(-name => $field_name, -value => \@legal_requestees);
        }
    }

    # Make sure the user is authorized to modify flags, see bug 180879
    # - The flag exists and is unchanged.
    return if ($flag && ($status eq $flag->status));

    # - User in the request_group can clear pending requests and set flags
    #   and can rerequest set flags.
    return if (($status eq 'X' || $status eq '?')
               && $setter->can_request_flag($flag_type));

    # - User in the grant_group can set/clear flags, including "+" and "-".
    return if $setter->can_set_flag($flag_type);

    # - Any other flag modification is denied
    ThrowUserError('flag_update_denied',
                    { name       => $flag_type->name,
                      status     => $status,
                      old_status => $flag ? $flag->status : 'X' });
}

sub snapshot {
    my ($bug_id, $attach_id) = @_;

    my $flags = match({ 'bug_id'    => $bug_id,
                        'attach_id' => $attach_id });
    my @summaries;
    foreach my $flag (@$flags) {
        my $summary = $flag->type->name . $flag->status;
        $summary .= "(" . $flag->requestee->login . ")" if $flag->requestee;
        push(@summaries, $summary);
    }
    return @summaries;
}


=pod

=over

=item C<process($bug, $attachment, $timestamp, $cgi)>

Processes changes to flags.

The bug and/or the attachment objects are the ones this flag is about,
the timestamp is the date/time the bug was last touched (so that changes
to the flag can be stamped with the same date/time), the cgi is the CGI
object used to obtain the flag fields that the user submitted.

=back

=cut

sub process {
    my ($bug, $attachment, $timestamp, $cgi) = @_;
    my $dbh = Bugzilla->dbh;

    # Make sure the bug (and attachment, if given) exists and is accessible
    # to the current user. Moreover, if an attachment object is passed,
    # make sure it belongs to the given bug.
    return if ($bug->error || ($attachment && $bug->bug_id != $attachment->bug_id));

    my $bug_id = $bug->bug_id;
    my $attach_id = $attachment ? $attachment->id : undef;

    # Use the date/time we were given if possible (allowing calling code
    # to synchronize the comment's timestamp with those of other records).
    $timestamp ||= $dbh->selectrow_array('SELECT NOW()');

    # Take a snapshot of flags before any changes.
    my @old_summaries = snapshot($bug_id, $attach_id);

    # Cancel pending requests if we are obsoleting an attachment.
    if ($attachment && $cgi->param('isobsolete')) {
        CancelRequests($bug, $attachment);
    }

    # Create new flags and update existing flags.
    my $new_flags = FormToNewFlags($bug, $attachment, $cgi);
    foreach my $flag (@$new_flags) { create($flag, $bug, $attachment, $timestamp) }
    modify($bug, $attachment, $cgi, $timestamp);

    # In case the bug's product/component has changed, clear flags that are
    # no longer valid.
    my $flag_ids = $dbh->selectcol_arrayref(
        "SELECT DISTINCT flags.id
           FROM flags
     INNER JOIN bugs
             ON flags.bug_id = bugs.bug_id
      LEFT JOIN flaginclusions AS i
             ON flags.type_id = i.type_id 
            AND (bugs.product_id = i.product_id OR i.product_id IS NULL)
            AND (bugs.component_id = i.component_id OR i.component_id IS NULL)
          WHERE bugs.bug_id = ?
            AND i.type_id IS NULL",
        undef, $bug_id);

    my $flags = Bugzilla::Flag->new_from_list($flag_ids);
    foreach my $flag (@$flags) {
        my $is_retargetted = retarget($flag, $bug);
        clear($flag, $bug, $flag->attachment) unless $is_retargetted;
    }

    $flag_ids = $dbh->selectcol_arrayref(
        "SELECT DISTINCT flags.id
        FROM flags, bugs, flagexclusions e
        WHERE bugs.bug_id = ?
        AND flags.bug_id = bugs.bug_id
        AND flags.type_id = e.type_id
        AND (bugs.product_id = e.product_id OR e.product_id IS NULL)
        AND (bugs.component_id = e.component_id OR e.component_id IS NULL)",
        undef, $bug_id);

    $flags = Bugzilla::Flag->new_from_list($flag_ids);
    foreach my $flag (@$flags) {
        my $is_retargetted = retarget($flag, $bug);
        clear($flag, $bug, $flag->attachment) unless $is_retargetted;
    }

    # Take a snapshot of flags after changes.
    my @new_summaries = snapshot($bug_id, $attach_id);

    update_activity($bug_id, $attach_id, $timestamp, \@old_summaries, \@new_summaries);
}

sub update_activity {
    my ($bug_id, $attach_id, $timestamp, $old_summaries, $new_summaries) = @_;
    my $dbh = Bugzilla->dbh;

    $old_summaries = join(", ", @$old_summaries);
    $new_summaries = join(", ", @$new_summaries);
    my ($removed, $added) = diff_strings($old_summaries, $new_summaries);
    if ($removed ne $added) {
        my $field_id = get_field_id('flagtypes.name');
        $dbh->do('INSERT INTO bugs_activity
                  (bug_id, attach_id, who, bug_when, fieldid, removed, added)
                  VALUES (?, ?, ?, ?, ?, ?, ?)',
                  undef, ($bug_id, $attach_id, Bugzilla->user->id,
                  $timestamp, $field_id, $removed, $added));

        $dbh->do('UPDATE bugs SET delta_ts = ? WHERE bug_id = ?',
                  undef, ($timestamp, $bug_id));
    }
}

=pod

=over

=item C<create($flag, $bug, $attachment, $timestamp)>

Creates a flag record in the database.

=back

=cut

sub create {
    my ($flag, $bug, $attachment, $timestamp) = @_;
    my $dbh = Bugzilla->dbh;

    my $attach_id = $attachment ? $attachment->id : undef;
    my $requestee_id;
    # Be careful! At this point, $flag is *NOT* yet an object!
    $requestee_id = $flag->{'requestee'}->id if $flag->{'requestee'};

    $dbh->do('INSERT INTO flags (type_id, bug_id, attach_id, requestee_id,
                                 setter_id, status, creation_date, modification_date)
              VALUES (?, ?, ?, ?, ?, ?, ?, ?)',
              undef, ($flag->{'type'}->id, $bug->bug_id,
                      $attach_id, $requestee_id, $flag->{'setter'}->id,
                      $flag->{'status'}, $timestamp, $timestamp));

    # Now that the new flag has been added to the DB, create a real flag object.
    # This is required to call notify() correctly.
    my $flag_id = $dbh->bz_last_key('flags', 'id');
    $flag = new Bugzilla::Flag($flag_id);

    # Send an email notifying the relevant parties about the flag creation.
    if ($flag->requestee && $flag->requestee->wants_mail([EVT_FLAG_REQUESTED])) {
        $flag->{'addressee'} = $flag->requestee;
    }

    notify($flag, $bug, $attachment);

    # Return the new flag object.
    return $flag;
}

=pod

=over

=item C<modify($bug, $attachment, $cgi, $timestamp)>

Modifies flags in the database when a user changes them.

=back

=cut

sub modify {
    my ($bug, $attachment, $cgi, $timestamp) = @_;
    my $setter = Bugzilla->user;
    my $dbh = Bugzilla->dbh;

    # Extract a list of flags from the form data.
    my @ids = map(/^flag-(\d+)$/ ? $1 : (), $cgi->param());

    # Loop over flags and update their record in the database if necessary.
    # Two kinds of changes can happen to a flag: it can be set to a different
    # state, and someone else can be asked to set it.  We take care of both
    # those changes.
    my @flags;
    foreach my $id (@ids) {
        my $flag = new Bugzilla::Flag($id);
        # If the flag no longer exists, ignore it.
        next unless $flag;

        my $status = $cgi->param("flag-$id");

        # If the user entered more than one name into the requestee field
        # (i.e. they want more than one person to set the flag) we can reuse
        # the existing flag for the first person (who may well be the existing
        # requestee), but we have to create new flags for each additional.
        my @requestees = $cgi->param("requestee-$id");
        my $requestee_email;
        if ($status eq "?"
            && scalar(@requestees) > 1
            && $flag->type->is_multiplicable)
        {
            # The first person, for which we'll reuse the existing flag.
            $requestee_email = shift(@requestees);

            # Create new flags like the existing one for each additional person.
            foreach my $login (@requestees) {
                create({ type      => $flag->type,
                         setter    => $setter, 
                         status    => "?",
                         requestee => new Bugzilla::User({ name => $login }) },
                       $bug, $attachment, $timestamp);
            }
        }
        else {
            $requestee_email = trim($cgi->param("requestee-$id") || '');
        }

        # Ignore flags the user didn't change. There are two components here:
        # either the status changes (trivial) or the requestee changes.
        # Change of either field will cause full update of the flag.

        my $status_changed = ($status ne $flag->status);

        # Requestee is considered changed, if all of the following apply:
        # 1. Flag status is '?' (requested)
        # 2. Flag can have a requestee
        # 3. The requestee specified on the form is different from the 
        #    requestee specified in the db.

        my $old_requestee = $flag->requestee ? $flag->requestee->login : '';

        my $requestee_changed = 
          ($status eq "?" && 
           $flag->type->is_requesteeble &&
           $old_requestee ne $requestee_email);

        next unless ($status_changed || $requestee_changed);

        # Since the status is validated, we know it's safe, but it's still
        # tainted, so we have to detaint it before using it in a query.
        trick_taint($status);

        if ($status eq '+' || $status eq '-') {
            $dbh->do('UPDATE flags
                         SET setter_id = ?, requestee_id = NULL,
                             status = ?, modification_date = ?
                       WHERE id = ?',
                       undef, ($setter->id, $status, $timestamp, $flag->id));

            # If the status of the flag was "?", we have to notify
            # the requester (if he wants to).
            my $requester;
            if ($flag->status eq '?') {
                $requester = $flag->setter;
                $flag->{'requester'} = $requester;
            }
            # Now update the flag object with its new values.
            $flag->{'setter'} = $setter;
            $flag->{'requestee'} = undef;
            $flag->{'requestee_id'} = undef;
            $flag->{'status'} = $status;

            # Send an email notifying the relevant parties about the fulfillment,
            # including the requester.
            if ($requester && $requester->wants_mail([EVT_REQUESTED_FLAG])) {
                $flag->{'addressee'} = $requester;
            }

            notify($flag, $bug, $attachment);
        }
        elsif ($status eq '?') {
            # Get the requestee, if any.
            my $requestee_id;
            if ($requestee_email) {
                $requestee_id = login_to_id($requestee_email);
                $flag->{'requestee'} = new Bugzilla::User($requestee_id);
                $flag->{'requestee_id'} = $requestee_id;
            }
            else {
                # If the status didn't change but we only removed the
                # requestee, we have to clear the requestee field.
                $flag->{'requestee'} = undef;
                $flag->{'requestee_id'} = undef;
            }

            # Update the database with the changes.
            $dbh->do('UPDATE flags
                         SET setter_id = ?, requestee_id = ?,
                             status = ?, modification_date = ?
                       WHERE id = ?',
                       undef, ($setter->id, $requestee_id, $status,
                               $timestamp, $flag->id));

            # Now update the flag object with its new values.
            $flag->{'setter'} = $setter;
            $flag->{'status'} = $status;

            # Send an email notifying the relevant parties about the request.
            if ($flag->requestee && $flag->requestee->wants_mail([EVT_FLAG_REQUESTED])) {
                $flag->{'addressee'} = $flag->requestee;
            }

            notify($flag, $bug, $attachment);
        }
        elsif ($status eq 'X') {
            clear($flag, $bug, $attachment);
        }

        push(@flags, $flag);
    }

    return \@flags;
}

=pod

=over

=item C<retarget($flag, $bug)>

Change the type of the flag, if possible. The new flag type must have
the same name as the current flag type, must exist in the product and
component the bug is in, and the current settings of the flag must pass
validation. If no such flag type can be found, the type remains unchanged.

Retargetting flags is a good way to keep flags when moving bugs from one
product where a flag type is available to another product where the flag
type is unavailable, but another flag type having the same name exists.
Most of the time, if they have the same name, this means that they have
the same meaning, but with different settings.

=back

=cut

sub retarget {
    my ($flag, $bug) = @_;
    my $dbh = Bugzilla->dbh;

    # We are looking for flagtypes having the same name as the flagtype
    # to which the current flag belongs, and being in the new product and
    # component of the bug.
    my $flagtypes = Bugzilla::FlagType::match(
                        {'name'         => $flag->name,
                         'target_type'  => $flag->type->target_type,
                         'is_active'    => 1,
                         'product_id'   => $bug->product_id,
                         'component_id' => $bug->component_id});

    # If we found no flagtype, the flag will be deleted.
    return 0 unless scalar(@$flagtypes);

    # If we found at least one, change the type of the flag,
    # assuming the setter/requester is allowed to set/request flags
    # belonging to this flagtype.
    my $requestee = $flag->requestee ? [$flag->requestee->login] : [];
    my $is_private = ($flag->attachment) ? $flag->attachment->isprivate : 0;
    my $is_retargetted = 0;

    foreach my $flagtype (@$flagtypes) {
        # Get the number of flags of this type already set for this target.
        my $has_flags = count(
            { 'type_id'     => $flagtype->id,
              'bug_id'      => $bug->bug_id,
              'attach_id'   => $flag->attach_id });

        # Do not create a new flag of this type if this flag type is
        # not multiplicable and already has a flag set.
        next if (!$flagtype->is_multiplicable && $has_flags);

        # Check user privileges.
        my $error_mode_cache = Bugzilla->error_mode;
        Bugzilla->error_mode(ERROR_MODE_DIE);
        eval {
            _validate(undef, $flagtype, $flag->status, $flag->setter,
                      $requestee, $is_private, $bug->bug_id, $flag->attach_id);
        };
        Bugzilla->error_mode($error_mode_cache);
        # If the validation failed, then we cannot use this flagtype.
        next if ($@);

        # Checks are successful, we can retarget the flag to this flagtype.
        $dbh->do('UPDATE flags SET type_id = ? WHERE id = ?',
                  undef, ($flagtype->id, $flag->id));

        $is_retargetted = 1;
        last;
    }
    return $is_retargetted;
}

=pod

=over

=item C<clear($flag, $bug, $attachment)>

Remove a flag from the DB.

=back

=cut

sub clear {
    my ($flag, $bug, $attachment) = @_;
    my $dbh = Bugzilla->dbh;

    $dbh->do('DELETE FROM flags WHERE id = ?', undef, $flag->id);

    # If we cancel a pending request, we have to notify the requester
    # (if he wants to).
    my $requester;
    if ($flag->status eq '?') {
        $requester = $flag->setter;
        $flag->{'requester'} = $requester;
    }

    # Now update the flag object to its new values. The last
    # requester/setter and requestee are kept untouched (for the
    # record). Else we could as well delete the flag completely.
    $flag->{'exists'} = 0;    
    $flag->{'status'} = "X";

    if ($requester && $requester->wants_mail([EVT_REQUESTED_FLAG])) {
        $flag->{'addressee'} = $requester;
    }

    notify($flag, $bug, $attachment);
}


######################################################################
# Utility Functions
######################################################################

=pod

=over

=item C<FormToNewFlags($bug, $attachment, $cgi)>

Checks whether or not there are new flags to create and returns an
array of flag objects. This array is then passed to Flag::create().

=back

=cut

sub FormToNewFlags {
    my ($bug, $attachment, $cgi) = @_;
    my $dbh = Bugzilla->dbh;
    my $setter = Bugzilla->user;
    
    # Extract a list of flag type IDs from field names.
    my @type_ids = map(/^flag_type-(\d+)$/ ? $1 : (), $cgi->param());
    @type_ids = grep($cgi->param("flag_type-$_") ne 'X', @type_ids);

    return () unless scalar(@type_ids);

    # Get a list of active flag types available for this target.
    my $flag_types = Bugzilla::FlagType::match(
        { 'target_type'  => $attachment ? 'attachment' : 'bug',
          'product_id'   => $bug->{'product_id'},
          'component_id' => $bug->{'component_id'},
          'is_active'    => 1 });

    my @flags;
    foreach my $flag_type (@$flag_types) {
        my $type_id = $flag_type->id;

        # We are only interested in flags the user tries to create.
        next unless scalar(grep { $_ == $type_id } @type_ids);

        # Get the number of flags of this type already set for this target.
        my $has_flags = count(
            { 'type_id'     => $type_id,
              'target_type' => $attachment ? 'attachment' : 'bug',
              'bug_id'      => $bug->bug_id,
              'attach_id'   => $attachment ? $attachment->id : undef });

        # Do not create a new flag of this type if this flag type is
        # not multiplicable and already has a flag set.
        next if (!$flag_type->is_multiplicable && $has_flags);

        my $status = $cgi->param("flag_type-$type_id");
        trick_taint($status);

        my @logins = $cgi->param("requestee_type-$type_id");
        if ($status eq "?" && scalar(@logins) > 0) {
            foreach my $login (@logins) {
                push (@flags, { type      => $flag_type ,
                                setter    => $setter , 
                                status    => $status ,
                                requestee => 
                                    new Bugzilla::User({ name => $login }) });
                last unless $flag_type->is_multiplicable;
            }
        }
        else {
            push (@flags, { type   => $flag_type ,
                            setter => $setter , 
                            status => $status });
        }
    }

    # Return the list of flags.
    return \@flags;
}

=pod

=over

=item C<notify($flag, $bug, $attachment)>

Sends an email notification about a flag being created, fulfilled
or deleted.

=back

=cut

sub notify {
    my ($flag, $bug, $attachment) = @_;

    # There is nobody to notify.
    return unless ($flag->{'addressee'} || $flag->type->cc_list);

    # If the target bug is restricted to one or more groups, then we need
    # to make sure we don't send email about it to unauthorized users
    # on the request type's CC: list, so we have to trawl the list for users
    # not in those groups or email addresses that don't have an account.
    my @bug_in_groups = grep {$_->{'ison'} || $_->{'mandatory'}} @{$bug->groups};
    my $attachment_is_private = $attachment ? $attachment->isprivate : undef;

    my %recipients;
    foreach my $cc (split(/[, ]+/, $flag->type->cc_list)) {
        my $ccuser = new Bugzilla::User({ name => $cc });
        next if (scalar(@bug_in_groups) && (!$ccuser || !$ccuser->can_see_bug($bug->bug_id)));
        next if $attachment_is_private && (!$ccuser || !$ccuser->is_insider);
        # Prevent duplicated entries due to case sensitivity.
        $cc = $ccuser ? $ccuser->email : $cc;
        $recipients{$cc} = $ccuser;
    }

    # Only notify if the addressee is allowed to receive the email.
    if ($flag->{'addressee'} && $flag->{'addressee'}->email_enabled) {
        $recipients{$flag->{'addressee'}->email} = $flag->{'addressee'};
    }
    # Process and send notification for each recipient.
    # If there are users in the CC list who don't have an account,
    # use the default language for email notifications.
    my $default_lang;
    if (grep { !$_ } values %recipients) {
        my $default_user = new Bugzilla::User();
        $default_lang = $default_user->settings->{'lang'}->{'value'};
    }

    foreach my $to (keys %recipients) {
        my $vars = { 'flag'       => $flag,
                     'to'         => $to,
                     'bug'        => $bug,
                     'attachment' => $attachment};

        my $lang = $recipients{$to} ?
          $recipients{$to}->settings->{'lang'}->{'value'} : $default_lang;

        my $template = Bugzilla->template_inner($lang);
        my $message;
        $template->process("request/email.txt.tmpl", $vars, \$message)
          || ThrowTemplateError($template->error());

        Bugzilla->template_inner("");
        MessageToMTA($message);
    }
}

# Cancel all request flags from the attachment being obsoleted.
sub CancelRequests {
    my ($bug, $attachment, $timestamp) = @_;
    my $dbh = Bugzilla->dbh;

    my $request_ids =
        $dbh->selectcol_arrayref("SELECT flags.id
                                  FROM flags
                                  LEFT JOIN attachments ON flags.attach_id = attachments.attach_id
                                  WHERE flags.attach_id = ?
                                  AND flags.status = '?'
                                  AND attachments.isobsolete = 0",
                                  undef, $attachment->id);

    return if (!scalar(@$request_ids));

    # Take a snapshot of flags before any changes.
    my @old_summaries = snapshot($bug->bug_id, $attachment->id) if ($timestamp);
    my $flags = Bugzilla::Flag->new_from_list($request_ids);
    foreach my $flag (@$flags) { clear($flag, $bug, $attachment) }

    # If $timestamp is undefined, do not update the activity table
    return unless ($timestamp);

    # Take a snapshot of flags after any changes.
    my @new_summaries = snapshot($bug->bug_id, $attachment->id);
    update_activity($bug->bug_id, $attachment->id, $timestamp,
                    \@old_summaries, \@new_summaries);
}

######################################################################
# Private Functions
######################################################################

=begin private

=head1 PRIVATE FUNCTIONS

=over

=item C<sqlify_criteria($criteria)>

Converts a hash of criteria into a list of SQL criteria.

=back

=cut

sub sqlify_criteria {
    # a reference to a hash containing the criteria (field => value)
    my ($criteria) = @_;

    # the generated list of SQL criteria; "1=1" is a clever way of making sure
    # there's something in the list so calling code doesn't have to check list
    # size before building a WHERE clause out of it
    my @criteria = ("1=1");
    
    # If the caller specified only bug or attachment flags,
    # limit the query to those kinds of flags.
    if (defined($criteria->{'target_type'})) {
        if    ($criteria->{'target_type'} eq 'bug')        { push(@criteria, "attach_id IS NULL") }
        elsif ($criteria->{'target_type'} eq 'attachment') { push(@criteria, "attach_id IS NOT NULL") }
    }
    
    # Go through each criterion from the calling code and add it to the query.
    foreach my $field (keys %$criteria) {
        my $value = $criteria->{$field};
        next unless defined($value);
        if    ($field eq 'type_id')      { push(@criteria, "type_id      = $value") }
        elsif ($field eq 'bug_id')       { push(@criteria, "bug_id       = $value") }
        elsif ($field eq 'attach_id')    { push(@criteria, "attach_id    = $value") }
        elsif ($field eq 'requestee_id') { push(@criteria, "requestee_id = $value") }
        elsif ($field eq 'setter_id')    { push(@criteria, "setter_id    = $value") }
        elsif ($field eq 'status')       { push(@criteria, "status       = '$value'") }
    }
    
    return @criteria;
}

=head1 SEE ALSO

=over

=item B<Bugzilla::FlagType>

=back


=head1 CONTRIBUTORS

=over

=item Myk Melez <myk@mozilla.org>

=item Jouni Heikniemi <jouni@heikniemi.net>

=item Kevin Benton <kevin.benton@amd.com>

=item Frédéric Buclin <LpSolit@gmail.com>

=back

=cut

1;
