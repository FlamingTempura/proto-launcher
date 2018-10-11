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

module.exports = lock;