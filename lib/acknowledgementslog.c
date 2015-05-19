/*----------------------------------------------------------------------------*/
/* Xymon monitor library.                                                     */
/*                                                                            */
/* This displays the "acknowledgements" log.                                  */
/*                                                                            */
/* Copyright (C) 2002-2011 Henrik Storner <henrik@storner.dk>                 */
/* Host/test/color/start/end filtering code by Eric Schwimmer 2005            */
/*                                                                            */
/* This program is released under the GNU General Public License (GPL),       */
/* version 2. See the file "COPYING" for details.                             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

static char rcsid[] = "$Id: acknowledgementslog.c 7085 2012-07-16 11:08:37Z storner $";

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <pcre.h>

#include "libxymon.h"

typedef struct acknowledgements_t {
	void *host;
	struct htnames_t *service;
	time_t  eventtime;
	char *recipient;
	char *message;
	struct acknowledgements_t *next;
} acknowledgements_t;


static time_t convert_time(char *timestamp)
{
	time_t event = 0;
	unsigned int year,month,day,hour,min,sec,count;
	struct tm timeinfo;

	count = sscanf(timestamp, "%u/%u/%u@%u:%u:%u",
		&year, &month, &day, &hour, &min, &sec);
	if(count != 6) {
		return -1;
	}
	if(year < 1970) {
		return 0;
	}
	else {
		memset(&timeinfo, 0, sizeof(timeinfo));
		timeinfo.tm_year  = year - 1900;
		timeinfo.tm_mon   = month - 1;
		timeinfo.tm_mday  = day;
		timeinfo.tm_hour  = hour;
		timeinfo.tm_min   = min;
		timeinfo.tm_sec   = sec;
		timeinfo.tm_isdst = -1;
		event = mktime(&timeinfo);		
	}

	return event;
}

static htnames_t *namehead = NULL;
static htnames_t *getname(char *name, int createit)
{
	htnames_t *walk;

	for (walk = namehead; (walk && strcmp(walk->name, name)); walk = walk->next) ;
	if (walk || (!createit)) return walk;

	walk = (htnames_t *)malloc(sizeof(htnames_t));
	walk->name = strdup(name);
	walk->next = namehead;
	namehead = walk;

	return walk;
}

void do_acknowledgementslog(FILE *output, 
		  int maxcount, int maxminutes, char *fromtime, char *totime, 
		  char *pageregex, char *expageregex,
		  char *hostregex, char *exhostregex,
		  char *testregex, char *extestregex,
		  char *rcptregex, char *exrcptregex)
{
	FILE *acknowledgementslog;
	char acknowledgementslogfilename[PATH_MAX];
	time_t firstevent = 0;
	time_t lastevent = getcurrenttime(NULL);
	acknowledgements_t *head, *walk;
	struct stat st;
	char l[MAX_LINE_LEN];
	char title[200];

	/* For the PCRE matching */
	const char *errmsg = NULL;
	int errofs = 0;
	pcre *pageregexp = NULL;
	pcre *expageregexp = NULL;
	pcre *hostregexp = NULL;
	pcre *exhostregexp = NULL;
	pcre *testregexp = NULL;
	pcre *extestregexp = NULL;
	pcre *rcptregexp = NULL;
	pcre *exrcptregexp = NULL;

	if (maxminutes && (fromtime || totime)) {
		fprintf(output, "<B>Only one time interval type is allowed!</B>");
		return;
	}

	if (fromtime) {
		firstevent = convert_time(fromtime);
		if(firstevent < 0) {
			fprintf(output,"<B>Invalid 'from' time: %s</B>", htmlquoted(fromtime));
			return;
		}
	}
	else if (maxminutes) {
		firstevent = getcurrenttime(NULL) - maxminutes*60;
	}
	else {
		firstevent = getcurrenttime(NULL) - 86400;
	}

	if (totime) {
		lastevent = convert_time(totime);
		if (lastevent < 0) {
			fprintf(output,"<B>Invalid 'to' time: %s</B>", htmlquoted(totime));
			return;
		}
		if (lastevent < firstevent) {
			fprintf(output,"<B>'to' time must be after 'from' time.</B>");
			return;
		}
	}

	if (!maxcount) maxcount = 100;

	if (pageregex && *pageregex) pageregexp = pcre_compile(pageregex, PCRE_CASELESS, &errmsg, &errofs, NULL);
	if (expageregex && *expageregex) expageregexp = pcre_compile(expageregex, PCRE_CASELESS, &errmsg, &errofs, NULL);
	if (hostregex && *hostregex) hostregexp = pcre_compile(hostregex, PCRE_CASELESS, &errmsg, &errofs, NULL);
	if (exhostregex && *exhostregex) exhostregexp = pcre_compile(exhostregex, PCRE_CASELESS, &errmsg, &errofs, NULL);
	if (testregex && *testregex) testregexp = pcre_compile(testregex, PCRE_CASELESS, &errmsg, &errofs, NULL);
	if (extestregex && *extestregex) extestregexp = pcre_compile(extestregex, PCRE_CASELESS, &errmsg, &errofs, NULL);
	if (rcptregex && *rcptregex) rcptregexp = pcre_compile(rcptregex, PCRE_CASELESS, &errmsg, &errofs, NULL);
	if (exrcptregex && *exrcptregex) exrcptregexp = pcre_compile(exrcptregex, PCRE_CASELESS, &errmsg, &errofs, NULL);

	sprintf(acknowledgementslogfilename, "%s/acknowledge.log", xgetenv("XYMONSERVERLOGS"));
	acknowledgementslog = fopen(acknowledgementslogfilename, "r");

	if (acknowledgementslog && (stat(acknowledgementslogfilename, &st) == 0)) {
		time_t curtime;
		int done = 0;

		/* Find a spot in the acknowledgements log file close to where the firstevent time is */
		fseeko(acknowledgementslog, 0, SEEK_END);
		do {
			/* Go back maxcount*80 bytes - one entry is ~80 bytes */
			if (ftello(acknowledgementslog) > maxcount*80) {
				unsigned int uicurtime;
				fseeko(acknowledgementslog, -maxcount*80, SEEK_CUR); 
				if (fgets(l, sizeof(l), acknowledgementslog) && /* Skip to start of line */
				    fgets(l, sizeof(l), acknowledgementslog)) {
                                        /* 2015-03-07 18:17:03 myserver disk andy 1 1425724570 1425752223 1425838623 testing message */
					if ( sscanf(l, "%*u-%*u-%*u %*u:%*u:%*u %*s %*s %*s %*u %*u %u %*u %*s", &uicurtime) == 0 ) {
					    /* that didnt work - try the old format
						1430040985      630949  30      630949  np_filename_not_used    myserver.procs red     testing log format \nAcked by: andy (127.0.0.1) */
					    sscanf(l, "%u\t%*u\t%*u\t%*u\tnp_filename_not_used\t%*s\t%*s\t%*s", &uicurtime);
					}
					curtime = uicurtime;
					done = (curtime < firstevent);
				}
				else { 
					fprintf(output, "Error reading logfile %s: %s\n", acknowledgementslogfilename, strerror(errno));
					return;
				}
			}
			else {
				rewind(acknowledgementslog);
				done = 1;
			}
		} while (!done);
	}
	
	head = NULL;

	while (acknowledgementslog && (fgets(l, sizeof(l), acknowledgementslog))) {

		unsigned int etim;
		time_t eventtime;
		char host[MAX_LINE_LEN];
		char svc[MAX_LINE_LEN];
		char recipient[MAX_LINE_LEN];
		char message[MAX_LINE_LEN];
		char *hostname, *svcname, *p;
		int itemsfound, pagematch, hostmatch, testmatch, rcptmatch;
		acknowledgements_t *newrec;
		void *eventhost;
		struct htnames_t *eventcolumn;
		int ovector[30];

                /* 2015-03-07 18:17:03 myserver disk andy 1 1425724570 1425752223 1425838623 testing message */
		itemsfound = sscanf(l, "%*u-%*u-%*u %*u:%*u:%*u %s %s %s %*u %*u %u %*u %[^\t\n]", host, svc, recipient, &etim, message);
		if (itemsfound != 5) {
		    /* 1430040985      630949  30      630949  np_filename_not_used    myserver.procs red     testing log format \nAcked by: andy (127.0.0.1) */
		    itemsfound = sscanf(l, "%u\t%*u\t%*u\t%*u\tnp_filename_not_used\t%s\t%*s\t%[^\n]", &etim, host, message);
		    if (itemsfound != 3) continue;
		    p = strrchr(host, '.');
		    if (p) {
                        *p = '\0';
			strcpy(svc,p+1);
                    }
		    /* Xymon uses \n in the ack message, for the "acked by" data. Cut it off. */
		    p = strstr(message, "\\nAcked by:");
		    if (p) {
			strcpy(recipient,p+12);
                        *(p-1) = '\0';
		    }
		    else {
			strcpy(recipient,"UnknownUser");
                    }
		    p = strchr(recipient, '('); if (p) *(p-1) = '\0';
                }
		eventtime = etim;
		if (eventtime < firstevent) continue;
		if (eventtime > lastevent) break;

		hostname = host; svcname = svc;
		eventhost = hostinfo(hostname);
		if (!eventhost) continue; /* Dont report hosts that no longer exist */
		eventcolumn = getname(svcname, 1);

		p = strchr(recipient, '['); if (p) *p = '\0';

		if (pageregexp) {
			char *pagename;

			pagename = xmh_item_multi(eventhost, XMH_PAGEPATH);
			pagematch = 0;
			while (!pagematch && pagename) {
			pagematch = (pcre_exec(pageregexp, NULL, pagename, strlen(pagename), 0, 0, 
					ovector, (sizeof(ovector)/sizeof(int))) >= 0);
				pagename = xmh_item_multi(NULL, XMH_PAGEPATH);
			}
		}
		else
			pagematch = 1;
		if (!pagematch) continue;

		if (expageregexp) {
			char *pagename;

			pagename = xmh_item_multi(eventhost, XMH_PAGEPATH);
			pagematch = 0;
			while (!pagematch && pagename) {
			pagematch = (pcre_exec(expageregexp, NULL, pagename, strlen(pagename), 0, 0, 
					ovector, (sizeof(ovector)/sizeof(int))) >= 0);
				pagename = xmh_item_multi(NULL, XMH_PAGEPATH);
			}
		}
		else
			pagematch = 0;
		if (pagematch) continue;

		if (hostregexp)
			hostmatch = (pcre_exec(hostregexp, NULL, hostname, strlen(hostname), 0, 0, 
					ovector, (sizeof(ovector)/sizeof(int))) >= 0);
		else
			hostmatch = 1;
		if (!hostmatch) continue;

		if (exhostregexp)
			hostmatch = (pcre_exec(exhostregexp, NULL, hostname, strlen(hostname), 0, 0, 
					ovector, (sizeof(ovector)/sizeof(int))) >= 0);
		else
			hostmatch = 0;
		if (hostmatch) continue;

		if (testregexp)
			testmatch = (pcre_exec(testregexp, NULL, svcname, strlen(svcname), 0, 0, 
					ovector, (sizeof(ovector)/sizeof(int))) >= 0);
		else
			testmatch = 1;
		if (!testmatch) continue;

		if (extestregexp)
			testmatch = (pcre_exec(extestregexp, NULL, svcname, strlen(svcname), 0, 0, 
					ovector, (sizeof(ovector)/sizeof(int))) >= 0);
		else
			testmatch = 0;
		if (testmatch) continue;

		if (rcptregexp)
			rcptmatch = (pcre_exec(rcptregexp, NULL, recipient, strlen(recipient), 0, 0, 
					ovector, (sizeof(ovector)/sizeof(int))) >= 0);
		else
			rcptmatch = 1;
		if (!rcptmatch) continue;

		if (exrcptregexp)
			rcptmatch = (pcre_exec(exrcptregexp, NULL, recipient, strlen(recipient), 0, 0, 
					ovector, (sizeof(ovector)/sizeof(int))) >= 0);
		else
			rcptmatch = 0;
		if (rcptmatch) continue;

		newrec = (acknowledgements_t *) malloc(sizeof(acknowledgements_t));
		newrec->host       = eventhost;
		newrec->service    = eventcolumn;
		newrec->eventtime  = eventtime;
		newrec->recipient  = strdup(recipient);
		newrec->message  = strdup(message);
		newrec->next       = head;
		head = newrec;
	}

	if (head) {
		char *bgcolors[2] = { "#000000", "#000066" };
		int  bgcolor = 0;
		int  count;
		struct acknowledgements_t *lasttoshow = head;

		count=0;
		walk=head; 
		do {
			count++;
			lasttoshow = walk;
			walk = walk->next;
		} while (walk && (count<maxcount));

		if (maxminutes)  { 
			sprintf(title, "%d acknowledgements in the past %u minutes", 
				count, (unsigned int)((getcurrenttime(NULL) - lasttoshow->eventtime) / 60));
		}
		else {
			sprintf(title, "%d events acknowledged.", count);
		}

		fprintf(output, "<BR><BR>\n");
		fprintf(output, "<TABLE SUMMARY=\"Acknowledgements log\" BORDER=0>\n");
		fprintf(output, "<TR BGCOLOR=\"#333333\">\n");
		fprintf(output, "<TD ALIGN=CENTER COLSPAN=5><FONT SIZE=-1 COLOR=\"#33ebf4\">%s</FONT></TD></TR>\n", htmlquoted(title));
		fprintf(output, "<TR BGCOLOR=\"#333333\"><TH>Time</TH><TH>Host</TH><TH>Service</TH><TH>Acknowledged By</TH><TH>Message</TH></TR>\n");

		for (walk=head; (walk != lasttoshow->next); walk=walk->next) {
			char *hostname = xmh_item(walk->host, XMH_HOSTNAME);

			fprintf(output, "<TR BGCOLOR=%s>\n", bgcolors[bgcolor]);
			bgcolor = ((bgcolor + 1) % 2);

			fprintf(output, "<TD ALIGN=LEFT>%s</TD>\n", ctime(&walk->eventtime));

			fprintf(output, "<TD ALIGN=LEFT>%s</TD>\n", hostname);
			fprintf(output, "<TD ALIGN=LEFT>%s</TD>\n", walk->service->name);
			fprintf(output, "<TD ALIGN=LEFT>%s</TD>\n", walk->recipient);
			fprintf(output, "<TD ALIGN=LEFT>%s</TD>\n", walk->message);
		}

		fprintf(output, "</TABLE>\n");

		/* Clean up */
		walk = head;
		do {
			struct acknowledgements_t *tmp = walk;

			walk = walk->next;
			xfree(tmp->recipient);
			xfree(tmp->message);
			xfree(tmp);
		} while (walk);
	}
	else {
		/* No acknowledgements during the past maxminutes */
		if (acknowledgementslog)
			sprintf(title, "No events acknowledged in the last %d minutes", maxminutes);
		else
			strcpy(title, "No acknowledgements logged");

		fprintf(output, "<CENTER><BR>\n");
		fprintf(output, "<TABLE SUMMARY=\"%s\" BORDER=0>\n", title);
		fprintf(output, "<TR BGCOLOR=\"#333333\">\n");
		fprintf(output, "<TD ALIGN=CENTER COLSPAN=6><FONT SIZE=-1 COLOR=\"#33ebf4\">%s</FONT></TD>\n", htmlquoted(title));
		fprintf(output, "</TR>\n");
		fprintf(output, "</TABLE>\n");
		fprintf(output, "</CENTER>\n");
	}

	if (acknowledgementslog) fclose(acknowledgementslog);

	if (pageregexp)   pcre_free(pageregexp);
	if (expageregexp) pcre_free(expageregexp);
	if (hostregexp)   pcre_free(hostregexp);
	if (exhostregexp) pcre_free(exhostregexp);
	if (testregexp)   pcre_free(testregexp);
	if (extestregexp) pcre_free(extestregexp);
	if (rcptregexp)   pcre_free(rcptregexp);
	if (exrcptregexp) pcre_free(exrcptregexp);
}

