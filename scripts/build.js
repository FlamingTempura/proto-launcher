/*
122MB!
*/

const path = require('path');
const fs = require('fs');
const rollup = require('rollup');
const resolve = require('rollup-plugin-node-resolve');
const commonjs = require('rollup-plugin-commonjs');
const packager = require('electron-packager');

try {
	fs.mkdirSync(path.resolve(__dirname, 'tmp'));
} catch (e) {}

const createBundle = () => {
	let html = fs.readFileSync(path.resolve(__dirname, '../src/index.html'), 'utf8').replace(/\$\$/g, '__');
	return rollup
		.rollup({
			input: path.resolve(__dirname, '../src/run.js'),
			plugins: [
				resolve(),
				commonjs()
			],
			external: ['electron', 'os', 'path', 'child_process', 'fs']
		})
		.then(bundle => bundle.generate({
			format: 'cjs'
		}))
		.then(({ code }) => {
			code = code.replace(/fs.readFileSync\(.*index.html.*'utf8'\)/m, JSON.stringify(html)); // inject HTML into bundle
			fs.writeFileSync(path.resolve(__dirname, 'tmp/index.js'), code, 'utf8');
		});
};

const createElectron = () => {
	let package  = {
		name: 'launcher',
		main: 'index.js',
		version: '1.0.0',
		'dependencies.electron': {}
	};
	fs.writeFileSync(path.resolve(__dirname, 'tmp/package.json'), JSON.stringify(package));
	return packager({
		dir: path.resolve(__dirname, 'tmp'),
		out: path.resolve(__dirname, '../bin'),
		overwrite: true,
		asar: true
		//executableName: 'bundle'
	});
};

const createLauncher = () => {
	let sh = fs.readFileSync(path.resolve(__dirname, '../src/launcher.sh'), 'utf8');
	sh = sh.replace(/^(\s*).*\/electron.*$/gm, '$1./launcher-linux-x64/launcher $$1');
	fs.writeFileSync(path.resolve(__dirname, '../bin/launcher'), sh, 'utf8');
	fs.chmodSync(path.resolve(__dirname, '../bin/launcher'), 0o755); // make executable
};


createBundle()
	.then(() => createElectron())
	.then(() => createLauncher())
	.then(() => {
		fs.unlinkSync(path.resolve(__dirname, 'tmp/index.js'));
		fs.unlinkSync(path.resolve(__dirname, 'tmp/package.json'));
		fs.rmdirSync(path.resolve(__dirname, 'tmp'));
	});