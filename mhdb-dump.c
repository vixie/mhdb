#include <sys/stat.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "mhdb.h"
#include "mhdb_pvt.h"

static void mhdb_dump_folder(mhdb_t);
static void mhdb_dump_message(mhdb_t, const char *mhpath);

int
main(int argc, char *argv[]) {
	struct stat sb;
	mhdb_t mhdb;

	if (argc != 2) {
		fprintf(stderr, "usage: mhdb-dump folder|message\n");
		exit(1);
	}
	mhdb = mhdb_openpath(argv[1], 0);
	if (mhdb == NULL)
		err(1, "mhdb_open failed (%s)", argv[1]);
	if (stat(argv[1], &sb) < 0)
		err(1, "stat failed (%s)", argv[1]);
	if (S_ISDIR(sb.st_mode)) {
		mhdb_dump_folder(mhdb);
	} else if (S_ISREG(sb.st_mode)) {
		mhdb_dump_message(mhdb, argv[1]);
	} else
		warn("%s is neither a directory nor a file", argv[1]);
	mhdb_close(mhdb);
	exit(0);
}

static void
mhdb_dump_folder(mhdb_t mhdb) {
	datum key;

	printf("high uid = %d\n", mhdb_high_uid(mhdb));
	for (key = dbm_firstkey(mhdb->d);
	     key.dptr != NULL;
	     key = dbm_nextkey(mhdb->d)) {
		datum data;

		data = dbm_fetch(mhdb->d, key);
		printf("'%.*s' => '%.*s'\n",
		       key.dsize, key.dptr,
		       data.dsize, data.dptr);
	}
}

static void
mhdb_dump_message(mhdb_t mhdb, const char *mhpath) {
	int uid = mhdb_get_uid(mhdb, mhpath);

	printf("uid = %d\n", uid);
}
