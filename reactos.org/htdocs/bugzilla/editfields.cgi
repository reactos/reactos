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
# Contributor(s): Frédéric Buclin <LpSolit@gmail.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Field;
use Bugzilla::Token;

my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;
my $vars = {};

# Make sure the user is logged in and is an administrator.
my $user = Bugzilla->login(LOGIN_REQUIRED);
$user->in_group('admin')
  || ThrowUserError('auth_failure', {group  => 'admin',
                                     action => 'edit',
                                     object => 'custom_fields'});

my $action = trim($cgi->param('action') || '');
my $token  = $cgi->param('token');

print $cgi->header();

# List all existing custom fields if no action is given.
if (!$action) {
    $template->process('admin/custom_fields/list.html.tmpl', $vars)
        || ThrowTemplateError($template->error());
}
# Interface to add a new custom field.
elsif ($action eq 'add') {
    $vars->{'token'} = issue_session_token('add_field');

    $template->process('admin/custom_fields/create.html.tmpl', $vars)
        || ThrowTemplateError($template->error());
}
elsif ($action eq 'new') {
    check_token_data($token, 'add_field');

    $vars->{'field'} = Bugzilla::Field->create({
        name        => scalar $cgi->param('name'),
        description => scalar $cgi->param('desc'),
        type        => scalar $cgi->param('type'),
        sortkey     => scalar $cgi->param('sortkey'),
        mailhead    => scalar $cgi->param('new_bugmail'),
        enter_bug   => scalar $cgi->param('enter_bug'),
        obsolete    => scalar $cgi->param('obsolete'),
        custom      => 1,
    });

    delete_token($token);

    $vars->{'message'} = 'custom_field_created';

    $template->process('admin/custom_fields/list.html.tmpl', $vars)
        || ThrowTemplateError($template->error());
}
elsif ($action eq 'edit') {
    my $name = $cgi->param('name') || ThrowUserError('field_missing_name');
    # Custom field names must start with "cf_".
    if ($name !~ /^cf_/) {
        $name = 'cf_' . $name;
    }
    my $field = new Bugzilla::Field({'name' => $name});
    $field || ThrowUserError('customfield_nonexistent', {'name' => $name});

    $vars->{'field'} = $field;
    $vars->{'token'} = issue_session_token('edit_field');

    $template->process('admin/custom_fields/edit.html.tmpl', $vars)
        || ThrowTemplateError($template->error());
}
elsif ($action eq 'update') {
    check_token_data($token, 'edit_field');
    my $name = $cgi->param('name');

    # Validate fields.
    $name || ThrowUserError('field_missing_name');
    # Custom field names must start with "cf_".
    if ($name !~ /^cf_/) {
        $name = 'cf_' . $name;
    }
    my $field = new Bugzilla::Field({'name' => $name});
    $field || ThrowUserError('customfield_nonexistent', {'name' => $name});

    $field->set_description($cgi->param('desc'));
    $field->set_sortkey($cgi->param('sortkey'));
    $field->set_in_new_bugmail($cgi->param('new_bugmail'));
    $field->set_enter_bug($cgi->param('enter_bug'));
    $field->set_obsolete($cgi->param('obsolete'));
    $field->update();

    delete_token($token);

    $vars->{'field'}   = $field;
    $vars->{'message'} = 'custom_field_updated';

    $template->process('admin/custom_fields/list.html.tmpl', $vars)
        || ThrowTemplateError($template->error());
}
else {
    ThrowUserError('no_valid_action', {'field' => 'custom_field'});
}
