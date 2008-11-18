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
# The Original Code are the Bugzilla tests.
#
# The Initial Developer of the Original Code is Jacob Steenhagen.
# Portions created by Jacob Steenhagen are
# Copyright (C) 2001 Jacob Steenhagen. All
# Rights Reserved.
#
# Contributor(s): Jacob Steenhagen <jake@bugzilla.org>
#                 David D. Kilzer <ddkilzer@kilzer.net>
#                 Tobias Burnus <burnus@net-b.de>
#

package Support::Templates;

use strict;

use lib 't';
use base qw(Exporter);
@Support::Templates::EXPORT = 
         qw(@languages @include_paths %include_path @referenced_files 
            %actual_files $num_actual_files);
use vars qw(@languages @include_paths %include_path @referenced_files 
            %actual_files $num_actual_files);

use Support::Files;

use File::Find;
use File::Spec;

# The available template languages
@languages = ();

# The colon separated includepath per language
%include_path = ();

# All include paths
@include_paths = ();

# Files which are referenced in the cgi files
@referenced_files = ();

# All files sorted by include_path
%actual_files = ();

# total number of actual_files
$num_actual_files = 0;

# Scan for the template available languages and include paths
{
    opendir(DIR, "template") || die "Can't open  'template': $!";
    my @files = grep { /^[a-z-]+$/i } readdir(DIR);
    closedir DIR;

    foreach my $langdir (@files) {
        next if($langdir =~ /^CVS$/i);

        my $path = File::Spec->catdir('template', $langdir, 'custom');
        my @dirs = ();
        push(@dirs, $path) if(-d $path);
        $path = File::Spec->catdir('template', $langdir, 'extension');
        push(@dirs, $path) if(-d $path);
        $path = File::Spec->catdir('template', $langdir, 'default');
        push(@dirs, $path) if(-d $path);

        next if(scalar(@dirs) == 0);
        push(@languages, $langdir);
        push(@include_paths, @dirs);
        $include_path{$langdir} = join(":",@dirs);
    }
}


my @files;

# Local subroutine used with File::Find
sub find_templates {
    # Prune CVS directories
    if (-d $_ && $_ eq 'CVS') {
        $File::Find::prune = 1;
        return;
    }

    # Only include files ending in '.tmpl'
    if (-f $_ && $_ =~ m/\.tmpl$/i) {
        my $filename;
        my $local_dir = File::Spec->abs2rel($File::Find::dir,
                                            $File::Find::topdir);

        # File::Spec 3.13 and newer return "." instead of "" if both
        # arguments of abs2rel() are identical.
        $local_dir = "" if ($local_dir eq ".");

        if ($local_dir) {
            $filename = File::Spec->catfile($local_dir, $_);
        } else {
            $filename = $_;
        }

        push(@files, $filename);
    }
}

# Scan the given template include path for templates
sub find_actual_files {
  my $include_path = $_[0];
  @files = ();
  find(\&find_templates, $include_path);
  return @files;
}


foreach my $include_path (@include_paths) {
  $actual_files{$include_path} = [ find_actual_files($include_path) ];
  $num_actual_files += scalar(@{$actual_files{$include_path}});
}

# Scan Bugzilla's perl code looking for templates used and put them
# in the @referenced_files array to be used by the 004template.t test.
my %seen;

foreach my $file (@Support::Files::testitems) {
    open (FILE, $file);
    my @lines = <FILE>;
    close (FILE);
    foreach my $line (@lines) {
        if ($line =~ m/template->process\(\"(.+?)\", .+?\)/) {
            my $template = $1;
            # Ignore templates with $ in the name, since they're
            # probably vars, not real files
            next if $template =~ m/\$/;
            next if $seen{$template};
            push (@referenced_files, $template);
            $seen{$template} = 1;
        }
    }
}

1;
