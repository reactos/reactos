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
# Contributor(s): Gervase Markham <gerv@gerv.net>
#                 <rdean@cambianetworks.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Field;

my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;
my $vars = {};
my $buffer = $cgi->query_string();

# Go straight back to query.cgi if we are adding a boolean chart.
if (grep(/^cmd-/, $cgi->param())) {
    my $params = $cgi->canonicalise_query("format", "ctype");
    my $location = "query.cgi?format=" . $cgi->param('query_format') . 
      ($params ? "&$params" : "");

    print $cgi->redirect($location);
    exit;
}

use Bugzilla::Search;

Bugzilla->login();

my $dbh = Bugzilla->switch_to_shadow_db();

my $action = $cgi->param('action') || 'menu';

if ($action eq "menu") {
    # No need to do any searching in this case, so bail out early.
    print $cgi->header();
    $template->process("reports/menu.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
    exit;
}

my $col_field = $cgi->param('x_axis_field') || '';
my $row_field = $cgi->param('y_axis_field') || '';
my $tbl_field = $cgi->param('z_axis_field') || '';

if (!($col_field || $row_field || $tbl_field)) {
    ThrowUserError("no_axes_defined");
}

my $width = $cgi->param('width');
my $height = $cgi->param('height');

if (defined($width)) {
   (detaint_natural($width) && $width > 0)
     || ThrowCodeError("invalid_dimensions");
   $width <= 2000 || ThrowUserError("chart_too_large");
}

if (defined($height)) {
   (detaint_natural($height) && $height > 0)
     || ThrowCodeError("invalid_dimensions");
   $height <= 2000 || ThrowUserError("chart_too_large");
}

# These shenanigans are necessary to make sure that both vertical and 
# horizontal 1D tables convert to the correct dimension when you ask to
# display them as some sort of chart.
if (defined $cgi->param('format') && $cgi->param('format') eq "table") {
    if ($col_field && !$row_field) {    
        # 1D *tables* should be displayed vertically (with a row_field only)
        $row_field = $col_field;
        $col_field = '';
    }
}
else {
    if ($row_field && !$col_field) {
        # 1D *charts* should be displayed horizontally (with an col_field only)
        $col_field = $row_field;
        $row_field = '';
    }
}

my %columns;
$columns{'bug_severity'}     = "bugs.bug_severity";        
$columns{'priority'}         = "bugs.priority";
$columns{'rep_platform'}     = "bugs.rep_platform";
$columns{'assigned_to'}      = "map_assigned_to.login_name";
$columns{'reporter'}         = "map_reporter.login_name";
$columns{'qa_contact'}       = "map_qa_contact.login_name";
$columns{'bug_status'}       = "bugs.bug_status";
$columns{'resolution'}       = "bugs.resolution";
$columns{'component'}        = "map_components.name";
$columns{'product'}          = "map_products.name";
$columns{'classification'}   = "map_classifications.name";
$columns{'version'}          = "bugs.version";
$columns{'op_sys'}           = "bugs.op_sys";
$columns{'votes'}            = "bugs.votes";
$columns{'keywords'}         = "bugs.keywords";
$columns{'target_milestone'} = "bugs.target_milestone";
# One which means "nothing". Any number would do, really. It just gets SELECTed
# so that we always select 3 items in the query.
$columns{''}                 = "42217354";

# Validate the values in the axis fields or throw an error.
!$row_field 
  || ($columns{$row_field} && trick_taint($row_field))
  || ThrowCodeError("report_axis_invalid", {fld => "x", val => $row_field});
!$col_field 
  || ($columns{$col_field} && trick_taint($col_field))
  || ThrowCodeError("report_axis_invalid", {fld => "y", val => $col_field});
!$tbl_field 
  || ($columns{$tbl_field} && trick_taint($tbl_field))
  || ThrowCodeError("report_axis_invalid", {fld => "z", val => $tbl_field});

my @axis_fields = ($row_field, $col_field, $tbl_field);
my @selectnames = map($columns{$_}, @axis_fields);

# Clone the params, so that Bugzilla::Search can modify them
my $params = new Bugzilla::CGI($cgi);
my $search = new Bugzilla::Search('fields' => \@selectnames, 
                                  'params' => $params);
my $query = $search->getSQL();

$::SIG{TERM} = 'DEFAULT';
$::SIG{PIPE} = 'DEFAULT';

my $results = $dbh->selectall_arrayref($query);

# We have a hash of hashes for the data itself, and a hash to hold the 
# row/col/table names.
my %data;
my %names;

# Read the bug data and count the bugs for each possible value of row, column
# and table.
#
# We detect a numerical field, and sort appropriately, if all the values are
# numeric.
my $col_isnumeric = 1;
my $row_isnumeric = 1;
my $tbl_isnumeric = 1;

foreach my $result (@$results) {
    my ($row, $col, $tbl) = @$result;

    # handle empty dimension member names
    $row = ' ' if ($row eq '');
    $col = ' ' if ($col eq '');
    $tbl = ' ' if ($tbl eq '');

    $row = "" if ($row eq $columns{''});
    $col = "" if ($col eq $columns{''});
    $tbl = "" if ($tbl eq $columns{''});
    
    # account for the fact that names may start with '_' or '.'.  Change this 
    # so the template doesn't hide hash elements with those keys
    $row =~ s/^([._])/ $1/;
    $col =~ s/^([._])/ $1/;
    $tbl =~ s/^([._])/ $1/;

    $data{$tbl}{$col}{$row}++;
    $names{"col"}{$col}++;
    $names{"row"}{$row}++;
    $names{"tbl"}{$tbl}++;
    
    $col_isnumeric &&= ($col =~ /^-?\d+(\.\d+)?$/o);
    $row_isnumeric &&= ($row =~ /^-?\d+(\.\d+)?$/o);
    $tbl_isnumeric &&= ($tbl =~ /^-?\d+(\.\d+)?$/o);
}

my @col_names = @{get_names($names{"col"}, $col_isnumeric, $col_field)};
my @row_names = @{get_names($names{"row"}, $row_isnumeric, $row_field)};
my @tbl_names = @{get_names($names{"tbl"}, $tbl_isnumeric, $tbl_field)};

# The GD::Graph package requires a particular format of data, so once we've
# gathered everything into the hashes and made sure we know the size of the
# data, we reformat it into an array of arrays of arrays of data.
push(@tbl_names, "-total-") if (scalar(@tbl_names) > 1);
    
my @image_data;
foreach my $tbl (@tbl_names) {
    my @tbl_data;
    push(@tbl_data, \@col_names);
    foreach my $row (@row_names) {
        my @col_data;
        foreach my $col (@col_names) {
            $data{$tbl}{$col}{$row} = $data{$tbl}{$col}{$row} || 0;
            push(@col_data, $data{$tbl}{$col}{$row});
            if ($tbl ne "-total-") {
                # This is a bit sneaky. We spend every loop except the last
                # building up the -total- data, and then last time round,
                # we process it as another tbl, and push() the total values 
                # into the image_data array.
                $data{"-total-"}{$col}{$row} += $data{$tbl}{$col}{$row};
            }
        }

        push(@tbl_data, \@col_data);
    }
    
    unshift(@image_data, \@tbl_data);
}

$vars->{'col_field'} = $col_field;
$vars->{'row_field'} = $row_field;
$vars->{'tbl_field'} = $tbl_field;
$vars->{'time'} = time();

$vars->{'col_names'} = \@col_names;
$vars->{'row_names'} = \@row_names;
$vars->{'tbl_names'} = \@tbl_names;

# Below a certain width, we don't see any bars, so there needs to be a minimum.
if ($width && $cgi->param('format') eq "bar") {
    my $min_width = (scalar(@col_names) || 1) * 20;

    if (!$cgi->param('cumulate')) {
        $min_width *= (scalar(@row_names) || 1);
    }

    $vars->{'min_width'} = $min_width;
}

$vars->{'width'} = $width if $width;
$vars->{'height'} = $height if $height;

$vars->{'query'} = $query;
$vars->{'debug'} = $cgi->param('debug');

my $formatparam = $cgi->param('format');

if ($action eq "wrap") {
    # So which template are we using? If action is "wrap", we will be using
    # no format (it gets passed through to be the format of the actual data),
    # and either report.csv.tmpl (CSV), or report.html.tmpl (everything else).
    # report.html.tmpl produces an HTML framework for either tables of HTML
    # data, or images generated by calling report.cgi again with action as
    # "plot".
    $formatparam =~ s/[^a-zA-Z\-]//g;
    trick_taint($formatparam);
    $vars->{'format'} = $formatparam;
    $formatparam = '';

    # We need to keep track of the defined restrictions on each of the 
    # axes, because buglistbase, below, throws them away. Without this, we
    # get buglistlinks wrong if there is a restriction on an axis field.
    $vars->{'col_vals'} = join("&", $buffer =~ /[&?]($col_field=[^&]+)/g);
    $vars->{'row_vals'} = join("&", $buffer =~ /[&?]($row_field=[^&]+)/g);
    $vars->{'tbl_vals'} = join("&", $buffer =~ /[&?]($tbl_field=[^&]+)/g);
    
    # We need a number of different variants of the base URL for different
    # URLs in the HTML.
    $vars->{'buglistbase'} = $cgi->canonicalise_query(
                                 "x_axis_field", "y_axis_field", "z_axis_field",
                               "ctype", "format", "query_format", @axis_fields);
    $vars->{'imagebase'}   = $cgi->canonicalise_query( 
                    $tbl_field, "action", "ctype", "format", "width", "height");
    $vars->{'switchbase'}  = $cgi->canonicalise_query( 
                "query_format", "action", "ctype", "format", "width", "height");
    $vars->{'data'} = \%data;
}
elsif ($action eq "plot") {
    # If action is "plot", we will be using a format as normal (pie, bar etc.)
    # and a ctype as normal (currently only png.)
    $vars->{'cumulate'} = $cgi->param('cumulate') ? 1 : 0;
    $vars->{'x_labels_vertical'} = $cgi->param('x_labels_vertical') ? 1 : 0;
    $vars->{'data'} = \@image_data;
}
else {
    ThrowCodeError("unknown_action", {action => $cgi->param('action')});
}

my $format = $template->get_format("reports/report", $formatparam,
                                   scalar($cgi->param('ctype')));

# If we get a template or CGI error, it comes out as HTML, which isn't valid
# PNG data, and the browser just displays a "corrupt PNG" message. So, you can
# set debug=1 to always get an HTML content-type, and view the error.
$format->{'ctype'} = "text/html" if $cgi->param('debug');

my @time = localtime(time());
my $date = sprintf "%04d-%02d-%02d", 1900+$time[5],$time[4]+1,$time[3];
my $filename = "report-$date.$format->{extension}";
print $cgi->header(-type => $format->{'ctype'},
                   -content_disposition => "inline; filename=$filename");

# Problems with this CGI are often due to malformed data. Setting debug=1
# prints out both data structures.
if ($cgi->param('debug')) {
    require Data::Dumper;
    print "<pre>data hash:\n";
    print Data::Dumper::Dumper(%data) . "\n\n";
    print "data array:\n";
    print Data::Dumper::Dumper(@image_data) . "\n\n</pre>";
}

$template->process("$format->{'template'}", $vars)
  || ThrowTemplateError($template->error());

exit;


sub get_names {
    my ($names, $isnumeric, $field) = @_;
  
    # These are all the fields we want to preserve the order of in reports.
    my %fields = ('priority'     => get_legal_field_values('priority'),
                  'bug_severity' => get_legal_field_values('bug_severity'),
                  'rep_platform' => get_legal_field_values('rep_platform'),
                  'op_sys'       => get_legal_field_values('op_sys'),
                  'bug_status'   => get_legal_field_values('bug_status'),
                  'resolution'   => [' ', @{get_legal_field_values('resolution')}]);
    
    my $field_list = $fields{$field};
    my @sorted;
    
    if ($field_list) {
        my @unsorted = keys %{$names};
        
        # Extract the used fields from the field_list, in the order they 
        # appear in the field_list. This lets us keep e.g. severities in
        # the normal order.
        #
        # This is O(n^2) but it shouldn't matter for short lists.
        @sorted = map {lsearch(\@unsorted, $_) == -1 ? () : $_} @{$field_list};
    }  
    elsif ($isnumeric) {
        # It's not a field we are preserving the order of, so sort it 
        # numerically...
        sub numerically { $a <=> $b }
        @sorted = sort numerically keys(%{$names});
    } else {
        # ...or alphabetically, as appropriate.
        @sorted = sort(keys(%{$names}));
    }
  
    return \@sorted;
}
