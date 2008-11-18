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
# Contributor(s): Harrison Page <harrison@netscape.com>,
# Terry Weissman <terry@mozilla.org>,
# Dawn Endico <endico@mozilla.org>
# Bryce Nesbitt <bryce@nextbus.COM>,
# Joe Robins <jmrobins@tgix.com>,
# Gervase Markham <gerv@gerv.net> and Adam Spiers <adam@spiers.net>
#    Added ability to chart any combination of resolutions/statuses.
#    Derive the choice of resolutions/statuses from the -All- data file
#    Removed hardcoded order of resolutions/statuses when reading from
#    daily stats file, so now works independently of collectstats.pl
#    version
#    Added image caching by date and datasets
# Myk Melez <myk@mozilla.org>:
#    Implemented form field validation and reorganized code.
# Frédéric Buclin <LpSolit@gmail.com>:
#    Templatization.

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;

eval "use GD";
$@ && ThrowCodeError("gd_not_installed");
eval "use Chart::Lines";
$@ && ThrowCodeError("chart_lines_not_installed");

my $dir       = bz_locations()->{'datadir'} . "/mining";
my $graph_url = 'graphs';
my $graph_dir = bz_locations()->{'libpath'} . '/' .$graph_url;

# If we're using bug groups for products, we should apply those restrictions
# to viewing reports, as well.  Time to check the login in that case.
my $user = Bugzilla->login();

Bugzilla->switch_to_shadow_db();

my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;
my $vars = {};

# We only want those products that the user has permissions for.
my @myproducts;
push( @myproducts, "-All-");
# Extract product names from objects and add them to the list.
push( @myproducts, map { $_->name } @{$user->get_selectable_products} );

if (! defined $cgi->param('product')) {
    # Can we do bug charts?
    (-d $dir && -d $graph_dir) 
      || ThrowCodeError('chart_dir_nonexistent',
                        {dir => $dir, graph_dir => $graph_dir});

    my %default_sel = map { $_ => 1 } qw/UNCONFIRMED NEW ASSIGNED REOPENED/;

    my @datasets;
    my @data = get_data($dir);

    foreach my $dataset (@data) {
        my $datasets = {};
        $datasets->{'value'} = $dataset;
        $datasets->{'selected'} = $default_sel{$dataset} ? 1 : 0;
        push(@datasets, $datasets);
    }

    $vars->{'datasets'} = \@datasets;
    $vars->{'products'} = \@myproducts;

    print $cgi->header();

    $template->process('reports/old-charts.html.tmpl', $vars)
      || ThrowTemplateError($template->error());
    exit;
}
else {
    my $product = $cgi->param('product');

    # For security and correctness, validate the value of the "product" form variable.
    # Valid values are those products for which the user has permissions which appear
    # in the "product" drop-down menu on the report generation form.
    grep($_ eq $product, @myproducts)
      || ThrowUserError("invalid_product_name", {product => $product});

    # We've checked that the product exists, and that the user can see it
    # This means that is OK to detaint
    trick_taint($product);

    defined($cgi->param('datasets')) || ThrowUserError('missing_datasets');

    my $datasets = join('', $cgi->param('datasets'));

    my $type = chart_image_type();
    my $data_file = daily_stats_filename($product);
    my $image_file = chart_image_name($data_file, $type, $datasets);
    my $url_image = correct_urlbase() . "$graph_url/$image_file";

    if (! -e "$graph_dir/$image_file") {
        generate_chart("$dir/$data_file", "$graph_dir/$image_file", $type,
                       $product, $datasets);
    }

    $vars->{'url_image'} = $url_image;

    print $cgi->header(-Content_Disposition=>'inline; filename=bugzilla_report.html');

    $template->process('reports/old-charts.html.tmpl', $vars)
      || ThrowTemplateError($template->error());
    exit;
}

#####################
#    Subroutines    #
#####################

sub get_data {
    my $dir = shift;

    my @datasets;
    my $datafile = daily_stats_filename('-All-');
    open(DATA, '<', "$dir/$datafile")
      || ThrowCodeError('chart_file_open_fail', {filename => "$dir/$datafile"});

    while (<DATA>) {
        if (/^# fields?: (.+)\s*$/) {
            @datasets = grep ! /date/i, (split /\|/, $1);
            last;
        }
    }
    close(DATA);
    return @datasets;
}

sub daily_stats_filename {
    my ($prodname) = @_;
    $prodname =~ s/\//-/gs;
    return $prodname;
}

sub chart_image_type {
    # what chart type should we be generating?
    my $testimg = Chart::Lines->new(2,2);
    my $type = $testimg->can('gif') ? "gif" : "png";

    undef $testimg;
    return $type;
}

sub chart_image_name {
    my ($data_file, $type, $datasets) = @_;

    # This routine generates a filename from the requested fields. The problem
    # is that we have to check the safety of doing this. We can't just require
    # that the fields exist, because what stats were collected could change
    # over time (eg by changing the resolutions available)
    # Instead, just require that each field name consists only of letters,
    # numbers, underscores and hyphens.

    if ($datasets !~ m/^[A-Za-z0-9:_-]+$/) {
        ThrowUserError('invalid_datasets', {'datasets' => $datasets});
    }

    # Since we pass the tests, consider it OK
    trick_taint($datasets);

    # Cache charts by generating a unique filename based on what they
    # show. Charts should be deleted by collectstats.pl nightly.
    my $id = join ("_", split (":", $datasets));

    return "${data_file}_${id}.$type";
}

sub generate_chart {
    my ($data_file, $image_file, $type, $product, $datasets) = @_;

    if (! open FILE, $data_file) {
        if ($product eq '-All-') {
            $product = '';
        }
        ThrowCodeError('chart_data_not_generated', {'product' => $product});
    }

    my @fields;
    my @labels = qw(DATE);
    my %datasets = map { $_ => 1 } split /:/, $datasets;

    my %data = ();
    while (<FILE>) {
        chomp;
        next unless $_;
        if (/^#/) {
            if (/^# fields?: (.*)\s*$/) {
                @fields = split /\||\r/, $1;
                unless ($fields[0] =~ /date/i) {
                    ThrowCodeError('chart_datafile_corrupt', {'file' => $data_file});
                }
                push @labels, grep($datasets{$_}, @fields);
            }
            next;
        }

        unless (@fields) {
            ThrowCodeError('chart_datafile_corrupt', {'file' => $data_file});
        }

        my @line = split /\|/;
        my $date = $line[0];
        my ($yy, $mm, $dd) = $date =~ /^\d{2}(\d{2})(\d{2})(\d{2})$/;
        push @{$data{DATE}}, "$mm/$dd/$yy";
        
        for my $i (1 .. $#fields) {
            my $field = $fields[$i];
            if (! defined $line[$i] or $line[$i] eq '') {
                # no data point given, don't plot (this will probably
                # generate loads of Chart::Base warnings, but that's not
                # our fault.)
                push @{$data{$field}}, undef;
            }
            else {
                push @{$data{$field}}, $line[$i];
            }
        }
    }
    
    shift @labels;

    close FILE;

    if (! @{$data{DATE}}) {
        ThrowUserError('insufficient_data_points');
    }

    my $img = Chart::Lines->new (800, 600);
    my $i = 0;

    my $MAXTICKS = 20;      # Try not to show any more x ticks than this.
    my $skip = 1;
    if (@{$data{DATE}} > $MAXTICKS) {
        $skip = int((@{$data{DATE}} + $MAXTICKS - 1) / $MAXTICKS);
    }

    my %settings =
        (
         "title" => "Status Counts for $product",
         "x_label" => "Dates",
         "y_label" => "Bug Counts",
         "legend_labels" => \@labels,
         "skip_x_ticks" => $skip,
         "y_grid_lines" => "true",
         "grey_background" => "false",
         "colors" => {
                      # default dataset colours are too alike
                      dataset4 => [0, 0, 0], # black
                     },
        );
    
    $img->set (%settings);
    $img->$type($image_file, [ @data{('DATE', @labels)} ]);
}
