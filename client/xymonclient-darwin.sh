#!/bin/sh
#
#----------------------------------------------------------------------------#
# Darwin (Mac OS X) client for Xymon                                         #
#                                                                            #
# Copyright (C) 2005-2011 Henrik Storner <henrik@hswn.dk>                    #
#                                                                            #
# This program is released under the GNU General Public License (GPL),       #
# version 2. See the file "COPYING" for details.                             #
#                                                                            #
#----------------------------------------------------------------------------#
#
# $Id: xymonclient-darwin.sh 7707 2015-10-19 22:34:59Z jccleaver $

# Use LANG=C, since some locales have different numeric delimiters
# causing the Xymon load-average calculation to fail
LANG=C
export LANG

echo "[date]"
date
echo "[uname]"
uname -a
echo "[uptime]"
uptime
echo "[who]"
who

FILESYSTEMS=`mount | sed -E '/[\( ](nobrowse|afs|read-only)[ ,\)]/d;s/^.* on (.*) \(.*$/\1/'`

echo "[df]"
(IFS=$'\n'
 set $FILESYSTEMS
 df -P -H $1; shift
 while test $# -gt 0
 do
   df -P -H $1 | tail -1 | sed 's/\([^ ]\) \([^ ]\)/\1_\2/g'
   shift
 done) | column -t -s " " | sed -e 's!Mounted *on!Mounted on!'

echo "[inode]"
(IFS=$'\n'
 set $FILESYSTEMS
 df -P -i $1; shift
 while test $# -gt 0
 do
   df -P -H $1 | tail -1 | sed 's/\([^0123456789% ]\) \([^ ]\)/\1_\2/g'
   shift
 done) | awk '
NR<2{printf "%-20s %10s %10s %10s %10s %s\n", $1, "itotal", $6, $7, $8, $9} 
(NR>=2 && $6>0) {printf "%-20s %10d %10d %10d %10s %s\n", $1, $6+$7, $6, $7, $8, $9}'

echo "[mount]"
mount
echo "[meminfo]"
vm_stat
echo "[ifconfig]"
ifconfig -a
echo "[route]"
netstat -rn
echo "[netstat]"
netstat -s
echo "[ifstat]"
netstat -ibn | egrep -v "^lo|<Link"
echo "[ports]"
netstat -an | grep -e "^tcp" -e "^udp"
echo "[ps]"
ps -ax -ww -o pid,ppid,user,start,state,pri,pcpu,time,pmem,rss,vsz,command

# $TOP must be set, the install utility should do that for us if it exists.
if test "$TOP" != "" -a "$AWK" != ""
then
    if test -x "$TOP" -a -x "$AWK"
    then
        echo "[nproc]"
        sysctl -n hw.ncpu
        echo "[top]"
	$TOP -l 2 -n 20 -o cpu | $AWK '/^Processes:/ {toprun++} toprun == 2'
    fi
fi

exit

