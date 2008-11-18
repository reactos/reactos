#!/usr/local/bin/ruby

# Queries an LDAP server for all email addresses (tested against Exchange 5.5),
# and makes nice bugzilla user entries out of them. Also disables Bugzilla users
# that are not found in LDAP.

# $Id: bugzilla_ldapsync.rb,v 1.2 2003/04/26 16:35:04 jake%bugzilla.org Exp $

require 'ldap'
require 'dbi'
require 'getoptlong'

opts = GetoptLong.new(
    ['--dbname',        '-d', GetoptLong::OPTIONAL_ARGUMENT],
    ['--dbpassword',    '-p', GetoptLong::OPTIONAL_ARGUMENT],
    ['--dbuser',        '-u',     GetoptLong::OPTIONAL_ARGUMENT],
    ['--dbpassfile',    '-P', GetoptLong::OPTIONAL_ARGUMENT],
    ['--ldaphost',      '-h', GetoptLong::REQUIRED_ARGUMENT],
    ['--ldapbase',      '-b', GetoptLong::OPTIONAL_ARGUMENT],
    ['--ldapquery',     '-q', GetoptLong::OPTIONAL_ARGUMENT],
    ['--maildomain',    '-m', GetoptLong::OPTIONAL_ARGUMENT],
    ['--noremove',      '-n', GetoptLong::OPTIONAL_ARGUMENT],
    ['--defaultpass',   '-D', GetoptLong::OPTIONAL_ARGUMENT],
    ['--checkmode',     '-c', GetoptLong::OPTIONAL_ARGUMENT]
)


# in hash to make it easy
optHash = Hash.new
opts.each do |opt, arg|
	optHash[opt]=arg
end    

# grab password from file if it's an option
if optHash['--dbpassfile']
    dbPassword=File.open(optHash['--dbpassfile'], 'r').readlines[0].chomp!
else
    dbPassword=optHash['--dbpassword'] || nil
end

# make bad assumptions.
dbName = optHash['--dbname'] || 'bugzilla'
dbUser = optHash['--dbuser'] || 'bugzilla'
ldapHost = optHash['--ldaphost'] || 'ldap'
ldapBase = optHash['--ldapbase'] || ''
mailDomain = optHash['--maildomain'] || `domainname`.chomp!
ldapQuery = optHash['--ldapquery'] || "(&(objectclass=person)(rfc822Mailbox=*@#{mailDomain}))"
checkMode = optHash['--checkmode'] || nil
noRemove = optHash['--noremove'] || nil
defaultPass = optHash['--defaultpass'] || 'bugzilla'

if (! dbPassword)
    puts "bugzilla_ldapsync v1.3 (c) 2003 Thomas Stromberg <thomas+bugzilla@stromberg.org>"
    puts ""
    puts " -d | --dbname        name of MySQL database            [#{dbName}]"
    puts " -u | --dbuser        username for MySQL database       [#{dbUser}]"
    puts " -p | --dbpassword    password for MySQL user           [#{dbPassword}]"
    puts " -P | --dbpassfile    filename containing password for MySQL user"
    puts " -h | --ldaphost      hostname for LDAP server          [#{ldapHost}]"
    puts " -b | --ldapbase      Base of LDAP query, for instance, o=Bugzilla.com"
    puts " -q | --ldapquery     LDAP query, uses maildomain       [#{ldapQuery}]"
    puts " -m | --maildomain    e-mail domain to use records from"
    puts " -n | --noremove      do not remove Bugzilla users that are not in LDAP"
    puts " -c | --checkmode     checkmode, does not perform any SQL changes"
    puts " -D | --defaultpass   default password for new users    [#{defaultPass}]"
    puts
    puts "example:"
    puts
    puts " bugzilla_ldapsync.rb -c -u taskzilla -P /tmp/test -d taskzilla -h bhncmail -m \"bowebellhowell.com\""
    exit
end


if (checkMode)
    puts '(checkmode enabled, no SQL writes will actually happen)'
    puts "ldapquery is #{ldapQuery}"
    puts
end
    

bugzillaUsers = Hash.new
ldapUsers = Hash.new
encPassword = defaultPass.crypt('xx')
sqlNewUser = "INSERT INTO profiles VALUES ('', ?, '#{encPassword}', ?, '', 1, NULL, '0000-00-00 00:00:00');"

# presumes that the MySQL database is local.
dbh = DBI.connect("DBI:Mysql:#{dbName}", dbUser, dbPassword)

# select all e-mail addresses where there is no disabledtext defined. Only valid users, please!
dbh.select_all('select login_name, realname, disabledtext from profiles') { |row|
    login = row[0].downcase
    bugzillaUsers[login] = Hash.new
    bugzillaUsers[login]['desc'] = row[1]
    bugzillaUsers[login]['disabled'] = row[2]
    #puts "bugzilla has #{login} - \"#{bugzillaUsers[login]['desc']}\" (#{bugzillaUsers[login]['disabled']})"
}


LDAP::Conn.new(ldapHost, 389).bind{|conn|
  sub = nil
  # perform the query, but only get the e-mail address, location, and name returned to us.
  conn.search(ldapBase, LDAP::LDAP_SCOPE_SUBTREE, ldapQuery,  
  	['rfc822Mailbox', 'physicalDeliveryOfficeName', 'cn'])  { |entry|
	
		# Get the users first (primary) e-mail address, but I only want what's before the @ sign.
		entry.vals("rfc822Mailbox")[0] =~ /([\w\.-]+)\@/
		email = $1
	
		# We put the officename in the users description, and nothing otherwise.
		if entry.vals("physicalDeliveryOfficeName")
			location = entry.vals("physicalDeliveryOfficeName")[0]
		else
			location = ''
		end
	
		# for whatever reason, we get blank entries. Do some double checking here.
		if (email && (email.length > 4) && (location !~ /Generic/) && (entry.vals("cn")))		
			if (location.length > 2) 
				desc = entry.vals("cn")[0] + " (" + location + ")"
			else
				desc = entry.vals("cn")[0]
			end
            
            # take care of the whitespace.
            desc.sub!("\s+$", "")
            desc.sub!("^\s+", "")
            
            # dumb hack. should be properly escaped, and apostrophes should never ever ever be in email.
			email.sub!("\'", "%")
			email.sub!('%', "\'")
            email=email.downcase
            ldapUsers[email.downcase] = Hash.new
            ldapUsers[email.downcase]['desc'] = desc
            ldapUsers[email.downcase]['disabled'] = nil
            #puts "ldap has #{email} - #{ldapUsers[email.downcase]['desc']}"
        end
	}
}

# This is the loop that takes the users that we found in Bugzilla originally, and 
# checks to see if they are still in the LDAP server. If they are not, away they go!

ldapUsers.each_key { |user|
    # user does not exist at all.
    #puts "checking ldap user #{user}"
    if (! bugzillaUsers[user])
        puts "+ Adding #{user} - #{ldapUsers[user]['desc']}"

        if (! checkMode) 
            dbh.do(sqlNewUser, user, ldapUsers[user]['desc'])
        end
        
        # short-circuit now.
        next
     end

     if (bugzillaUsers[user]['desc'] != ldapUsers[user]['desc'])
     puts "* Changing #{user} from \"#{bugzillaUsers[user]['desc']}\" to \"#{ldapUsers[user]['desc']}\""
         if (! checkMode)
            # not efficient.
            dbh.do("UPDATE profiles SET realname = ? WHERE login_name = ?", ldapUsers[user]['desc'], user)
         end
     end

     if (bugzillaUsers[user]['disabled'].length > 0)
         puts "+ Enabling #{user} (was \"#{bugzillaUsers[user]['disabled']}\")" 
         if (! checkMode)
             dbh.do("UPDATE profiles SET disabledtext = NULL WHERE login_name=\"#{user}\"")
         end
     end
}

if (! noRemove)
    bugzillaUsers.each_key { |user|
        if ((bugzillaUsers[user]['disabled'].length < 1) && (! ldapUsers[user]))
            puts "- Disabling #{user} (#{bugzillaUsers[user]['disabled']})"

            if (! checkMode)
                 dbh.do("UPDATE profiles SET disabledtext = \'auto-disabled by ldap sync\' WHERE login_name=\"#{user}\"")
            end
        end
    }
end
dbh.disconnect

