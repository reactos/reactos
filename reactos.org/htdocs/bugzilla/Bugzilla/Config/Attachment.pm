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
#                 Dawn Endico <endico@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Joe Robins <jmrobins@tgix.com>
#                 Jacob Steenhagen <jake@bugzilla.org>
#                 J. Paul Reed <preed@sigkill.com>
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Joseph Heenan <joseph@heenan.me.uk>
#                 Erik Stambaugh <erik@dasbistro.com>
#                 Frédéric Buclin <LpSolit@gmail.com>
#

package Bugzilla::Config::Attachment;

use strict;

use Bugzilla::Config::Common;

$Bugzilla::Config::Attachment::sortkey = "025";

sub get_param_list {
  my $class = shift;
  my @param_list = (
  {
  name => 'allow_attachment_deletion',
  type => 'b',
  default => 0
  },
  {
  name => 'allow_attach_url',
  type => 'b',
  default => 0
  },
  {
   name => 'maxpatchsize',
   type => 't',
   default => '1000',
   checker => \&check_numeric
  },

  {
   name => 'maxattachmentsize',
   type => 't',
   default => '1000',
   checker => \&check_numeric
  },

  # The maximum size (in bytes) for patches and non-patch attachments.
  # The default limit is 1000KB, which is 24KB less than mysql's default
  # maximum packet size (which determines how much data can be sent in a
  # single mysql packet and thus how much data can be inserted into the
  # database) to provide breathing space for the data in other fields of
  # the attachment record as well as any mysql packet overhead (I don't
  # know of any, but I suspect there may be some.)

  {
   name => 'maxlocalattachment',
   type => 't',
   default => '0',
   checker => \&check_numeric
  },
  
  {
   name => 'convert_uncompressed_images',
   type => 'b',
   default => 0,
   checker => \&check_image_converter
  } );
  return @param_list;
}

1;
