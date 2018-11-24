const x11 = require('x11');
const freetype = require('freetype2');
const path = require('path');
const fs = require('fs');
const { promisify } = require('util');

const FONT = path.join(__dirname, 'fonts/OpenBaskerville-0.0.53.otf');

let fontface = {};
freetype.New_Memory_Face(fs.readFileSync(FONT), 0, fontface);
fontface = fontface.face;

var { Exposure, KeyPress } = x11.eventMask;

const promisifyAll = obj => {
	for (let key in obj) {
		if (typeof obj[key] === 'function') {
			obj[`${key}Async`] = promisify(obj[key].bind(obj));
		}
	}
	return obj;
};

const loadAtoms = async (X, props) => {
	let atoms = {};
	for (let prop of props) {
		atoms[prop] = await X.InternAtomAsync(true, prop);
	}
	return atoms;
};

// X.InternAtom(atom_name, only_if_exists)
// X.ChangeProperty(mode, window, property, type, format, data)

promisifyAll(x11);

const renderLauncher = async () => {
	let display = await x11.createClientAsync(),
		client = promisifyAll(display.client),
		Render = await client.requireAsync('render'),
		X = promisifyAll(display.client),
		root = display.screen[0].root,
		window = X.AllocID(),
		//cidBlack = X.AllocID(),
		cidWhite = X.AllocID(),
		//gc = X.AllocID(),
		white = display.screen[0].white_pixel,
		//black = display.screen[0].black_pixel,
		atoms = await loadAtoms(X, [
			'_MOTIF_WM_HINTS',
			'STRING',
			'WM_NAME',
			'ATOM'
		]);

	console.log(atoms);

	// window id, parent, x, y, width, height, border, depth, class, visual
	X.CreateWindow(window, root, 100, 0, 800, 800, 0, 0, 0, 0, {
		backgroundPixel: white,
		eventMask: Exposure | KeyPress
	});
	X.ChangeProperty(0, window, atoms._MOTIF_WM_HINTS, atoms._MOTIF_WM_HINTS, 8, [0x2,0x0,0x0,0x0,0x0]); // remove window decorations
	X.MapWindow(window);

	//X.CreateGC(gc, window);
	//X.CreateGC(cidBlack, window, { foreground: black, background: white });
	//X.CreateGC(cidWhite, window, { foreground: white, background: black });

	let query = '',
		cursor = 0;

	let render = () => {
		//X.PolyFillRectangle(window, cidWhite, [0, 0, 500, 500]);

		/*let queryWithCursor = query.slice(0, cursor) + '|' + query.slice(cursor);
		X.PolyText8(window, cidBlack, 50, 50, [queryWithCursor]);*/

		let glyphSet = X.AllocID();
		Render.CreateGlyphSet(glyphSet, Render.a8);

		var pict = X.AllocID();
		Render.CreatePicture(pict, window, Render.rgb24);

		var pix = X.AllocID();
		X.CreatePixmap(pix, root, 32, 1, 1);
		var pictSolidPix = X.AllocID();
		Render.CreatePicture(pictSolidPix, pix, Render.rgba32, {
			repeat: 1
		});
		Render.FillRectangles(1, pictSolidPix, [0x0, 0x0, 0x0, 0xffff], [0, 0, 100, 100]);

		//console.log(fontface)

		var gindex = {},
			charcode,
			chars = [];

		charcode = freetype.Get_First_Char(fontface, gindex);
		while (gindex.gindex !== 0) {
			chars.push(charcode);
			charcode = freetype.Get_Next_Char(fontface, charcode, gindex);
		}

		console.log('available characters', chars.length);
		freetype.Set_Pixel_Sizes(fontface, 0, 96);
		//freetype.Set_Char_Size(fontface, 0, 40 << 6, 0, 0);
		//freetype.Set_Transform(fontface, [ 0, -1 << 16, 1 << 16, 0 ], 0);

		var glyphFromCode = [];
		let glyphs = chars.map(charcode => {
			//return fontface.render(ch, size, 0, 96, 0); // ??
			freetype.Load_Char(fontface, charcode, freetype.LOAD_DEFAULT);
			freetype.Render_Glyph(fontface.glyph, freetype.RENDER_MODE_MONO);
			//console.log(fontface.glyph);
			let g = Object.assign({}, fontface.glyph, { image: fontface.glyph.bitmap.buffer });
			//g = {};

			
			g.width = fontface.glyph.bitmap.width;
			g.height = fontface.glyph.bitmap.rows;
			//g.x = 0;
			//g.y = 0;

			g = {};
			g.image = Buffer.alloc(80*80);
			g.image.fill(10);
			g.width = 80;
			g.height = 80;
			g.x = 20;

			g.y = 20;

			glyphFromCode[charcode] = g;
			return g;
		});

		console.log(glyphs['b'.charCodeAt(0)]);

		Render.AddGlyphs(glyphSet, glyphs);

		let text = 'boo!';
		let elems = [
			[0, 0, 'b'],
			[10, 0, 't']
		];

		Render.FillRectangles(1, pict, [0xFFFF, 0xffff, 0x0000, 0xffff], [0, 0, 300, 3000]);
		Render.CompositeGlyphs32(3, pictSolidPix, pict, 0, glyphSet, 260, 260, elems);
		Render.CompositeGlyphs32(3, pictSolidPix, pict, 0, glyphSet, 50, 50, [[0, 140, text]]);
		console.log('worked?')
	};

	render();

	X.on('event', async e => {
		return;
		if (e.type == 12) {
			//render();
		}
		if (e.name === 'KeyPress') {
			let key = await X.GetKeyboardMappingAsync(e.keycode, 2),
				keyname = Object.keys(x11.keySyms).find(name => x11.keySyms[name].code === key[0][e.buttons]),
				symbol = x11.keySyms[keyname];
			//console.log(keyname)
			if (keyname === 'XK_Escape') {
				//quit
			} else if (keyname === 'XK_BackSpace') {
				query = query.slice(0, cursor - 1) + query.slice(cursor);
				cursor--;
			} else if (keyname === 'XK_Delete') {
				query = query.slice(0, cursor) + query.slice(cursor + 1);
			} else if (keyname === 'XK_Left') {
				cursor--;
				if (cursor < 0) { cursor = 0; }
			} else if (keyname === 'XK_Right') {
				cursor++;
				if (cursor > query.length) { cursor = query.length; }
			} else if (keyname === 'XK_Home') {
				cursor = 0;
			} else if (keyname === 'XK_End') {
				cursor = query.length;
			} else if (keyname === 'XK_Up') {
				//selectedIndex--;
			} else if (keyname === 'XK_Down') {
				//selectedIndex++;
			} else if (symbol && symbol.description && symbol.description.startsWith('(')) {
				let char = symbol.description.match(/\(([^)]+)\)/)[1];
				query = query.slice(0, cursor) + char + query.slice(cursor);
				cursor++;
			}
			render();
		}
	});
	X.on('error', function(e) {
		console.log(e);
	});
};

const init = async () => {
	try {
		await renderLauncher();
	} catch (e) {
		console.error(e);
	}
}

init();



// NOTES

//X.ChangeProperty(0, window, atoms._NET_WM_STATE, atoms.ATOM, 32, atoms._NET_WM_STATE_MAXIMIZED_HORZ);

	/*X.ChangeProperty(0,window, atoms._NET_WM_STATE, X.atoms.ATOM, 32, [
                atoms._NET_WM_STATE_ABOVE,
                atoms._NET_WM_STATE_SKIP_TASKBAR
            ]);*/



	//X.ChangeProperty(0, window, atoms.WM_NAME, atoms.STRING, 8, 'Hello, NodeJS');
	//X.ChangeProperty(0, window, atoms._NET_WM_WINDOW_TYPE, atoms.ATOM, 8, [atoms._NET_WM_WINDOW_TYPE_DOCK]);

	//X.ChangeProperty(0, window, atoms._NET_WM_WINDOW_TYPE, atoms.ATOM, 8, [atoms._NET_WM_WINDOW_TYPE_DESKTOP]);


/*'_NET_WM_WINDOW_TYPE',
			'_NET_WM_WINDOW_TYPE_DOCK',
			'_NET_WM_WINDOW_TYPE_SPLASH',
			'_NET_WM_WINDOW_TYPE_MENU',
			'_NET_WM_WINDOW_TYPE_DESKTOP',
			'_NET_WM_WINDOW_TYPE_NOTIFICATION',
			'_NET_WM_STATE_SHADED',
			'_NET_WM_STATE',
			'_NET_WM_STATE_SKIP_TASKBAR',
			'_NET_WM_STATE_ABOVE',
			'_NET_WM_STATE_BELOW',
			'_NET_WM_STATE_MAXIMIZED_HORZ',*/