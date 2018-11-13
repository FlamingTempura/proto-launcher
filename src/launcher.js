'use strict';

const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');
const { homedir } = require('os');
const getPrograms = require('./programs');
const preferences = require('./preferences');

const AS_SERVICE = process.argv.includes('--service');

let launcher, programs, prevHTML;

const reload = () => {
	programs = getPrograms();
	let html = fs.readFileSync(path.resolve(__dirname, './index.html'), 'utf8').replace('[/*PROGRAMS*/]', JSON.stringify(programs));
	if (html !== prevHTML) {
		prevHTML = html;
		launcher.loadURL('data:text/html;charset=UTF-8,' + encodeURIComponent(html));
	}
};

const init = () => {
	preferences.load();
	let electron = require('electron');
	let display;
	electron.app.disableHardwareAcceleration();
	electron.app.whenReady().then(() => {
		display = electron.screen.getAllDisplays()[0];
		launcher = new electron.BrowserWindow({
			width: 100,
			height: 100,
			x: 0,
			y: 0,
			frame: false,
			alwaysOnTop: true,
			resizable: false,
			minimizable: false,
			maximizable: false,
			show: false
		});
		console.log(AS_SERVICE ? 'launcher: service running' : 'launcher: successfully launched');
		reload();
		setInterval(reload, 60000); // periodically refresh the list of apps
	});

	let exited;
	let close = () => {
		launcher.hide();
		if (!AS_SERVICE) {
			exited = true;
			electron.app.quit();
		}
	};

	electron.app.on('browser-window-blur', () => close());

	electron.ipcMain.on('open-program', (e, id) => {
		let program = programs.find(a => a.id === id);
		launcher.hide();
		console.log(`launcher: running ${program.Exec}`);
		spawn(program.Exec, [], { cwd: homedir(), shell: true, detached: true }).unref(); // prevent parent from waiting for subprocess to exit
		program.count++;
		preferences.count[program.id] = program.count;
		preferences.save();
		close();
	});

	electron.ipcMain.on('hide', () => close());

	electron.ipcMain.on('resize', (e, [width, height]) => {
		if (exited) { return; }
		launcher.setMinimumSize(width - 1, height - 1);
		launcher.setSize(width - 1, height - 1);
		launcher.setPosition(Math.round(display.bounds.width / 2 - width / 2), 100);
		if (!AS_SERVICE) { launcher.show(); }
	});

	electron.ipcMain.on('log', (e, data) => console.log('launcher: ', ...data));
};

const show = () => launcher.show();

module.exports = { init, show };