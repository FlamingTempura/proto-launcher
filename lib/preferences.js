'use strict';

const path = require('path');
const fs = require('fs');
const os = require('os');

const PREFERENCES = path.resolve(os.homedir(), '.launcher-preferences');

const preferences = {};

preferences.load = () => {
	try {
		Object.assign(preferences, JSON.parse(fs.readFileSync(PREFERENCES, 'utf8')));
	} catch (e) {}
	if (!preferences.count) { preferences.count = {}; }
};

preferences.save = () => {
	fs.writeFileSync(PREFERENCES, JSON.stringify(preferences, null, '\t'), 'utf8');
};

module.exports = preferences;