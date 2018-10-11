/* 

This script checks if a launcher process is already running, and if it is,
responds with an error code. The existing process will consequently show the
launcher. This script is run before running the main electron script because
electron is slow to load.

*/

'use strict';

const os = require('os');
const path = require('path');
const lockSingleInstance = require('./lib/lockfile');

const LOCKFILE = path.resolve(os.homedir(), '.launcher-lock');

lockSingleInstance(LOCKFILE, {
	alreadyRunning() {
		console.log('launcher is already running, cancelling this process.');
		process.exit(1); // exit with error
	},
	run() {
		console.log('starting launcher...');
		process.exit(0); // exit success
	}
});
