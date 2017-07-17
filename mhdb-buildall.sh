#! /bin/sh

PATH=/home/vixie/src/mhdb:$PATH

folders -fast -recurse | while read folder; do
	echo +$folder
	mhdb-build.sh +$folder
done

exit
