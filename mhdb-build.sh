#! /bin/sh

PATH=/home/vixie/src/mhdb:$PATH

case "$1" in
+*) folder=$1 ;;
*) echo usage: $0 +folder; exit 1
esac

path=`mhpath $folder`
if [ ! -d $path ]; then
	echo $0: not a folder: $folder
	exit 1
fi

rm -f $path/mhindex.db
mhpath all $folder | mhdb-add -

exit
