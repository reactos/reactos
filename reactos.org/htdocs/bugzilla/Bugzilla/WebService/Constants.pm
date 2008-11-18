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
# Contributor(s): Marc Schumann <wurblzap@gmail.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::WebService::Constants;

use strict;
use base qw(Exporter);

@Bugzilla::WebService::Constants::EXPORT = qw(
    WS_ERROR_CODE
    ERROR_UNKNOWN_FATAL
    ERROR_UNKNOWN_TRANSIENT

    ERROR_AUTH_NODATA
    ERROR_UNIMPLEMENTED

    LOGIN_EXEMPT
);

# This maps the error names in global/*-error.html.tmpl to numbers.
# Generally, transient errors should have a number above 0, and
# fatal errors should have a number below 0.
#
# This hash should generally contain any error that could be thrown
# by the WebService interface. If it's extremely unlikely that the
# error could be thrown (like some CodeErrors), it doesn't have to
# be listed here.
#
# "Transient" means "If you resubmit that request with different data,
# it may work."
#
# "Fatal" means, "There's something wrong with Bugzilla, probably
# something an administrator would have to fix."
#
# NOTE: Numbers must never be recycled. If you remove a number, leave a
# comment that it was retired. Also, if an error changes its name, you'll
# have to fix it here.
use constant WS_ERROR_CODE => {
    # Bug errors usually occupy the 100-200 range.
    invalid_bug_id_or_alias     => 100,
    invalid_bug_id_non_existent => 101,
    bug_access_denied           => 102,
    bug_access_query            => 102,
    invalid_field_name          => 108,
    # These all mean "invalid alias"
    alias_not_defined        => 103,
    alias_too_long           => 103,
    alias_in_use             => 103,
    alias_is_numeric         => 103,
    alias_has_comma_or_space => 103,
    # Misc. bug field errors
    illegal_field => 104,
    freetext_too_long => 104,
    # Component errors
    require_component       => 105,
    component_name_too_long => 105,
    component_not_valid     => 105,
    # Invalid Product
    no_products         => 106,
    entry_access_denied => 106,
    product_access_denied => 106,
    product_disabled    => 106,
    # Invalid Summary
    require_summary => 107,

    # Authentication errors are usually 300-400.
    invalid_username_or_password => 300,
    account_disabled             => 301,
    auth_invalid_email           => 302,
    extern_id_conflict           => -303,

    # User errors are 500-600.
    account_exists        => 500,
    illegal_email_address => 501,
    account_creation_disabled   => 501,
    password_too_short    => 502,
    password_too_long     => 503,
    invalid_username      => 504,
    # This is from strict_isolation, but it also basically means 
    # "invalid user."
    invalid_user_group    => 504,
};

# These are the fallback defaults for errors not in ERROR_CODE.
use constant ERROR_UNKNOWN_FATAL     => -32000;
use constant ERROR_UNKNOWN_TRANSIENT => 32000;

use constant ERROR_AUTH_NODATA   => 410;
use constant ERROR_UNIMPLEMENTED => 910;
use constant ERROR_GENERAL       => 999;

# For some methods, we shouldn't call Bugzilla->login before we call them.
# This is a hash--package names pointing to an arrayref of method names.
use constant LOGIN_EXEMPT => {
    # Callers may have to know the Bugzilla version before logging in,
    # even on a requirelogin installation.
    Bugzilla => ['version', 'timezone'],
    User     => ['offer_account_by_email', 'login'],
};

1;
