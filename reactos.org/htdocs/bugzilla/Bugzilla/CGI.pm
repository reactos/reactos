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
# Contributor(s): Bradley Baetz <bbaetz@student.usyd.edu.au>
#                 Byron Jones <bugzilla@glob.com.au>
#                 Marc Schumann <wurblzap@gmail.com>

use strict;

package Bugzilla::CGI;

BEGIN {
    if ($^O =~ /MSWin32/i) {
        # Help CGI find the correct temp directory as the default list
        # isn't Windows friendly (Bug 248988)
        $ENV{'TMPDIR'} = $ENV{'TEMP'} || $ENV{'TMP'} || "$ENV{'WINDIR'}\\TEMP";
    }
}

use CGI qw(-no_xhtml -oldstyle_urls :private_tempfiles :unique_headers SERVER_PUSH);

use base qw(CGI);

use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;

# We need to disable output buffering - see bug 179174
$| = 1;

# Ignore SIGTERM and SIGPIPE - this prevents DB corruption. If the user closes
# their browser window while a script is running, the webserver sends these
# signals, and we don't want to die half way through a write.
$::SIG{TERM} = 'IGNORE';
$::SIG{PIPE} = 'IGNORE';

# CGI.pm uses AUTOLOAD, but explicitly defines a DESTROY sub.
# We need to do so, too, otherwise perl dies when the object is destroyed
# and we don't have a DESTROY method (because CGI.pm's AUTOLOAD will |die|
# on getting an unknown sub to try to call)
sub DESTROY {
    my $self = shift;
    $self->SUPER::DESTROY(@_);
};

sub new {
    my ($invocant, @args) = @_;
    my $class = ref($invocant) || $invocant;

    my $self = $class->SUPER::new(@args);

    if (Bugzilla->error_mode eq ERROR_MODE_WEBPAGE) {
        # This happens here so that command-line scripts don't spit out
        # their errors in HTML format.
        require CGI::Carp;
        import CGI::Carp qw(fatalsToBrowser);
    }

    # Make sure our outgoing cookie list is empty on each invocation
    $self->{Bugzilla_cookie_list} = [];

    # Send appropriate charset
    $self->charset(Bugzilla->params->{'utf8'} ? 'UTF-8' : '');

    # Redirect to SSL if required
    if (Bugzilla->params->{'sslbase'} ne ''
        && Bugzilla->params->{'ssl'} eq 'always'
        && i_am_cgi())
    {
        $self->require_https(Bugzilla->params->{'sslbase'});
    }

    # Check for errors
    # All of the Bugzilla code wants to do this, so do it here instead of
    # in each script

    my $err = $self->cgi_error;

    if ($err) {
        # Note that this error block is only triggered by CGI.pm for malformed
        # multipart requests, and so should never happen unless there is a
        # browser bug.

        print $self->header(-status => $err);

        # ThrowCodeError wants to print the header, so it grabs Bugzilla->cgi
        # which creates a new Bugzilla::CGI object, which fails again, which
        # ends up here, and calls ThrowCodeError, and then recurses forever.
        # So don't use it.
        # In fact, we can't use templates at all, because we need a CGI object
        # to determine the template lang as well as the current url (from the
        # template)
        # Since this is an internal error which indicates a severe browser bug,
        # just die.
        die "CGI parsing error: $err";
    }

    return $self;
}

# We want this sorted plus the ability to exclude certain params
sub canonicalise_query {
    my ($self, @exclude) = @_;

    # Reconstruct the URL by concatenating the sorted param=value pairs
    my @parameters;
    foreach my $key (sort($self->param())) {
        # Leave this key out if it's in the exclude list
        next if lsearch(\@exclude, $key) != -1;

        my $esc_key = url_quote($key);

        foreach my $value ($self->param($key)) {
            if (defined($value)) {
                my $esc_value = url_quote($value);

                push(@parameters, "$esc_key=$esc_value");
            }
        }
    }

    return join("&", @parameters);
}

sub clean_search_url {
    my $self = shift;
    # Delete any empty URL parameter
    my @cgi_params = $self->param;

    foreach my $param (@cgi_params) {
        if (defined $self->param($param) && $self->param($param) eq '') {
            $self->delete($param);
            $self->delete("${param}_type");
        }

        # Boolean Chart stuff is empty if it's "noop"
        if ($param =~ /\d-\d-\d/ && defined $self->param($param)
            && $self->param($param) eq 'noop')
        {
            $self->delete($param);
        }
    }

    # Delete certain parameters if the associated parameter is empty.
    $self->delete('bugidtype')  if !$self->param('bug_id');
    $self->delete('emailtype1') if !$self->param('email1');
    $self->delete('emailtype2') if !$self->param('email2');
}

# Overwrite to ensure nph doesn't get set, and unset HEADERS_ONCE
sub multipart_init {
    my $self = shift;

    # Keys are case-insensitive, map to lowercase
    my %args = @_;
    my %param;
    foreach my $key (keys %args) {
        $param{lc $key} = $args{$key};
    }

    # Set the MIME boundary and content-type
    my $boundary = $param{'-boundary'} || '------- =_aaaaaaaaaa0';
    delete $param{'-boundary'};
    $self->{'separator'} = "\r\n--$boundary\r\n";
    $self->{'final_separator'} = "\r\n--$boundary--\r\n";
    $param{'-type'} = SERVER_PUSH($boundary);

    # Note: CGI.pm::multipart_init up to v3.04 explicitly set nph to 0
    # CGI.pm::multipart_init v3.05 explicitly sets nph to 1
    # CGI.pm's header() sets nph according to a param or $CGI::NPH, which
    # is the desired behaviour.

    return $self->header(
        %param,
    ) . "WARNING: YOUR BROWSER DOESN'T SUPPORT THIS SERVER-PUSH TECHNOLOGY." . $self->multipart_end;
}

# Have to add the cookies in.
sub multipart_start {
    my $self = shift;
    
    my %args = @_;

    # CGI.pm::multipart_start doesn't accept a -charset parameter, so
    # we do it ourselves here
    if (defined $args{-charset} && defined $args{-type}) {
        # Remove any existing charset specifier
        $args{-type} =~ s/;.*$//;
        # and add the specified one
        $args{-type} .= "; charset=$args{-charset}";
    }
        
    my $headers = $self->SUPER::multipart_start(%args);
    # Eliminate the one extra CRLF at the end.
    $headers =~ s/$CGI::CRLF$//;
    # Add the cookies. We have to do it this way instead of
    # passing them to multpart_start, because CGI.pm's multipart_start
    # doesn't understand a '-cookie' argument pointing to an arrayref.
    foreach my $cookie (@{$self->{Bugzilla_cookie_list}}) {
        $headers .= "Set-Cookie: ${cookie}${CGI::CRLF}";
    }
    $headers .= $CGI::CRLF;
    return $headers;
}

# Override header so we can add the cookies in
sub header {
    my $self = shift;

    # Add the cookies in if we have any
    if (scalar(@{$self->{Bugzilla_cookie_list}})) {
        if (scalar(@_) == 1) {
            # if there's only one parameter, then it's a Content-Type.
            # Since we're adding parameters we have to name it.
            unshift(@_, '-type' => shift(@_));
        }
        unshift(@_, '-cookie' => $self->{Bugzilla_cookie_list});
    }

    return $self->SUPER::header(@_) || "";
}

# The various parts of Bugzilla which create cookies don't want to have to
# pass them around to all of the callers. Instead, store them locally here,
# and then output as required from |header|.
sub send_cookie {
    my $self = shift;

    # Move the param list into a hash for easier handling.
    my %paramhash;
    my @paramlist;
    my ($key, $value);
    while ($key = shift) {
        $value = shift;
        $paramhash{$key} = $value;
    }

    # Complain if -value is not given or empty (bug 268146).
    if (!exists($paramhash{'-value'}) || !$paramhash{'-value'}) {
        ThrowCodeError('cookies_need_value');
    }

    # Add the default path and the domain in.
    $paramhash{'-path'} = Bugzilla->params->{'cookiepath'};
    $paramhash{'-domain'} = Bugzilla->params->{'cookiedomain'}
        if Bugzilla->params->{'cookiedomain'};

    # Move the param list back into an array for the call to cookie().
    foreach (keys(%paramhash)) {
        unshift(@paramlist, $_ => $paramhash{$_});
    }

    push(@{$self->{'Bugzilla_cookie_list'}}, $self->cookie(@paramlist));
}

# Cookies are removed by setting an expiry date in the past.
# This method is a send_cookie wrapper doing exactly this.
sub remove_cookie {
    my $self = shift;
    my ($cookiename) = (@_);

    # Expire the cookie, giving a non-empty dummy value (bug 268146).
    $self->send_cookie('-name'    => $cookiename,
                       '-expires' => 'Tue, 15-Sep-1998 21:49:00 GMT',
                       '-value'   => 'X');
}

# Redirect to https if required
sub require_https {
    my $self = shift;
    if ($self->protocol ne 'https') {
        my $url = shift;
        if (defined $url) {
            $url .= $self->url('-path_info' => 1, '-query' => 1, '-relative' => 1);
        } else {
            $url = $self->self_url;
            $url =~ s/^http:/https:/i;
        }
        print $self->redirect(-location => $url);
        exit;
    }
}

1;

__END__

=head1 NAME

Bugzilla::CGI - CGI handling for Bugzilla

=head1 SYNOPSIS

  use Bugzilla::CGI;

  my $cgi = new Bugzilla::CGI();

=head1 DESCRIPTION

This package inherits from the standard CGI module, to provide additional
Bugzilla-specific functionality. In general, see L<the CGI.pm docs|CGI> for
documention.

=head1 CHANGES FROM L<CGI.PM|CGI>

Bugzilla::CGI has some differences from L<CGI.pm|CGI>.

=over 4

=item C<cgi_error> is automatically checked

After creating the CGI object, C<Bugzilla::CGI> automatically checks
I<cgi_error>, and throws a CodeError if a problem is detected.

=back

=head1 ADDITIONAL FUNCTIONS

I<Bugzilla::CGI> also includes additional functions.

=over 4

=item C<canonicalise_query(@exclude)>

This returns a sorted string of the parameters, suitable for use in a url.
Values in C<@exclude> are not included in the result.

=item C<send_cookie>

This routine is identical to the cookie generation part of CGI.pm's C<cookie>
routine, except that it knows about Bugzilla's cookie_path and cookie_domain
parameters and takes them into account if necessary.
This should be used by all Bugzilla code (instead of C<cookie> or the C<-cookie>
argument to C<header>), so that under mod_perl the headers can be sent
correctly, using C<print> or the mod_perl APIs as appropriate.

To remove (expire) a cookie, use C<remove_cookie>.

=item C<remove_cookie>

This is a wrapper around send_cookie, setting an expiry date in the past,
effectively removing the cookie.

As its only argument, it takes the name of the cookie to expire.

=item C<require_https($baseurl)>

This routine checks if the current page is being served over https, and
redirects to the https protocol if required, retaining QUERY_STRING.

It takes an option argument which will be used as the base URL.  If $baseurl
is not provided, the current URL is used.

=back

=head1 SEE ALSO

L<CGI|CGI>, L<CGI::Cookie|CGI::Cookie>
