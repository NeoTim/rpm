/** \ingroup rpmbuild
 * \file build/buildEnv.c
 *  Set certain environment variables in the for the rpm build process based on the spec file
 */

#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include <rpm/rpmtypes.h>
#include <rpm/header.h>
#include <rpm/rpmtd.h>
#include <rpm/rpmbuild.h>
#include "build/rpmbuild_internal.h"

static char * uint32tostring(uint32_t number) {
    char * string = NULL;
    size_t len = 2;

    len = (ceil(log10(number)) + 1) * sizeof(char);
    string = malloc(len);
    snprintf(string, len, "%" PRIu32, number);

    return string;
}

static char * getChangelogTime(rpmSpec spec) {
    uint32_t changelogTime = 0;
    char * timestring = NULL;
    struct rpmtd_s times;

    if (headerGet(rpmSpecSourceHeader(spec), RPMTAG_CHANGELOGTIME, &times, HEADERGET_DEFAULT)) {
        if (times.count >= 1)
            changelogTime = ((uint32_t*)times.data)[0];
    }

    timestring = uint32tostring(changelogTime);

    return timestring;
}

void setBuildEnvironment(rpmSpec spec) {
    char * timestring = getChangelogTime(spec);

    setenv("RPM_CHANGELOG_DATE", timestring, 1);

    free(timestring);
}
