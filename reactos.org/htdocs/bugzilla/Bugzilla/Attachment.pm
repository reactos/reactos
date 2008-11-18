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
#                 Myk Melez <myk@mozilla.org>
#                 Marc Schumann <wurblzap@gmail.com>
#                 Frédéric Buclin <LpSolit@gmail.com>

use strict;

package Bugzilla::Attachment;

=head1 NAME

Bugzilla::Attachment - a file related to a bug that a user has uploaded
                       to the Bugzilla server

=head1 SYNOPSIS

  use Bugzilla::Attachment;

  # Get the attachment with the given ID.
  my $attachment = Bugzilla::Attachment->get($attach_id);

  # Get the attachments with the given IDs.
  my $attachments = Bugzilla::Attachment->get_list($attach_ids);

=head1 DESCRIPTION

This module defines attachment objects, which represent files related to bugs
that users upload to the Bugzilla server.

=cut

use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Flag;
use Bugzilla::User;
use Bugzilla::Util;
use Bugzilla::Field;

sub get {
    my $invocant = shift;
    my $id = shift;

    my $attachments = _retrieve([$id]);
    my $self = $attachments->[0];
    bless($self, ref($invocant) || $invocant) if $self;

    return $self;
}

sub get_list {
    my $invocant = shift;
    my $ids = shift;

    my $attachments = _retrieve($ids);
    foreach my $attachment (@$attachments) {
        bless($attachment, ref($invocant) || $invocant);
    }

    return $attachments;
}

sub _retrieve {
    my ($ids) = @_;

    return [] if scalar(@$ids) == 0;

    my @columns = (
        'attachments.attach_id AS id',
        'attachments.bug_id AS bug_id',
        'attachments.description AS description',
        'attachments.mimetype AS contenttype',
        'attachments.submitter_id AS _attacher_id',
        Bugzilla->dbh->sql_date_format('attachments.creation_ts',
                                       '%Y.%m.%d %H:%i') . " AS attached",
        'attachments.filename AS filename',
        'attachments.ispatch AS ispatch',
        'attachments.isurl AS isurl',
        'attachments.isobsolete AS isobsolete',
        'attachments.isprivate AS isprivate'
    );
    my $columns = join(", ", @columns);

    my $records = Bugzilla->dbh->selectall_arrayref("SELECT $columns
                                                     FROM attachments
                                                     WHERE attach_id IN (" .
                                                     join(",", @$ids) . ")
                                                     ORDER BY attach_id",
                                                    { Slice => {} });
    return $records;
}

=pod

=head2 Instance Properties

=over

=item C<id>

the unique identifier for the attachment

=back

=cut

sub id {
    my $self = shift;
    return $self->{id};
}

=over

=item C<bug_id>

the ID of the bug to which the attachment is attached

=back

=cut

# XXX Once Bug.pm slims down sufficiently this should become a reference
# to a bug object.
sub bug_id {
    my $self = shift;
    return $self->{bug_id};
}

=over

=item C<description>

user-provided text describing the attachment

=back

=cut

sub description {
    my $self = shift;
    return $self->{description};
}

=over

=item C<contenttype>

the attachment's MIME media type

=back

=cut

sub contenttype {
    my $self = shift;
    return $self->{contenttype};
}

=over

=item C<attacher>

the user who attached the attachment

=back

=cut

sub attacher {
    my $self = shift;
    return $self->{attacher} if exists $self->{attacher};
    $self->{attacher} = new Bugzilla::User($self->{_attacher_id});
    return $self->{attacher};
}

=over

=item C<attached>

the date and time on which the attacher attached the attachment

=back

=cut

sub attached {
    my $self = shift;
    return $self->{attached};
}

=over

=item C<filename>

the name of the file the attacher attached

=back

=cut

sub filename {
    my $self = shift;
    return $self->{filename};
}

=over

=item C<ispatch>

whether or not the attachment is a patch

=back

=cut

sub ispatch {
    my $self = shift;
    return $self->{ispatch};
}

=over

=item C<isurl>

whether or not the attachment is a URL

=back

=cut

sub isurl {
    my $self = shift;
    return $self->{isurl};
}

=over

=item C<isobsolete>

whether or not the attachment is obsolete

=back

=cut

sub isobsolete {
    my $self = shift;
    return $self->{isobsolete};
}

=over

=item C<isprivate>

whether or not the attachment is private

=back

=cut

sub isprivate {
    my $self = shift;
    return $self->{isprivate};
}

=over

=item C<data>

the content of the attachment

=back

=cut

sub data {
    my $self = shift;
    return $self->{data} if exists $self->{data};

    # First try to get the attachment data from the database.
    ($self->{data}) = Bugzilla->dbh->selectrow_array("SELECT thedata
                                                      FROM attach_data
                                                      WHERE id = ?",
                                                     undef,
                                                     $self->{id});

    # If there's no attachment data in the database, the attachment is stored
    # in a local file, so retrieve it from there.
    if (length($self->{data}) == 0) {
        if (open(AH, $self->_get_local_filename())) {
            local $/;
            binmode AH;
            $self->{data} = <AH>;
            close(AH);
        }
    }

    return $self->{data};
}

=over

=item C<datasize>

the length (in characters) of the attachment content

=back

=cut

# datasize is a property of the data itself, and it's unclear whether we should
# expose it at all, since you can easily derive it from the data itself: in TT,
# attachment.data.size; in Perl, length($attachment->{data}).  But perhaps
# it makes sense for performance reasons, since accessing the data forces it
# to get retrieved from the database/filesystem and loaded into memory,
# while datasize avoids loading the attachment into memory, calling SQL's
# LENGTH() function or stat()ing the file instead.  I've left it in for now.

sub datasize {
    my $self = shift;
    return $self->{datasize} if exists $self->{datasize};

    # If we have already retrieved the data, return its size.
    return length($self->{data}) if exists $self->{data};

    $self->{datasize} =
        Bugzilla->dbh->selectrow_array("SELECT LENGTH(thedata)
                                        FROM attach_data
                                        WHERE id = ?",
                                       undef, $self->{id}) || 0;

    # If there's no attachment data in the database, either the attachment
    # is stored in a local file, and so retrieve its size from the file,
    # or the attachment has been deleted.
    unless ($self->{datasize}) {
        if (open(AH, $self->_get_local_filename())) {
            binmode AH;
            $self->{datasize} = (stat(AH))[7];
            close(AH);
        }
    }

    return $self->{datasize};
}

=over

=item C<flags>

flags that have been set on the attachment

=back

=cut

sub flags {
    my $self = shift;
    return $self->{flags} if exists $self->{flags};

    $self->{flags} = Bugzilla::Flag::match({ 'attach_id' => $self->id });
    return $self->{flags};
}

# Instance methods; no POD documentation here yet because the only ones so far
# are private.

sub _get_local_filename {
    my $self = shift;
    my $hash = ($self->id % 100) + 100;
    $hash =~ s/.*(\d\d)$/group.$1/;
    return bz_locations()->{'attachdir'} . "/$hash/attachment." . $self->id;
}

sub _validate_filename {
    my ($throw_error) = @_;
    my $cgi = Bugzilla->cgi;
    defined $cgi->upload('data')
        || ($throw_error ? ThrowUserError("file_not_specified") : return 0);

    my $filename = $cgi->upload('data');

    # Remove path info (if any) from the file name.  The browser should do this
    # for us, but some are buggy.  This may not work on Mac file names and could
    # mess up file names with slashes in them, but them's the breaks.  We only
    # use this as a hint to users downloading attachments anyway, so it's not
    # a big deal if it munges incorrectly occasionally.
    $filename =~ s/^.*[\/\\]//;

    # Truncate the filename to 100 characters, counting from the end of the
    # string to make sure we keep the filename extension.
    $filename = substr($filename, -100, 100);

    return $filename;
}

sub _validate_data {
    my ($throw_error, $hr_vars) = @_;
    my $cgi = Bugzilla->cgi;
    my $maxsize = $cgi->param('ispatch') ? Bugzilla->params->{'maxpatchsize'} 
                  : Bugzilla->params->{'maxattachmentsize'};
    $maxsize *= 1024; # Convert from K
    my $fh;
    # Skip uploading into a local variable if the user wants to upload huge
    # attachments into local files.
    if (!$cgi->param('bigfile')) {
        $fh = $cgi->upload('data');
    }
    my $data;

    # We could get away with reading only as much as required, except that then
    # we wouldn't have a size to print to the error handler below.
    if (!$cgi->param('bigfile')) {
        # enable 'slurp' mode
        local $/;
        $data = <$fh>;
    }

    $data
        || ($cgi->param('bigfile'))
        || ($throw_error ? ThrowUserError("zero_length_file") : return 0);

    # Windows screenshots are usually uncompressed BMP files which
    # makes for a quick way to eat up disk space. Let's compress them.
    # We do this before we check the size since the uncompressed version
    # could easily be greater than maxattachmentsize.
    if (Bugzilla->params->{'convert_uncompressed_images'}
        && $cgi->param('contenttype') eq 'image/bmp') {
        require Image::Magick;
        my $img = Image::Magick->new(magick=>'bmp');
        $img->BlobToImage($data);
        $img->set(magick=>'png');
        my $imgdata = $img->ImageToBlob();
        $data = $imgdata;
        $cgi->param('contenttype', 'image/png');
        $$hr_vars->{'convertedbmp'} = 1;
    }

    # Make sure the attachment does not exceed the maximum permitted size
    my $len = $data ? length($data) : 0;
    if ($maxsize && $len > $maxsize) {
        my $vars = { filesize => sprintf("%.0f", $len/1024) };
        if ($cgi->param('ispatch')) {
            $throw_error ? ThrowUserError("patch_too_large", $vars) : return 0;
        }
        else {
            $throw_error ? ThrowUserError("file_too_large", $vars) : return 0;
        }
    }

    return $data || '';
}

=pod

=head2 Class Methods

=over

=item C<get_attachments_by_bug($bug_id)>

Description: retrieves and returns the attachments the currently logged in
             user can view for the given bug.

Params:     C<$bug_id> - integer - the ID of the bug for which
            to retrieve and return attachments.

Returns:    a reference to an array of attachment objects.

=cut

sub get_attachments_by_bug {
    my ($class, $bug_id) = @_;
    my $user = Bugzilla->user;
    my $dbh = Bugzilla->dbh;

    # By default, private attachments are not accessible, unless the user
    # is in the insider group or submitted the attachment.
    my $and_restriction = '';
    my @values = ($bug_id);

    unless ($user->is_insider) {
        $and_restriction = 'AND (isprivate = 0 OR submitter_id = ?)';
        push(@values, $user->id);
    }

    my $attach_ids = $dbh->selectcol_arrayref("SELECT attach_id FROM attachments
                                               WHERE bug_id = ? $and_restriction",
                                               undef, @values);
    my $attachments = Bugzilla::Attachment->get_list($attach_ids);
    return $attachments;
}

=pod

=item C<validate_is_patch()>

Description: validates the "patch" flag passed in by CGI.

Returns:    1 on success.

=cut

sub validate_is_patch {
    my ($class, $throw_error) = @_;
    my $cgi = Bugzilla->cgi;

    # Set the ispatch flag to zero if it is undefined, since the UI uses
    # an HTML checkbox to represent this flag, and unchecked HTML checkboxes
    # do not get sent in HTML requests.
    $cgi->param('ispatch', $cgi->param('ispatch') ? 1 : 0);

    # Set the content type to text/plain if the attachment is a patch.
    $cgi->param('contenttype', 'text/plain') if $cgi->param('ispatch');

    return 1;
}

=pod

=item C<validate_description()>

Description: validates the description passed in by CGI.

Returns:    1 on success.

=cut

sub validate_description {
    my ($class, $throw_error) = @_;
    my $cgi = Bugzilla->cgi;

    $cgi->param('description')
        || ($throw_error ? ThrowUserError("missing_attachment_description") : return 0);

    return 1;
}

=pod

=item C<validate_content_type()>

Description: validates the content type passed in by CGI.

Returns:    1 on success.

=cut

sub validate_content_type {
    my ($class, $throw_error) = @_;
    my $cgi = Bugzilla->cgi;

    if (!defined $cgi->param('contenttypemethod')) {
        $throw_error ? ThrowUserError("missing_content_type_method") : return 0;
    }
    elsif ($cgi->param('contenttypemethod') eq 'autodetect') {
        my $contenttype =
            $cgi->uploadInfo($cgi->param('data'))->{'Content-Type'};
        # The user asked us to auto-detect the content type, so use the type
        # specified in the HTTP request headers.
        if ( !$contenttype ) {
            $throw_error ? ThrowUserError("missing_content_type") : return 0;
        }
        $cgi->param('contenttype', $contenttype);
    }
    elsif ($cgi->param('contenttypemethod') eq 'list') {
        # The user selected a content type from the list, so use their
        # selection.
        $cgi->param('contenttype', $cgi->param('contenttypeselection'));
    }
    elsif ($cgi->param('contenttypemethod') eq 'manual') {
        # The user entered a content type manually, so use their entry.
        $cgi->param('contenttype', $cgi->param('contenttypeentry'));
    }
    else {
        $throw_error ?
            ThrowCodeError("illegal_content_type_method",
                           { contenttypemethod => $cgi->param('contenttypemethod') }) :
            return 0;
    }

    if ( $cgi->param('contenttype') !~
           /^(application|audio|image|message|model|multipart|text|video)\/.+$/ ) {
        $throw_error ?
            ThrowUserError("invalid_content_type",
                           { contenttype => $cgi->param('contenttype') }) :
            return 0;
    }

    return 1;
}

=pod

=item C<validate_can_edit($attachment, $product_id)>

Description: validates if the user is allowed to view and edit the attachment.
             Only the submitter or someone with editbugs privs can edit it.
             Only the submitter and users in the insider group can view
             private attachments.

Params:      $attachment - the attachment object being edited.
             $product_id - the product ID the attachment belongs to.

Returns:     1 on success. Else an error is thrown.

=cut

sub validate_can_edit {
    my ($attachment, $product_id) = @_;
    my $dbh = Bugzilla->dbh;
    my $user = Bugzilla->user;

    # Bug 97729 - the submitter can edit their attachments.
    return if ($attachment->attacher->id == $user->id);

    # Only users in the insider group can view private attachments.
    if ($attachment->isprivate && !$user->is_insider) {
        ThrowUserError('illegal_attachment_edit', {attach_id => $attachment->id});
    }

    # Users with editbugs privs can edit all attachments.
    return if $user->in_group('editbugs', $product_id);

    # If we come here, then this attachment cannot be seen by the user.
    ThrowUserError('illegal_attachment_edit', { attach_id => $attachment->id });
}

=item C<validate_obsolete($bug)>

Description: validates if attachments the user wants to mark as obsolete
             really belong to the given bug and are not already obsolete.
             Moreover, a user cannot mark an attachment as obsolete if
             he cannot view it (due to restrictions on it).

Params:      $bug - The bug object obsolete attachments should belong to.

Returns:     1 on success. Else an error is thrown.

=cut

sub validate_obsolete {
    my ($class, $bug) = @_;
    my $cgi = Bugzilla->cgi;

    # Make sure the attachment id is valid and the user has permissions to view
    # the bug to which it is attached. Make sure also that the user can view
    # the attachment itself.
    my @obsolete_attachments;
    foreach my $attachid ($cgi->param('obsolete')) {
        my $vars = {};
        $vars->{'attach_id'} = $attachid;

        detaint_natural($attachid)
          || ThrowCodeError('invalid_attach_id_to_obsolete', $vars);

        my $attachment = Bugzilla::Attachment->get($attachid);

        # Make sure the attachment exists in the database.
        ThrowUserError('invalid_attach_id', $vars) unless $attachment;

        # Check that the user can view and edit this attachment.
        $attachment->validate_can_edit($bug->product_id);

        $vars->{'description'} = $attachment->description;

        if ($attachment->bug_id != $bug->bug_id) {
            $vars->{'my_bug_id'} = $bug->bug_id;
            $vars->{'attach_bug_id'} = $attachment->bug_id;
            ThrowCodeError('mismatched_bug_ids_on_obsolete', $vars);
        }

        if ($attachment->isobsolete) {
          ThrowCodeError('attachment_already_obsolete', $vars);
        }

        push(@obsolete_attachments, $attachment);
    }
    return @obsolete_attachments;
}


=pod

=item C<insert_attachment_for_bug($throw_error, $bug, $user, $timestamp, $hr_vars)>

Description: inserts an attachment from CGI input for the given bug.

Params:     C<$bug> - Bugzilla::Bug object - the bug for which to insert
            the attachment.
            C<$user> - Bugzilla::User object - the user we're inserting an
            attachment for.
            C<$timestamp> - scalar - timestamp of the insert as returned
            by SELECT NOW().
            C<$hr_vars> - hash reference - reference to a hash of template
            variables.

Returns:    the ID of the new attachment.

=back

=cut

sub insert_attachment_for_bug {
    my ($class, $throw_error, $bug, $user, $timestamp, $hr_vars) = @_;

    my $cgi = Bugzilla->cgi;
    my $dbh = Bugzilla->dbh;
    my $attachurl = $cgi->param('attachurl') || '';
    my $data;
    my $filename;
    my $contenttype;
    my $isurl;
    $class->validate_is_patch($throw_error) || return 0;
    $class->validate_description($throw_error) || return 0;

    if (Bugzilla->params->{'allow_attach_url'}
        && ($attachurl =~ /^(http|https|ftp):\/\/\S+/)
        && !defined $cgi->upload('data'))
    {
        $filename = '';
        $data = $attachurl;
        $isurl = 1;
        $contenttype = 'text/plain';
        $cgi->param('ispatch', 0);
        $cgi->delete('bigfile');
    }
    else {
        $filename = _validate_filename($throw_error) || return 0;
        # need to validate content type before data as
        # we now check the content type for image/bmp in _validate_data()
        unless ($cgi->param('ispatch')) {
            $class->validate_content_type($throw_error) || return 0;

            # Set the ispatch flag to 1 if we're set to autodetect
            # and the content type is text/x-diff or text/x-patch
            if ($cgi->param('contenttypemethod') eq 'autodetect'
                && $cgi->param('contenttype') =~ m{text/x-(?:diff|patch)})
            {
                $cgi->param('ispatch', 1);
                $cgi->param('contenttype', 'text/plain');
            }
        }
        $data = _validate_data($throw_error, $hr_vars);
        # If the attachment is stored locally, $data eq ''.
        # If an error is thrown, $data eq '0'.
        ($data ne '0') || return 0;
        $contenttype = $cgi->param('contenttype');

        # These are inserted using placeholders so no need to panic
        trick_taint($filename);
        trick_taint($contenttype);
        $isurl = 0;
    }

    # Check attachments the user tries to mark as obsolete.
    my @obsolete_attachments;
    if ($cgi->param('obsolete')) {
        @obsolete_attachments = $class->validate_obsolete($bug);
    }

    # The order of these function calls is important, as Flag::validate
    # assumes User::match_field has ensured that the
    # values in the requestee fields are legitimate user email addresses.
    my $match_status = Bugzilla::User::match_field($cgi, {
        '^requestee(_type)?-(\d+)$' => { 'type' => 'multi' },
    }, MATCH_SKIP_CONFIRM);

    $$hr_vars->{'match_field'} = 'requestee';
    if ($match_status == USER_MATCH_FAILED) {
        $$hr_vars->{'message'} = 'user_match_failed';
    }
    elsif ($match_status == USER_MATCH_MULTIPLE) {
        $$hr_vars->{'message'} = 'user_match_multiple';
    }

    # Escape characters in strings that will be used in SQL statements.
    my $description = $cgi->param('description');
    trick_taint($description);
    my $isprivate = $cgi->param('isprivate') ? 1 : 0;

    # Insert the attachment into the database.
    my $sth = $dbh->do(
        "INSERT INTO attachments
            (bug_id, creation_ts, filename, description,
             mimetype, ispatch, isurl, isprivate, submitter_id)
         VALUES (?,?,?,?,?,?,?,?,?)", undef, ($bug->bug_id, $timestamp, $filename,
              $description, $contenttype, $cgi->param('ispatch'),
              $isurl, $isprivate, $user->id));
    # Retrieve the ID of the newly created attachment record.
    my $attachid = $dbh->bz_last_key('attachments', 'attach_id');

    # We only use $data here in this INSERT with a placeholder,
    # so it's safe.
    $sth = $dbh->prepare("INSERT INTO attach_data
                         (id, thedata) VALUES ($attachid, ?)");
    trick_taint($data);
    $sth->bind_param(1, $data, $dbh->BLOB_TYPE);
    $sth->execute();

    # If the file is to be stored locally, stream the file from the webserver
    # to the local file without reading it into a local variable.
    if ($cgi->param('bigfile')) {
        my $attachdir = bz_locations()->{'attachdir'};
        my $fh = $cgi->upload('data');
        my $hash = ($attachid % 100) + 100;
        $hash =~ s/.*(\d\d)$/group.$1/;
        mkdir "$attachdir/$hash", 0770;
        chmod 0770, "$attachdir/$hash";
        open(AH, ">$attachdir/$hash/attachment.$attachid");
        binmode AH;
        my $sizecount = 0;
        my $limit = (Bugzilla->params->{"maxlocalattachment"} * 1048576);
        while (<$fh>) {
            print AH $_;
            $sizecount += length($_);
            if ($sizecount > $limit) {
                close AH;
                close $fh;
                unlink "$attachdir/$hash/attachment.$attachid";
                $throw_error ? ThrowUserError("local_file_too_large") : return 0;
            }
        }
        close AH;
        close $fh;
    }

    # Make existing attachments obsolete.
    my $fieldid = get_field_id('attachments.isobsolete');

    foreach my $obsolete_attachment (@obsolete_attachments) {
        # If the obsolete attachment has request flags, cancel them.
        # This call must be done before updating the 'attachments' table.
        Bugzilla::Flag::CancelRequests($bug, $obsolete_attachment, $timestamp);

        $dbh->do('UPDATE attachments SET isobsolete = 1 WHERE attach_id = ?',
                 undef, $obsolete_attachment->id);

        $dbh->do('INSERT INTO bugs_activity (bug_id, attach_id, who, bug_when,
                                             fieldid, removed, added)
                       VALUES (?,?,?,?,?,?,?)',
                  undef, ($bug->bug_id, $obsolete_attachment->id, $user->id,
                          $timestamp, $fieldid, 0, 1));
    }

    my $attachment = Bugzilla::Attachment->get($attachid);

    # 1. Add flags, if any. To avoid dying if something goes wrong
    # while processing flags, we will eval() flag validation.
    # This requires errors to die().
    # XXX: this can go away as soon as flag validation is able to
    #      fail without dying.
    #
    # 2. Flag::validate() should not detect any reference to existing flags
    # when creating a new attachment. Setting the third param to -1 will
    # force this function to check this point.
    my $error_mode_cache = Bugzilla->error_mode;
    Bugzilla->error_mode(ERROR_MODE_DIE);
    eval {
        Bugzilla::Flag::validate($cgi, $bug->bug_id, -1, SKIP_REQUESTEE_ON_ERROR);
        Bugzilla::Flag::process($bug, $attachment, $timestamp, $cgi);
    };
    Bugzilla->error_mode($error_mode_cache);
    if ($@) {
        $$hr_vars->{'message'} = 'flag_creation_failed';
        $$hr_vars->{'flag_creation_error'} = $@;
    }

    # Return the ID of the new attachment.
    return $attachid;
}

1;
