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
# Contributor(s): Myk Melez <myk@mozilla.org>
#                 Frédéric Buclin <LpSolit@gmail.com>

################################################################################
# Script Initialization
################################################################################

# Make it harder for us to do dangerous things in Perl.
use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Flag;
use Bugzilla::FlagType;
use Bugzilla::User;
use Bugzilla::Product;
use Bugzilla::Component;

# Make sure the user is logged in.
my $user = Bugzilla->login();
my $cgi = Bugzilla->cgi;

################################################################################
# Main Body Execution
################################################################################

my $fields;
$fields->{'requester'}->{'type'} = 'single';
# If the user doesn't restrict his search to requests from the wind
# (requestee ne '-'), include the requestee for completion.
unless (defined $cgi->param('requestee')
        && $cgi->param('requestee') eq '-')
{
    $fields->{'requestee'}->{'type'} = 'single';
}

Bugzilla::User::match_field($cgi, $fields);

queue();
exit;

################################################################################
# Functions
################################################################################

sub queue {
    my $cgi = Bugzilla->cgi;
    # There are some user privilege checks to do. We do them against the main DB.
    my $dbh = Bugzilla->dbh;
    my $template = Bugzilla->template;
    my $user = Bugzilla->user;
    my $userid = $user->id;
    my $vars = {};

    my $status = validateStatus($cgi->param('status'));
    my $form_group = validateGroup($cgi->param('group'));

    my $query = 
    # Select columns describing each flag, the bug/attachment on which
    # it has been set, who set it, and of whom they are requesting it.
    " SELECT    flags.id, flagtypes.name,
                flags.status,
                flags.bug_id, bugs.short_desc,
                products.name, components.name,
                flags.attach_id, attachments.description,
                requesters.realname, requesters.login_name,
                requestees.realname, requestees.login_name,
    " . $dbh->sql_date_format('flags.modification_date', '%Y.%m.%d %H:%i') .
    # Use the flags and flagtypes tables for information about the flags,
    # the bugs and attachments tables for target info, the profiles tables
    # for setter and requestee info, the products/components tables
    # so we can display product and component names, and the bug_group_map
    # table to help us weed out secure bugs to which the user should not have
    # access.
    "
      FROM           flags 
           LEFT JOIN attachments
                  ON flags.attach_id = attachments.attach_id
          INNER JOIN flagtypes
                  ON flags.type_id = flagtypes.id
          INNER JOIN profiles AS requesters
                  ON flags.setter_id = requesters.userid
           LEFT JOIN profiles AS requestees
                  ON flags.requestee_id  = requestees.userid
          INNER JOIN bugs
                  ON flags.bug_id = bugs.bug_id
          INNER JOIN products
                  ON bugs.product_id = products.id
          INNER JOIN components
                  ON bugs.component_id = components.id
           LEFT JOIN bug_group_map AS bgmap
                  ON bgmap.bug_id = bugs.bug_id
                 AND bgmap.group_id NOT IN (" .
                     join(', ', (-1, values(%{$user->groups}))) . ")
           LEFT JOIN cc AS ccmap
                  ON ccmap.who = $userid
                 AND ccmap.bug_id = bugs.bug_id
    " .

    # Weed out bug the user does not have access to
    " WHERE     ((bgmap.group_id IS NULL) OR
                 (ccmap.who IS NOT NULL AND cclist_accessible = 1) OR
                 (bugs.reporter = $userid AND bugs.reporter_accessible = 1) OR
                 (bugs.assigned_to = $userid) " .
                 (Bugzilla->params->{'useqacontact'} ? "OR
                 (bugs.qa_contact = $userid))" : ")");

    unless ($user->is_insider) {
        $query .= " AND (attachments.attach_id IS NULL
                         OR attachments.isprivate = 0
                         OR attachments.submitter_id = $userid)";
    }

    # Limit query to pending requests.
    $query .= " AND flags.status = '?' " unless $status;

    # The set of criteria by which we filter records to display in the queue.
    # We now move to the shadow DB to query the DB.
    my @criteria = ();
    $dbh = Bugzilla->switch_to_shadow_db;

    # A list of columns to exclude from the report because the report conditions
    # limit the data being displayed to exact matches for those columns.
    # In other words, if we are only displaying "pending" , we don't
    # need to display a "status" column in the report because the value for that
    # column will always be the same.
    my @excluded_columns = ();
    
    # Filter requests by status: "pending", "granted", "denied", "all" 
    # (which means any), or "fulfilled" (which means "granted" or "denied").
    if ($status) {
        if ($status eq "+-") {
            push(@criteria, "flags.status IN ('+', '-')");
            push(@excluded_columns, 'status') unless $cgi->param('do_union');
        }
        elsif ($status ne "all") {
            push(@criteria, "flags.status = '$status'");
            push(@excluded_columns, 'status') unless $cgi->param('do_union');
        }
    }
    
    # Filter results by exact email address of requester or requestee.
    if (defined $cgi->param('requester') && $cgi->param('requester') ne "") {
        my $requester = $dbh->quote($cgi->param('requester'));
        trick_taint($requester); # Quoted above
        push(@criteria, $dbh->sql_istrcmp('requesters.login_name', $requester));
        push(@excluded_columns, 'requester') unless $cgi->param('do_union');
    }
    if (defined $cgi->param('requestee') && $cgi->param('requestee') ne "") {
        if ($cgi->param('requestee') ne "-") {
            my $requestee = $dbh->quote($cgi->param('requestee'));
            trick_taint($requestee); # Quoted above
            push(@criteria, $dbh->sql_istrcmp('requestees.login_name',
                            $requestee));
        }
        else { push(@criteria, "flags.requestee_id IS NULL") }
        push(@excluded_columns, 'requestee') unless $cgi->param('do_union');
    }
    
    # Filter results by exact product or component.
    if (defined $cgi->param('product') && $cgi->param('product') ne "") {
        my $product = Bugzilla::Product::check_product(scalar $cgi->param('product'));
        push(@criteria, "bugs.product_id = " . $product->id);
        push(@excluded_columns, 'product') unless $cgi->param('do_union');
        if (defined $cgi->param('component') && $cgi->param('component') ne "") {
            my $component =
                Bugzilla::Component::check_component($product, scalar $cgi->param('component'));
            push(@criteria, "bugs.component_id = " . $component->id);
            push(@excluded_columns, 'component') unless $cgi->param('do_union');
        }
    }

    # Filter results by flag types.
    my $form_type = $cgi->param('type');
    if (defined $form_type && !grep($form_type eq $_, ("", "all"))) {
        # Check if any matching types are for attachments.  If not, don't show
        # the attachment column in the report.
        my $has_attachment_type =
            Bugzilla::FlagType::count({ 'name' => $form_type,
                                        'target_type' => 'attachment' });

        if (!$has_attachment_type) { push(@excluded_columns, 'attachment') }

        my $quoted_form_type = $dbh->quote($form_type);
        trick_taint($quoted_form_type); # Already SQL quoted
        push(@criteria, "flagtypes.name = " . $quoted_form_type);
        push(@excluded_columns, 'type') unless $cgi->param('do_union');
    }
    
    # Add the criteria to the query.  We do an intersection by default 
    # but do a union if the "do_union" URL parameter (for which there is no UI 
    # because it's an advanced feature that people won't usually want) is true.
    my $and_or = $cgi->param('do_union') ? " OR " : " AND ";
    $query .= " AND (" . join($and_or, @criteria) . ") " if scalar(@criteria);
    
    # Group the records by flag ID so we don't get multiple rows of data
    # for each flag.  This is only necessary because of the code that
    # removes flags on bugs the user is unauthorized to access.
    $query .= ' ' . $dbh->sql_group_by('flags.id',
               'flagtypes.name, flags.status, flags.bug_id, bugs.short_desc,
                products.name, components.name, flags.attach_id,
                attachments.description, requesters.realname,
                requesters.login_name, requestees.realname,
                requestees.login_name, flags.modification_date,
                cclist_accessible, bugs.reporter, bugs.reporter_accessible,
                bugs.assigned_to');

    # Group the records, in other words order them by the group column
    # so the loop in the display template can break them up into separate
    # tables every time the value in the group column changes.

    $form_group ||= "requestee";
    if ($form_group eq "requester") {
        $query .= " ORDER BY requesters.realname, requesters.login_name";
    }
    elsif ($form_group eq "requestee") {
        $query .= " ORDER BY requestees.realname, requestees.login_name";
    }
    elsif ($form_group eq "category") {
        $query .= " ORDER BY products.name, components.name";
    }
    elsif ($form_group eq "type") {
        $query .= " ORDER BY flagtypes.name";
    }

    # Order the records (within each group).
    $query .= " , flags.modification_date";

    # Pass the query to the template for use when debugging this script.
    $vars->{'query'} = $query;
    $vars->{'debug'} = $cgi->param('debug') ? 1 : 0;
    
    my $results = $dbh->selectall_arrayref($query);
    my @requests = ();
    foreach my $result (@$results) {
        my @data = @$result;
        my $request = {
          'id'              => $data[0] , 
          'type'            => $data[1] , 
          'status'          => $data[2] , 
          'bug_id'          => $data[3] , 
          'bug_summary'     => $data[4] , 
          'category'        => "$data[5]: $data[6]" , 
          'attach_id'       => $data[7] , 
          'attach_summary'  => $data[8] ,
          'requester'       => ($data[9] ? "$data[9] <$data[10]>" : $data[10]) , 
          'requestee'       => ($data[11] ? "$data[11] <$data[12]>" : $data[12]) , 
          'created'         => $data[13]
        };
        push(@requests, $request);
    }

    # Get a list of request type names to use in the filter form.
    my @types = ("all");
    my $flagtypes = $dbh->selectcol_arrayref(
                         "SELECT DISTINCT(name) FROM flagtypes ORDER BY name");
    push(@types, @$flagtypes);

    # We move back to the main DB to get the list of products the user can see.
    $dbh = Bugzilla->switch_to_main_db;

    $vars->{'products'} = $user->get_selectable_products;
    $vars->{'excluded_columns'} = \@excluded_columns;
    $vars->{'group_field'} = $form_group;
    $vars->{'requests'} = \@requests;
    $vars->{'types'} = \@types;

    # Return the appropriate HTTP response headers.
    print $cgi->header();

    # Generate and return the UI (HTML page) from the appropriate template.
    $template->process("request/queue.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

################################################################################
# Data Validation / Security Authorization
################################################################################

sub validateStatus {
    my $status = shift;
    return if !defined $status;

    grep($status eq $_, qw(? +- + - all))
      || ThrowCodeError("flag_status_invalid",
                        { status => $status });
    trick_taint($status);
    return $status;
}

sub validateGroup {
    my $group = shift;
    return if !defined $group;

    grep($group eq $_, qw(requester requestee category type))
      || ThrowCodeError("request_queue_group_invalid", 
                        { group => $group });
    trick_taint($group);
    return $group;
}

