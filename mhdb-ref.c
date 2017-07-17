#include <stdlib.h>
#include <string.h>

#include "mhdb.h"

int
main(int argc, char *argv[]) {
	char *p1, *p2;
	mhdb_t d1, d2;
	int uid;

	/* Here we call mhdb_path and mhdb_open separately (vs mh-add/mh-del)
	 * because if it's the same folder we don't want to open the database
	 * twice.
	 */
	p1 = mhdb_path(argv[1]);
	p2 = mhdb_path(argv[2]);
	d1 = mhdb_open(p1, mhdb_acc_exclusive);
	d2 = (filenamecmp(p1, p2) == 0)
		? d1
		: mhdb_open(p2, mhdb_acc_exclusive);
	free(p2);
	free(p1);
	uid = mhdb_get_uid(d1, argv[1]);
	mhdb_delete_uid(d1, argv[1], uid);
	uid = mhdb_assign_uid(d2, argv[2], (d1 == d2) ? uid : 0);
	mhdb_close(d2);
	if (d1 != d2)
		mhdb_close(d1);
	exit(0);
}
