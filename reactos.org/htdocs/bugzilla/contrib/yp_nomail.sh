#!/bin/sh
# -*- Mode: ksh -*-
##############################################################################
# $Id: yp_nomail.sh,v 1.1 2000/09/12 23:50:31 cyeh%bluemartini.com Exp $
# yp_nomail
#
# Our mail admins got annoyed when bugzilla kept sending email
# to people who'd had bugzilla entries and left the company.  They
# were no longer in the list of valid email users so it'd bounce.
# Maintaining the 'data/nomail' file was a pain.  Luckily, our UNIX
# admins list all the users that ever were, but the people who've left
# have a distinct marker in their password file. For example:
#
# fired:*LK*:2053:1010:You're Fired Dude:/home/loser:/bin/false
#
# This script takes advantage of the "*LK*" convention seen via 
# ypcat passwd and dumps those people into the nomail file. Any
# manual additions are kept in a "nomail.(domainname)" file and 
# appended to the list of yp lockouts every night via Cron
#
# 58 23 * * * /export/bugzilla/contrib/yp_nomail.sh > /dev/null 2>&1
#
# Tak ( Mark Takacs ) 08/2000
#
# XXX: Maybe should crosscheck w/bugzilla users?
##############################################################################

####
# Configure this section to suite yer installation
####

DOMAIN=`domainname`
MOZILLA_HOME="/export/mozilla"
BUGZILLA_HOME="${MOZILLA_HOME}/bugzilla"
NOMAIL_DIR="${BUGZILLA_HOME}/data"
NOMAIL="${NOMAIL_DIR}/nomail"
NOMAIL_ETIME="${NOMAIL}.${DOMAIN}"
NOMAIL_YP="${NOMAIL}.yp"
FIRED_FLAG="\*LK\*"

YPCAT="/usr/bin/ypcat"
GREP="/usr/bin/grep"
SORT="/usr/bin/sort"

########################## no more config needed  #################

# This dir comes w/Bugzilla. WAY too paranoid
if [ ! -d ${NOMAIL_DIR} ] ; then
    echo "Creating $date_dir"
    mkdir -p ${NOMAIL_DIR}
fi

#
# Do some (more) paranoid checking
#
touch ${NOMAIL}
if [ ! -w ${NOMAIL} ] ; then
    echo "Can't write nomail file: ${NOMAIL} -- exiting"
    exit
fi
if [ ! -r ${NOMAIL_ETIME} ] ; then
    echo "Can't access custom nomail file: ${NOMAIL_ETIME} -- skipping"
    NOMAIL_ETIME=""
fi

#
# add all the people with '*LK*' password to the nomail list
# XXX: maybe I should customize the *LK* string. Doh.
#

LOCKOUT=`$YPCAT passwd | $GREP "${FIRED_FLAG}" | cut -d: -f1 | sort > ${NOMAIL_YP}`
`cat ${NOMAIL_YP} ${NOMAIL_ETIME} > ${NOMAIL}`

exit


# end

