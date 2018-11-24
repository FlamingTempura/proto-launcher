const { spawn } = require('child_process');

let opts = {
	stdio: 'inherit'
};

spawn('make', [], opts)
	.on('close', code => {
		console.log(`build ${code === 0 ? 'success' : 'FAILED!'}`);
		if (code === 0) {
			let app = spawn('./x', [], opts)
				.on('close', code => {
					console.log('app code', code);
				});
			process.once('SIGUSR2', () => app.kill('SIGKILL'));
		}
	});