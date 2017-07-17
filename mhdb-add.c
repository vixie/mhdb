#include <sys/param.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mhdb.h"

static mhdb_t d;
static char *p;

static void	mhdb_add(const char *mhpath);

int
main(int argc, char *argv[]) {
	if (strcmp(argv[1], "-") != 0) {
		mhdb_add(argv[1]);
	} else {
		char line[MAXPATHLEN+1];

		while (fgets(line, sizeof line, stdin)) {
			char *t = strrchr(line, '\n');

			if (t != NULL)
				*t = '\0';
			mhdb_add(line);
		}
	}
		       
	if (d != NULL)
		mhdb_close(d);
	if (p != NULL)
		free(p);
	exit(0);
}

static void
mhdb_add(const char *mhpath) {
	char *t = mhdb_path(mhpath);
	int uid;

	if (d != NULL && p != NULL && strcmp(t, p) != 0) {
		mhdb_close(d);
		d = NULL;
		free(p);
		p = NULL;
	}
	if (d == NULL) {
		assert(p == NULL);
		d = mhdb_open(t, mhdb_acc_exclusive);
		p = t;
	}
	uid = mhdb_assign_uid(d, mhpath, 0);
}
