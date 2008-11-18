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
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Frédéric Buclin <LpSolit@gmail.com>

use strict;
use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Product;

my $user = Bugzilla->login();

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;
my $template = Bugzilla->template;
my $vars = {};

print $cgi->header();

my $product_name = trim($cgi->param('product') || '');
my $product = new Bugzilla::Product({'name' => $product_name});

unless ($product && $user->can_enter_product($product->name)) {
    # Products which the user is allowed to see.
    my @products = @{$user->get_enterable_products};

    if (scalar(@products) == 0) {
        ThrowUserError("no_products");
    }
    # If there is only one product available but the user entered
    # another product name, we display a list with this single
    # product only, to not confuse the user with components of a
    # product he didn't request.
    elsif (scalar(@products) > 1 || $product_name) {
        $vars->{'classifications'} = [{object => undef, products => \@products}];
        $vars->{'target'} = "describecomponents.cgi";
        # If an invalid product name is given, or the user is not
        # allowed to access that product, a message is displayed
        # with a list of the products the user can choose from.
        if ($product_name) {
            $vars->{'message'} = "product_invalid";
            # Do not use $product->name here, else you could use
            # this way to determine whether the product exists or not.
            $vars->{'product'} = $product_name;
        }

        $template->process("global/choose-product.html.tmpl", $vars)
          || ThrowTemplateError($template->error());
        exit;
    }

    # If there is only one product available and the user didn't specify
    # any product name, we show this product.
    $product = $products[0];
}

######################################################################
# End Data/Security Validation
######################################################################

$vars->{'product'} = $product;

$template->process("reports/components.html.tmpl", $vars)
  || ThrowTemplateError($template->error());
