#include <sys/file.h>

#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mhdb.h"
#include "mhdb_pvt.h"

static const char base[] = "mhindex";

static int mhdb_atoi(datum a);
static int mhdb_isnum(const char *s);
static int mhdb_msgno(const char *mhpath);

/* Compute a dbname corresponding to this full MH path name, which could
 * be of a message (if it has an all-numeric base name) or folder (if not).
 *
 * Danger: if given /home/vixie/Mail/123 (all numeric name) this code will
 * silently do the wrong thing (/home/vixie/Mail/mhindex.db).  Unlike MH,
 * it does not stat() the argument to find out if it's a directory or not.
 *
 * Danger: allocates on the heap and returns that -- caller must call free().
 */
char *
mhdb_path(const char *mhpath) {
	char *t, *p;

	t = malloc(strlen(mhpath) + strlen(base) + sizeof "/");
	if (t == NULL)
		err(1, "malloc failed");
	strcpy(t, mhpath);
	p = strrchr(t, '/');
	if (p != NULL) {
		if (mhdb_isnum(p + 1))		/* ends in /msgnum */
			p[1] = '\0';		/* ...so, truncate. */
		else if (p[1] != '\0')		/* ends in non-'/' */
			strcat(t, "/");		/* ...so, append '/' */
	}
	strcat(t, base);
	return (t);
}

/* Open the database, creating if nec'y, and lock it.  Since we use flock()
 * the unlock is implicit upon close() or exit().
 */
mhdb_t
mhdb_open(const char *dbpath, mhdb_acc_t acc) {
	mhdb_t self;
	DBM *d;
	int writable_p = (acc == mhdb_acc_exclusive);

	d = dbm_open(dbpath, writable_p ? (O_CREAT | O_RDWR) : O_RDONLY, 0666);
	if (d == NULL) {
		if (writable_p)
			err(1, "dbm_open failed");
		else
			return (NULL);
	}

	if (acc != mhdb_acc_nolock)
		if (flock(dbm_dirfno(d), writable_p ? LOCK_EX : LOCK_SH) < 0)
			err(1, "flock failed");

	self = malloc(sizeof *self);
	self->d = d;
	return (self);
}

/* Convenience function so that folks who don't need to know the dbpath can
 * avoid the hassle of calling free().  (imapd and dovecot i'm looking at YOU.)
 */
mhdb_t
mhdb_openpath(const char *mhpath, mhdb_acc_t acc) {
	mhdb_t self;
	char *dbpath;

	dbpath = mhdb_path(mhpath);
	self = mhdb_open(dbpath, acc);
	free(dbpath);
	return (self);
}

/* Close and release.
 */
void
mhdb_close(mhdb_t self) {
	dbm_close(self->d);
	free(self);
}

/* Write something to the audit log.
 */
void
mhdb_audit(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

/* Get an existing uid from the database.
 */
int
mhdb_get_uid(mhdb_t self, const char *mhpath) {
	datum key, data;
	int msg, uid;
	char *t;

	msg = mhdb_msgno(mhpath);
	if (msg == 0) {
		warnx("mhdb_add: no message number (%s)", mhpath);
		return (0);
	}

	/* Get the UID for this message number, if any. */
	asprintf(&t, "%d:msg:uid", msg);
	key.dptr = t;
	key.dsize = strlen(t);
	data = dbm_fetch(self->d, key);
	uid = (data.dptr != NULL) ? mhdb_atoi(data) : 0;
	free(key.dptr);

	return (uid);
}

/* Add a new msg<->uid mapping to the database.  Returns new (or given) UID.
 */
int
mhdb_assign_uid(mhdb_t self, const char *mhpath, int given_uid) {
	datum key, data;
	int msg, uid;
	char *t;

	msg = mhdb_msgno(mhpath);
	if (msg == 0) {
		warnx("mhdb_add: no message number (%s)", mhpath);
		return (0);
	}

	/* Get next UID from file header, unless it was given. */
	if (given_uid != 0) {
		uid = given_uid;
	} else {
		key.dptr = "file_header";
		key.dsize = strlen(key.dptr);
		data = dbm_fetch(self->d, key);
		if (data.dptr == NULL) {
			data.dptr = "1";
			data.dsize = strlen(data.dptr);
		}
		uid = mhdb_atoi(data);
	}

	/* Store mapping from this UID to the message number. */
	asprintf(&t, "%d:uid:msg", uid);
	key.dptr = t;
	key.dsize = strlen(t);
	asprintf(&t, "%d", msg);
	data.dptr = t;
	data.dsize = strlen(t);
	switch (dbm_store(self->d, key, data, DBM_INSERT)) {
		case -1: err(1, "dbm_store");
		case 1: errx(1, "key %s already present", key.dptr);
	}
	free(data.dptr);
	free(key.dptr);

	/* Store mapping from this message number to this UID. */
	asprintf(&t, "%d:msg:uid", msg);
	key.dptr = t;
	key.dsize = strlen(t);
	asprintf(&t, "%d", uid);
	data.dptr = t;
	data.dsize = strlen(t);
	switch (dbm_store(self->d, key, data, DBM_INSERT)) {
		case -1: err(1, "dbm_store");
		case 1: errx(1, "key %s already present", key.dptr);
	}
	free(data.dptr);
	free(key.dptr);

	/* Increment the UID and store it back in the database,
	 * unless it was given.
	 */
	if (given_uid == 0) {
		key.dptr = "file_header";
		key.dsize = strlen(key.dptr);
		asprintf(&t, "%d", uid + 1);
		data.dptr = t;
		data.dsize = strlen(t);
		if (dbm_store(self->d, key, data, DBM_REPLACE) < 0)
			err(1, "dbm_store");
		free(data.dptr);
	}

	return (uid);
}

/* Return the highest UID now in use.
 */
int
mhdb_high_uid(mhdb_t self) {
	datum key, data;
	int uid;

	key.dptr = "file_header";
	key.dsize = strlen(key.dptr);
	data = dbm_fetch(self->d, key);
	if (data.dptr == NULL) {
		data.dptr = "1";
		data.dsize = strlen(data.dptr);
	}
	uid = mhdb_atoi(data);
	return (uid - 1);
}

/* Remove a UID mapping from the database.
 */
void
mhdb_delete_uid(mhdb_t self, const char *mhpath, int uid) {
	datum key;
	int msg;
	char *t;

	msg = mhdb_msgno(mhpath);
	if (msg == 0) {
		warnx("mhdb_add: no message number (%s)", mhpath);
		return;
	}

	/* Remove mapping from msg to uid (or fail silently). */
	asprintf(&t, "%d:msg:uid", msg);
	key.dptr = t;
	key.dsize = strlen(t);
	dbm_delete(self->d, key);
	free(key.dptr);

	/* Remove mapping from uid to msg if there is one. */
	asprintf(&t, "%d:uid:msg", uid);
	key.dptr = t;
	key.dsize = strlen(t);
	dbm_delete(self->d, key);
	free(key.dptr);
}

     #include <dirent.h>

     int
     scandir(const char *dirname, struct dirent ***namelist,
         int (*select)(const struct dirent *),
         int (*compar)(const struct dirent **, const struct dirent **));

/* Build a new index for a given folder.
 */
void
mhdb_build(const char *mhpath) {
	mhdb_t self;
	char *dbpath, *t;
	struct direct *de;
	int nde, i;

	dbpath = mhdb_path(mhpath);
	vasprintf(&t, "%s.db", dbpath);
	unlink(t);
	free(t);
	self = mhdb_open(dbpath, mhdb_acc_exclusive);
	free(dbpath);
	nde = scandir(mhpath, &de, mhdb_build_select, NULL);

}

/* --- private --- */

/* Do what atoi would do but for a counted string (not null terminated)
 * held in a datum.
 */
static int
mhdb_atoi(datum a) {
	int i = 0;

	while (a.dsize-- > 0)
		i *= 10, i += *a.dptr++ - '0';
	return (i);
}

/* Is this string all-numeric?
 */
static int
mhdb_isnum(const char *s) {
	char ch;

	while ((ch = *s++) != '\0')
		if (isascii(ch) && isdigit(ch))
			/*next*/;
		else
			return (0);
	return (1);
}

/* Retrieve MH message number from path name.
 */
static int
mhdb_msgno(const char *mhpath) {
	const char *p = strrchr(mhpath, '/');

	if (p != NULL)
		p++;
	else
		p = mhpath;
	return (atoi(p));
}
