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
# Contributor(s): Shane H. W. Travis <travis@sedsystems.ca>
#

use strict;
use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::User::Setting;
use Bugzilla::Token;

my $template = Bugzilla->template;
local our $vars = {};

###############################
###  Subroutine Definitions ###
###############################

sub LoadSettings {

    $vars->{'settings'} = Bugzilla::User::Setting::get_defaults();

    my @setting_list = keys %{$vars->{'settings'}};
    $vars->{'setting_names'} = \@setting_list;
}

sub SaveSettings{

    my $cgi = Bugzilla->cgi;

    $vars->{'settings'} = Bugzilla::User::Setting::get_defaults();
    my @setting_list = keys %{$vars->{'settings'}};

    foreach my $name (@setting_list) {
        my $changed = 0;
        my $old_enabled = $vars->{'settings'}->{$name}->{'is_enabled'};
        my $old_value   = $vars->{'settings'}->{$name}->{'default_value'};
        my $enabled = defined $cgi->param("${name}-enabled") || 0;
        my $value = $cgi->param("${name}");
        my $setting = new Bugzilla::User::Setting($name);

        $setting->validate_value($value);

        if ( ($old_enabled != $enabled) ||
             ($old_value ne $value) ) {
            Bugzilla::User::Setting::set_default($name, $value, $enabled);
        }
    }
}

###################
###  Live code  ###
###################

my $user = Bugzilla->login(LOGIN_REQUIRED);

my $cgi = Bugzilla->cgi;
print $cgi->header;

$user->in_group('tweakparams')
  || ThrowUserError("auth_failure", {group  => "tweakparams",
                                     action => "modify",
                                     object => "settings"});

my $action  = trim($cgi->param('action')  || 'load');
my $token   = $cgi->param('token');

if ($action eq 'update') {
    check_token_data($token, 'edit_settings');
    SaveSettings();
    delete_token($token);
    $vars->{'changes_saved'} = 1;

    $template->process("admin/settings/updated.html.tmpl", $vars)
      || ThrowTemplateError($template->error());

    exit;
}

if ($action eq 'load') {
    LoadSettings();
    $vars->{'token'} = issue_session_token('edit_settings');

    $template->process("admin/settings/edit.html.tmpl", $vars)
      || ThrowTemplateError($template->error());

    exit;
}

#
# No valid action found
#
ThrowUserError('no_valid_action', {'field' => "settings"});
