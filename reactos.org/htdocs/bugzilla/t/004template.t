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
#                 Zach Lipton <zach@zachlipton.com>
#                 David D. Kilzer <ddkilzer@kilzer.net>
#                 Tobias Burnus <burnus@net-b.de>
#

#################
#Bugzilla Test 4#
####Templates####

use strict;

use lib 't';

use Support::Templates;

# Bug 137589 - Disable command-line input of CGI.pm when testing
use CGI qw(-no_debug);

use File::Spec;
use Template;
use Test::More tests => ( scalar(@referenced_files) * scalar(@languages)
                        + $num_actual_files * 2 );

# Capture the TESTOUT from Test::More or Test::Builder for printing errors.
# This will handle verbosity for us automatically.
my $fh;
{
    local $^W = 0;  # Don't complain about non-existent filehandles
    if (-e \*Test::More::TESTOUT) {
        $fh = \*Test::More::TESTOUT;
    } elsif (-e \*Test::Builder::TESTOUT) {
        $fh = \*Test::Builder::TESTOUT;
    } else {
        $fh = \*STDOUT;
    }
}

# Checks whether one of the passed files exists
sub existOnce {
  foreach my $file (@_) {
    return $file  if -e $file;
  }
  return 0;
}

# Check to make sure all templates that are referenced in
# Bugzilla exist in the proper place.

foreach my $lang (@languages) {
    foreach my $file (@referenced_files) {
        my @path = map(File::Spec->catfile($_, $file),
                       split(':', $include_path{$lang} . ":" . $include_path{"en"}));
        if (my $path = existOnce(@path)) {
            ok(1, "$path exists");
        } else {
            ok(0, "$file cannot be located --ERROR");
            print $fh "Looked in:\n  " . join("\n  ", @path) . "\n";
        }
    }
}

foreach my $include_path (@include_paths) {
    # Processes all the templates to make sure they have good syntax
    my $provider = Template::Provider->new(
    {
        INCLUDE_PATH => $include_path ,
        # Need to define filters used in the codebase, they don't
        # actually have to function in this test, just be defined.
        # See Template.pm for the actual codebase definitions.

        # Initialize templates (f.e. by loading plugins like Hook).
        PRE_PROCESS => "global/initialize.none.tmpl",

        FILTERS =>
        {
            html_linebreak => sub { return $_; },
            no_break => sub { return $_; } ,
            js        => sub { return $_ } ,
            base64   => sub { return $_ } ,
            inactive => [ sub { return sub { return $_; } }, 1] ,
            closed => [ sub { return sub { return $_; } }, 1] ,
            obsolete => [ sub { return sub { return $_; } }, 1] ,
            url_quote => sub { return $_ } ,
            css_class_quote => sub { return $_ } ,
            xml       => sub { return $_ } ,
            quoteUrls => sub { return $_ } ,
            bug_link => [ sub { return sub { return $_; } }, 1] ,
            csv       => sub { return $_ } ,
            unitconvert => sub { return $_ },
            time      => sub { return $_ } ,
            wrap_comment => sub { return $_ },
            none      => sub { return $_ } ,
            ics       => [ sub { return sub { return $_; } }, 1] ,
        },
    }
    );

    foreach my $file (@{$actual_files{$include_path}}) {
        my $path = File::Spec->catfile($include_path, $file);
        if (-e $path) {
            my ($data, $err) = $provider->fetch($file);

            if (!$err) {
                ok(1, "$file syntax ok");
            }
            else {
                ok(0, "$file has bad syntax --ERROR");
                print $fh $data . "\n";
            }
        }
        else {
            ok(1, "$path doesn't exist, skipping test");
        }
    }

    # check to see that all templates have a version string:

    foreach my $file (@{$actual_files{$include_path}}) {
        my $path = File::Spec->catfile($include_path, $file);
        open(TMPL, $path);
        my $firstline = <TMPL>;
        if ($firstline =~ /\d+\.\d+\@[\w\.-]+/) {
            ok(1,"$file has a version string");
        } else {
            ok(0,"$file does not have a version string --ERROR");
        }
        close(TMPL);
    }
}

exit 0;
