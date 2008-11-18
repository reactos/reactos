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
#                 Gervase Markham <gerv@gerv.net>

use strict;

use lib qw(.);

use File::Temp;

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Bug;

Bugzilla->login();

my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;
my $vars = {};
# Connect to the shadow database if this installation is using one to improve
# performance.
my $dbh = Bugzilla->switch_to_shadow_db();

local our (%seen, %edgesdone, %bugtitles);

# CreateImagemap: This sub grabs a local filename as a parameter, reads the 
# dot-generated image map datafile residing in that file and turns it into
# an HTML map element. THIS SUB IS ONLY USED FOR LOCAL DOT INSTALLATIONS.
# The map datafile won't necessarily contain the bug summaries, so we'll
# pull possible HTML titles from the %bugtitles hash (filled elsewhere
# in the code)

# The dot mapdata lines have the following format (\nsummary is optional):
# rectangle (LEFTX,TOPY) (RIGHTX,BOTTOMY) URLBASE/show_bug.cgi?id=BUGNUM BUGNUM[\nSUMMARY]

sub CreateImagemap {
    my $mapfilename = shift;
    my $map = "<map name=\"imagemap\">\n";
    my $default;

    open MAP, "<$mapfilename";
    while(my $line = <MAP>) {
        if($line =~ /^default ([^ ]*)(.*)$/) {
            $default = qq{<area alt="" shape="default" href="$1">\n};
        }

        if ($line =~ /^rectangle \((.*),(.*)\) \((.*),(.*)\) (http[^ ]*) (\d+)(\\n.*)?$/) {
            my ($leftx, $rightx, $topy, $bottomy, $url, $bugid) = ($1, $3, $2, $4, $5, $6);

            # Pick up bugid from the mapdata label field. Getting the title from
            # bugtitle hash instead of mapdata allows us to get the summary even
            # when showsummary is off, and also gives us status and resolution.
            my $bugtitle = value_quote($bugtitles{$bugid});
            $map .= qq{<area alt="bug $bugid" name="bug$bugid" shape="rect" } .
                    qq{title="$bugtitle" href="$url" } .
                    qq{coords="$leftx,$topy,$rightx,$bottomy">\n};
        }
    }
    close MAP;

    $map .= "$default</map>";
    return $map;
}

sub AddLink {
    my ($blocked, $dependson, $fh) = (@_);
    my $key = "$blocked,$dependson";
    if (!exists $edgesdone{$key}) {
        $edgesdone{$key} = 1;
        print $fh "$blocked -> $dependson\n";
        $seen{$blocked} = 1;
        $seen{$dependson} = 1;
    }
}

# The list of valid directions. Some are not proposed in the dropdrown
# menu despite they are valid ones.
my @valid_rankdirs = ('LR', 'RL', 'TB', 'BT');

my $rankdir = $cgi->param('rankdir') || "LR";
# Make sure the submitted 'rankdir' value is valid.
if (lsearch(\@valid_rankdirs, $rankdir) < 0) {
    $rankdir = 'LR';
}

my $webdotdir = bz_locations()->{'webdotdir'};

if (!defined $cgi->param('id') && !defined $cgi->param('doall')) {
    ThrowCodeError("missing_bug_id");
}

my ($fh, $filename) = File::Temp::tempfile("XXXXXXXXXX",
                                           SUFFIX => '.dot',
                                           DIR => $webdotdir);
my $urlbase = Bugzilla->params->{'urlbase'};

print $fh "digraph G {";
print $fh qq{
graph [URL="${urlbase}query.cgi", rankdir=$rankdir]
node [URL="${urlbase}show_bug.cgi?id=\\N", style=filled, color=lightgrey]
};

my %baselist;

if ($cgi->param('doall')) {
    my $dependencies = $dbh->selectall_arrayref(
                           "SELECT blocked, dependson FROM dependencies");

    foreach my $dependency (@$dependencies) {
        my ($blocked, $dependson) = @$dependency;
        AddLink($blocked, $dependson, $fh);
    }
} else {
    foreach my $i (split('[\s,]+', $cgi->param('id'))) {
        $i = trim($i);
        ValidateBugID($i);
        $baselist{$i} = 1;
    }

    my @stack = keys(%baselist);
    my $sth = $dbh->prepare(
                  q{SELECT blocked, dependson
                      FROM dependencies
                     WHERE blocked = ? or dependson = ?});
    foreach my $id (@stack) {
        my $dependencies = $dbh->selectall_arrayref($sth, undef, ($id, $id));
        foreach my $dependency (@$dependencies) {
            my ($blocked, $dependson) = @$dependency;
            if ($blocked != $id && !exists $seen{$blocked}) {
                push @stack, $blocked;
            }

            if ($dependson != $id && !exists $seen{$dependson}) {
                push @stack, $dependson;
            }

            AddLink($blocked, $dependson, $fh);
        }
    }

    foreach my $k (keys(%baselist)) {
        $seen{$k} = 1;
    }
}

my $sth = $dbh->prepare(
              q{SELECT bug_status, resolution, short_desc
                  FROM bugs
                 WHERE bugs.bug_id = ?});
foreach my $k (keys(%seen)) {
    # Retrieve bug information from the database
    my ($stat, $resolution, $summary) = $dbh->selectrow_array($sth, undef, $k);
    $stat ||= 'NEW';
    $resolution ||= '';
    $summary ||= '';

    # Resolution and summary are shown only if user can see the bug
    if (!Bugzilla->user->can_see_bug($k)) {
        $resolution = $summary = '';
    }

    $vars->{'short_desc'} = $summary if ($k eq $cgi->param('id'));

    my @params;

    if ($summary ne "" && $cgi->param('showsummary')) {
        $summary =~ s/([\\\"])/\\$1/g;
        push(@params, qq{label="$k\\n$summary"});
    }

    if (exists $baselist{$k}) {
        push(@params, "shape=box");
    }

    if (is_open_state($stat)) {
        push(@params, "color=green");
    }

    if (@params) {
        print $fh "$k [" . join(',', @params) . "]\n";
    } else {
        print $fh "$k\n";
    }

    # Push the bug tooltip texts into a global hash so that 
    # CreateImagemap sub (used with local dot installations) can
    # use them later on.
    $bugtitles{$k} = trim("$stat $resolution");

    # Show the bug summary in tooltips only if not shown on 
    # the graph and it is non-empty (the user can see the bug)
    if (!$cgi->param('showsummary') && $summary ne "") {
        $bugtitles{$k} .= " - $summary";
    }
}


print $fh "}\n";
close $fh;

chmod 0777, $filename;

my $webdotbase = Bugzilla->params->{'webdotbase'};

if ($webdotbase =~ /^https?:/) {
     # Remote dot server
     my $url = perform_substs($webdotbase) . $filename;
     $vars->{'image_url'} = $url . ".gif";
     $vars->{'map_url'} = $url . ".map";
} else {
    # Local dot installation

    # First, generate the png image file from the .dot source

    my ($pngfh, $pngfilename) = File::Temp::tempfile("XXXXXXXXXX",
                                                     SUFFIX => '.png',
                                                     DIR => $webdotdir);
    binmode $pngfh;
    open(DOT, "\"$webdotbase\" -Tpng $filename|");
    binmode DOT;
    print $pngfh $_ while <DOT>;
    close DOT;
    close $pngfh;
    
    # On Windows $pngfilename will contain \ instead of /
    $pngfilename =~ s|\\|/|g if $^O eq 'MSWin32';

    # Under mod_perl, pngfilename will have an absolute path, and we
    # need to make that into a relative path.
    my $cgi_root = bz_locations()->{cgi_path};
    $pngfilename =~ s#^\Q$cgi_root\E/?##;
    
    $vars->{'image_url'} = $pngfilename;

    # Then, generate a imagemap datafile that contains the corner data
    # for drawn bug objects. Pass it on to CreateImagemap that
    # turns this monster into html.

    my ($mapfh, $mapfilename) = File::Temp::tempfile("XXXXXXXXXX",
                                                     SUFFIX => '.map',
                                                     DIR => $webdotdir);
    binmode $mapfh;
    open(DOT, "\"$webdotbase\" -Tismap $filename|");
    binmode DOT;
    print $mapfh $_ while <DOT>;
    close DOT;
    close $mapfh;
    $vars->{'image_map'} = CreateImagemap($mapfilename);
}

# Cleanup any old .dot files created from previous runs.
my $since = time() - 24 * 60 * 60;
# Can't use glob, since even calling that fails taint checks for perl < 5.6
opendir(DIR, $webdotdir);
my @files = grep { /\.dot$|\.png$|\.map$/ && -f "$webdotdir/$_" } readdir(DIR);
closedir DIR;
foreach my $f (@files)
{
    $f = "$webdotdir/$f";
    # Here we are deleting all old files. All entries are from the
    # $webdot directory. Since we're deleting the file (not following
    # symlinks), this can't escape to delete anything it shouldn't
    # (unless someone moves the location of $webdotdir, of course)
    trick_taint($f);
    if (file_mod_time($f) < $since) {
        unlink $f;
    }
}

# Make sure we only include valid integers (protects us from XSS attacks).
my @bugs = grep(detaint_natural($_), split(/[\s,]+/, $cgi->param('id')));
$vars->{'bug_id'} = join(', ', @bugs);
$vars->{'multiple_bugs'} = ($cgi->param('id') =~ /[ ,]/);
$vars->{'doall'} = $cgi->param('doall');
$vars->{'rankdir'} = $rankdir;
$vars->{'showsummary'} = $cgi->param('showsummary');

# Generate and return the UI (HTML page) from the appropriate template.
print $cgi->header();
$template->process("bug/dependency-graph.html.tmpl", $vars)
  || ThrowTemplateError($template->error());
