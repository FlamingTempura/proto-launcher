# This script checks if a launcher process is already running, and if it is,
# responds with an error code. The existing process will consequently show the
# launcher. This script is run before running the main electron script because
# electron is slow to load.

LOCKFILE=~/.launcher-lock;

if [ -f $LOCKFILE ]; then
	echo "launcher-lock: lockfile exists, checking if in use."
	echo "open" > $LOCKFILE

	# quit when "cancel" is seen, but timeout after 1 second
	timeout 1s bash -c "tail -f $LOCKFILE | sed '/cancel/ q'" > /dev/null

	# was cancel found?
	if [ $? -eq 0 ]; then
		echo "launcher-lock: launcher already running."
	else
		echo "launcher-lock: timed out, starting new instance."
		npm start -- $1
	fi
else
	echo "launcher-lock: no lock file, starting new instance."
	npm start -- $1
fi