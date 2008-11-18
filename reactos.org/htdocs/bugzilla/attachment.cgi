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
#                 Myk Melez <myk@mozilla.org>
#                 Daniel Raichle <draichle@gmx.net>
#                 Dave Miller <justdave@syndicomm.com>
#                 Alexander J. Vincent <ajvincent@juno.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Greg Hendricks <ghendricks@novell.com>
#                 Frédéric Buclin <LpSolit@gmail.com>
#                 Marc Schumann <wurblzap@gmail.com>

################################################################################
# Script Initialization
################################################################################

# Make it harder for us to do dangerous things in Perl.
use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Flag; 
use Bugzilla::FlagType; 
use Bugzilla::User;
use Bugzilla::Util;
use Bugzilla::Bug;
use Bugzilla::Field;
use Bugzilla::Attachment;
use Bugzilla::Attachment::PatchReader;
use Bugzilla::Token;

Bugzilla->login();

# For most scripts we don't make $cgi and $template global variables. But
# when preparing Bugzilla for mod_perl, this script used these
# variables in so many subroutines that it was easier to just
# make them globals.
local our $cgi = Bugzilla->cgi;
local our $template = Bugzilla->template;
local our $vars = {};

################################################################################
# Main Body Execution
################################################################################

# All calls to this script should contain an "action" variable whose
# value determines what the user wants to do.  The code below checks
# the value of that variable and runs the appropriate code. If none is
# supplied, we default to 'view'.

# Determine whether to use the action specified by the user or the default.
my $action = $cgi->param('action') || 'view';

if ($action eq "view")  
{
    view();
}
elsif ($action eq "interdiff")
{
    interdiff();
}
elsif ($action eq "diff")
{
    diff();
}
elsif ($action eq "viewall") 
{ 
    viewall(); 
}
elsif ($action eq "enter") 
{ 
    Bugzilla->login(LOGIN_REQUIRED);
    enter(); 
}
elsif ($action eq "insert")
{
    Bugzilla->login(LOGIN_REQUIRED);
    insert();
}
elsif ($action eq "edit") 
{ 
    edit(); 
}
elsif ($action eq "update") 
{ 
    Bugzilla->login(LOGIN_REQUIRED);
    update();
}
elsif ($action eq "delete") {
    delete_attachment();
}
else 
{ 
  ThrowCodeError("unknown_action", { action => $action });
}

exit;

################################################################################
# Data Validation / Security Authorization
################################################################################

# Validates an attachment ID. Optionally takes a parameter of a form
# variable name that contains the ID to be validated. If not specified,
# uses 'id'.
# 
# Will throw an error if 1) attachment ID is not a valid number,
# 2) attachment does not exist, or 3) user isn't allowed to access the
# attachment.
#
# Returns a list, where the first item is the validated, detainted
# attachment id, and the 2nd item is the bug id corresponding to the
# attachment.
# 
sub validateID
{
    my $param = @_ ? $_[0] : 'id';
    my $dbh = Bugzilla->dbh;
    my $user = Bugzilla->user;

    # If we're not doing interdiffs, check if id wasn't specified and
    # prompt them with a page that allows them to choose an attachment.
    # Happens when calling plain attachment.cgi from the urlbar directly
    if ($param eq 'id' && !$cgi->param('id')) {

        print $cgi->header();
        $template->process("attachment/choose.html.tmpl", $vars) ||
            ThrowTemplateError($template->error());
        exit;
    }
    
    my $attach_id = $cgi->param($param);

    # Validate the specified attachment id. detaint kills $attach_id if
    # non-natural, so use the original value from $cgi in our exception
    # message here.
    detaint_natural($attach_id)
     || ThrowUserError("invalid_attach_id", { attach_id => $cgi->param($param) });
  
    # Make sure the attachment exists in the database.
    my ($bugid, $isprivate, $submitter_id) = $dbh->selectrow_array(
                                    "SELECT bug_id, isprivate, submitter_id
                                     FROM attachments 
                                     WHERE attach_id = ?",
                                     undef, $attach_id);
    ThrowUserError("invalid_attach_id", { attach_id => $attach_id }) 
        unless $bugid;

    # Make sure the user is authorized to access this attachment's bug.
    ValidateBugID($bugid);
    if ($isprivate && $user->id != $submitter_id && !$user->is_insider) {
        ThrowUserError('auth_failure', {action => 'access',
                                        object => 'attachment'});
    }

    return ($attach_id, $bugid);
}

# Validates format of a diff/interdiff. Takes a list as an parameter, which
# defines the valid format values. Will throw an error if the format is not
# in the list. Returns either the user selected or default format.
sub validateFormat
{
  # receives a list of legal formats; first item is a default
  my $format = $cgi->param('format') || $_[0];
  if ( lsearch(\@_, $format) == -1)
  {
     ThrowUserError("invalid_format", { format  => $format, formats => \@_ });
  }

  return $format;
}

# Validates context of a diff/interdiff. Will throw an error if the context
# is not number, "file" or "patch". Returns the validated, detainted context.
sub validateContext
{
  my $context = $cgi->param('context') || "patch";
  if ($context ne "file" && $context ne "patch") {
    detaint_natural($context)
      || ThrowUserError("invalid_context", { context => $cgi->param('context') });
  }

  return $context;
}

sub validateCanChangeAttachment 
{
    my ($attachid) = @_;
    my $dbh = Bugzilla->dbh;
    my ($productid) = $dbh->selectrow_array(
            "SELECT product_id
             FROM attachments
             INNER JOIN bugs
             ON bugs.bug_id = attachments.bug_id
             WHERE attach_id = ?", undef, $attachid);

    Bugzilla->user->can_edit_product($productid)
      || ThrowUserError("illegal_attachment_edit",
                        { attach_id => $attachid });
}

sub validateCanChangeBug
{
    my ($bugid) = @_;
    my $dbh = Bugzilla->dbh;
    my ($productid) = $dbh->selectrow_array(
            "SELECT product_id
             FROM bugs 
             WHERE bug_id = ?", undef, $bugid);

    Bugzilla->user->can_edit_product($productid)
      || ThrowUserError("illegal_attachment_edit_bug",
                        { bug_id => $bugid });
}

sub validateIsObsolete
{
    # Set the isobsolete flag to zero if it is undefined, since the UI uses
    # an HTML checkbox to represent this flag, and unchecked HTML checkboxes
    # do not get sent in HTML requests.
    $cgi->param('isobsolete', $cgi->param('isobsolete') ? 1 : 0);
}

sub validatePrivate
{
    # Set the isprivate flag to zero if it is undefined, since the UI uses
    # an HTML checkbox to represent this flag, and unchecked HTML checkboxes
    # do not get sent in HTML requests.
    $cgi->param('isprivate', $cgi->param('isprivate') ? 1 : 0);
}

# Returns 1 if the parameter is a content-type viewable in this browser
# Note that we don't use $cgi->Accept()'s ability to check if a content-type
# matches, because this will return a value even if it's matched by the generic
# */* which most browsers add to the end of their Accept: headers.
sub isViewable
{
  my $contenttype = trim(shift);
    
  # We assume we can view all text and image types  
  if ($contenttype =~ /^(text|image)\//) {
    return 1;
  }
  
  # Mozilla can view XUL. Note the trailing slash on the Gecko detection to
  # avoid sending XUL to Safari.
  if (($contenttype =~ /^application\/vnd\.mozilla\./) &&
      ($cgi->user_agent() =~ /Gecko\//))
  {
    return 1;
  }

  # If it's not one of the above types, we check the Accept: header for any 
  # types mentioned explicitly.
  my $accept = join(",", $cgi->Accept());
  
  if ($accept =~ /^(.*,)?\Q$contenttype\E(,.*)?$/) {
    return 1;
  }
  
  return 0;
}

################################################################################
# Functions
################################################################################

# Display an attachment.
sub view
{
    # Retrieve and validate parameters
    my ($attach_id) = validateID();
    my $dbh = Bugzilla->dbh;
    
    # Retrieve the attachment content and its content type from the database.
    my ($contenttype, $filename, $thedata) = $dbh->selectrow_array(
            "SELECT mimetype, filename, thedata FROM attachments " .
            "INNER JOIN attach_data ON id = attach_id " .
            "WHERE attach_id = ?", undef, $attach_id);
   
    # Bug 111522: allow overriding content-type manually in the posted form
    # params.
    if (defined $cgi->param('content_type'))
    {
        $cgi->param('contenttypemethod', 'manual');
        $cgi->param('contenttypeentry', $cgi->param('content_type'));
        Bugzilla::Attachment->validate_content_type(THROW_ERROR);
        $contenttype = $cgi->param('content_type');
    }

    # Return the appropriate HTTP response headers.
    $filename =~ s/^.*[\/\\]//;
    my $filesize = length($thedata);
    # A zero length attachment in the database means the attachment is 
    # stored in a local file
    if ($filesize == 0)
    {
        my $hash = ($attach_id % 100) + 100;
        $hash =~ s/.*(\d\d)$/group.$1/;
        if (open(AH, bz_locations()->{'attachdir'} . "/$hash/attachment.$attach_id")) {
            binmode AH;
            $filesize = (stat(AH))[7];
        }
    }
    if ($filesize == 0)
    {
        ThrowUserError("attachment_removed");
    }


    # escape quotes and backslashes in the filename, per RFCs 2045/822
    $filename =~ s/\\/\\\\/g; # escape backslashes
    $filename =~ s/"/\\"/g; # escape quotes

    print $cgi->header(-type=>"$contenttype; name=\"$filename\"",
                       -content_disposition=> "inline; filename=\"$filename\"",
                       -content_length => $filesize);

    if ($thedata) {
        print $thedata;
    } else {
        while (<AH>) {
            print $_;
        }
        close(AH);
    }

}

sub interdiff {
    # Retrieve and validate parameters
    my ($old_id) = validateID('oldid');
    my ($new_id) = validateID('newid');
    my $format = validateFormat('html', 'raw');
    my $context = validateContext();

    # XXX - validateID should be replaced by Attachment::check_attachment()
    # and should return an attachment object. This would save us a lot of
    # trouble.
    my $old_attachment = Bugzilla::Attachment->get($old_id);
    my $new_attachment = Bugzilla::Attachment->get($new_id);

    Bugzilla::Attachment::PatchReader::process_interdiff(
        $old_attachment, $new_attachment, $format, $context);
}

sub diff {
    # Retrieve and validate parameters
    my ($attach_id) = validateID();
    my $format = validateFormat('html', 'raw');
    my $context = validateContext();

    my $attachment = Bugzilla::Attachment->get($attach_id);

    # If it is not a patch, view normally.
    if (!$attachment->ispatch) {
        view();
        return;
    }

    Bugzilla::Attachment::PatchReader::process_diff($attachment, $format, $context);
}

# Display all attachments for a given bug in a series of IFRAMEs within one
# HTML page.
sub viewall {
    # Retrieve and validate parameters
    my $bugid = $cgi->param('bugid');
    ValidateBugID($bugid);
    my $bug = new Bugzilla::Bug($bugid);

    my $attachments = Bugzilla::Attachment->get_attachments_by_bug($bugid);

    foreach my $a (@$attachments) {
        $a->{'isviewable'} = isViewable($a->contenttype);
    }

    # Define the variables and functions that will be passed to the UI template.
    $vars->{'bug'} = $bug;
    $vars->{'attachments'} = $attachments;

    print $cgi->header();

    # Generate and return the UI (HTML page) from the appropriate template.
    $template->process("attachment/show-multiple.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

# Display a form for entering a new attachment.
sub enter
{
  # Retrieve and validate parameters
  my $bugid = $cgi->param('bugid');
  ValidateBugID($bugid);
  validateCanChangeBug($bugid);
  my $dbh = Bugzilla->dbh;
  my $user = Bugzilla->user;

  my $bug = new Bugzilla::Bug($bugid, $user->id);
  # Retrieve the attachments the user can edit from the database and write
  # them into an array of hashes where each hash represents one attachment.
  my $canEdit = "";
  if (!$user->in_group('editbugs', $bug->product_id)) {
      $canEdit = "AND submitter_id = " . $user->id;
  }
  my $attachments = $dbh->selectall_arrayref(
          "SELECT attach_id AS id, description, isprivate
           FROM attachments
           WHERE bug_id = ? 
           AND isobsolete = 0 $canEdit
           ORDER BY attach_id",{'Slice' =>{}}, $bugid);

  # Define the variables and functions that will be passed to the UI template.
  $vars->{'bug'} = $bug;
  $vars->{'attachments'} = $attachments;

  my $flag_types = Bugzilla::FlagType::match({'target_type'  => 'attachment',
                                              'product_id'   => $bug->product_id,
                                              'component_id' => $bug->component_id});
  $vars->{'flag_types'} = $flag_types;
  $vars->{'any_flags_requesteeble'} = grep($_->is_requesteeble, @$flag_types);

  print $cgi->header();

  # Generate and return the UI (HTML page) from the appropriate template.
  $template->process("attachment/create.html.tmpl", $vars)
    || ThrowTemplateError($template->error());
}

# Insert a new attachment into the database.
sub insert
{
    my $dbh = Bugzilla->dbh;
    my $user = Bugzilla->user;

    # Retrieve and validate parameters
    my $bugid = $cgi->param('bugid');
    ValidateBugID($bugid);
    validateCanChangeBug($bugid);
    ValidateComment(scalar $cgi->param('comment'));
    my ($timestamp) = Bugzilla->dbh->selectrow_array("SELECT NOW()"); 

    my $bug = new Bugzilla::Bug($bugid);
    my $attachid =
        Bugzilla::Attachment->insert_attachment_for_bug(THROW_ERROR, $bug, $user,
                                                        $timestamp, \$vars);

  # Insert a comment about the new attachment into the database.
  my $comment = "Created an attachment (id=$attachid)\n" .
                $cgi->param('description') . "\n";
  $comment .= ("\n" . $cgi->param('comment')) if defined $cgi->param('comment');

  my $isprivate = $cgi->param('isprivate') ? 1 : 0;
  AppendComment($bugid, $user->id, $comment, $isprivate, $timestamp);

  # Assign the bug to the user, if they are allowed to take it
  my $owner = "";
  
  if ($cgi->param('takebug') && $user->in_group('editbugs', $bug->product_id)) {
      
      my @fields = ("assigned_to", "bug_status", "resolution", "everconfirmed",
                    "login_name");
      
      # Get the old values, for the bugs_activity table
      my @oldvalues = $dbh->selectrow_array(
              "SELECT " . join(", ", @fields) . " " .
              "FROM bugs " .
              "INNER JOIN profiles " .
              "ON profiles.userid = bugs.assigned_to " .
              "WHERE bugs.bug_id = ?", undef, $bugid);
      
      my @newvalues = ($user->id, "ASSIGNED", "", 1, $user->login);
      
      # Make sure the person we are taking the bug from gets mail.
      $owner = $oldvalues[4];  

      # Update the bug record. Note that this doesn't involve login_name.
      $dbh->do('UPDATE bugs SET delta_ts = ?, ' .
               join(', ', map("$fields[$_] = ?", (0..3))) . ' WHERE bug_id = ?',
               undef, ($timestamp, map($newvalues[$_], (0..3)) , $bugid));

      # If the bug was a dupe, we have to remove its entry from the
      # 'duplicates' table.
      $dbh->do('DELETE FROM duplicates WHERE dupe = ?', undef, $bugid);

      # We store email addresses in the bugs_activity table rather than IDs.
      $oldvalues[0] = $oldvalues[4];
      $newvalues[0] = $newvalues[4];

      for (my $i = 0; $i < 4; $i++) {
          if ($oldvalues[$i] ne $newvalues[$i]) {
              LogActivityEntry($bugid, $fields[$i], $oldvalues[$i],
                               $newvalues[$i], $user->id, $timestamp);
          }
      }      
  }   

  # Define the variables and functions that will be passed to the UI template.
  $vars->{'mailrecipients'} =  { 'changer' => $user->login,
                                 'owner'   => $owner };
  $vars->{'bugid'} = $bugid;
  $vars->{'attachid'} = $attachid;
  $vars->{'description'} = $cgi->param('description');
  $vars->{'contenttypemethod'} = $cgi->param('contenttypemethod');
  $vars->{'contenttype'} = $cgi->param('contenttype');

  print $cgi->header();

  # Generate and return the UI (HTML page) from the appropriate template.
  $template->process("attachment/created.html.tmpl", $vars)
    || ThrowTemplateError($template->error());
}

# Displays a form for editing attachment properties.
# Any user is allowed to access this page, unless the attachment
# is private and the user does not belong to the insider group.
# Validations are done later when the user submits changes.
sub edit {
  my ($attach_id) = validateID();
  my $dbh = Bugzilla->dbh;

  my $attachment = Bugzilla::Attachment->get($attach_id);
  my $isviewable = !$attachment->isurl && isViewable($attachment->contenttype);

  # Retrieve a list of attachments for this bug as well as a summary of the bug
  # to use in a navigation bar across the top of the screen.
  my $bugattachments =
      Bugzilla::Attachment->get_attachments_by_bug($attachment->bug_id);
  # We only want attachment IDs.
  @$bugattachments = map { $_->id } @$bugattachments;

  my ($bugsummary, $product_id, $component_id) =
      $dbh->selectrow_array('SELECT short_desc, product_id, component_id
                               FROM bugs
                              WHERE bug_id = ?', undef, $attachment->bug_id);

  # Get a list of flag types that can be set for this attachment.
  my $flag_types = Bugzilla::FlagType::match({ 'target_type'  => 'attachment' ,
                                               'product_id'   => $product_id ,
                                               'component_id' => $component_id });
  foreach my $flag_type (@$flag_types) {
    $flag_type->{'flags'} = Bugzilla::Flag::match({ 'type_id'   => $flag_type->id,
                                                    'attach_id' => $attachment->id });
  }
  $vars->{'flag_types'} = $flag_types;
  $vars->{'any_flags_requesteeble'} = grep($_->is_requesteeble, @$flag_types);
  $vars->{'attachment'} = $attachment;
  $vars->{'bugsummary'} = $bugsummary; 
  $vars->{'isviewable'} = $isviewable; 
  $vars->{'attachments'} = $bugattachments; 

  # Determine if PatchReader is installed
  eval {
    require PatchReader;
    $vars->{'patchviewerinstalled'} = 1;
  };
  print $cgi->header();

  # Generate and return the UI (HTML page) from the appropriate template.
  $template->process("attachment/edit.html.tmpl", $vars)
    || ThrowTemplateError($template->error());
}

# Updates an attachment record. Users with "editbugs" privileges, (or the
# original attachment's submitter) can edit the attachment's description,
# content type, ispatch and isobsolete flags, and statuses, and they can
# also submit a comment that appears in the bug.
# Users cannot edit the content of the attachment itself.
sub update
{
    my $user = Bugzilla->user;
    my $userid = $user->id;
    my $dbh = Bugzilla->dbh;

    # Retrieve and validate parameters
    ValidateComment(scalar $cgi->param('comment'));
    my ($attach_id, $bugid) = validateID();
    my $bug = new Bugzilla::Bug($bugid);
    my $attachment = Bugzilla::Attachment->get($attach_id);
    $attachment->validate_can_edit($bug->product_id);
    validateCanChangeAttachment($attach_id);
    Bugzilla::Attachment->validate_description(THROW_ERROR);
    Bugzilla::Attachment->validate_is_patch(THROW_ERROR);
    Bugzilla::Attachment->validate_content_type(THROW_ERROR) unless $cgi->param('ispatch');
    validateIsObsolete();
    validatePrivate();

    # If the submitter of the attachment is not in the insidergroup,
    # be sure that he cannot overwrite the private bit.
    # This check must be done before calling Bugzilla::Flag*::validate(),
    # because they will look at the private bit when checking permissions.
    # XXX - This is a ugly hack. Ideally, we shouldn't have to look at the
    # old private bit twice (first here, and then below again), but this is
    # the less risky change.
    unless ($user->is_insider) {
        my $oldisprivate = $dbh->selectrow_array('SELECT isprivate FROM attachments
                                                  WHERE attach_id = ?', undef, $attach_id);
        $cgi->param('isprivate', $oldisprivate);
    }

    # The order of these function calls is important, as Flag::validate
    # assumes User::match_field has ensured that the values in the
    # requestee fields are legitimate user email addresses.
    Bugzilla::User::match_field($cgi, {
        '^requestee(_type)?-(\d+)$' => { 'type' => 'multi' }
    });
    Bugzilla::Flag::validate($cgi, $bugid, $attach_id);

    # Lock database tables in preparation for updating the attachment.
    $dbh->bz_lock_tables('attachments WRITE', 'flags WRITE' ,
          'flagtypes READ', 'fielddefs READ', 'bugs_activity WRITE',
          'flaginclusions AS i READ', 'flagexclusions AS e READ',
          # cc, bug_group_map, user_group_map, and groups are in here so we
          # can check the permissions of flag requestees and email addresses
          # on the flag type cc: lists via the CanSeeBug
          # function call in Flag::notify. group_group_map is in here si
          # Bugzilla::User can flatten groups.
          'bugs WRITE', 'profiles READ', 'email_setting READ',
          'setting READ', 'profile_setting READ',
          'cc READ', 'bug_group_map READ', 'user_group_map READ',
          'group_group_map READ', 'groups READ', 'group_control_map READ');

  # Get a copy of the attachment record before we make changes
  # so we can record those changes in the activity table.
  my ($olddescription, $oldcontenttype, $oldfilename, $oldispatch,
      $oldisobsolete, $oldisprivate) = $dbh->selectrow_array(
      "SELECT description, mimetype, filename, ispatch, isobsolete, isprivate
       FROM attachments WHERE attach_id = ?", undef, $attach_id);

  # Quote the description and content type for use in the SQL UPDATE statement.
  my $description = $cgi->param('description');
  my $contenttype = $cgi->param('contenttype');
  my $filename = $cgi->param('filename');
  # we can detaint this way thanks to placeholders
  trick_taint($description);
  trick_taint($contenttype);
  trick_taint($filename);

  # Figure out when the changes were made.
  my ($timestamp) = $dbh->selectrow_array("SELECT NOW()");
    
  # Update flags.  We have to do this before committing changes
  # to attachments so that we can delete pending requests if the user
  # is obsoleting this attachment without deleting any requests
  # the user submits at the same time.
  Bugzilla::Flag::process($bug, $attachment, $timestamp, $cgi);

  # Update the attachment record in the database.
  $dbh->do("UPDATE  attachments 
            SET     description = ?,
                    mimetype    = ?,
                    filename    = ?,
                    ispatch     = ?,
                    isobsolete  = ?,
                    isprivate   = ?
            WHERE   attach_id   = ?",
            undef, ($description, $contenttype, $filename,
            $cgi->param('ispatch'), $cgi->param('isobsolete'), 
            $cgi->param('isprivate'), $attach_id));

  # Record changes in the activity table.
  if ($olddescription ne $cgi->param('description')) {
    my $fieldid = get_field_id('attachments.description');
    $dbh->do("INSERT INTO bugs_activity (bug_id, attach_id, who, bug_when,
                                        fieldid, removed, added)
              VALUES (?,?,?,?,?,?,?)",
              undef, ($bugid, $attach_id, $userid, $timestamp, $fieldid,
                     $olddescription, $description));
  }
  if ($oldcontenttype ne $cgi->param('contenttype')) {
    my $fieldid = get_field_id('attachments.mimetype');
    $dbh->do("INSERT INTO bugs_activity (bug_id, attach_id, who, bug_when,
                                        fieldid, removed, added)
              VALUES (?,?,?,?,?,?,?)",
              undef, ($bugid, $attach_id, $userid, $timestamp, $fieldid,
                     $oldcontenttype, $contenttype));
  }
  if ($oldfilename ne $cgi->param('filename')) {
    my $fieldid = get_field_id('attachments.filename');
    $dbh->do("INSERT INTO bugs_activity (bug_id, attach_id, who, bug_when,
                                        fieldid, removed, added)
              VALUES (?,?,?,?,?,?,?)", 
              undef, ($bugid, $attach_id, $userid, $timestamp, $fieldid,
                     $oldfilename, $filename));
  }
  if ($oldispatch ne $cgi->param('ispatch')) {
    my $fieldid = get_field_id('attachments.ispatch');
    $dbh->do("INSERT INTO bugs_activity (bug_id, attach_id, who, bug_when,
                                        fieldid, removed, added)
              VALUES (?,?,?,?,?,?,?)",
              undef, ($bugid, $attach_id, $userid, $timestamp, $fieldid,
                     $oldispatch, $cgi->param('ispatch')));
  }
  if ($oldisobsolete ne $cgi->param('isobsolete')) {
    my $fieldid = get_field_id('attachments.isobsolete');
    $dbh->do("INSERT INTO bugs_activity (bug_id, attach_id, who, bug_when,
                                        fieldid, removed, added)
              VALUES (?,?,?,?,?,?,?)",
              undef, ($bugid, $attach_id, $userid, $timestamp, $fieldid,
                     $oldisobsolete, $cgi->param('isobsolete')));
  }
  if ($oldisprivate ne $cgi->param('isprivate')) {
    my $fieldid = get_field_id('attachments.isprivate');
    $dbh->do("INSERT INTO bugs_activity (bug_id, attach_id, who, bug_when,
                                        fieldid, removed, added)
              VALUES (?,?,?,?,?,?,?)",
              undef, ($bugid, $attach_id, $userid, $timestamp, $fieldid,
                     $oldisprivate, $cgi->param('isprivate')));
  }
  
  # Unlock all database tables now that we are finished updating the database.
  $dbh->bz_unlock_tables();

  # If the user submitted a comment while editing the attachment,
  # add the comment to the bug.
  if ($cgi->param('comment'))
  {
    # Prepend a string to the comment to let users know that the comment came
    # from the "edit attachment" screen.
    my $comment = qq|(From update of attachment $attach_id)\n| .
                  $cgi->param('comment');

    # Append the comment to the list of comments in the database.
    AppendComment($bugid, $userid, $comment, $cgi->param('isprivate'), $timestamp);
  }
  
  # Define the variables and functions that will be passed to the UI template.
  $vars->{'mailrecipients'} = { 'changer' => Bugzilla->user->login };
  $vars->{'attachid'} = $attach_id; 
  $vars->{'bugid'} = $bugid; 

  print $cgi->header();

  # Generate and return the UI (HTML page) from the appropriate template.
  $template->process("attachment/updated.html.tmpl", $vars)
    || ThrowTemplateError($template->error());
}

# Only administrators can delete attachments.
sub delete_attachment {
    my $user = Bugzilla->login(LOGIN_REQUIRED);
    my $dbh = Bugzilla->dbh;

    print $cgi->header();

    $user->in_group('admin')
      || ThrowUserError('auth_failure', {group  => 'admin',
                                         action => 'delete',
                                         object => 'attachment'});

    Bugzilla->params->{'allow_attachment_deletion'}
      || ThrowUserError('attachment_deletion_disabled');

    # Make sure the administrator is allowed to edit this attachment.
    my ($attach_id, $bug_id) = validateID();
    my $attachment = Bugzilla::Attachment->get($attach_id);
    validateCanChangeAttachment($attach_id);

    $attachment->datasize || ThrowUserError('attachment_removed');

    # We don't want to let a malicious URL accidentally delete an attachment.
    my $token = trim($cgi->param('token'));
    if ($token) {
        my ($creator_id, $date, $event) = Bugzilla::Token::GetTokenData($token);
        unless ($creator_id
                  && ($creator_id == $user->id)
                  && ($event eq "attachment$attach_id"))
        {
            # The token is invalid.
            ThrowUserError('token_inexistent');
        }

        # The token is valid. Delete the content of the attachment.
        my $msg;
        $vars->{'attachid'} = $attach_id;
        $vars->{'bugid'} = $bug_id;
        $vars->{'date'} = $date;
        $vars->{'reason'} = clean_text($cgi->param('reason') || '');
        $vars->{'mailrecipients'} = { 'changer' => $user->login };

        $template->process("attachment/delete_reason.txt.tmpl", $vars, \$msg)
          || ThrowTemplateError($template->error());

        $dbh->bz_lock_tables('attachments WRITE', 'attach_data WRITE', 'flags WRITE');
        $dbh->do('DELETE FROM attach_data WHERE id = ?', undef, $attach_id);
        $dbh->do('UPDATE attachments SET mimetype = ?, ispatch = ?, isurl = ?,
                         isobsolete = ?
                  WHERE attach_id = ?', undef,
                 ('text/plain', 0, 0, 1, $attach_id));
        $dbh->do('DELETE FROM flags WHERE attach_id = ?', undef, $attach_id);
        $dbh->bz_unlock_tables;

        # If the attachment is stored locally, remove it.
        if (-e $attachment->_get_local_filename) {
            unlink $attachment->_get_local_filename;
        }

        # Now delete the token.
        delete_token($token);

        # Paste the reason provided by the admin into a comment.
        AppendComment($bug_id, $user->id, $msg);

        $template->process("attachment/updated.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
    }
    else {
        # Create a token.
        $token = issue_session_token('attachment' . $attach_id);

        $vars->{'a'} = $attachment;
        $vars->{'token'} = $token;

        $template->process("attachment/confirm-delete.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
    }
}
