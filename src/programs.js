'use strict';

const fs = require('fs');
const path = require('path');
const preferences = require('./preferences');

const APP_DIRS = [
	'/usr/share/applications'
];

const props = ['Name', 'Comment', 'GenericName', 'Exec', 'Icon'];

const getPrograms = () => {
	let apps = [];

	APP_DIRS.forEach(dir => {
		fs.readdirSync(dir).map(file => {
			let id = path.join(dir, file),
				desktop = fs.readFileSync(id, 'utf8'),
				app = { id };

			desktop.trim().split('\n').forEach(line => {
				line = line.trim();
				let kv = line.split('=');
				if (props.includes(kv[0]) && !app[kv[0]]) {
					app[kv[0]] = kv[1];
				}
			});
			if (!app.Exec) { return; }

			app.Exec = app.Exec.replace(/\s%\w/g, ''); // remove parameters
			app.count = preferences.count[id] || 0;

			app.keywords = [ // array of [keyword, weight]. weight is used to order results
				...(app.Name || '').toLowerCase().split(/\s+/).map(k => [k, 1000]),
				...(app.Comment || '').toLowerCase().split(/\s+/).map(k => [k, 1]),
				...(app.GenericName || '').toLowerCase().split(/\s+/).map(k => [k, 1])
			].slice(0, 20);
			
			apps.push(app);
		});
	});

	return apps.sort((a, b) => b.count - a.count); // most frequently used first
};

module.exports = getPrograms;