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
# Contributor(s): Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Bill Barry <after.fallout@gmail.com>

package Bugzilla::Install::Filesystem;

# NOTE: This package may "use" any modules that it likes,
# and localconfig is available. However, all functions in this
# package should assume that:
#
# * Templates are not available.
# * Files do not have the correct permissions.
# * The database does not exist.

use strict;

use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Install::Localconfig;
use Bugzilla::Util;

use File::Find;
use File::Path;
use File::Basename;
use IO::File;
use POSIX ();

use base qw(Exporter);
our @EXPORT = qw(
    update_filesystem
    create_htaccess
    fix_all_file_permissions
);

# This looks like a constant because it effectively is, but
# it has to call other subroutines and read the current filesystem,
# so it's defined as a sub. This is not exported, so it doesn't have
# a perldoc. However, look at the various hashes defined inside this 
# function to understand what it returns. (There are comments throughout.)
#
# The rationale for the file permissions is that the web server generally 
# runs as apache, so the cgi scripts should not be writable for apache,
# otherwise someone may find it possible to change the cgis when exploiting
# some security flaw somewhere (not necessarily in Bugzilla!)
sub FILESYSTEM {
    my $datadir       = bz_locations()->{'datadir'};
    my $attachdir     = bz_locations()->{'attachdir'};
    my $extensionsdir = bz_locations()->{'extensionsdir'};
    my $webdotdir     = bz_locations()->{'webdotdir'};
    my $templatedir   = bz_locations()->{'templatedir'};
    my $libdir        = bz_locations()->{'libpath'};
    my $skinsdir      = bz_locations()->{'skinsdir'};

    my $ws_group      = Bugzilla->localconfig->{'webservergroup'};

    # The set of permissions that we use:

    # FILES
    # Executable by the web server
    my $ws_executable = $ws_group ? 0750 : 0755;
    # Executable by the owner only.
    my $owner_executable = 0700;
    # Readable by the web server.
    my $ws_readable = $ws_group ? 0640 : 0644;
    # Readable by the owner only.
    my $owner_readable = 0600;
    # Writeable by the web server.
    my $ws_writeable = $ws_group ? 0660 : 0666;

    # DIRECTORIES
    # Readable by the web server.
    my $ws_dir_readable  = $ws_group ? 0750 : 0755;
    # Readable only by the owner.
    my $owner_dir_readable = 0700;
    # Writeable by the web server.
    my $ws_dir_writeable = $ws_group ? 0770 : 01777;
    # The webserver can overwrite files owned by other users, 
    # in this directory.
    my $ws_dir_full_control = $ws_group ? 0770 : 0777;

    # Note: When being processed by checksetup, these have their permissions
    # set in this order: %all_dirs, %recurse_dirs, %all_files.
    #
    # Each is processed in alphabetical order of keys, so shorter keys
    # will have their permissions set before longer keys (thus setting
    # the permissions on parent directories before setting permissions
    # on their children).

    # --- FILE PERMISSIONS (Non-created files) --- #
    my %files = (
        '*'               => { perms => $ws_readable },
        '*.cgi'           => { perms => $ws_executable },
        'whineatnews.pl'  => { perms => $ws_executable },
        'collectstats.pl' => { perms => $ws_executable },
        'checksetup.pl'   => { perms => $owner_executable },
        'importxml.pl'    => { perms => $ws_executable },
        'runtests.pl'     => { perms => $owner_executable },
        'testserver.pl'   => { perms => $ws_executable },
        'whine.pl'        => { perms => $ws_executable },
        'customfield.pl'  => { perms => $owner_executable },
        'email_in.pl'     => { perms => $ws_executable },

        'docs/makedocs.pl'   => { perms => $owner_executable },
        'docs/rel_notes.txt' => { perms => $ws_readable },
        'docs/README.docs'   => { perms => $owner_readable },
        "$datadir/bugzilla-update.xml" => { perms => $ws_writeable },
        "$datadir/params" => { perms => $ws_writeable },
        "$datadir/mailer.testfile" => { perms => $ws_writeable },
    );

    # Directories that we want to set the perms on, but not
    # recurse through. These are directories we didn't create
    # in checkesetup.pl.
    my %non_recurse_dirs = (
        '.'  => $ws_dir_readable,
        docs => $ws_dir_readable,
    );

    # This sets the permissions for each item inside each of these 
    # directories, including the directory itself. 
    # 'CVS' directories are special, though, and are never readable by 
    # the webserver.
    my %recurse_dirs = (
        # Writeable directories
        "$datadir/template" => { files => $ws_readable, 
                                  dirs => $ws_dir_full_control },
         $attachdir         => { files => $ws_writeable,
                                  dirs => $ws_dir_writeable },
         $webdotdir         => { files => $ws_writeable,
                                  dirs => $ws_dir_writeable },
         graphs             => { files => $ws_writeable,
                                  dirs => $ws_dir_writeable },

         # Readable directories
         "$datadir/mining"     => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         "$datadir/duplicates" => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         "$libdir/Bugzilla"    => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         $templatedir          => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         images                => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         css                   => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         js                    => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         skins                 => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         t                     => { files => $owner_readable,
                                     dirs => $owner_dir_readable },
         'docs/html'           => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         'docs/pdf'            => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         'docs/txt'            => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         'docs/images'         => { files => $ws_readable,
                                     dirs => $ws_dir_readable },
         'docs/lib'            => { files => $owner_readable,
                                     dirs => $owner_dir_readable },
         'docs/xml'            => { files => $owner_readable,
                                     dirs => $owner_dir_readable },
    );

    # --- FILES TO CREATE --- #

    # The name of each directory that we should actually *create*,
    # pointing at its default permissions.
    my %create_dirs = (
        $datadir                => $ws_dir_full_control,
        "$datadir/mimedump-tmp" => $ws_dir_writeable,
        "$datadir/mining"       => $ws_dir_readable,
        "$datadir/duplicates"   => $ws_dir_readable,
        $attachdir              => $ws_dir_writeable,
        $extensionsdir          => $ws_dir_readable,
        graphs                  => $ws_dir_writeable,
        $webdotdir              => $ws_dir_writeable,
        'skins/custom'          => $ws_dir_readable,
        'skins/contrib'         => $ws_dir_readable,
    );

    # The name of each file, pointing at its default permissions and
    # default contents.
    my %create_files = (
        "$datadir/mail"    => { perms => $ws_readable },
    );

    # Each standard stylesheet has an associated custom stylesheet that
    # we create. Also, we create placeholders for standard stylesheets
    # for contrib skins which don't provide them themselves.
    foreach my $skin_dir ("$skinsdir/custom", <$skinsdir/contrib/*>) {
        next unless -d $skin_dir;
        next if basename($skin_dir) =~ /^cvs$/i;
        foreach (<$skinsdir/standard/*.css>) {
            my $standard_css_file = basename($_);
            my $custom_css_file = "$skin_dir/$standard_css_file";
            $create_files{$custom_css_file} = { perms => $ws_readable, contents => <<EOT
/*
 * Custom rules for $standard_css_file.
 * The rules you put here override rules in that stylesheet.
 */
EOT
            }
        }
    }

    # Because checksetup controls the creation of index.html separately
    # from all other files, it gets its very own hash.
    my %index_html = (
        'index.html' => { perms => $ws_readable, contents => <<EOT
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
  <meta http-equiv="Refresh" content="0; URL=index.cgi">
</head>
<body>
  <h1>I think you are looking for <a href="index.cgi">index.cgi</a></h1>
</body>
</html>
EOT
        }
    );

    # Because checksetup controls the .htaccess creation separately
    # by a localconfig variable, these go in a separate variable from
    # %create_files.
    my $ht_default_deny = <<EOT;
# nothing in this directory is retrievable unless overridden by an .htaccess
# in a subdirectory
deny from all
EOT

    my %htaccess = (
        "$attachdir/.htaccess"       => { perms    => $ws_readable,
                                          contents => $ht_default_deny },
        "$libdir/Bugzilla/.htaccess" => { perms    => $ws_readable,
                                          contents => $ht_default_deny },
        "$templatedir/.htaccess"     => { perms    => $ws_readable,
                                          contents => $ht_default_deny },

        '.htaccess' => { perms => $ws_readable, contents => <<EOT
# Don't allow people to retrieve non-cgi executable files or our private data
<FilesMatch ^(.*\\.pm|.*\\.pl|.*localconfig.*)\$>
  deny from all
</FilesMatch>
EOT
        },

        "$webdotdir/.htaccess" => { perms => $ws_readable, contents => <<EOT
# Restrict access to .dot files to the public webdot server at research.att.com
# if research.att.com ever changes their IP, or if you use a different
# webdot server, you'll need to edit this
<FilesMatch \\.dot\$>
  Allow from 192.20.225.0/24
  Deny from all
</FilesMatch>

# Allow access to .png files created by a local copy of 'dot'
<FilesMatch \\.png\$>
  Allow from all
</FilesMatch>

# And no directory listings, either.
Deny from all
EOT
        },

        # Even though $datadir may not (and should not) be in the webtree,
        # we can't know for sure, so create the .htaccess anyway. It's harmless
        # if it's not accessible...
        "$datadir/.htaccess" => { perms    => $ws_readable, contents => <<EOT
# Nothing in this directory is retrievable unless overridden by an .htaccess
# in a subdirectory; the only exception is duplicates.rdf, which is used by
# duplicates.xul and must be loadable over the web
deny from all
<Files duplicates.rdf>
  allow from all
</Files>
EOT


        },
    );

    my %all_files = (%create_files, %htaccess, %index_html, %files);
    my %all_dirs  = (%create_dirs, %non_recurse_dirs);

    return {
        create_dirs  => \%create_dirs,
        recurse_dirs => \%recurse_dirs,
        all_dirs     => \%all_dirs,

        create_files => \%create_files,
        htaccess     => \%htaccess,
        index_html   => \%index_html,
        all_files    => \%all_files,
    };
}

sub update_filesystem {
    my ($params) = @_;
    my $fs = FILESYSTEM();
    my %dirs  = %{$fs->{create_dirs}};
    my %files = %{$fs->{create_files}};

    my $datadir = bz_locations->{'datadir'};
    # If the graphs/ directory doesn't exist, we're upgrading from
    # a version old enough that we need to update the $datadir/mining 
    # format.
    if (-d "$datadir/mining" && !-d 'graphs') {
        _update_old_charts($datadir);
    }

    # By sorting the dirs, we assure that shorter-named directories
    # (meaning parent directories) are always created before their
    # child directories.
    foreach my $dir (sort keys %dirs) {
        unless (-d $dir) {
            print "Creating $dir directory...\n";
            mkdir $dir || die $!;
            # For some reason, passing in the permissions to "mkdir"
            # doesn't work right, but doing a "chmod" does.
            chmod $dirs{$dir}, $dir || die $!;
        }
    }

    _create_files(%files);
    if ($params->{index_html}) {
        _create_files(%{$fs->{index_html}});
    }
    elsif (-e 'index.html') {
        my $templatedir = bz_locations()->{'templatedir'};
        print <<EOT;

*** It appears that you still have an old index.html hanging around.
    Either the contents of this file should be moved into a template and 
    placed in the '$templatedir/en/custom' directory, or you should delete 
    the file.

EOT
    }

    # Delete old files that no longer need to exist

    # 2001-04-29 jake@bugzilla.org - Remove oldemailtech
    #   http://bugzilla.mozilla.org/show_bugs.cgi?id=71552
    if (-d 'shadow') {
        print "Removing shadow directory...\n";
        rmtree("shadow");
    }

    if (-e "$datadir/versioncache") {
        print "Removing versioncache...\n";
        unlink "$datadir/versioncache";
    }

}

sub create_htaccess {
    _create_files(%{FILESYSTEM()->{htaccess}});

    # Repair old .htaccess files
    my $htaccess = new IO::File('.htaccess', 'r') || die ".htaccess: $!";
    my $old_data;
    { local $/; $old_data = <$htaccess>; }
    $htaccess->close;

    my $repaired = 0;
    if ($old_data =~ s/\|localconfig\|/\|.*localconfig.*\|/) {
        $repaired = 1;
    }
    if ($old_data !~ /\(\.\*\\\.pm\|/) {
        $old_data =~ s/\(/(.*\\.pm\|/;
        $repaired = 1;
    }
    if ($repaired) {
        print "Repairing .htaccess...\n";
        $htaccess = new IO::File('.htaccess', 'w') || die $!;
        print $htaccess $old_data;
        $htaccess->close;
    }


    my $webdot_dir = bz_locations()->{'webdotdir'};
    # The public webdot IP address changed.
    my $webdot = new IO::File("$webdot_dir/.htaccess", 'r')
        || die "$webdot_dir/.htaccess: $!";
    my $webdot_data;
    { local $/; $webdot_data = <$webdot>; }
    $webdot->close;
    if ($webdot_data =~ /192\.20\.225\.10/) {
        print "Repairing $webdot_dir/.htaccess...\n";
        $webdot_data =~ s/192\.20\.225\.10/192.20.225.0\/24/g;
        $webdot = new IO::File("$webdot_dir/.htaccess", 'w') || die $!;
        print $webdot $webdot_data;
        $webdot->close;
    }
}

# A helper for the above functions.
sub _create_files {
    my (%files) = @_;

    # It's not necessary to sort these, but it does make the
    # output of checksetup.pl look a bit nicer.
    foreach my $file (sort keys %files) {
        unless (-e $file) {
            print "Creating $file...\n";
            my $info = $files{$file};
            my $fh = new IO::File($file, O_WRONLY | O_CREAT, $info->{perms})
                || die $!;
            print $fh $info->{contents} if $info->{contents};
            $fh->close;
        }
    }
}

# If you ran a REALLY old version of Bugzilla, your chart files are in the
# wrong format. This code is a little messy, because it's very old, and
# when moving it into this module, I couldn't test it so I left it almost 
# completely alone.
sub _update_old_charts {
    my ($datadir) = @_;
    print "Updating old chart storage format...\n";
    foreach my $in_file (glob("$datadir/mining/*")) {
        # Don't try and upgrade image or db files!
        next if (($in_file =~ /\.gif$/i) ||
                 ($in_file =~ /\.png$/i) ||
                 ($in_file =~ /\.db$/i) ||
                 ($in_file =~ /\.orig$/i));

        rename("$in_file", "$in_file.orig") or next;
        open(IN, "$in_file.orig") or next;
        open(OUT, '>', $in_file) or next;

        # Fields in the header
        my @declared_fields;

        # Fields we changed to half way through by mistake
        # This list comes from an old version of collectstats.pl
        # This part is only for people who ran later versions of 2.11 (devel)
        my @intermediate_fields = qw(DATE UNCONFIRMED NEW ASSIGNED REOPENED
                                     RESOLVED VERIFIED CLOSED);

        # Fields we actually want (matches the current collectstats.pl)
        my @out_fields = qw(DATE NEW ASSIGNED REOPENED UNCONFIRMED RESOLVED
                            VERIFIED CLOSED FIXED INVALID WONTFIX LATER REMIND
                            DUPLICATE WORKSFORME MOVED);

         while (<IN>) {
            if (/^# fields?: (.*)\s$/) {
                @declared_fields = map uc, (split /\||\r/, $1);
                print OUT "# fields: ", join('|', @out_fields), "\n";
            }
            elsif (/^(\d+\|.*)/) {
                my @data = split(/\||\r/, $1);
                my %data;
                if (@data == @declared_fields) {
                    # old format
                    for my $i (0 .. $#declared_fields) {
                        $data{$declared_fields[$i]} = $data[$i];
                    }
                }
                elsif (@data == @intermediate_fields) {
                    # Must have changed over at this point
                    for my $i (0 .. $#intermediate_fields) {
                        $data{$intermediate_fields[$i]} = $data[$i];
                    }
                }
                elsif (@data == @out_fields) {
                    # This line's fine - it has the right number of entries
                    for my $i (0 .. $#out_fields) {
                        $data{$out_fields[$i]} = $data[$i];
                    }
                }
                else {
                    print "Oh dear, input line $. of $in_file had " .
                          scalar(@data) . " fields\nThis was unexpected.",
                          " You may want to check your data files.\n";
                }

                print OUT join('|', 
                    map { defined ($data{$_}) ? ($data{$_}) : "" } @out_fields),
                    "\n";
            }
            else {
                print OUT;
            }
        }

        close(IN);
        close(OUT);
    } 
}


sub fix_all_file_permissions {
    my ($output) = @_;

    my $ws_group = Bugzilla->localconfig->{'webservergroup'};
    my $group_id = _check_web_server_group($ws_group, $output);

    return if ON_WINDOWS;

    my $fs = FILESYSTEM();
    my %files = %{$fs->{all_files}};
    my %dirs  = %{$fs->{all_dirs}};
    my %recurse_dirs = %{$fs->{recurse_dirs}};

    print get_text('install_file_perms_fix') . "\n" if $output;

    my $owner_id = POSIX::getuid();
    $group_id = POSIX::getgid() unless defined $group_id;

    foreach my $dir (sort keys %dirs) {
        next unless -d $dir;
        _fix_perms($dir, $owner_id, $group_id, $dirs{$dir});
    }

    foreach my $dir (sort keys %recurse_dirs) {
        next unless -d $dir;
        # Set permissions on the directory itself.
        my $perms = $recurse_dirs{$dir};
        _fix_perms($dir, $owner_id, $group_id, $perms->{dirs});
        # Now recurse through the directory and set the correct permissions
        # on subdirectories and files.
        find({ no_chdir => 1, wanted => sub {
            my $name = $File::Find::name;
            if (-d $name) {
                _fix_perms($name, $owner_id, $group_id, $perms->{dirs});
            }
            else {
                _fix_perms($name, $owner_id, $group_id, $perms->{files});
            }
        }}, $dir);
    }

    foreach my $file (sort keys %files) {
        # %files supports globs
        foreach my $filename (glob $file) {
            # Don't touch directories.
            next if -d $filename || !-e $filename;
            _fix_perms($filename, $owner_id, $group_id, 
                       $files{$file}->{perms});
        }
    }

    _fix_cvs_dirs($owner_id, '.');
}

# A helper for fix_all_file_permissions
sub _fix_cvs_dirs {
    my ($owner_id, $dir) = @_;
    my $owner_gid = POSIX::getgid();
    find({ no_chdir => 1, wanted => sub {
        my $name = $File::Find::name;
        if ($File::Find::dir =~ /\/CVS/ || $_ eq '.cvsignore'
            || (-d $name && $_ eq 'CVS')) {
            _fix_perms($name, $owner_id, $owner_gid, 0700);
        }
    }}, $dir);
}

sub _fix_perms {
    my ($name, $owner, $group, $perms) = @_;
    #printf ("Changing $name to %o\n", $perms);
    chown $owner, $group, $name 
        || warn "Failed to change ownership of $name: $!";
    chmod $perms, $name
        || warn "Failed to change permissions of $name: $!";
}

sub _check_web_server_group {
    my ($group, $output) = @_;

    my $filename = bz_locations()->{'localconfig'};
    my $group_id;

    # If we are on Windows, webservergroup does nothing
    if (ON_WINDOWS && $group && $output) {
        print "\n\n" . get_text('install_webservergroup_windows') . "\n\n";
    }

    # If we're not on Windows, make sure that webservergroup isn't
    # empty.
    elsif (!ON_WINDOWS && !$group && $output) {
        print "\n\n" . get_text('install_webservergroup_empty') . "\n\n";
    }

    # If we're not on Windows, make sure we are actually a member of
    # the webservergroup.
    elsif (!ON_WINDOWS && $group) {
        $group_id = getgrnam($group);
        ThrowCodeError('invalid_webservergroup', { group => $group }) 
            unless defined $group_id;

        # If on unix, see if we need to print a warning about a webservergroup
        # that we can't chgrp to
        if ($output && $< != 0 && !grep($_ eq $group_id, split(" ", $)))) {
            print "\n\n" . get_text('install_webservergroup_not_in') . "\n\n";
        }
    }

    return $group_id;
}

1;

__END__

=head1 NAME

Bugzilla::Install::Filesystem - Fix up the filesystem during
  installation.

=head1 DESCRIPTION

This module is used primarily by L<checksetup.pl> to modify the 
filesystem during installation, including creating the data/ directory.

=head1 SUBROUTINES

=over

=item C<update_filesystem({ index_html => 0 })>

Description: Creates all the directories and files that Bugzilla
             needs to function but doesn't ship with. Also does
             any updates to these files as necessary during an
             upgrade.

Params:      C<index_html> - Whether or not we should create
               the F<index.html> file.

Returns:     nothing

=item C<create_htaccess()>

Description: Creates all of the .htaccess files for Apache,
             in the various Bugzilla directories. Also updates
             the .htaccess files if they need updating.

Params:      none

Returns:     nothing

=item C<fix_all_file_permissions($output)>

Description: Sets all the file permissions on all of Bugzilla's files
             to what they should be. Note that permissions are different
             depending on whether or not C<$webservergroup> is set
             in F<localconfig>.

Params:      C<$output> - C<true> if you want this function to print
                 out information about what it's doing.

Returns:     nothing

=back
