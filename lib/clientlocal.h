/*----------------------------------------------------------------------------*/
/* Hobbit monitor library.                                                    */
/*                                                                            */
/* Copyright (C) 2004-2009 Henrik Storner <henrik@hswn.dk>                    */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#ifndef __CLIENTLOCAL_H__
#define __CLIENTLOCAL_H__

extern void load_clientconfig(void);
extern char *get_clientconfig(char *hostname, char *hostclass, char *hostos);

#endif
