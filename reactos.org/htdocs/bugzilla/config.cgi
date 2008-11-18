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
#                 Myk Melez <myk@mozilla.org>

################################################################################
# Script Initialization
################################################################################

# Make it harder for us to do dangerous things in Perl.
use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Keyword;
use Bugzilla::Bug;
use Bugzilla::Field;

my $user = Bugzilla->login(LOGIN_OPTIONAL);
my $cgi  = Bugzilla->cgi;

# If the 'requirelogin' parameter is on and the user is not
# authenticated, return empty fields.
if (Bugzilla->params->{'requirelogin'} && !$user->id) {
    display_data();
}

# Pass a bunch of Bugzilla configuration to the templates.
my $vars = {};
$vars->{'priority'}  = get_legal_field_values('priority');
$vars->{'severity'}  = get_legal_field_values('bug_severity');
$vars->{'platform'}  = get_legal_field_values('rep_platform');
$vars->{'op_sys'}    = get_legal_field_values('op_sys');
$vars->{'keyword'}    = [map($_->name, Bugzilla::Keyword->get_all)];
$vars->{'resolution'} = get_legal_field_values('resolution');
$vars->{'status'}    = get_legal_field_values('bug_status');
$vars->{'custom_fields'} =
  [Bugzilla->get_fields({custom => 1, obsolete => 0, type => FIELD_TYPE_SINGLE_SELECT})];

# Include a list of product objects.
if ($cgi->param('product')) {
    my @products = $cgi->param('product');
    foreach my $product_name (@products) {
        # We don't use check_product because config.cgi outputs mostly
        # in XML and JS and we don't want to display an HTML error
        # instead of that.
        my $product = new Bugzilla::Product({ name => $product_name });
        if ($product && $user->can_see_product($product->name)) {
            push (@{$vars->{'products'}}, $product);
        }
    }
} else {
    $vars->{'products'} = $user->get_selectable_products;
}

# Create separate lists of open versus resolved statuses.  This should really
# be made part of the configuration.
my @open_status;
my @closed_status;
foreach my $status (@{$vars->{'status'}}) {
    is_open_state($status) ? push(@open_status, $status) 
                           : push(@closed_status, $status);
}
$vars->{'open_status'} = \@open_status;
$vars->{'closed_status'} = \@closed_status;

# Generate a list of fields that can be queried.
$vars->{'field'} = [Bugzilla->dbh->bz_get_field_defs()];

display_data($vars);


sub display_data {
    my $vars = shift;

    my $cgi      = Bugzilla->cgi;
    my $template = Bugzilla->template;

    # Determine how the user would like to receive the output; 
    # default is JavaScript.
    my $format = $template->get_format("config", scalar($cgi->param('format')),
                                       scalar($cgi->param('ctype')) || "js");

    # Return HTTP headers.
    print "Content-Type: $format->{'ctype'}\n\n";

    # Generate the configuration file and return it to the user.
    $template->process($format->{'template'}, $vars)
      || ThrowTemplateError($template->error());
    exit;
}
