// list all programs installed

const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawn } = require('child_process');
const electron = require('electron');

const PREFERENCES = path.resolve(os.homedir(), '.launcher-preferences');
const APPLICATIONS = [
	'/usr/share/applications'
]
const AS_SERVICE = process.argv.includes('--service');

if (!electron.app.requestSingleInstanceLock()) { // don't launch more than once
	console.log('already running, showing launcher');
	electron.app.quit();
	return;
}

let preferences;
try {
	preferences = JSON.parse(fs.readFileSync(PREFERENCES, 'utf8'));
} catch (e) {
	preferences = {};
}
if (!preferences.count) { preferences.count = {}; }

const props = ['Name', 'Comment', 'GenericName', 'Exec', 'Icon']
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

programs = programs.sort((a, b) => b.count - a.count); // most frequently used first

let launcher,
	display;
electron.app.on('ready', () => {
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

const close = () => {
	launcher.hide();
	if (!AS_SERVICE) {
		electron.app.quit();
	}
};

electron.app.on('second-instance', () => {
	console.log('second instance!')
	launcher.show();
});

electron.app.on('browser-window-blur', () => close())

electron.ipcMain.on('open-app', (e, id) => {
	let program = programs.find(a => a.id === id);
	console.log(`Launching ${program.Exec}`);
	spawn(program.Exec, [], { shell: true, detached: true }).unref(); // prevent parent from waiting for subprocess to exit
	close();
	
	program.count++;
	preferences.count[program.id] = program.count;
	fs.writeFileSync(PREFERENCES, JSON.stringify(preferences, null, '\t'), 'utf8');
});

electron.ipcMain.on('hide', () => close());

electron.ipcMain.on('resize', (e, [width, height]) => {
	launcher.setSize(width - 1, height - 1);
	launcher.setPosition(Math.round(display.bounds.width / 2 - width / 2), 100);
});

electron.ipcMain.on('log', (e, data) => console.log(...data))