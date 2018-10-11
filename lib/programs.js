'use strict';

const fs = require('fs');
const path = require('path');
const preferences = require('./preferences');

const APPLICATIONS = [
	'/usr/share/applications'
];

const props = ['Name', 'Comment', 'GenericName', 'Exec', 'Icon'];

const getPrograms = () => {
	let programs = [];

	APPLICATIONS.forEach(dir => {
		fs.readdirSync(dir).map(file => {
			let id = path.join(dir, file),
				desktop = fs.readFileSync(id, 'utf8'),
				program = { id };

			desktop.trim().split('\n').forEach(line => {
				line = line.trim();
				let kv = line.split('=');
				if (props.includes(kv[0]) && !program[kv[0]]) {
					program[kv[0]] = kv[1];
				}
			});
			if (!program.Exec) { return; }

			program.Exec = program.Exec.replace(/\s%\w/g, ''); // remove parameters
			program.querysearch = (program.Name || '').toLowerCase() + ':' + (program.Comment || '').toLowerCase() + ':' + (program.GenericName || '').toLowerCase();
			program.count = preferences.count[id] || 0;
			
			programs.push(program);
		});
	});

	return programs.sort((a, b) => b.count - a.count); // most frequently used first
};

module.exports = getPrograms;