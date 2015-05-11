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

static char rcsid[] = "$Id: errormsg.c 7612 2015-03-24 00:18:08Z jccleaver $";

#include <sys/types.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "libxymon.h"

char *errbuf = NULL;
static char msg[4096];
static char timestr[20];
static size_t timesize = sizeof(timestr);
static struct timeval now;
static time_t then = 0;
int save_errbuf = 1;
static unsigned int errbufsize = 0;
static char *errappname = NULL;

int debug = 0;
static FILE *tracefd = NULL;
static FILE *debugfd = NULL;

void errprintf(const char *fmt, ...)
{
	va_list args;
	gettimeofday(&now, NULL);

	if (now.tv_sec != then) {
		strftime(timestr, timesize, "%Y-%m-%d %H:%M:%S", localtime(&now.tv_sec));
		then = now.tv_sec;
	}
	fprintf(stderr, "%s.%06d ", timestr, (int) now.tv_usec);
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
}


void real_dbgprintf(const char *fmt, ...)
{
	va_list args;
	gettimeofday(&now, NULL);

	if (!debugfd) debugfd = stdout;
	if (now.tv_sec != then) {
		strftime(timestr, timesize, "%Y-%m-%d %H:%M:%S", localtime(&now.tv_sec));
		then = now.tv_sec;
	}

	fprintf(debugfd, "%lu %s.%06d ", (unsigned long)getpid(), timestr, (int) now.tv_usec);

	va_start(args, fmt);
	vfprintf(debugfd, fmt, args);
	va_end(args);
	fflush(debugfd);
}

void flush_errbuf(void)
{
	if (errbuf) xfree(errbuf);
	errbuf = NULL;
}


void set_debugfile(char *fn, int appendtofile)
{
	/* Always close and reopen when re-setting */
	if (debugfd && (debugfd != stdout) && (debugfd != stderr)) fclose(debugfd);

	if (fn) {
		if (strcasecmp(fn, "stderr") == 0) debugfd = stderr;
		else if (strcasecmp(fn, "stdout") == 0) debugfd = stdout;
		else {
			debugfd = fopen(fn, (appendtofile ? "a" : "w"));
			if (debugfd == NULL) errprintf("Cannot open debug log '%s': %s\n", fn, strerror(errno));
		}
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
		fprintf(tracefd, "%08lu %s ", (unsigned long)getpid(), timestr);

		va_start(args, fmt);
		vfprintf(tracefd, fmt, args);
		va_end(args);
		fflush(tracefd);

		MEMUNDEFINE(timestr);
	}
}

void reopen_file(char *fn, char *mode, FILE *fd)
{
	FILE *testfd;

	testfd = fopen(fn, mode);
	if (!testfd) {
		fprintf(stderr, "reopen_file: Cannot open new file: %s\n", strerror(errno));
		return;
	}
	fclose(testfd);

	if (freopen(fn, mode, fd) == NULL) {
		/* Ugh ... lost the filedescriptor :-(( */
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
	reopen_file(logfn, "a", stderr);

	/* If debugging, setup the debug logfile */
	if (debug) {
		sprintf(logfn, "%s/%s.dbg", cgilogdir, (errappname ? errappname : "cgi"));
		set_debugfile(logfn, 1);
	}
}


