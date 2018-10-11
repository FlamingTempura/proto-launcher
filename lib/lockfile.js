'use strict';

const fs = require('fs');

const cleanup = lockfile => {
	console.log('deleting lockfile');
	try { // if an error happens, don't prevent process from exiting
		fs.unlinkSync(lockfile);
	} catch (e) {}
};

const lock = (lockfile, runAgain) => {
	fs.writeFileSync(lockfile, '\n', 'utf8');
	fs.watch(lockfile, (evt) => {
		if (evt === 'change' && fs.existsSync(lockfile) && fs.readFileSync(lockfile, 'utf8').trim() === 'open') {
			fs.writeFileSync(lockfile, 'cancel\n');
			if (runAgain) { runAgain(); }
		}
	});
	process.on('exit', () => cleanup(lockfile));
	process.on('SIGINT', () => cleanup(lockfile));
};

const lockSingleInstance = (lockfile, { alreadyRunning, run, runAgain }) => {

	if (!fs.existsSync(lockfile)) { // don't launch more than once

		console.log('no lock file, starting new instance');
		lock(lockfile, runAgain);
		if (run) { run(); }

	} else {

		console.log('lockfile exists, checking if in use');

		let timeout = setTimeout(() => {
			console.log('timed out, starting new instance');
			lock(lockfile, runAgain);
			if (run) { run(); }
		}, 100);

		fs.watch(lockfile, evt => {
			if (evt === 'change' && fs.existsSync(lockfile) && fs.readFileSync(lockfile, 'utf8') === 'cancel') {
				clearTimeout(timeout);
				if (alreadyRunning) { alreadyRunning(); }
			}
		});

		fs.writeFileSync(lockfile, 'open\n', 'utf8');
		
	}

};

module.exports = lockSingleInstance;