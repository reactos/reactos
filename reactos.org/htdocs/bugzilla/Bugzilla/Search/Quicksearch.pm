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
# Contributor(s): C. Begle
#                 Jesse Ruderman
#                 Andreas Franke <afranke@mathweb.org>
#                 Stephen Lee <slee@uk.bnsmc.com>
#                 Marc Schumann <wurblzap@gmail.com>

package Bugzilla::Search::Quicksearch;

# Make it harder for us to do dangerous things in Perl.
use strict;

use Bugzilla::Error;
use Bugzilla::Constants;
use Bugzilla::Keyword;
use Bugzilla::Bug;
use Bugzilla::Field;
use Bugzilla::Util;

use base qw(Exporter);
@Bugzilla::Search::Quicksearch::EXPORT = qw(quicksearch);

# Word renamings
use constant MAPPINGS => {
                # Status, Resolution, Platform, OS, Priority, Severity
                "status" => "bug_status",
                "resolution" => "resolution",  # no change
                "platform" => "rep_platform",
                "os" => "op_sys",
                "opsys" => "op_sys",
                "priority" => "priority",    # no change
                "pri" => "priority",
                "severity" => "bug_severity",
                "sev" => "bug_severity",
                # People: AssignedTo, Reporter, QA Contact, CC, Added comment (?)
                "owner" => "assigned_to",    # deprecated since bug 76507
                "assignee" => "assigned_to",
                "assignedto" => "assigned_to",
                "reporter" => "reporter",    # no change
                "rep" => "reporter",
                "qa" => "qa_contact",
                "qacontact" => "qa_contact",
                "cc" => "cc",          # no change
                # Product, Version, Component, Target Milestone
                "product" => "product",     # no change
                "prod" => "product",
                "version" => "version",     # no change
                "ver" => "version",
                "component" => "component",   # no change
                "comp" => "component",
                "milestone" => "target_milestone",
                "target" => "target_milestone",
                "targetmilestone" => "target_milestone",
                # Summary, Description, URL, Status whiteboard, Keywords
                "summary" => "short_desc",
                "shortdesc" => "short_desc",
                "desc" => "longdesc",
                "description" => "longdesc",
                #"comment" => "longdesc",    # ???
                          # reserve "comment" for "added comment" email search?
                "longdesc" => "longdesc",
                "url" => "bug_file_loc",
                "whiteboard" => "status_whiteboard",
                "statuswhiteboard" => "status_whiteboard",
                "sw" => "status_whiteboard",
                "keywords" => "keywords",    # no change
                "kw" => "keywords",
                # Attachments
                "attachment" => "attachments.description",
                "attachmentdesc" => "attachments.description",
                "attachdesc" => "attachments.description",
                "attachmentdata" => "attach_data.thedata",
                "attachdata" => "attach_data.thedata",
                "attachmentmimetype" => "attachments.mimetype",
                "attachmimetype" => "attachments.mimetype"
};

# We might want to put this into localconfig or somewhere
use constant PLATFORMS => ('pc', 'sun', 'macintosh', 'mac');
use constant PRODUCT_EXCEPTIONS => (
    'row',   # [Browser]
             #   ^^^
    'new',   # [MailNews]
             #      ^^^
);
use constant COMPONENT_EXCEPTIONS => (
    'hang'   # [Bugzilla: Component/Keyword Changes]
             #                               ^^^^
);

# Quicksearch-wide globals for boolean charts.
our ($chart, $and, $or);

sub quicksearch {
    my ($searchstring) = (@_);
    my $cgi = Bugzilla->cgi;
    my $urlbase = correct_urlbase();

    $chart = 0;
    $and   = 0;
    $or    = 0;

    # Remove leading and trailing commas and whitespace.
    $searchstring =~ s/(^[\s,]+|[\s,]+$)//g;
    ThrowUserError('buglist_parameters_required') unless ($searchstring);

    if ($searchstring =~ m/^[0-9,\s]*$/) {
        # Bug number(s) only.

        # Allow separation by comma or whitespace.
        $searchstring =~ s/[,\s]+/,/g;

        if (index($searchstring, ',') < $[) {
            # Single bug number; shortcut to show_bug.cgi.
            print $cgi->redirect(-uri => "${urlbase}show_bug.cgi?id=$searchstring");
            exit;
        }
        else {
            # List of bug numbers.
            $cgi->param('bug_id', $searchstring);
            $cgi->param('order', 'bugs.bug_id');
            $cgi->param('bugidtype', 'include');
        }
    }
    else {
        # It's not just a bug number or a list of bug numbers.
        # Maybe it's an alias?
        if ($searchstring =~ /^([^,\s]+)$/) {
            if (Bugzilla->dbh->selectrow_array(q{SELECT COUNT(*)
                                                   FROM bugs
                                                  WHERE alias = ?},
                                               undef,
                                               $1)) {
                print $cgi->redirect(-uri => "${urlbase}show_bug.cgi?id=$1");
                exit;
            }
        }

        # It's no alias either, so it's a more complex query.
        my $legal_statuses = get_legal_field_values('bug_status');
        my $legal_resolutions = get_legal_field_values('resolution');

        # Globally translate " AND ", " OR ", " NOT " to space, pipe, dash.
        $searchstring =~ s/\s+AND\s+/ /g;
        $searchstring =~ s/\s+OR\s+/|/g;
        $searchstring =~ s/\s+NOT\s+/ -/g;

        my @words = splitString($searchstring);
        my $searchComments = 
            $#words < Bugzilla->params->{'quicksearch_comment_cutoff'};
        my @openStates = BUG_STATE_OPEN;
        my @closedStates;
        my @unknownFields;
        my (%states, %resolutions);

        foreach (@$legal_statuses) {
            push(@closedStates, $_) unless is_open_state($_);
        }
        foreach (@openStates) { $states{$_} = 1 }
        if ($words[0] eq 'ALL') {
            foreach (@$legal_statuses) { $states{$_} = 1 }
            shift @words;
        }
        elsif ($words[0] eq 'OPEN') {
            shift @words;
        }
        elsif ($words[0] =~ /^\+[A-Z]+(,[A-Z]+)*$/) {
            # e.g. +DUP,FIX
            if (matchPrefixes(\%states,
                              \%resolutions,
                              [split(/,/, substr($words[0], 1))],
                              \@closedStates,
                              $legal_resolutions)) {
                shift @words;
                # Allowing additional resolutions means we need to keep
                # the "no resolution" resolution.
                $resolutions{'---'} = 1;
            }
            else {
                # Carry on if no match found.
            }
        }
        elsif ($words[0] =~ /^[A-Z]+(,[A-Z]+)*$/) {
            # e.g. NEW,ASSI,REOP,FIX
            undef %states;
            if (matchPrefixes(\%states,
                              \%resolutions,
                              [split(/,/, $words[0])],
                              $legal_statuses,
                              $legal_resolutions)) {
                shift @words;
            }
            else {
                # Carry on if no match found
                foreach (@openStates) { $states{$_} = 1 }
            }
        }
        else {
            # Default: search for unresolved bugs only.
            # Put custom code here if you would like to change this behaviour.
        }

        # If we have wanted resolutions, allow closed states
        if (keys(%resolutions)) {
            foreach (@closedStates) { $states{$_} = 1 }
        }

        $cgi->param('bug_status', keys(%states));
        $cgi->param('resolution', keys(%resolutions));

        # Loop over all main-level QuickSearch words.
        foreach my $qsword (@words) {
            my $negate = substr($qsword, 0, 1) eq '-';
            if ($negate) {
                $qsword = substr($qsword, 1);
            }

            my $firstChar = substr($qsword, 0, 1);
            my $baseWord = substr($qsword, 1);
            my @subWords = split(/[\|,]/, $baseWord);
            if ($firstChar eq '+') {
                foreach (@subWords) {
                    addChart('short_desc', 'substring', $qsword, $negate);
                }
            }
            elsif ($firstChar eq '#') {
                addChart('short_desc', 'anywords', $baseWord, $negate);
                if ($searchComments) {
                    addChart('longdesc', 'anywords', $baseWord, $negate);
                }
            }
            elsif ($firstChar eq ':') {
                foreach (@subWords) {
                    addChart('product', 'substring', $_, $negate);
                    addChart('component', 'substring', $_, $negate);
                }
            }
            elsif ($firstChar eq '@') {
                foreach (@subWords) {
                    addChart('assigned_to', 'substring', $_, $negate);
                }
            }
            elsif ($firstChar eq '[') {
                addChart('short_desc', 'substring', $baseWord, $negate);
                addChart('status_whiteboard', 'substring', $baseWord, $negate);
            }
            elsif ($firstChar eq '!') {
                addChart('keywords', 'anywords', $baseWord, $negate);

            }
            else { # No special first char

                # Split by '|' to get all operands for a boolean OR.
                foreach my $or_operand (split(/\|/, $qsword)) {
                    if ($or_operand =~ /^votes:([0-9]+)$/) {
                        # votes:xx ("at least xx votes")
                        addChart('votes', 'greaterthan', $1 - 1, $negate);
                    }
                    elsif ($or_operand =~ /^([^:]+):([^:]+)$/) {
                        # generic field1,field2,field3:value1,value2 notation
                        my @fields = split(/,/, $1);
                        my @values = split(/,/, $2);
                        foreach my $field (@fields) {
                            # Skip and record any unknown fields
                            if (!defined(MAPPINGS->{$field})) {
                                push(@unknownFields, $field);
                                next;
                            }
                            $field = MAPPINGS->{$field};
                            foreach (@values) {
                                addChart($field, 'substring', $_, $negate);
                            }
                        }

                    }
                    else {

                        # Having ruled out the special cases, we may now split
                        # by comma, which is another legal boolean OR indicator.
                        foreach my $word (split(/,/, $or_operand)) {
                            # Platform
                            if (grep({lc($word) eq $_} PLATFORMS)) {
                                addChart('rep_platform', 'substring',
                                         $word, $negate);
                            }
                            # Priority
                            elsif ($word =~ m/^[pP]([1-5](-[1-5])?)$/) {
                                addChart('priority', 'regexp',
                                         "[$1]", $negate);
                            }
                            # Severity
                            elsif (grep({lc($word) eq substr($_, 0, 3)}
                                        @{get_legal_field_values('bug_severity')})) {
                                addChart('bug_severity', 'substring',
                                         $word, $negate);
                            }
                            # Votes (votes>xx)
                            elsif ($word =~ m/^votes>([0-9]+)$/) {
                                addChart('votes', 'greaterthan',
                                         $1, $negate);
                            }
                            # Votes (votes>=xx, votes=>xx)
                            elsif ($word =~ m/^votes(>=|=>)([0-9]+)$/) {
                                addChart('votes', 'greaterthan',
                                         $2-1, $negate);

                            }
                            else { # Default QuickSearch word

                                if (!grep({lc($word) eq $_}
                                          PRODUCT_EXCEPTIONS) &&
                                    length($word)>2
                                   ) {
                                    addChart('product', 'substring',
                                             $word, $negate);
                                }
                                if (!grep({lc($word) eq $_}
                                          COMPONENT_EXCEPTIONS) &&
                                    length($word)>2
                                   ) {
                                    addChart('component', 'substring',
                                             $word, $negate);
                                }
                                if (grep({lc($word) eq $_}
                                         map($_->name, Bugzilla::Keyword->get_all))) {
                                    addChart('keywords', 'substring',
                                             $word, $negate);
                                    if (length($word)>2) {
                                        addChart('short_desc', 'substring',
                                                 $word, $negate);
                                        addChart('status_whiteboard',
                                                 'substring',
                                                 $word, $negate);
                                    }

                                }
                                else {

                                    addChart('short_desc', 'substring',
                                             $word, $negate);
                                    addChart('status_whiteboard', 'substring',
                                             $word, $negate);
                                }
                                if ($searchComments) {
                                    addChart('longdesc', 'substring',
                                             $word, $negate);
                                }
                            }
                            # URL field (for IP addrs, host.names,
                            # scheme://urls)
                            if ($word =~ m/[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+/
                                  || $word =~ /^[A-Za-z]+(\.[A-Za-z]+)+/
                                  || $word =~ /:[\\\/][\\\/]/
                                  || $word =~ /localhost/
                                  || $word =~ /mailto[:]?/
                                  # || $word =~ /[A-Za-z]+[:][0-9]+/ #host:port
                                  ) {
                                addChart('bug_file_loc', 'substring',
                                         $word, $negate);
                            }
                        } # foreach my $word (split(/,/, $qsword))
                    } # votes and generic field detection
                } # foreach (split(/\|/, $_))
            } # "switch" $firstChar
            $chart++;
            $and = 0;
            $or = 0;
        } # foreach (@words)

        # Inform user about any unknown fields
        if (scalar(@unknownFields)) {
            ThrowUserError("quicksearch_unknown_field",
                           { fields => \@unknownFields });
        }

        # Make sure we have some query terms left
        scalar($cgi->param())>0 || ThrowUserError("buglist_parameters_required");
    }

    # List of quicksearch-specific CGI parameters to get rid of.
    my @params_to_strip = ('quicksearch', 'load', 'run');
    my $modified_query_string = $cgi->canonicalise_query(@params_to_strip);

    if ($cgi->param('load')) {
        # Param 'load' asks us to display the query in the advanced search form.
        print $cgi->redirect(-uri => "${urlbase}query.cgi?format=advanced&amp;"
                             . $modified_query_string);
    }

    # Otherwise, pass the modified query string to the caller.
    # We modified $cgi->params, so the caller can choose to look at that, too,
    # and disregard the return value.
    $cgi->delete(@params_to_strip);
    return $modified_query_string;
}

###########################################################################
# Helpers
###########################################################################

# Split string on whitespace, retaining quoted strings as one
sub splitString {
    my $string = shift;
    my @quoteparts;
    my @parts;
    my $i = 0;

    # Now split on quote sign; be tolerant about unclosed quotes
    @quoteparts = split(/"/, $string);
    foreach my $part (@quoteparts) {
        # After every odd quote, quote special chars
        $part = url_quote($part) if $i++ % 2;
    }
    # Join again
    $string = join('"', @quoteparts);

    # Now split on unescaped whitespace
    @parts = split(/\s+/, $string);
    foreach (@parts) {
        # Remove quotes
        s/"//g;
    }
                        
    return @parts;
}

# Expand found prefixes to states or resolutions
sub matchPrefixes {
    my $hr_states = shift;
    my $hr_resolutions = shift;
    my $ar_prefixes = shift;
    my $ar_check_states = shift;
    my $ar_check_resolutions = shift;
    my $foundMatch = 0;

    foreach my $prefix (@$ar_prefixes) {
        foreach (@$ar_check_states) {
            if (/^$prefix/) {
                $$hr_states{$_} = 1;
                $foundMatch = 1;
            }
        }
        foreach (@$ar_check_resolutions) {
            if (/^$prefix/) {
                $$hr_resolutions{$_} = 1;
                $foundMatch = 1;
            }
        }
    }
    return $foundMatch;
}

# Negate comparison type
sub negateComparisonType {
    my $comparisonType = shift;

    if ($comparisonType eq 'substring') {
        return 'notsubstring';
    }
    elsif ($comparisonType eq 'anywords') {
        return 'nowords';
    }
    elsif ($comparisonType eq 'regexp') {
        return 'notregexp';
    }
    else {
        # Don't know how to negate that
        ThrowCodeError('unknown_comparison_type');
    }
}

# Add a boolean chart
sub addChart {
    my ($field, $comparisonType, $value, $negate) = @_;

    $negate && ($comparisonType = negateComparisonType($comparisonType));
    makeChart("$chart-$and-$or", $field, $comparisonType, $value);
    if ($negate) {
        $and++;
        $or = 0;
    }
    else {
        $or++;
    }
}

# Create the CGI parameters for a boolean chart
sub makeChart {
    my ($expr, $field, $type, $value) = @_;

    my $cgi = Bugzilla->cgi;
    $cgi->param("field$expr", $field);
    $cgi->param("type$expr",  $type);
    $cgi->param("value$expr", url_decode($value));
}

1;
