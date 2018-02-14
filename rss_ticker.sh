#/bin/bash

set -x
while true;  do
	TEXT="$( paste -d'\n' \
		     <( rsstail -n 10 -H -1 -N -u http://rss.cnn.com/rss/cnn_topstories.rss | sed -e 's/^/CNN -/' ) \
		     <( rsstail -n 10 -H -1 -N -u http://feeds.foxnews.com/foxnews/most-popular | sed -e 's/^/FOX -/' )  \
		| paste -sd'~' | sed -e 's/~/      /g' )      ";

	echo "$TEXT" | while true ; do
		stty -F /dev/ttyACM0 -echo -hup
		SPACE=$(head -1 /dev/ttyACM0)
		PIECE=$(head -c $SPACE)
		echo -n "$PIECE" >> /dev/ttyACM0
		if [[ -z "$PIECE" ]] ; then break ; fi
	done
done
