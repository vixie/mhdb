#ifndef MHDB_H
#define MHDB_H 1

#ifdef APPLE
#define filenamecmp strcasecmp
#else
#define filenamecmp strcmp
#endif

#include <sys/types.h>
#include <fcntl.h>

struct mhdb;
typedef struct mhdb *mhdb_t;

typedef enum {
	mhdb_acc_nolock = 0,
	mhdb_acc_shared,
	mhdb_acc_exclusive
} mhdb_acc_t;

char		*mhdb_path(const char *mhpath);
mhdb_t		mhdb_open(const char *dbpath, mhdb_acc_t);
mhdb_t		mhdb_openpath(const char *mhpath, mhdb_acc_t);
void		mhdb_close(mhdb_t);
void		mhdb_audit(const char *fmt, ...);
int		mhdb_get_uid(mhdb_t, const char *mhpath);
int		mhdb_high_uid(mhdb_t);
int		mhdb_assign_uid(mhdb_t, const char *mhpath, int given_uid);
void		mhdb_delete_uid(mhdb_t, const char *mhpath, int uid);

#endif /*MHDB_H*/
