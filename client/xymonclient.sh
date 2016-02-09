#!/bin/sh
#----------------------------------------------------------------------------#
# Xymon client main script.                                                  #
#                                                                            #
# This invokes the OS-specific script to build a client message, and sends   #
# if off to the Xymon server.                                                #
#                                                                            #
# Copyright (C) 2005-2011 Henrik Storner <henrik@hswn.dk>                    #
#                                                                            #
# This program is released under the GNU General Public License (GPL),       #
# version 2. See the file "COPYING" for details.                             #
#                                                                            #
#----------------------------------------------------------------------------#
#
# $Id: xymonclient.sh 7878 2016-01-26 05:21:46Z jccleaver $

# Must make sure the commands return standard (english) texts.
LANG=C
LC_ALL=C
LC_MESSAGES=C
export LANG LC_ALL LC_MESSAGES

if test $# -ge 1; then
	if test "$1" = "--local"; then
		LOCALMODE="yes"
	fi
	shift
fi

if test "$XYMONOSSCRIPT" = ""; then
	XYMONOSSCRIPT="xymonclient-`uname -s | tr '[ABCDEFGHIJKLMNOPQRSTUVWXYZ/]' '[abcdefghijklmnopqrstuvwxyz_]'`.sh"
fi

MSGFILE="$XYMONTMP/msg.$MACHINEDOTS.txt"
MSGTMPFILE="$MSGFILE.$$"
if test "$LOCALMODE" = "yes"; then
    if test -z "$LOCAL_CLIENTCONFIG"; then
	LOCAL_CLIENTCONFIG="$XYMONHOME/etc/localclient.cfg"
    fi
    if test -z "$LOCAL_LOGFETCHCFG"; then
	LOCAL_LOGFETCHCFG="$XYMONHOME/etc/logfetch.cfg"
    fi

    # If not present, fall back to dynamic logfetch location below
    if test -f "$LOCAL_LOGFETCHCFG"; then
	LOGFETCHCFG="$LOCAL_LOGFETCHCFG"
    fi
fi

if test -z "$LOGFETCHCFG"; then
	LOGFETCHCFG=$XYMONTMP/logfetch.$MACHINEDOTS.cfg
fi
if test -z "$LOGFETCHSTATUS"; then
	LOGFETCHSTATUS=$XYMONTMP/logfetch.$MACHINEDOTS.status
fi

export LOGFETCHCFG LOGFETCHSTATUS

rm -f $MSGTMPFILE
touch $MSGTMPFILE


CLIENTVERSION="`$XYMONHOME/bin/clientupdate --level`"
if test -z "$CLIENTVERSION"; then
	CLIENTVERSION="`$XYMON --version`"
fi

if test "$LOCALMODE" = "yes"; then
	echo "@@client#1|1|127.0.0.1|$MACHINEDOTS|$SERVEROSTYPE" >> $MSGTMPFILE
fi

echo "client $MACHINE.$SERVEROSTYPE $CONFIGCLASS"  >>  $MSGTMPFILE
$XYMONHOME/bin/$XYMONOSSCRIPT >> $MSGTMPFILE
# logfiles
if test -f $LOGFETCHCFG
then
    $XYMONHOME/bin/logfetch $LOGFETCHOPTS $LOGFETCHCFG $LOGFETCHSTATUS >>$MSGTMPFILE
fi
# Client version
echo "[clientversion]"  >>$MSGTMPFILE
echo "$CLIENTVERSION"   >> $MSGTMPFILE

# See if there are any local add-ons (must do this before checking the clock)
if test -d $XYMONHOME/local; then
	for MODULE in $XYMONHOME/local/*
	do
		if test -x $MODULE
		then
			echo "[local:`basename $MODULE`]" >>$MSGTMPFILE
			$MODULE >>$MSGTMPFILE
		fi
	done
fi

# System clock
echo "[clock]"          >> $MSGTMPFILE
$XYMONHOME/bin/logfetch --clock >> $MSGTMPFILE

if test "$LOCALMODE" = "yes"; then
	echo "@@" >> $MSGTMPFILE
	$XYMONHOME/bin/xymond_client --local $LOCAL_CLIENTOPTS --config=$LOCAL_CLIENTCONFIG <$MSGTMPFILE
else
	$XYMON $XYMSRV "@" < $MSGTMPFILE >$LOGFETCHCFG.tmp
	if test -f $LOGFETCHCFG.tmp
	then
		if test -s $LOGFETCHCFG.tmp
		then
			mv $LOGFETCHCFG.tmp $LOGFETCHCFG
		else
			rm -f $LOGFETCHCFG.tmp
		fi
	fi
fi

# Save the latest file for debugging.
rm -f $MSGFILE
mv $MSGTMPFILE $MSGFILE

if test "$LOCALMODE" != "yes" -a -f $LOGFETCHCFG; then
	# Check for client updates
	SERVERVERSION=`grep "^clientversion:" $LOGFETCHCFG | cut -d: -f2`
	if test "$SERVERVERSION" != "" -a "$SERVERVERSION" != "$CLIENTVERSION"; then
		exec $XYMONHOME/bin/clientupdate --update=$SERVERVERSION --reexec
	fi
fi

exit 0

