/*----------------------------------------------------------------------------*/
/* Xymon monitor library.                                                     */
/*                                                                            */
/* This is a library module, part of libxymon.                                */
/* It contains routines for error- and debug-message display.                 */
/*                                                                            */
/* Copyright (C) 2002-2011 Henrik Storner <henrik@storner.dk>                 */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: errormsg.c 7068 2012-07-14 20:55:50Z storner $";

#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "libxymon.h"

char *errbuf = NULL;
int save_errbuf = 1;
static unsigned int errbufsize = 0;
static char *errappname = NULL;

int debug = 0;
static FILE *tracefd = NULL;
static FILE *debugfd = NULL;

void errprintf(const char *fmt, ...)
{
	char timestr[30];
	char msg[4096];
	va_list args;

	time_t now = getcurrenttime(NULL);

	MEMDEFINE(timestr);
	MEMDEFINE(msg);

	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
	fprintf(stderr, "%s ", timestr);
	if (errappname) fprintf(stderr, "%s ", errappname);

	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);

	fprintf(stderr, "%s", msg);
	fflush(stderr);

	if (save_errbuf) {
		if (errbuf == NULL) {
			errbufsize = 8192;
			errbuf = (char *) malloc(errbufsize);
			*errbuf = '\0';
		}
		else if ((strlen(errbuf) + strlen(msg)) > errbufsize) {
			errbufsize += 8192;
			errbuf = (char *) realloc(errbuf, errbufsize);
		}

		strcat(errbuf, msg);
	}

	MEMUNDEFINE(timestr);
	MEMUNDEFINE(msg);
}


void dbgprintf(const char *fmt, ...)
{
	if (debug) {
		va_list args;
		char timestr[30];
		time_t now = getcurrenttime(NULL);

		MEMDEFINE(timestr);

		if (!debugfd) debugfd = stdout;

		strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S",
			 localtime(&now));
		fprintf(debugfd, "%d %s ", (int)getpid(), timestr);

		va_start(args, fmt);
		vfprintf(debugfd, fmt, args);
		va_end(args);
		fflush(debugfd);

		MEMUNDEFINE(timestr);
	}
}

void flush_errbuf(void)
{
	if (errbuf) xfree(errbuf);
	errbuf = NULL;
}


void set_debugfile(char *fn, int appendtofile)
{
	if (debugfd && (debugfd != stdout)) fclose(debugfd);

	if (fn) {
		debugfd = fopen(fn, (appendtofile ? "a" : "w"));
		if (debugfd == NULL) errprintf("Cannot open debug log %s\n", fn);
	}

	if (!debugfd) debugfd = stdout;
}


void starttrace(const char *fn)
{
	if (tracefd) fclose(tracefd);
	if (fn) {
		tracefd = fopen(fn, "a"); 
		if (tracefd == NULL) errprintf("Cannot open tracefile %s\n", fn);
	}
	else tracefd = stdout;
}

void stoptrace(void)
{
	if (tracefd) fclose(tracefd);
	tracefd = NULL;
}

void traceprintf(const char *fmt, ...)
{
	va_list args;

	if (tracefd) {
		char timestr[40];
		time_t now = getcurrenttime(NULL);

		MEMDEFINE(timestr);

		strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
		fprintf(tracefd, "%08u %s ", (unsigned int)getpid(), timestr);

		va_start(args, fmt);
		vfprintf(tracefd, fmt, args);
		va_end(args);
		fflush(tracefd);

		MEMUNDEFINE(timestr);
	}
}

void redirect_cgilog(char *cginame)
{
	char logfn[PATH_MAX];
	char *cgilogdir;
	FILE *fd;
	
	cgilogdir = getenv("XYMONCGILOGDIR");
	if (!cgilogdir) return;

	if (cginame) errappname = strdup(cginame);
	sprintf(logfn, "%s/cgierror.log", cgilogdir);
	if (fd = fopen(logfn, "a")) {
		fclose(fd);
		fd = freopen(logfn, "a", stderr);
		/* If freopen fails, stderr is now silently invalid. Print a warning to stdout to at least prevent mysterious HTTP 500 errors */
		if (!fd) fprintf(stdout, "Content-type: text/plain\n\nError redirecting CGI stderr to %s\n", logfn);
	} else {
		fprintf(stderr, "Cannot redirect CGI errors to %s\n", logfn);
	}

	/* If debugging, setup the debug logfile */
	if (debug) {
		sprintf(logfn, "%s/%s.dbg", cgilogdir, (errappname ? errappname : "cgi"));
		set_debugfile(logfn, 1);
	}
}

