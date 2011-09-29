#!/bin/sh
#----------------------------------------------------------------------------#
# Hobbit client bootup script.                                               #
#                                                                            #
# This invokes hobbitlaunch, which in turn runs the Hobbit client and any    #
# extensions configured.                                                     #
#                                                                            #
# Copyright (C) 2005-2009 Henrik Storner <henrik@hswn.dk>                    #
# "status" section (C) Scott Smith 2006                                      #
#                                                                            #
# This program is released under the GNU General Public License (GPL),       #
# version 2. See the file "COPYING" for details.                             #
#                                                                            #
#----------------------------------------------------------------------------#
#
# $Id: runclient.sh 6158 2009-02-22 10:34:14Z storner $

# Default settings for this client
MACHINEDOTS="`uname -n`"			# This systems hostname
BBOSTYPE="`uname -s | tr '[ABCDEFGHIJKLMNOPQRSTUVWXYZ/]' '[abcdefghijklmnopqrstuvwxyz_]'`"	# This systems operating system in lowercase
BBOSSCRIPT="hobbitclient-$BBOSTYPE.sh"

# Commandline mods for the defaults
while test "$1" != ""
do
	case "$1" in
	  --hostname=*)
	  	MACHINEDOTS="`echo $1 | sed -e 's/--hostname=//'`"
		;;
	  --os=*)
	  	BBOSTYPE="`echo $1 | sed -e 's/--os=//' | tr '[ABCDEFGHIJKLMNOPQRSTUVWXYZ/]' '[abcdefghijklmnopqrstuvwxyz_]'`"
		;;
	  --class=*)
	        CONFIGCLASS="`echo $1 | sed -e 's/--class=//' | tr '[ABCDEFGHIJKLMNOPQRSTUVWXYZ/]' '[abcdefghijklmnopqrstuvwxyz_]'`"
		;;
	  --help)
	  	echo "Usage: $0 [--hostname=CLIENTNAME] [--os=rhel3|linux22] [--class=CLASSNAME] start|stop"
		exit 0
		;;
	  start)
	  	CMD=$1
		;;
	  stop)
	  	CMD=$1
		;;
	  restart)
	  	CMD=$1
		;;
	  status)
	  	CMD=$1
		;;
	esac

	shift
done

HOBBITCLIENTHOME="`dirname $0`"
export MACHINEDOTS BBOSTYPE BBOSSCRIPT HOBBITCLIENTHOME CONFIGCLASS

MACHINE="`echo $MACHINEDOTS | sed -e 's/\./,/g'`"
export MACHINE

case "$CMD" in
  "start")
  	if test ! -w $HOBBITCLIENTHOME/logs; then
		echo "Cannot write to the $HOBBITCLIENTHOME/logs directory"
		exit 1
	fi
  	if test ! -w $HOBBITCLIENTHOME/tmp; then
		echo "Cannot write to the $HOBBITCLIENTHOME/tmp directory"
		exit 1
	fi

  	if test -s $HOBBITCLIENTHOME/logs/clientlaunch.$MACHINEDOTS.pid; then
		echo "Hobbit client already running, re-starting it"
		$0 --hostname="$MACHINEDOTS" stop
		rm -f $HOBBITCLIENTHOME/logs/clientlaunch.$MACHINEDOTS.pid
	fi

	$HOBBITCLIENTHOME/bin/hobbitlaunch --config=$HOBBITCLIENTHOME/etc/clientlaunch.cfg --log=$HOBBITCLIENTHOME/logs/clientlaunch.log --pidfile=$HOBBITCLIENTHOME/logs/clientlaunch.$MACHINEDOTS.pid
	if test $? -eq 0; then
		echo "Hobbit client for $BBOSTYPE started on $MACHINEDOTS"
	else
		echo "Hobbit client startup failed"
	fi
	;;

  "stop")
  	if test -s $HOBBITCLIENTHOME/logs/clientlaunch.$MACHINEDOTS.pid; then
		kill `cat $HOBBITCLIENTHOME/logs/clientlaunch.$MACHINEDOTS.pid`
		echo "Hobbit client stopped"
	else
		echo "Hobbit client not running"
	fi
	;;

  "restart")
  	if test -s $HOBBITCLIENTHOME/logs/clientlaunch.$MACHINEDOTS.pid; then
		$0 --hostname="$MACHINEDOTS" stop
	else
		echo "Hobbit client not running, continuing to start it"
	fi

	$0 --hostname="$MACHINEDOTS" --os="$BBOSTYPE" start
	;;

  "status")
	if test -s $HOBBITCLIENTHOME/logs/clientlaunch.$MACHINEDOTS.pid
	then
		kill -0 `cat $HOBBITCLIENTHOME/logs/clientlaunch.$MACHINEDOTS.pid`
		if test $? -eq 0
		then
			echo "Hobbit client (clientlaunch) running with PID `cat $HOBBITCLIENTHOME/logs/clientlaunch.$MACHINEDOTS.pid`"
		else
			echo "Hobbit client not running, removing stale PID file"
			rm -f $HOBBITCLIENTHOME/logs/clientlaunch.$MACHINEDOTS.pid
		fi
	else
		echo "Hobbit client (clientlaunch) does not appear to be running"
	fi
	;;

  *)
	echo "Usage: $0 start|stop|restart|status"
	break;

esac

exit 0

