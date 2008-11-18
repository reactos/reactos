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
#                 Andreas Franke <afranke@mathweb.org>
#                 Christian Reis <kiko@async.com.br>
#                 Myk Melez <myk@mozilla.org>
#                 Frédéric Buclin <LpSolit@gmail.com>

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Error;
use Bugzilla::Bug;

use List::Util qw(max);

my $user = Bugzilla->login();

my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;
my $vars = {};
# Connect to the shadow database if this installation is using one to improve
# performance.
my $dbh = Bugzilla->switch_to_shadow_db();

################################################################################
# Data/Security Validation                                                     #
################################################################################

# Make sure the bug ID is a positive integer representing an existing
# bug that the user is authorized to access.
my $id = $cgi->param('id') || ThrowUserError('invalid_bug_id_or_alias');
ValidateBugID($id);
my $current_bug = new Bugzilla::Bug($id);

local our $hide_resolved = $cgi->param('hide_resolved') ? 1 : 0;

local our $maxdepth = $cgi->param('maxdepth') || 0;
if ($maxdepth !~ /^\d+$/) { $maxdepth = 0 };

################################################################################
# Main Section                                                                 #
################################################################################

# Stores the greatest depth to which either tree goes.
local our $realdepth = 0;

# Generate the tree of bugs that this bug depends on and a list of IDs
# appearing in the tree.
my $dependson_tree = { $id => $current_bug };
my $dependson_ids = {};
GenerateTree($id, "dependson", 1, $dependson_tree, $dependson_ids);
$vars->{'dependson_tree'} = $dependson_tree;
$vars->{'dependson_ids'} = [keys(%$dependson_ids)];

# Generate the tree of bugs that this bug blocks and a list of IDs
# appearing in the tree.
my $blocked_tree = { $id => $current_bug };
my $blocked_ids = {};
GenerateTree($id, "blocked", 1, $blocked_tree, $blocked_ids);
$vars->{'blocked_tree'} = $blocked_tree;
$vars->{'blocked_ids'} = [keys(%$blocked_ids)];

$vars->{'realdepth'}      = $realdepth;

$vars->{'bugid'}          = $id;
$vars->{'maxdepth'}       = $maxdepth;
$vars->{'hide_resolved'}  = $hide_resolved;

print $cgi->header();
$template->process("bug/dependency-tree.html.tmpl", $vars)
  || ThrowTemplateError($template->error());

################################################################################
# Recursive Tree Generation Function                                           #
################################################################################

sub GenerateTree {
    # Generates a dependency tree for a given bug.  Calls itself recursively
    # to generate sub-trees for the bug's dependencies.
    my ($bug_id, $relationship, $depth, $bugs, $ids) = @_;

    my @dependencies;
    if ($relationship eq 'dependson') {
        @dependencies = @{$bugs->{$bug_id}->dependson};
    }
    else {
        @dependencies = @{$bugs->{$bug_id}->blocked};
    }

    # Don't do anything if this bug doesn't have any dependencies.
    return unless scalar(@dependencies);

    # Record this depth in the global $realdepth variable if it's farther 
    # than we've gone before.
    $realdepth = max($realdepth, $depth);

    foreach my $dep_id (@dependencies) {
        # Get this dependency's record from the database and generate
        # its sub-tree if we haven't already done so (which happens
        # when bugs appear in dependency trees multiple times).
        if (!$bugs->{$dep_id}) {
            $bugs->{$dep_id} = new Bugzilla::Bug($dep_id);
            GenerateTree($dep_id, $relationship, $depth+1, $bugs, $ids);
        }

        # Add this dependency to the list of this bug's dependencies 
        # if it exists, if we haven't exceeded the maximum depth the user 
        # wants the tree to go, and if the dependency isn't resolved 
        # (if we're ignoring resolved dependencies).
        if (!$bugs->{$dep_id}->{'error'}
            && Bugzilla->user->can_see_bug($dep_id)
            && (!$maxdepth || $depth <= $maxdepth) 
            && ($bugs->{$dep_id}->{'isopened'} || !$hide_resolved))
        {
            # Due to AUTOLOAD in Bug.pm, we cannot add 'dependencies'
            # as a bug object attribute from here.
            push(@{$bugs->{'dependencies'}->{$bug_id}}, $dep_id);
            $ids->{$dep_id} = 1;
        }
    }
}
