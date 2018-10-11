'use strict';

const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');
const getPrograms = require('./programs');
const preferences = require('./preferences');

const AS_SERVICE = process.argv.includes('--service');

let launcher;

const init = () => {
	preferences.load();
	let programs = getPrograms();
	let electron = require('electron');
	let display;
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
			show: !AS_SERVICE
		});
		let html = fs.readFileSync(path.resolve(__dirname, './index.html'), 'utf8').replace('[/*PROGRAMS*/]', JSON.stringify(programs));
		launcher.loadURL('data:text/html;charset=UTF-8,' + encodeURIComponent(html));
		console.log(AS_SERVICE ? 'service running' : 'successfully launched');
	});

	let close = () => {
		launcher.hide();
		if (!AS_SERVICE) {
			electron.app.quit();
		}
	};

	electron.app.on('browser-window-blur', () => close())

	electron.ipcMain.on('open-app', (e, id) => {
		let program = programs.find(a => a.id === id);
		console.log(`Launching ${program.Exec}`);
		spawn(program.Exec, [], { shell: true, detached: true }).unref(); // prevent parent from waiting for subprocess to exit
		close();
		
		program.count++;
		preferences.count[program.id] = program.count;
		preferences.save();
	});

	electron.ipcMain.on('hide', () => close());

	electron.ipcMain.on('resize', (e, [width, height]) => {
		launcher.setSize(width - 1, height - 1);
		launcher.setPosition(Math.round(display.bounds.width / 2 - width / 2), 100);
	});

	electron.ipcMain.on('log', (e, data) => console.log(...data));
};

const show = () => launcher.show();

module.exports = { init, show };