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
# The Initial Developer of the Original Code is Everything Solved.
# Portions created by Everything Solved are Copyright (C) 2006
# Everything Solved. All Rights Reserved.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# Contributor(s): Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::Install::Localconfig;

# NOTE: This package may "use" any modules that it likes. However,
# all functions in this package should assume that:
#
# * The data/ directory does not exist.
# * Templates are not available.
# * Files do not have the correct permissions
# * The database is not up to date

use strict;

use Bugzilla::Constants;

use Data::Dumper;
use IO::File;
use Safe;

use base qw(Exporter);

our @EXPORT_OK = qw(
    read_localconfig
    update_localconfig
);

use constant LOCALCONFIG_VARS => (
    {
        name    => 'create_htaccess',
        default => 1,
        desc    => <<EOT
# If you are using Apache as your web server, Bugzilla can create .htaccess
# files for you that will instruct Apache not to serve files that shouldn't
# be accessed from the web (like your local configuration data and non-cgi
# executable files).  For this to work, the directory your Bugzilla
# installation is in must be within the jurisdiction of a <Directory> block
# in the httpd.conf file that has 'AllowOverride Limit' in it.  If it has
# 'AllowOverride All' or other options with Limit, that's fine.
# (Older Apache installations may use an access.conf file to store these
# <Directory> blocks.)
# If this is set to 1, Bugzilla will create these files if they don't exist.
# If this is set to 0, Bugzilla will not create these files.
EOT
    },
    {
        name    => 'webservergroup',
        default => ON_WINDOWS ? '' : 'apache',
        desc    => q{# This is the group your web server runs as.
# If you have a Windows box, ignore this setting.
# If you do not have access to the group your web server runs under,
# set this to "". If you do set this to "", then your Bugzilla installation
# will be _VERY_ insecure, because some files will be world readable/writable,
# and so anyone who can get local access to your machine can do whatever they
# want. You should only have this set to "" if this is a testing installation
# and you cannot set this up any other way. YOU HAVE BEEN WARNED!
# If you set this to anything other than "", you will need to run checksetup.pl
# as} . ROOT_USER . qq{, or as a user who is a member of the specified group.\n}
    },
    {
        name    => 'db_driver',
        default => 'mysql',
        desc    => <<EOT
# What SQL database to use. Default is mysql. List of supported databases
# can be obtained by listing Bugzilla/DB directory - every module corresponds
# to one supported database and the name corresponds to a driver name.
EOT
    },
    {
        name    => 'db_host',
        default => 'localhost',
        desc    => 
            "# The DNS name of the host that the database server runs on.\n"
    },
    {
        name    => 'db_name',
        default => 'bugs',
        desc    => "# The name of the database\n"
    },
    {
        name    => 'db_user',
        default => 'bugs',
        desc    => "# Who we connect to the database as.\n"
    },
    {
        name    => 'db_pass',
        default => '',
        desc    => <<EOT
# Enter your database password here. It's normally advisable to specify
# a password for your bugzilla database user.
# If you use apostrophe (') or a backslash (\\) in your password, you'll
# need to escape it by preceding it with a '\\' character. (\\') or (\\)
# (Far simpler just not to use those characters.)
EOT
    },
    {
        name    => 'db_port',
        default => 0,
        desc    => <<EOT
# Sometimes the database server is running on a non-standard port. If that's
# the case for your database server, set this to the port number that your
# database server is running on. Setting this to 0 means "use the default
# port for my database server."
EOT
    },
    {
        name    => 'db_sock',
        default => '',
        desc    => <<EOT
# MySQL Only: Enter a path to the unix socket for MySQL. If this is
# blank, then MySQL's compiled-in default will be used. You probably
# want that.
EOT
    },
    {
        name    => 'db_check',
        default => 1,
        desc    => <<EOT
# Should checksetup.pl try to verify that your database setup is correct?
# (with some combinations of database servers/Perl modules/moonphase this
# doesn't work)
EOT
    },
    {
        name    => 'index_html',
        default => 0,
        desc    => <<EOT
# With the introduction of a configurable index page using the
# template toolkit, Bugzilla's main index page is now index.cgi.
# Most web servers will allow you to use index.cgi as a directory
# index, and many come preconfigured that way, but if yours doesn't
# then you'll need an index.html file that provides redirection
# to index.cgi. Setting \$index_html to 1 below will allow
# checksetup.pl to create one for you if it doesn't exist.
# NOTE: checksetup.pl will not replace an existing file, so if you
#       wish to have checksetup.pl create one for you, you must
#       make sure that index.html doesn't already exist
EOT
    },
    {
        name    => 'cvsbin',
        default => \&_get_default_cvsbin,
        desc    => <<EOT
# For some optional functions of Bugzilla (such as the pretty-print patch
# viewer), we need the cvs binary to access files and revisions.
# Because it's possible that this program is not in your path, you can specify
# its location here.  Please specify the full path to the executable.
EOT
    },
    {
        name    => 'interdiffbin',
        default => \&_get_default_interdiffbin,
        desc    => <<EOT
# For some optional functions of Bugzilla (such as the pretty-print patch
# viewer), we need the interdiff binary to make diffs between two patches.
# Because it's possible that this program is not in your path, you can specify
# its location here.  Please specify the full path to the executable.
EOT
    },
    {
        name    => 'diffpath',
        default => \&_get_default_diffpath,
        desc    => <<EOT
# The interdiff feature needs diff, so we have to have that path.
# Please specify the directory name only; do not use trailing slash.
EOT
    },
);

use constant OLD_LOCALCONFIG_VARS => qw(
    mysqlpath
    contenttypes
    pages
    severities platforms opsys priorities
);

sub read_localconfig {
    my ($include_deprecated) = @_;
    my $filename = bz_locations()->{'localconfig'};

    my %localconfig;
    if (-e $filename) {
        my $s = new Safe;
        # Some people like to store their database password in another file.
        $s->permit('dofile');

        $s->rdo($filename);
        if ($@ || $!) {
            my $err_msg = $@ ? $@ : $!;
            die <<EOT;
An error has occurred while reading your 'localconfig' file.  The text of 
the error message is:

$err_msg

Please fix the error in your 'localconfig' file. Alternately, rename your
'localconfig' file, rerun checksetup.pl, and re-enter your answers.

  \$ mv -f localconfig localconfig.old
  \$ ./checksetup.pl
EOT
        }

        my @vars = map($_->{name}, LOCALCONFIG_VARS);
        push(@vars, OLD_LOCALCONFIG_VARS) if $include_deprecated;
        foreach my $var (@vars) {
            my $glob = $s->varglob($var);
            # We can't get the type of a variable out of a Safe automatically.
            # We can only get the glob itself. So we figure out its type this
            # way, by trying first a scalar, then an array, then a hash.
            #
            # The interesting thing is that this converts all deprecated 
            # array or hash vars into hashrefs or arrayrefs, but that's 
            # fine since as I write this all modern localconfig vars are 
            # actually scalars.
            if (defined $$glob) {
                $localconfig{$var} = $$glob;
            }
            elsif (defined @$glob) {
                $localconfig{$var} = \@$glob;
            }
            elsif (defined %$glob) {
                $localconfig{$var} = \%$glob;
            }
        }
    }

    return \%localconfig;
}


#
# This is quite tricky. But fun!
#
# First we read the file 'localconfig'. Then we check if the variables we
# need are defined. If not, we will append the new settings to
# localconfig, instruct the user to check them, and stop.
#
# Why do it this way?
#
# Assume we will enhance Bugzilla and eventually more local configuration
# stuff arises on the horizon.
#
# But the file 'localconfig' is not in the Bugzilla CVS or tarfile. You
# know, we never want to overwrite your own version of 'localconfig', so
# we can't put it into the CVS/tarfile, can we?
#
# Now, when we need a new variable, we simply add the necessary stuff to
# LOCALCONFIG_VARS. When the user gets the new version of Bugzilla from CVS and
# runs checksetup, it finds out "Oh, there is something new". Then it adds
# some default value to the user's local setup and informs the user to
# check that to see if it is what the user wants.
#
# Cute, ey?
#
sub update_localconfig {
    my ($params) = @_;

    my $output      = $params->{output} || 0;
    my $answer      = Bugzilla->installation_answers;
    my $localconfig = read_localconfig('include deprecated');

    my @new_vars;
    foreach my $var (LOCALCONFIG_VARS) {
        my $name = $var->{name};
        if (!defined $localconfig->{$name}) {
            push(@new_vars, $name);
            $var->{default} = &{$var->{default}} if ref($var->{default}) eq 'CODE';
            $localconfig->{$name} = $answer->{$name} || $var->{default};
        }
    }

    my @old_vars;
    foreach my $name (OLD_LOCALCONFIG_VARS) {
        push(@old_vars, $name) if defined $localconfig->{$name};
    }

    if (!$localconfig->{'interdiffbin'} && $output) {
        print <<EOT

OPTIONAL NOTE: If you want to be able to use the 'difference between two
patches' feature of Bugzilla (which requires the PatchReader Perl module
as well), you should install patchutils from:

    http://cyberelk.net/tim/patchutils/

EOT
    }

    my $filename = bz_locations->{'localconfig'};

    if (scalar @old_vars) {
        my $oldstuff = join(', ', @old_vars);
        print <<EOT

The following variables are no longer used in $filename, and
should be removed: $oldstuff

EOT
    }

    if (scalar @new_vars) {
        my $filename = bz_locations->{'localconfig'};
        my $fh = new IO::File($filename, '>>') || die "$filename: $!";
        $fh->seek(0, SEEK_END);
        foreach my $var (LOCALCONFIG_VARS) {
            if (grep($_ eq $var->{name}, @new_vars)) {
                print $fh "\n", $var->{desc},
                      Data::Dumper->Dump([$localconfig->{$var->{name}}], 
                                         ["*$var->{name}"]);
            }
        }

        my $newstuff = join(', ', @new_vars);
        print <<EOT;

This version of Bugzilla contains some variables that you may want to 
change and adapt to your local settings. Please edit the file 
$filename and rerun checksetup.pl.

The following variables are new to $filename since you last ran
checksetup.pl:  $newstuff

EOT
        exit;
    }

    # Reset the cache for Bugzilla->localconfig so that it will be re-read
    delete Bugzilla->request_cache->{localconfig};

    return { old_vars => \@old_vars, new_vars => \@new_vars };
}

sub _get_default_cvsbin {
    return '' if ON_WINDOWS;

    my $cvs_executable = `which cvs`;
    if ($cvs_executable =~ /no cvs/ || $cvs_executable eq '') {
        # If which didn't find it, just set to blank
        $cvs_executable = "";
    } else {
        chomp $cvs_executable;
    }
    return $cvs_executable;
}

sub _get_default_interdiffbin {
    return '' if ON_WINDOWS;

    my $interdiff = `which interdiff`;
    if ($interdiff =~ /no interdiff/ || $interdiff eq '') {
        # If which didn't find it, just set to blank
        $interdiff = '';
    } else {
        chomp $interdiff;
    }
    return $interdiff;
}

sub _get_default_diffpath {
    return '' if ON_WINDOWS;

    my $diff_binaries;
    $diff_binaries = `which diff`;
    if ($diff_binaries =~ /no diff/ || $diff_binaries eq '') {
        # If which didn't find it, set to blank
        $diff_binaries = "";
    } else {
        $diff_binaries =~ s:/diff\n$::;
    }
    return $diff_binaries;
}

1;

__END__

=head1 NAME

Bugzilla::Install::Localconfig - Functions and variables dealing
  with the manipulation and creation of the F<localconfig> file.

=head1 SYNOPSIS

 use Bugzilla::Install::Requirements qw(update_localconfig);
 update_localconfig({ output => 1 });

=head1 DESCRIPTION

This module is used primarily by L<checksetup.pl> to create and
modify the localconfig file. Most scripts should use L<Bugzilla/localconfig>
to access localconfig variables.

=head1 CONSTANTS

=over

=item C<LOCALCONFIG_VARS>

An array of hashrefs. These hashrefs contain three keys:

 name    - The name of the variable.
 default - The default value for the variable. Should always be
           something that can fit in a scalar.
 desc    - Additional text to put in localconfig before the variable
           definition. Must end in a newline. Each line should start
           with "#" unless you have some REALLY good reason not
           to do that.

=item C<OLD_LOCALCONFIG_VARS>

An array of names of variables. If C<update_localconfig> finds these
variables defined in localconfig, it will print out a warning.

=back

=head1 SUBROUTINES

=over

=item C<read_localconfig($include_deprecated)>

Description: Reads the localconfig file and returns all valid
             values in a hashref.

Params:      C<$include_deprecated> - C<true> if you want the returned
                 hashref to also include variables listed in 
                 C<OLD_LOCALCONFIG_VARS>, if they exist. Generally
                 this is only for use by C<update_localconfig>.

Returns:     A hashref of the localconfig variables. If an array
             is defined, it will be an arrayref in the returned hash. If a
             hash is defined, it will be a hashref in the returned hash.
             Only includes variables specified in C<LOCALCONFIG_VARS>
             (and C<OLD_LOCALCONFIG_VARS> if C<$include_deprecated> is
             specified).

=item C<update_localconfig({ output =E<gt> 1 })>

Description: Adds any new variables to localconfig that aren't
             currently defined there. Also optionally prints out
             a message about vars that *should* be there and aren't.
             Exits the program if it adds any new vars.

Params:      C<output> - C<true> if the function should display informational
                 output and warnings. It will always display errors or
                 any message which would cause program execution to halt.

Returns:     A hashref, with C<old_vals> being an array of names of variables
             that were removed, and C<new_vals> being an array of names
             of variables that were added to localconfig.

=back
