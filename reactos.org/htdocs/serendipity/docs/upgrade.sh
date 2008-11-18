#!/bin/sh
# File: serendipity_safe_upgrade.sh
# Author: Jez Hancock
#
# Description:
# This shell script performs a backup of the serendipity MySQL database and
# files prior to upgrading from one version to another.
echo "This bash file is out of date and does not respect the new directory structure, templates and compiled templates. It needs to be fixed, so if you don't know how to do bash scripting, please don't use this file. Use the web-based installer instead. If you DO want to make this file work, make sure you sure know how to do it. :-)";
exit;

########################################################################
# Configuration options start
########################################################################
# Change this to the IP address from which you will be updgrading the blog from
# in a browser.  This IP address will be used in a .htaccess file to restrict
# access during the upgrade process:
ip=192.168.0.1

# Change this to the directory containing the current serendipity web files:
blog_dir=/usr/local/www/htdocs/serendipity

# Change this to the directory containing the new serendipity web files:
# This should be the directory containing the cvs version of s9y or the s9y
# tarball contents:
import_dir=/home/username/serendipity

# Change this to the directory you wish your weblog files and database to be
# backed up to.  A subdirectory 's9y.YYYYMMDD' will be created under here into
# which a tarball will be created containing the backed up webfiles and
# database.  

# For example if your backup_dir is set to '/home/user/backup', a directory
# called '/home/user/backup/serendipity.20040406' will be created to contain
# the backup tarball 'serendipity_backup.tar.gz' and the backup database file
# 'serendipity_backup.sql'.
# Note if the backup directory already contains a backup from the current day,
# the script will exit without doing anything.
backup_dir=/home/username/backup

# Change this to match your database user/name.  Note you will be prompted for
# your password on the commandline.
db_name="serendipity"
db_user="serendipity"

########################################################################
# Configuration options end
# You should not need to modify anything below here!
########################################################################

ok() { 
	echo "[OK]" ; echo ""
}

nl() {
	echo ""
}

# the date ... ymd=20040331 etc ... used in backup
ymd=`date "+%Y%m%d"`

# Our backup directory:
backup_dir=$backup_dir/serendipity.$ymd

# if the backup dir exists already, exit gracefully:
if [ -d $backup_dir ];then
	echo "$backup_dir already exists - exiting."
	exit
fi

# make backup dir:
echo "Creating backup directory:"
nl
echo "$backup_dir"
mkdir $backup_dir && ok || \
	echo "Could not create $backup_dir - exiting."

# backup current weblog to a tarball in the backup dir:
echo "Backing up weblog files from:"
nl
echo "$blog_dir"
nl
echo "to the tarball:"
nl
echo "$backup_dir/serendipity_backup.tar.gz"
set `echo "$blog_dir" | sed -e 's,\(.*\)/\(.*\),\1 \2,'`
tar zcf $backup_dir/serendipity_backup.tar.gz -C $1 $2 && ok

echo "Backing up database $db_name to file:"
nl
echo "$backup_dir/serendipity_backup.sql"
nl
echo -n "MySQL user $db_user - "
mysqldump -u$db_user -p $db_name > $backup_dir/serendipity_backup.sql \
	&& ok

# make backup dir safe:
echo "Changing perms on $backup_dir to 700"
chmod 700 $backup_dir && ok

# move current blogdir out of way:
echo "Moving:"
nl
echo "$blog_dir"
nl
echo "to:"
nl
echo "$blog_dir.$ymd"
mv $blog_dir $blog_dir.$ymd && ok

# now copy in the new files:
echo "Copying files from:"
nl
echo "$import_dir"
nl
echo "to:"
nl
echo "$blog_dir"
cp -R $import_dir $blog_dir && ok

# check if a .htaccess file exists in old blog dir - if so copy over:
if [ -f "$blog_dir.$ymd/.htaccess" ]; then
	cp $blog_dir.$ymd/.htaccess $blog_dir
fi

# allow only our ip during upgrade - remember to remove these lines 
# after you've finished updating!:
echo "Adding .htaccess directives to restrict browser access to blog to $ip"
echo "during upgrade process"
exec >> $blog_dir/.htaccess
echo "deny from all" 
echo "allow from $ip"
echo 'ErrorDocument 403 "Upgrading, please check back soon!!!'
exec > /dev/tty
ok

# copy old uploads folder over:
if [ -d $blog_dir.$ymd/uploads ];then
	echo "Copying old uploads folder from:"
	nl
	echo "$blog_dir.$ymd/uploads"
	nl
	echo "to:"
	nl
	echo "$blog_dir/uploads"
	cp -R $blog_dir.$ymd/uploads $blog_dir/uploads && ok
fi


# most importantly don't forget the local config file - this is checked
# to see whether or not your current config setup needs upgrading or
# not:
echo "Copying $blog_dir.$ymd/serendipity_config_local.inc.php to $blog_dir"
cp $blog_dir.$ymd/serendipity_config_local.inc.php $blog_dir \
	&& ok

echo "########################################################################"
echo "Important!"
echo "########################################################################"
echo "Backup is now complete.  Continue to upgrade your serendipity"
echo "installation by browsing to it in a web browser - you will be prompted"
echo "with instructions from there."
nl
echo "After completing the upgrade via a browser, remember to remove the lines"
echo "starting:"
nl
echo "deny from all"
echo "allow from $ip"
echo "ErrorDocument 403 ..."
nl
echo "from the .htaccess file:"
nl
echo "$blog_dir/.htaccess"
nl
nl
echo "A copy of the original serendipity web folder can be found here:"
nl
echo "$blog_dir.$ymd"
nl
echo "After confirming the upgrade was successful, you can safely remove"
echo "this directory."

# This is the place to do any custom backup stuff - for example I have various
# directory structures that I copy from my blog base directory, a few custom
# plugins I copy, etc etc.
# ...

# Uncomment/modify the following line if you run the server:
#chown -R www:www $blog_dir && chmod 600 $blog_dir/serendipity_config_local.inc.php
