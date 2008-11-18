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
# Contributor(s): John Keiser <john@johnkeiser.com>
#                 Frédéric Buclin <LpSolit@gmail.com>

use strict;

package Bugzilla::Attachment::PatchReader;

use Bugzilla::Error;
use Bugzilla::Attachment;


sub process_diff {
    my ($attachment, $format, $context) = @_;
    my $dbh = Bugzilla->dbh;
    my $cgi = Bugzilla->cgi;
    my $lc  = Bugzilla->localconfig;
    my $vars = {};

    my ($reader, $last_reader) = setup_patch_readers(undef, $context);

    if ($format eq 'raw') {
        require PatchReader::DiffPrinter::raw;
        $last_reader->sends_data_to(new PatchReader::DiffPrinter::raw());
        # Actually print out the patch.
        print $cgi->header(-type => 'text/plain',
                           -expires => '+3M');

        $reader->iterate_string('Attachment ' . $attachment->id, $attachment->data);
    }
    else {
        my @other_patches = ();
        if ($lc->{interdiffbin} && $lc->{diffpath}) {
            # Get the list of attachments that the user can view in this bug.
            my @attachments =
                @{Bugzilla::Attachment->get_attachments_by_bug($attachment->bug_id)};
            # Extract patches only.
            @attachments = grep {$_->ispatch == 1} @attachments;
            # We want them sorted from newer to older.
            @attachments = sort { $b->id <=> $a->id } @attachments;

            # Ignore the current patch, but select the one right before it
            # chronologically.
            my $select_next_patch = 0;
            foreach my $attach (@attachments) {
                if ($attach->id == $attachment->id) {
                    $select_next_patch = 1;
                }
                else {
                    push(@other_patches, { 'id'       => $attach->id,
                                           'desc'     => $attach->description,
                                           'selected' => $select_next_patch });
                    $select_next_patch = 0;
                }
            }
        }

        $vars->{'bugid'} = $attachment->bug_id;
        $vars->{'attachid'} = $attachment->id;
        $vars->{'description'} = $attachment->description;
        $vars->{'other_patches'} = \@other_patches;

        setup_template_patch_reader($last_reader, $format, $context, $vars);
        # Actually print out the patch.
        $reader->iterate_string('Attachment ' . $attachment->id, $attachment->data);
    }
}

sub process_interdiff {
    my ($old_attachment, $new_attachment, $format, $context) = @_;
    my $cgi = Bugzilla->cgi;
    my $lc  = Bugzilla->localconfig;
    my $vars = {};

    # Get old patch data.
    my ($old_filename, $old_file_list) = get_unified_diff($old_attachment);
    # Get new patch data.
    my ($new_filename, $new_file_list) = get_unified_diff($new_attachment);

    my $warning = warn_if_interdiff_might_fail($old_file_list, $new_file_list);

    # Send through interdiff, send output directly to template.
    # Must hack path so that interdiff will work.
    $ENV{'PATH'} = $lc->{diffpath};
    open my $interdiff_fh, "$lc->{interdiffbin} $old_filename $new_filename|";
    binmode $interdiff_fh;
    my ($reader, $last_reader) = setup_patch_readers("", $context);

    if ($format eq 'raw') {
        require PatchReader::DiffPrinter::raw;
        $last_reader->sends_data_to(new PatchReader::DiffPrinter::raw());
        # Actually print out the patch.
        print $cgi->header(-type => 'text/plain',
                           -expires => '+3M');
    }
    else {
        $vars->{'warning'} = $warning if $warning;
        $vars->{'bugid'} = $new_attachment->bug_id;
        $vars->{'oldid'} = $old_attachment->id;
        $vars->{'old_desc'} = $old_attachment->description;
        $vars->{'newid'} = $new_attachment->id;
        $vars->{'new_desc'} = $new_attachment->description;

        setup_template_patch_reader($last_reader, $format, $context, $vars);
    }
    $reader->iterate_fh($interdiff_fh, 'interdiff #' . $old_attachment->id .
                        ' #' . $new_attachment->id);
    close $interdiff_fh;
    $ENV{'PATH'} = '';

    # Delete temporary files.
    unlink($old_filename) or warn "Could not unlink $old_filename: $!";
    unlink($new_filename) or warn "Could not unlink $new_filename: $!";
}

######################
#  Internal routines
######################

sub get_unified_diff {
    my $attachment = shift;

    # Bring in the modules we need.
    require PatchReader::Raw;
    require PatchReader::FixPatchRoot;
    require PatchReader::DiffPrinter::raw;
    require PatchReader::PatchInfoGrabber;
    require File::Temp;

    $attachment->ispatch
      || ThrowCodeError('must_be_patch', { 'attach_id' => $attachment->id });

    # Reads in the patch, converting to unified diff in a temp file.
    my $reader = new PatchReader::Raw;
    my $last_reader = $reader;

    # Fixes patch root (makes canonical if possible).
    if (Bugzilla->params->{'cvsroot'}) {
        my $fix_patch_root =
            new PatchReader::FixPatchRoot(Bugzilla->params->{'cvsroot'});
        $last_reader->sends_data_to($fix_patch_root);
        $last_reader = $fix_patch_root;
    }

    # Grabs the patch file info.
    my $patch_info_grabber = new PatchReader::PatchInfoGrabber();
    $last_reader->sends_data_to($patch_info_grabber);
    $last_reader = $patch_info_grabber;

    # Prints out to temporary file.
    my ($fh, $filename) = File::Temp::tempfile();
    my $raw_printer = new PatchReader::DiffPrinter::raw($fh);
    $last_reader->sends_data_to($raw_printer);
    $last_reader = $raw_printer;

    # Iterate!
    $reader->iterate_string($attachment->id, $attachment->data);

    return ($filename, $patch_info_grabber->patch_info()->{files});
}

sub warn_if_interdiff_might_fail {
    my ($old_file_list, $new_file_list) = @_;

    # Verify that the list of files diffed is the same.
    my @old_files = sort keys %{$old_file_list};
    my @new_files = sort keys %{$new_file_list};
    if (@old_files != @new_files
        || join(' ', @old_files) ne join(' ', @new_files))
    {
        return 'interdiff1';
    }

    # Verify that the revisions in the files are the same.
    foreach my $file (keys %{$old_file_list}) {
        if ($old_file_list->{$file}{old_revision} ne
            $new_file_list->{$file}{old_revision})
        {
            return 'interdiff2';
        }
    }
    return undef;
}

sub setup_patch_readers {
    my ($diff_root, $context) = @_;

    # Parameters:
    # format=raw|html
    # context=patch|file|0-n
    # collapsed=0|1
    # headers=0|1

    # Define the patch readers.
    # The reader that reads the patch in (whatever its format).
    require PatchReader::Raw;
    my $reader = new PatchReader::Raw;
    my $last_reader = $reader;
    # Fix the patch root if we have a cvs root.
    if (Bugzilla->params->{'cvsroot'}) {
        require PatchReader::FixPatchRoot;
        $last_reader->sends_data_to(new PatchReader::FixPatchRoot(Bugzilla->params->{'cvsroot'}));
        $last_reader->sends_data_to->diff_root($diff_root) if defined($diff_root);
        $last_reader = $last_reader->sends_data_to;
    }

    # Add in cvs context if we have the necessary info to do it
    if ($context ne 'patch' && Bugzilla->localconfig->{cvsbin} 
        && Bugzilla->params->{'cvsroot_get'}) 
    {
        require PatchReader::AddCVSContext;
        # We need to set $cvsbin as global, because PatchReader::CVSClient
        # needs it in order to find 'cvs'.
        $main::cvsbin = Bugzilla->localconfig->{cvsbin};
        $last_reader->sends_data_to(
          new PatchReader::AddCVSContext($context, Bugzilla->params->{'cvsroot_get'}));
        $last_reader = $last_reader->sends_data_to;
    }

    return ($reader, $last_reader);
}

sub setup_template_patch_reader {
    my ($last_reader, $format, $context, $vars) = @_;
    my $cgi = Bugzilla->cgi;
    my $template = Bugzilla->template;

    require PatchReader::DiffPrinter::template;

    # Define the vars for templates.
    if (defined $cgi->param('headers')) {
        $vars->{'headers'} = $cgi->param('headers');
    }
    else {
        $vars->{'headers'} = 1 if !defined $cgi->param('headers');
    }

    $vars->{'collapsed'} = $cgi->param('collapsed');
    $vars->{'context'} = $context;
    $vars->{'do_context'} = Bugzilla->localconfig->{cvsbin} 
                            && Bugzilla->params->{'cvsroot_get'} && !$vars->{'newid'};

    # Print everything out.
    print $cgi->header(-type => 'text/html',
                       -expires => '+3M');

    $last_reader->sends_data_to(new PatchReader::DiffPrinter::template($template,
                                "attachment/diff-header.$format.tmpl",
                                "attachment/diff-file.$format.tmpl",
                                "attachment/diff-footer.$format.tmpl",
                                { %{$vars},
                                  bonsai_url => Bugzilla->params->{'bonsai_url'},
                                  lxr_url => Bugzilla->params->{'lxr_url'},
                                  lxr_root => Bugzilla->params->{'lxr_root'},
                                }));
}

1;

__END__
