#!/bin/sh
#
#----------------------------------------------------------------------------#
# FreeBSD client for Xymon                                                   #
#                                                                            #
# Copyright (C) 2005-2011 Henrik Storner <henrik@hswn.dk>                    #
#                                                                            #
# This program is released under the GNU General Public License (GPL),       #
# version 2. See the file "COPYING" for details.                             #
#                                                                            #
#----------------------------------------------------------------------------#
#
# $Id: xymonclient-freebsd.sh 7707 2015-10-19 22:34:59Z jccleaver $

echo "[date]"
date
echo "[uname]"
uname -a
echo "[uptime]"
uptime
echo "[who]"
who
echo "[df]"
# The sed stuff is to make sure lines are not split into two.
df -H -tnonfs,nullfs,cd9660,procfs,devfs,linprocfs,fdescfs | sed -e '/^[^ 	][^ 	]*$/{
N
s/[ 	]*\n[ 	]*/ /
}'
echo "[inode]"
# The sed stuff is to make sure lines are not split into two.
df -i -tnonfs,nullfs,cd9660,procfs,devfs,linprocfs,fdescfs | sed -e '/^[^ 	][^ 	]*$/{
N
s/[ 	]*\n[ 	]*/ /
}' | awk '
NR<2{printf "%-20s %10s %10s %10s %10s %s\n", $1, "itotal", $6, $7, $8, $9} 
NR>=2{printf "%-20s %10d %10d %10d %10s %s\n", $1, $6+$7, $6, $7, $8, $9}'
echo "[mount]"
mount
echo "[meminfo]"
$XYMONHOME/bin/freebsd-meminfo
echo "[swapinfo]"
swapinfo -k
echo "[vmtotal]"
sysctl vm.vmtotal
echo "[ifconfig]"
ifconfig -a
echo "[route]"
netstat -rn
echo "[ifstat]"
netstat -i -b -n | egrep -v "^lo|<Link"
echo "[netstat]"
netstat -s
echo "[ports]"
(netstat -Wna -f inet; netstat -Wna -f inet6) | grep "^tcp"
echo "[ps]"
# Don't use "args". "command" works also in FreeBSD 4.x.
ps -axd -ww -o pid,ppid,user,start,state,pri,pcpu,cputime,pmem,rss,vsz,command

# $TOP must be set, the install utility should do that for us if it exists.
if test "$TOP" != ""
then
    if test -x "$TOP"
    then
        echo "[nproc]"
	sysctl -n hw.ncpu
        echo "[top]"
	$TOP -n 20
    fi
fi

# vmstat
nohup sh -c "vmstat 300 2 1>$XYMONTMP/xymon_vmstat.$MACHINEDOTS.$$ 2>&1; mv $XYMONTMP/xymon_vmstat.$MACHINEDOTS.$$ $XYMONTMP/xymon_vmstat.$MACHINEDOTS" </dev/null >/dev/null 2>&1 &
sleep 5
if test -f $XYMONTMP/xymon_vmstat.$MACHINEDOTS; then echo "[vmstat]"; cat $XYMONTMP/xymon_vmstat.$MACHINEDOTS; rm -f $XYMONTMP/xymon_vmstat.$MACHINEDOTS; fi

exit

