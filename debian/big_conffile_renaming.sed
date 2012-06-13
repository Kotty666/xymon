#!/usr/bin/sed -f

/^#BIG_CONFFILE_RENAMING#/ {
r debian/big_conffile_renaming
d
}
