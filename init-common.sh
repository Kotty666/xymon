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

	test -d /etc/hobbit/hobbitlaunch.d || return 0
	for cfg in /etc/hobbit/hobbitlaunch.d/*.cfg ; do
		test -e $cfg && echo "include $cfg"
	done > /var/run/hobbit/hobbitlaunch-include.cfg
}
