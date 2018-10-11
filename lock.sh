#check if lockfile exists

LOCKFILE=~/.launcher-lock;

if [ -f $LOCKFILE ]; then
	echo "lockfile exists, checking if in use"
	echo "open" > $LOCKFILE

	# quit when "cancel" is seen, but timeout after 1 second
	timeout 1s bash -c "tail -f $LOCKFILE | sed '/cancel/ q'"

	# was cancel found?
	if [ $? -eq 0 ]; then
		echo "already running"
	else
		echo "timed out, starting new instance"
		#./launcher
		npm start
	fi
else
	echo "no lock file, starting new instance"
	#./launcher
	npm start
fi