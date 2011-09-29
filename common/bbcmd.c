/*----------------------------------------------------------------------------*/
/* Hobbit application launcher.                                               */
/*                                                                            */
/* This is used to launch a single Hobbit application, with the environment   */
/* that would normally be established by hobbitlaunch.                        */
/*                                                                            */
/* Copyright (C) 2004-2009 Henrik Storner <henrik@hswn.dk>                    */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: bbcmd.c 6125 2009-02-12 13:09:34Z storner $";

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "libbbgen.h"

static void hobbit_default_envs(char *envfn)
{
	FILE *fd;
	char buf[1024];
	char *evar;
	char *homedir, *p;

	if (getenv("MACHINEDOTS") == NULL) {
		fd = popen("uname -n", "r");
		if (fd) {
			char *p;
			fgets(buf, sizeof(buf), fd);
			p = strchr(buf, '\n'); if (p) *p = '\0';
			pclose(fd);
		}
		else strcpy(buf, "localhost");

		evar = (char *)malloc(strlen(buf) + 13);
		sprintf(evar, "MACHINEDOTS=%s", buf);
		putenv(evar);
	}

	xgetenv("MACHINE");

	if (getenv("BBOSTYPE") == NULL) {
		char *p;

		fd = popen("uname -s", "r");
		if (fd) {
			fgets(buf, sizeof(buf), fd);
			pclose(fd);
		}
		else strcpy(buf, "unix");
		for (p=buf; (*p); p++) *p = (char) tolower((int)*p);

		evar = (char *)malloc(strlen(buf) + 10);
		sprintf(evar, "BBOSTYPE=%s", buf);
		putenv(evar);
	}

	if (getenv("HOBBITCLIENTHOME") == NULL) {
		homedir = strdup(envfn);
		p = strrchr(homedir, '/');
		if (p) {
			*p = '\0';
			if (strlen(homedir) > 4) {
				p = homedir + strlen(homedir) - 4;
				if (strcmp(p, "/etc") == 0) {
					*p = '\0';
					evar = (char *)malloc(20 + strlen(homedir));
					sprintf(evar, "HOBBITCLIENTHOME=%s", homedir);
					putenv(evar);
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	int argi;
	char *cmd = NULL;
	char **cmdargs = NULL;
	int argcount = 0;
	char *envfile = NULL;
	char *envarea = NULL;
	char envfn[PATH_MAX];

	cmdargs = (char **) calloc(argc+2, sizeof(char *));
	for (argi=1; (argi < argc); argi++) {
		if ((argcount == 0) && (strcmp(argv[argi], "--debug") == 0)) {
			debug = 1;
		}
		else if ((argcount == 0) && (argnmatch(argv[argi], "--env="))) {
			char *p = strchr(argv[argi], '=');
			envfile = strdup(p+1);
		}
		else if ((argcount == 0) && (argnmatch(argv[argi], "--area="))) {
			char *p = strchr(argv[argi], '=');
			envarea = strdup(p+1);
		}
		else if ((argcount == 0) && (strcmp(argv[argi], "--version") == 0)) {
			fprintf(stdout, "Hobbit version %s\n", VERSION);
			return 0;
		}
		else {
			if (argcount == 0) {
				cmdargs[0] = cmd = strdup(expand_env(argv[argi]));
				argcount = 1;
			}
			else cmdargs[argcount++] = strdup(expand_env(argv[argi]));
		}
	}

	if (!envfile) {
		struct stat st;

		sprintf(envfn, "%s/etc/hobbitserver.cfg", xgetenv("BBHOME"));
		if (stat(envfn, &st) == -1) sprintf(envfn, "%s/etc/hobbitclient.cfg", xgetenv("BBHOME"));
		errprintf("Using default environment file %s\n", envfn);

		/* Make sure BBOSTYPE, MACHINEDOTS and MACHINE are setup for our child */
		hobbit_default_envs(envfn);
		loadenv(envfn, envarea);
	}
	else {
		/* Make sure BBOSTYPE, MACHINEDOTS and MACHINE are setup for our child */
		hobbit_default_envs(envfile);
		loadenv(envfile, envarea);
	}


	/* Go! */
	if (cmd == NULL) cmd = cmdargs[0] = "/bin/sh";
	execvp(cmd, cmdargs);

	/* Should never go here */
	errprintf("execvp() failed: %s\n", strerror(errno));

	return 0;
}

