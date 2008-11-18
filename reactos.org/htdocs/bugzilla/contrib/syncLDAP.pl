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
# The Original Code is the LDAP to Bugzilla User Sync Tool.
#
# The Initial Developer of the Original Code is Andreas Höfler.
# Portions created by Andreas Höfler are Copyright (C) 2003
# Andreas Höfler. All Rights Reserved.
#
# Contributor(s): Andreas Höfler <andreas.hoefler@bearingpoint.com>
#

use strict;

use lib qw(.);

use Net::LDAP;
use Bugzilla;
use Bugzilla::User;

my $cgi = Bugzilla->cgi;
my $dbh = Bugzilla->dbh;

my $readonly = 0;
my $nodisable = 0;
my $noupdate = 0;
my $nocreate = 0;
my $quiet    = 0;

###
# Do some preparations
###
foreach my $arg (@ARGV)
{
   if($arg eq '-r') {
      $readonly = 1;
   }
   elsif($arg eq '-d') {
      $nodisable = 1;
   }
   elsif($arg eq '-u') {
      $noupdate = 1;
   }
   elsif($arg eq '-c') {
      $nocreate = 1;
   }
   elsif($arg eq '-q') {
      $quiet = 1;
   }
   else {
         print "LDAP Sync Script\n";
         print "Syncronizes the users table from the LDAP server with the Bugzilla users.\n";
         print "Takes mail-attribute from preferences and description from 'cn' or,\n";
         print "if not available, from the uid-attribute.\n\n";
         print "usage:\n syncLDAP.pl [options]\n\n";
         print "options:\n";
         print " -r Readonly, do not make changes to Bugzilla tables\n";
         print " -d No disable, don't disable users, which are not in LDAP\n";
         print " -u No update, don't update users, which have different description in LDAP\n";
         print " -c No create, don't create users, which are in LDAP but not in Bugzilla\n";
         print " -q Quiet mode, give less output\n";
         print "\n";
         exit;
   }
}

my %ldap_users;

###
# Get current bugzilla users
###
my $bugzilla_users = $dbh->selectall_hashref(
    'SELECT login_name AS new_login_name, realname, disabledtext ' .
    'FROM profiles', 'new_login_name');

foreach my $login_name (keys %$bugzilla_users) {
    # remove whitespaces
    $bugzilla_users->{$login_name}{'realname'} =~ s/^\s+|\s+$//g;
}

###
# Get current LDAP users
###
my $LDAPserver = Bugzilla->params->{"LDAPserver"};
if ($LDAPserver eq "") {
   print "No LDAP server defined in bugzilla preferences.\n";
   exit;
}
my $LDAPport = "389";  # default LDAP port
if($LDAPserver =~ /:/) {
    ($LDAPserver, $LDAPport) = split(":",$LDAPserver);
}

my $LDAPconn = Net::LDAP->new($LDAPserver, port => $LDAPport, version => 3);
if(!$LDAPconn) {
   print "Connecting to LDAP server failed. Check LDAPserver setting.\n";
   exit;
}
my $mesg;
if (Bugzilla->params->{"LDAPbinddn"}) {
    my ($LDAPbinddn,$LDAPbindpass) = split(":",Bugzilla->params->{"LDAPbinddn"});
    $mesg = $LDAPconn->bind($LDAPbinddn, password => $LDAPbindpass);
}
else {
    $mesg = $LDAPconn->bind();
}
if($mesg->code) {
   print "Binding to LDAP server failed: " . $mesg->error . "\nCheck LDAPbinddn setting.\n";
   exit;
}

# We've got our anonymous bind;  let's look up the users.
$mesg = $LDAPconn->search( base   => Bugzilla->params->{"LDAPBaseDN"},
                           scope  => "sub",
                           filter => '(&(' . Bugzilla->params->{"LDAPuidattribute"} . "=*)" . Bugzilla->params->{"LDAPfilter"} . ')',
                         );
                         

if(! $mesg->count) {
   print "LDAP lookup failure. Check LDAPBaseDN setting.\n";
   exit;
}
   
my $val = $mesg->as_struct;

while( my ($key, $value) = each(%$val) ) {

   my $login_name = @$value{Bugzilla->params->{"LDAPmailattribute"}};
   my $realname  = @$value{"cn"};

   # no mail entered? go to next
   if(! defined $login_name) { 
      print "$key has no valid mail address\n";
      next; 
   }

   # no cn entered? use uid instead
   if(! defined $realname) { 
      $realname = @$value{Bugzilla->params->{"LDAPuidattribute"}};
   }
  
   my $login = shift @$login_name;
   my $real = shift @$realname;
   $ldap_users{$login} = { realname => $real };
}

print "\n" unless $quiet;

###
# Sort the users into disable/update/create-Lists and display everything
###
my %disable_users;
my %update_users;
my %create_users;

print "Bugzilla-Users: \n" unless $quiet;
while( my ($key, $value) = each(%$bugzilla_users) ) {
  print " " . $key . " '" . @$value{'realname'} . "' " . @$value{'disabledtext'} ."\n" unless $quiet==1;
  if(!exists $ldap_users{$key}){
     if(@$value{'disabledtext'} eq '') {
       $disable_users{$key} = $value;
     }
  }
}

print "\nLDAP-Users: \n" unless $quiet;
while( my ($key, $value) = each(%ldap_users) ) {
  print " " . $key . " '" . @$value{'realname'} . "'\n" unless $quiet==1;
  if(!defined $bugzilla_users->{$key}){
    $create_users{$key} = $value;
  }
  else { 
    my $bugzilla_user_value = $bugzilla_users->{$key};
    if(@$bugzilla_user_value{'realname'} ne @$value{'realname'}) {
      $update_users{$key} = $value;
    }
  }
}

print "\nDetecting email changes: \n" unless $quiet;
while( my ($create_key, $create_value) = each(%create_users) ) {
  while( my ($disable_key, $disable_value) = each(%disable_users) ) {
    if(@$create_value{'realname'} eq @$disable_value{'realname'}) {
       print " " . $disable_key . " => " . $create_key ."'\n" unless $quiet==1;
       $update_users{$disable_key} = { realname => @$create_value{'realname'},
                                       new_login_name => $create_key };
       delete $create_users{$create_key};
       delete $disable_users{$disable_key};
    }
  }
}

if($quiet == 0) {
   print "\nUsers to disable: \n";
   while( my ($key, $value) = each(%disable_users) ) {
     print " " . $key . " '" . @$value{'realname'} . "'\n";
   }
   
   print "\nUsers to update: \n";
   while( my ($key, $value) = each(%update_users) ) {
     print " " . $key . " '" . @$value{'realname'} . "' ";
     if(defined @$value{'new_login_name'}) {
       print "has changed email to " . @$value{'new_login_name'};
     }
     print "\n";
   }
   
   print "\nUsers to create: \n";
   while( my ($key, $value) = each(%create_users) ) {
     print " " . $key . " '" . @$value{'realname'} . "'\n";
   }
   
   print "\n\n";
}


###
# now do the DB-Update
###
if($readonly == 0) {
   print "Performing DB update:\nPhase 1: disabling not-existing users... " unless $quiet;

   my $sth_disable = $dbh->prepare(
       'UPDATE profiles
           SET disabledtext = ?
         WHERE ' . $dbh->sql_istrcmp('login_name', '?'));

   if($nodisable == 0) {
      while( my ($key, $value) = each(%disable_users) ) {
        $sth_disable->execute('auto-disabled by ldap sync', $key);
      }
      print "done!\n" unless $quiet;
   }
   else {
      print "disabled!\n" unless $quiet;
   }
   
   print "Phase 2: updating existing users... " unless $quiet;

   my $sth_update_login = $dbh->prepare(
       'UPDATE profiles
           SET login_name = ? 
         WHERE ' . $dbh->sql_istrcmp('login_name', '?'));
   my $sth_update_realname = $dbh->prepare(
       'UPDATE profiles
           SET realname = ? 
         WHERE ' . $dbh->sql_istrcmp('login_name', '?'));

   if($noupdate == 0) {
      while( my ($key, $value) = each(%update_users) ) {
        if(defined @$value{'new_login_name'}) {
          $sth_update_login->execute(@$value{'new_login_name'}, $key);
        } else {
          $sth_update_realname->execute(@$value{'realname'}, $key);
        }
      }
      print "done!\n" unless $quiet;
   }
   else {
      print "disabled!\n" unless $quiet;
   }
   
   print "Phase 3: creating new users... " unless $quiet;
   if($nocreate == 0) {
      while( my ($key, $value) = each(%create_users) ) {
        Bugzilla::User->create({
            login_name => $key, 
            realname   => @$value{'realname'},
            cryptpassword   => '*'});
      }
      print "done!\n" unless $quiet;
   }
   else {
      print "disabled!\n" unless $quiet;
   }
}
else
{
   print "No changes to DB because readonly mode\n" unless $quiet;
}

