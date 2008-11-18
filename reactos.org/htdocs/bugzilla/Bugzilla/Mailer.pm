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
# Contributor(s): Terry Weissman <terry@mozilla.org>,
#                 Bryce Nesbitt <bryce-mozilla@nextbus.com>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Alan Raetz <al_raetz@yahoo.com>
#                 Jacob Steenhagen <jake@actex.net>
#                 Matthew Tuck <matty@chariot.net.au>
#                 Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 J. Paul Reed <preed@sigkill.com>
#                 Gervase Markham <gerv@gerv.net>
#                 Byron Jones <bugzilla@glob.com.au>
#                 Frédéric Buclin <LpSolit@gmail.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::Mailer;

use strict;

use base qw(Exporter);
@Bugzilla::Mailer::EXPORT = qw(MessageToMTA);

use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;

use Date::Format qw(time2str);

use Encode qw(encode);
use Encode::MIME::Header;
use Email::Address;
use Email::MIME;
# Loading this gives us encoding_set.
use Email::MIME::Modifier;
use Email::Send;

sub MessageToMTA {
    my ($msg) = (@_);
    my $method = Bugzilla->params->{'mail_delivery_method'};
    return if $method eq 'None';

    my $email = ref($msg) ? $msg : Email::MIME->new($msg);
    foreach my $part ($email->parts) {
        $part->charset_set('UTF-8') if Bugzilla->params->{'utf8'};
        $part->encoding_set('quoted-printable') if !is_7bit_clean($part->body);
    }

    # MIME-Version must be set otherwise some mailsystems ignore the charset
    $email->header_set('MIME-Version', '1.0') if !$email->header('MIME-Version');

    # Encode the headers correctly in quoted-printable
    foreach my $header qw(From To Cc Reply-To Sender Errors-To Subject) {
        if (my $value = $email->header($header)) {
            $value = Encode::decode("UTF-8", $value) if Bugzilla->params->{'utf8'};

            # avoid excessive line wrapping done by Encode.
            local $Encode::Encoding{'MIME-Q'}->{'bpl'} = 998;

            my $encoded = encode('MIME-Q', $value);
            $email->header_set($header, $encoded);
        }
    }

    my $from = $email->header('From');

    my ($hostname, @args);
    if ($method eq "Sendmail") {
        if (ON_WINDOWS) {
            $Email::Send::Sendmail::SENDMAIL = SENDMAIL_EXE;
        }
        push @args, "-i";
        # We want to make sure that we pass *only* an email address.
        if ($from) {
            my ($email_obj) = Email::Address->parse($from);
            if ($email_obj) {
                my $from_email = $email_obj->address;
                push(@args, "-f$from_email") if $from_email;
            }
        }
        push(@args, "-ODeliveryMode=deferred")
            if !Bugzilla->params->{"sendmailnow"};
    }
    else {
        # Sendmail will automatically append our hostname to the From
        # address, but other mailers won't.
        my $urlbase = Bugzilla->params->{'urlbase'};
        $urlbase =~ m|//([^:/]+)[:/]?|;
        $hostname = $1;
        $from .= "\@$hostname" if $from !~ /@/;
        $email->header_set('From', $from);
        
        # Sendmail adds a Date: header also, but others may not.
        if (!defined $email->header('Date')) {
            $email->header_set('Date', time2str("%a, %e %b %Y %T %z", time()));
        }
    }

    if ($method eq "SMTP") {
        push @args, Host  => Bugzilla->params->{"smtpserver"},
                    Hello => $hostname, 
                    Debug => Bugzilla->params->{'smtp_debug'};
    }

    if ($method eq "Test") {
        my $filename = bz_locations()->{'datadir'} . '/mailer.testfile';
        open TESTFILE, '>>', $filename;
        # From - <date> is required to be a valid mbox file.
        print TESTFILE "\n\nFrom - " . $email->header('Date') . "\n" . $email->as_string;
        close TESTFILE;
    }
    else {
        # This is useful for both Sendmail and Qmail, so we put it out here.
        local $ENV{PATH} = SENDMAIL_PATH;
        my $mailer = Email::Send->new({ mailer => $method, 
                                        mailer_args => \@args });
        my $retval = $mailer->send($email);
        ThrowCodeError('mail_send_error', { msg => $retval, mail => $email })
            if !$retval;
    }
}

1;
