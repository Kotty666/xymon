# common init functions for hobbit and hobbit-client

create_includefiles ()
{
	if [ "$HOBBITSERVERS" = "" ]; then
		echo "Please configure HOBBITSERVERS in /etc/default/hobbit-client"
		exit 0
	fi

	set -- $HOBBITSERVERS
	if [ $# -eq 1 ]; then
		echo "BBDISP=\"$HOBBITSERVERS\""
		echo "BBDISPLAYS=\"\""
	else
		echo "BBDISP=\"0.0.0.0\""
		echo "BBDISPLAYS=\"$HOBBITSERVERS\""
	fi > /var/run/hobbit/bbdisp-runtime.cfg

	for cfg in /etc/hobbit/clientlaunch.d/*.cfg ; do
		test -e $cfg && echo "include $cfg"
	done > /var/run/hobbit/clientlaunch-include.cfg

	if test -d /etc/hobbit/hobbitlaunch.d ; then
		for cfg in /etc/hobbit/hobbitlaunch.d/*.cfg ; do
			test -e $cfg && echo "include $cfg"
		done > /var/run/hobbit/hobbitlaunch-include.cfg
	fi

	if test -d /etc/hobbit/hobbitgraph.d ; then
		for cfg in /etc/hobbit/hobbitgraph.d/*.cfg ; do
			test -e $cfg && echo "include $cfg"
		done > /var/run/hobbit/hobbitgraph-include.cfg
	fi

	if test -d /etc/hobbit/hobbitserver.d ; then
		for cfg in /etc/hobbit/hobbitserver.d/*.cfg ; do
			test -e $cfg && echo "include $cfg"
		done > /var/run/hobbit/hobbitserver-include.cfg
	fi

	return 0
}
