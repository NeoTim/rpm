#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messages.h"
#include "misc.h"
#include "rpmerr.h"
#include "rpmlib.h"

/* the rpmrc is read from /etc/rpmrc or $HOME/.rpmrc - it is not affected
   by a --root option */

struct option {
    char * name;
    int var;
    int archSpecific;
} ;


/* this *must* be kept in alphabetical order */
struct option optionTable[] = {
    { "arch_sensitive",		RPMVAR_ARCHSENSITIVE,		0 },
    { "builddir",		RPMVAR_BUILDDIR,		0 },
    { "distribution",		RPMVAR_DISTRIBUTION,		0 },
    { "docdir",			RPMVAR_DOCDIR,			0 },
    { "messagelevel",		RPMVAR_MESSAGELEVEL,		0 },
    { "optflags",		RPMVAR_OPTFLAGS,		1 },
    { "require_distribution",	RPMVAR_REQUIREDISTRIBUTION,	0 },
    { "require_group",		RPMVAR_REQUIREGROUP,		0 },
    { "require_icon",		RPMVAR_REQUIREICON,		0 },
    { "require_vendor",		RPMVAR_REQUIREVENDOR,		0 },
    { "root",			RPMVAR_ROOT,			0 },
    { "rpmdir",			RPMVAR_RPMDIR,			0 },
    { "sourcedir",		RPMVAR_SOURCEDIR,		0 },
    { "specdir",		RPMVAR_SPECDIR,			0 },
    { "srcrpmdir",		RPMVAR_SRPMDIR,			0 },
    { "timecheck",		RPMVAR_TIMECHECK,		0 },
    { "topdir",			RPMVAR_TOPDIR,			0 },
    { "vendor",			RPMVAR_VENDOR,			0 },
} ;
static int optionTableSize = sizeof(optionTable) / sizeof(struct option);

static int readRpmrc(FILE * fd, char * fn);
static void setDefaults(void);
static void setPathDefault(int var, char * s);
static int optionCompare(const void * a, const void * b);

static int optionCompare(const void * a, const void * b) {
    return strcmp(((struct option *) a)->name, ((struct option *) b)->name);
}

static int readRpmrc(FILE * f, char * fn) {
    char buf[1024];
    char * start;
    char * chptr;
    static char * archName = NULL;
    int linenum = 0;
    struct option * option, searchOption;

    if (!archName) archName = getArchName();

    while (fgets(buf, sizeof(buf), f)) {
	linenum++;

	/* strip off leading spaces */
	start = buf;
	while (*start && isspace(*start)) start++;

	/* I guess #.* should be a comment, but I don't know that. I'll
	   just assume that and hope I'm write. For kicks, I'll take
	   \# to escape it */

	chptr = start;
	while (*chptr) {
	    switch (*chptr) {
	      case '#':
		*chptr = '\0';
		break;

	      case '\n':
		*chptr = '\0';
		break;

	      case '\\':
		chptr++;
		/* fallthrough */
	      default:
		chptr++;
	    }
	}

	/* strip off trailing spaces - chptr is at the end of the line already*/
	for (chptr--; chptr >= start && isspace(*chptr); chptr++);
	*(chptr + 1) = '\0';

	if (!start[0]) continue;			/* blank line */
	
	/* look for a :, the id part ends there */
	for (chptr = start; *chptr && *chptr != ':'; chptr++);

	if (! *chptr) {
	    error(RPMERR_RPMRC, "missing ':' at %s:%d\n", fn, linenum);
	    return 1;
	}

	*chptr = '\0';
	chptr++;	 /* now points at beginning of argument */
	while (*chptr && isspace(*chptr)) chptr++;

	if (! *chptr) {
	    error(RPMERR_RPMRC, "missing argument for %s at %s:%d\n", 
		  start, fn, linenum);
	    return 1;
	}
	

	message(MESS_DEBUG, "got var '%s' arg '%s'\n", start, chptr);

	searchOption.name = start;
	option = bsearch(&searchOption, optionTable, optionTableSize,
			 sizeof(struct option), optionCompare);
	if (!option) {
	    error(RPMERR_RPMRC, "bad option '%s' at %s:%d\n", 
			start, fn, linenum);
	    return 1;
	}

	if (option->archSpecific) {
	    start = chptr;

	    for (chptr = start; *chptr && !isspace(*chptr); chptr++);
	    if (! *chptr) {
		error(RPMERR_RPMRC, "missing argument for %s at %s:%d\n", 
			option->name, fn, linenum);
		return 1;
	    }
	    *chptr++ = '\0';

	    while (*chptr && isspace(*chptr)) chptr++;
	    if (! *chptr) {
		error(RPMERR_RPMRC, "missing argument for %s at %s:%d\n", 
			option->name, fn, linenum);
		return 1;
	    }
	
	    if (!strcmp(archName, start)) {
		message(MESS_DEBUG, "%s is arg for %s platform", chptr,
			archName);
		setVar(option->var, chptr);
	    }
	} else {
	    setVar(option->var, chptr);
	}
    }

    return 0;
}

static void setDefaults(void) {
    setVar(RPMVAR_ARCHSENSITIVE, NULL);
    setVar(RPMVAR_TOPDIR, "/usr/src");
    setVar(RPMVAR_DOCDIR, "/usr/doc");
    setVar(RPMVAR_OPTFLAGS, "-O2");
}

int readConfigFiles(void) {
    FILE * f;
    char * fn;
    char * home;
    int rc = 0;

    setDefaults();

    f = fopen("/etc/rpmrc", "r");
    if (f) {
	rc = readRpmrc(f, "/etc/rpmrc");
	fclose(f);
	if (rc) return rc;
    }

    home = getenv("HOME");
    if (home) {
	fn = alloca(strlen(home) + 8);
	strcpy(fn, home);
	strcat(fn, "/.rpmrc");
	f = fopen("/etc/rpmrc", "r");
	if (f) {
	    rc |= readRpmrc(f, fn);
	    fclose(f);
	    if (rc) return rc;
	}
    }
	
    /* set default directories here */

    setPathDefault(RPMVAR_BUILDDIR, "BUILD");    
    setPathDefault(RPMVAR_RPMDIR, "RPMS");    
    setPathDefault(RPMVAR_SRPMDIR, "SRPMS");    
    setPathDefault(RPMVAR_SOURCEDIR, "SOURCES");    
    setPathDefault(RPMVAR_SPECDIR, "SPECS");    

    return 0;
}

static void setPathDefault(int var, char * s) {
    char * topdir;
    char * fn;

    if (getVar(var)) return;

    topdir = getVar(RPMVAR_TOPDIR);
    fn = alloca(strlen(topdir) + strlen(s) + 2);
    strcpy(fn, topdir);
    if (fn[strlen(topdir) - 1] != '/')
	strcat(fn, "/");
    strcat(fn, s);
   
    setVar(var, fn);
}

