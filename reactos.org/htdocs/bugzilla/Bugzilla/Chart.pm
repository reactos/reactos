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
#                 Albert Ting <altlst@sonic.net>
#                 A. Karl Kornel <karl@kornel.name>

use strict;
use lib ".";

# This module represents a chart.
#
# Note that it is perfectly legal for the 'lines' member variable of this
# class (which is an array of Bugzilla::Series objects) to have empty members
# in it. If this is true, the 'labels' array will also have empty members at
# the same points.
package Bugzilla::Chart;

use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Series;

use Date::Format;
use Date::Parse;
use List::Util qw(max);

sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
  
    # Create a ref to an empty hash and bless it
    my $self = {};
    bless($self, $class);

    if ($#_ == 0) {
        # Construct from a CGI object.
        $self->init($_[0]);
    } 
    else {
        die("CGI object not passed in - invalid number of args \($#_\)($_)");
    }

    return $self;
}

sub init {
    my $self = shift;
    my $cgi = shift;

    # The data structure is a list of lists (lines) of Series objects. 
    # There is a separate list for the labels.
    #
    # The URL encoding is:
    # line0=67&line0=73&line1=81&line2=67...
    # &label0=B+/+R+/+NEW&label1=...
    # &select0=1&select3=1...    
    # &cumulate=1&datefrom=2002-02-03&dateto=2002-04-04&ctype=html...
    # &gt=1&labelgt=Grand+Total    
    foreach my $param ($cgi->param()) {
        # Store all the lines
        if ($param =~ /^line(\d+)$/) {
            foreach my $series_id ($cgi->param($param)) {
                detaint_natural($series_id) 
                                     || ThrowCodeError("invalid_series_id");
                my $series = new Bugzilla::Series($series_id);
                push(@{$self->{'lines'}[$1]}, $series) if $series;
            }
        }

        # Store all the labels
        if ($param =~ /^label(\d+)$/) {
            $self->{'labels'}[$1] = $cgi->param($param);
        }        
    }
    
    # Store the miscellaneous metadata
    $self->{'cumulate'} = $cgi->param('cumulate') ? 1 : 0;
    $self->{'gt'}       = $cgi->param('gt') ? 1 : 0;
    $self->{'labelgt'}  = $cgi->param('labelgt');
    $self->{'datefrom'} = $cgi->param('datefrom');
    $self->{'dateto'}   = $cgi->param('dateto');
    
    # If we are cumulating, a grand total makes no sense
    $self->{'gt'} = 0 if $self->{'cumulate'};
    
    # Make sure the dates are ones we are able to interpret
    foreach my $date ('datefrom', 'dateto') {
        if ($self->{$date}) {
            $self->{$date} = str2time($self->{$date}) 
              || ThrowUserError("illegal_date", { date => $self->{$date}});
        }
    }

    # datefrom can't be after dateto
    if ($self->{'datefrom'} && $self->{'dateto'} && 
        $self->{'datefrom'} > $self->{'dateto'}) 
    {
          ThrowUserError("misarranged_dates", 
                                         {'datefrom' => $cgi->param('datefrom'),
                                          'dateto' => $cgi->param('dateto')});
    }    
}

# Alter Chart so that the selected series are added to it.
sub add {
    my $self = shift;
    my @series_ids = @_;

    # Get the current size of the series; required for adding Grand Total later
    my $current_size = scalar($self->getSeriesIDs());
    
    # Count the number of added series
    my $added = 0;
    # Create new Series and push them on to the list of lines.
    # Note that new lines have no label; the display template is responsible
    # for inventing something sensible.
    foreach my $series_id (@series_ids) {
        my $series = new Bugzilla::Series($series_id);
        if ($series) {
            push(@{$self->{'lines'}}, [$series]);
            push(@{$self->{'labels'}}, "");
            $added++;
        }
    }
    
    # If we are going from < 2 to >= 2 series, add the Grand Total line.
    if (!$self->{'gt'}) {
        if ($current_size < 2 &&
            $current_size + $added >= 2) 
        {
            $self->{'gt'} = 1;
        }
    }
}

# Alter Chart so that the selections are removed from it.
sub remove {
    my $self = shift;
    my @line_ids = @_;
    
    foreach my $line_id (@line_ids) {
        if ($line_id == 65536) {
            # Magic value - delete Grand Total.
            $self->{'gt'} = 0;
        } 
        else {
            delete($self->{'lines'}->[$line_id]);
            delete($self->{'labels'}->[$line_id]);
        }
    }
}

# Alter Chart so that the selections are summed.
sub sum {
    my $self = shift;
    my @line_ids = @_;
    
    # We can't add the Grand Total to things.
    @line_ids = grep(!/^65536$/, @line_ids);
        
    # We can't add less than two things.
    return if scalar(@line_ids) < 2;
    
    my @series;
    my $label = "";
    my $biggestlength = 0;
    
    # We rescue the Series objects of all the series involved in the sum.
    foreach my $line_id (@line_ids) {
        my @line = @{$self->{'lines'}->[$line_id]};
        
        foreach my $series (@line) {
            push(@series, $series);
        }
        
        # We keep the label that labels the line with the most series.
        if (scalar(@line) > $biggestlength) {
            $biggestlength = scalar(@line);
            $label = $self->{'labels'}->[$line_id];
        }
    }

    $self->remove(@line_ids);

    push(@{$self->{'lines'}}, \@series);
    push(@{$self->{'labels'}}, $label);
}

sub data {
    my $self = shift;
    $self->{'_data'} ||= $self->readData();
    return $self->{'_data'};
}

# Convert the Chart's data into a plottable form in $self->{'_data'}.
sub readData {
    my $self = shift;
    my @data;
    my @maxvals;

    # Note: you get a bad image if getSeriesIDs returns nothing
    # We need to handle errors better.
    my $series_ids = join(",", $self->getSeriesIDs());

    return [] unless $series_ids;

    # Work out the date boundaries for our data.
    my $dbh = Bugzilla->dbh;
    
    # The date used is the one given if it's in a sensible range; otherwise,
    # it's the earliest or latest date in the database as appropriate.
    my $datefrom = $dbh->selectrow_array("SELECT MIN(series_date) " . 
                                         "FROM series_data " .
                                         "WHERE series_id IN ($series_ids)");
    $datefrom = str2time($datefrom);

    if ($self->{'datefrom'} && $self->{'datefrom'} > $datefrom) {
        $datefrom = $self->{'datefrom'};
    }

    my $dateto = $dbh->selectrow_array("SELECT MAX(series_date) " . 
                                       "FROM series_data " .
                                       "WHERE series_id IN ($series_ids)");
    $dateto = str2time($dateto); 

    if ($self->{'dateto'} && $self->{'dateto'} < $dateto) {
        $dateto = $self->{'dateto'};
    }

    # Convert UNIX times back to a date format usable for SQL queries.
    my $sql_from = time2str('%Y-%m-%d', $datefrom);
    my $sql_to = time2str('%Y-%m-%d', $dateto);

    # Prepare the query which retrieves the data for each series
    my $query = "SELECT " . $dbh->sql_to_days('series_date') . " - " .
                            $dbh->sql_to_days('?') . ", series_value " .
                "FROM series_data " .
                "WHERE series_id = ? " .
                "AND series_date >= ?";
    if ($dateto) {
        $query .= " AND series_date <= ?";
    }
    
    my $sth = $dbh->prepare($query);

    my $gt_index = $self->{'gt'} ? scalar(@{$self->{'lines'}}) : undef;
    my $line_index = 0;

    $maxvals[$gt_index] = 0 if $gt_index;

    my @datediff_total;

    foreach my $line (@{$self->{'lines'}}) {        
        # Even if we end up with no data, we need an empty arrayref to prevent
        # errors in the PNG-generating code
        $data[$line_index] = [];
        $maxvals[$line_index] = 0;

        foreach my $series (@$line) {

            # Get the data for this series and add it on
            if ($dateto) {
                $sth->execute($sql_from, $series->{'series_id'}, $sql_from, $sql_to);
            }
            else {
                $sth->execute($sql_from, $series->{'series_id'}, $sql_from);
            }
            my $points = $sth->fetchall_arrayref();

            foreach my $point (@$points) {
                my ($datediff, $value) = @$point;
                $data[$line_index][$datediff] ||= 0;
                $data[$line_index][$datediff] += $value;
                if ($data[$line_index][$datediff] > $maxvals[$line_index]) {
                    $maxvals[$line_index] = $data[$line_index][$datediff];
                }

                $datediff_total[$datediff] += $value;

                # Add to the grand total, if we are doing that
                if ($gt_index) {
                    $data[$gt_index][$datediff] ||= 0;
                    $data[$gt_index][$datediff] += $value;
                    if ($data[$gt_index][$datediff] > $maxvals[$gt_index]) {
                        $maxvals[$gt_index] = $data[$gt_index][$datediff];
                    }
                }
            }
        }

        # We are done with the series making up this line, go to the next one
        $line_index++;
    }

    # calculate maximum y value
    if ($self->{'cumulate'}) {
        # Make sure we do not try to take the max of an array with undef values
        my @processed_datediff;
        while (@datediff_total) {
            my $datediff = shift @datediff_total;
            push @processed_datediff, $datediff if defined($datediff);
        }
        $self->{'y_max_value'} = max(@processed_datediff);
    }
    else {
        $self->{'y_max_value'} = max(@maxvals);
    }
    $self->{'y_max_value'} |= 1; # For log()

    # Align the max y value:
    #  For one- or two-digit numbers, increase y_max_value until divisible by 8
    #  For larger numbers, see the comments below to figure out what's going on
    if ($self->{'y_max_value'} < 100) {
        do {
            ++$self->{'y_max_value'};
        } while ($self->{'y_max_value'} % 8 != 0);
    }
    else {
        #  First, get the # of digits in the y_max_value
        my $num_digits = 1+int(log($self->{'y_max_value'})/log(10));

        # We want to zero out all but the top 2 digits
        my $mask_length = $num_digits - 2;
        $self->{'y_max_value'} /= 10**$mask_length;
        $self->{'y_max_value'} = int($self->{'y_max_value'});
        $self->{'y_max_value'} *= 10**$mask_length;

        # Add 10^$mask_length to the max value
        # Continue to increase until it's divisible by 8 * 10^($mask_length-1)
        # (Throwing in the -1 keeps at least the smallest digit at zero)
        do {
            $self->{'y_max_value'} += 10**$mask_length;
        } while ($self->{'y_max_value'} % (8*(10**($mask_length-1))) != 0);
    }

        
    # Add the x-axis labels into the data structure
    my $date_progression = generateDateProgression($datefrom, $dateto);
    unshift(@data, $date_progression);

    if ($self->{'gt'}) {
        # Add Grand Total to label list
        push(@{$self->{'labels'}}, $self->{'labelgt'});

        $data[$gt_index] ||= [];
    }

    return \@data;
}

# Flatten the data structure into a list of series_ids
sub getSeriesIDs {
    my $self = shift;
    my @series_ids;

    foreach my $line (@{$self->{'lines'}}) {
        foreach my $series (@$line) {
            push(@series_ids, $series->{'series_id'});
        }
    }

    return @series_ids;
}

# Class method to get the data necessary to populate the "select series"
# widgets on various pages.
sub getVisibleSeries {
    my %cats;

    # List of groups the user is in; use -1 to make sure it's not empty.
    my $grouplist = join(", ", (-1, values(%{Bugzilla->user->groups})));
    
    # Get all visible series
    my $dbh = Bugzilla->dbh;
    my $serieses = $dbh->selectall_arrayref("SELECT cc1.name, cc2.name, " .
                        "series.name, series.series_id " .
                        "FROM series " .
                        "INNER JOIN series_categories AS cc1 " .
                        "    ON series.category = cc1.id " .
                        "INNER JOIN series_categories AS cc2 " .
                        "    ON series.subcategory = cc2.id " .
                        "LEFT JOIN category_group_map AS cgm " .
                        "    ON series.category = cgm.category_id " .
                        "    AND cgm.group_id NOT IN($grouplist) " .
                        "WHERE creator = " . Bugzilla->user->id . " OR " .
                        "      cgm.category_id IS NULL " . 
                   $dbh->sql_group_by('series.series_id', 'cc1.name, cc2.name, ' .
                                      'series.name'));
    foreach my $series (@$serieses) {
        my ($cat, $subcat, $name, $series_id) = @$series;
        $cats{$cat}{$subcat}{$name} = $series_id;
    }

    return \%cats;
}

sub generateDateProgression {
    my ($datefrom, $dateto) = @_;
    my @progression;

    $dateto = $dateto || time();
    my $oneday = 60 * 60 * 24;

    # When the from and to dates are converted by str2time(), you end up with
    # a time figure representing midnight at the beginning of that day. We
    # adjust the times by 1/3 and 2/3 of a day respectively to prevent
    # edge conditions in time2str().
    $datefrom += $oneday / 3;
    $dateto += (2 * $oneday) / 3;

    while ($datefrom < $dateto) {
        push (@progression, time2str("%Y-%m-%d", $datefrom));
        $datefrom += $oneday;
    }

    return \@progression;
}

sub dump {
    my $self = shift;

    # Make sure we've read in our data
    my $data = $self->data;
    
    require Data::Dumper;
    print "<pre>Bugzilla::Chart object:\n";
    print Data::Dumper::Dumper($self);
    print "</pre>";
}

1;
