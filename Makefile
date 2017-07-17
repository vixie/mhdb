ALL = mhdb-add mhdb-del mhdb-ref mhdb-dump

DEFS = -DDBM_HEADER='<ndbm.h>'
OPT = -g
CFLAGS = -Wall ${DEFS} ${OPT}

all: ${ALL}

clean:
	rm -f *.o
	rm -f ${ALL}

mhdb-add: mhdb-add.o mhdb.o
	${CC} -o $@ $>
mhdb-del: mhdb-del.o mhdb.o
	${CC} -o $@ $>
mhdb-ref: mhdb-ref.o mhdb.o
	${CC} -o $@ $>
mhdb-dump: mhdb-dump.o mhdb.o
	${CC} -o $@ $>

mhdb.o: mhdb.c mhdb.h mhdb_pvt.h Makefile
mhdb-add.o: mhdb-add.c mhdb.h Makefile
mhdb-del.o: mhdb-del.c mhdb.h Makefile
mhdb-ref.o: mhdb-ref.c mhdb.h Makefile
mhdb-dump.o: mhdb-dump.c mhdb.h mhdb_pvt.h Makefile
