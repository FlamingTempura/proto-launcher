'use strict';

const os = require('os');
const path = require('path');
const lockSingleInstance = require('./lib/lockfile');
const launcher = require('./lib/launcher');

const LOCKFILE = path.resolve(os.homedir(), '.launcher-lock');

lockSingleInstance(LOCKFILE, () => { // if a second instance tried to run, show the launcher
	console.log('launcher: lockfile changed, showing launcher');
	launcher.show();
});

launcher.init();