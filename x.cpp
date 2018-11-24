#include <fstream> // for input file stream
#include <string>
#include <vector> // easy arrays
#include <algorithm> // for sorting
#include <chrono> // sleep and duration
#include <thread>
#include <X11/Xlib.h> // X11 api
#include <X11/Xatom.h> // X11 Atoms (for preferences)
#include <X11/Xutil.h> // used to handle keyboard events
#include <X11/Xft/Xft.h> // fonts (requires libxft)
#include <experimental/filesystem> // used for scanning application dirs

namespace fs = std::experimental::filesystem; // todo: change to std::experimental (and remove experimental from include)
using namespace std;

struct Keyword {
	string word;
	int weight;
};

struct Application {
	string id, name, genericName, comment, cmd;
	vector<Keyword> keywords;
	int count;
};

struct Result {
	Application app;
	int score;
};

struct Color {
	unsigned long x;
	XftColor xft;
};

struct Style {
	Color black, gray, white, highlight;
	XftFont *regular, *bold, *regularSmall, *boldSmall;
};

const string APP_DIRS[] = {
	"/usr/share/applications"
};

#define ROW_HEIGHT 72
#define LINE_WIDTH 6

Display *display;
int screen;
Window window;
Style style;
GC gc;
string query = "";
int selected = 0;
int cursor = 0;
int cursorX;
bool cursorVisible = false;
vector<Application> applications;
XftDraw *xftdraw;

char lowercase (const char &ch) {
	return ch <= 'Z' && ch >= 'A' ? ch - ('Z' - 'z') : ch;
}

string lowercase (const string &str) {
	string out = "";
	for (const char ch : str) {
		out += lowercase(ch);
	}
	return out;
}

bool compareByScore(const Result &a, const Result &b) {
	return a.score > b.score;
}

void renderText(int *x, const int y, const string &text, XftFont *font, Color color) {
	XftDrawString8(xftdraw, &color.xft, font, *x, y, (XftChar8 *) text.c_str(), text.length());
	XGlyphInfo extents;
	XftTextExtents8(display, font, (FcChar8 *) text.c_str(), text.length(), &extents);
	*x += extents.width;
}

vector<Result> search () {
	vector <Result> results;
	const string lquery = lowercase(query);
	for (const Application &app : applications) {
		int score = 0;
		int i = 0;
		for (const Keyword &keyword : app.keywords) {
			int matchIndex = keyword.word.find(lquery);
			if (matchIndex != string::npos) {
				// score determined by:
				// - apps whose names begin with the query string appear first
				// - apps whose names or descriptions contain the query string then appear
				// - apps which have been opened most frequently should be prioritised
				score = (100 - i) * keyword.weight * (matchIndex == 0 ? 100 : 1);
				break;
			}
			i++;
		}
		if (score > 0) {
			results.push_back({ app, score });
		}
	}
	sort(results.begin(), results.end(), compareByScore);
	return results;
}

auto lastBlink = std::chrono::system_clock::now();
void toggleCursor (const bool show) {
	lastBlink = std::chrono::system_clock::now();
	XSetForeground(display, gc, show ? style.black.x : style.white.x);
	XFillRectangle(display, window, gc, cursorX, ROW_HEIGHT / 4, 3, ROW_HEIGHT / 2);
	cursorVisible = show;
}

void cursorBlink () {
	if (cursorX > 0) {
		auto now = std::chrono::system_clock::now();
		std::chrono::duration<double> delta = now - lastBlink;
		if (delta > (std::chrono::milliseconds) 700) { // seconds
			toggleCursor(!cursorVisible);
		}
	}
}

void render () {
	string disp = query.substr(0, cursor) + '|' + query.substr(cursor, query.length());
	
	vector<Result> results = search();

	int resultCount = results.size();
	if (resultCount > 10) {
		resultCount = 10;
	}
	int screenWidth = DisplayWidth(display, screen);
	int width = screenWidth / 3.4;
	int x = screenWidth / 2 - width / 2;
	int height = (resultCount + 1) * ROW_HEIGHT;

	XMoveResizeWindow(display, window, x, 200, width, height);

	XClearWindow(display, window);
	cursorX = 14;
	int ty = 0.63 * ROW_HEIGHT;
	renderText(&cursorX, ty, query.substr(0, cursor), style.regular, style.white); // invisible text just to figure out cursor position
	toggleCursor(true);
	int tx = 14;
	renderText(&tx, ty, query, style.regular, style.black);
	XSetForeground(display, gc, style.highlight.x);
	XSetLineAttributes(display, gc, LINE_WIDTH, LineSolid, CapButt, JoinRound);
	XDrawRectangle(display, window, gc, 0, 0, width - 1, ROW_HEIGHT);
	XDrawRectangle(display, window, gc, 0, ROW_HEIGHT - 1, width - 1, resultCount * ROW_HEIGHT - 1);
	
	for (int i = 0; i < resultCount; i++) {
		const Result result = results[i];
		const string d = result.app.name + " - " + result.app.genericName + result.app.comment;
		if (i == selected) {
			XSetForeground(display, gc, style.highlight.x);
			XFillRectangle(display, window, gc, 0, (i + 1) * ROW_HEIGHT, width, ROW_HEIGHT);
		}
		
		int y = (i + 1) * ROW_HEIGHT + 0.63 * ROW_HEIGHT;
		int x = 14;
		int namei = lowercase(result.app.name).find(lowercase(query));
		if (namei == string::npos) {
			renderText(&x, y, result.app.name.c_str(), style.regular, style.black);
		} else {
			string str = result.app.name.substr(0, namei);
			renderText(&x, y, str.c_str(), style.regular, style.black);
			str = result.app.name.substr(namei, query.length());
			renderText(&x, y, str.c_str(), style.bold, style.black);
			str = result.app.name.substr(namei + query.length(), string::npos);
			renderText(&x, y, str.c_str(), style.regular, style.black);
		}

		x += 8;
		int commenti = lowercase(result.app.comment).find(lowercase(query));
		if (commenti == string::npos) {
			renderText(&x, y, result.app.comment.c_str(), style.regularSmall, style.gray);
		} else {
			string str = result.app.comment.substr(0, commenti);
			renderText(&x, y, str.c_str(), style.regularSmall, style.gray);
			str = result.app.comment.substr(commenti, query.length());
			renderText(&x, y, str.c_str(), style.boldSmall, style.gray);
			str = result.app.comment.substr(commenti + query.length(), string::npos);
			renderText(&x, y, str.c_str(), style.regularSmall, style.gray);
		}
	}

}

void getApplications () {
	for (const string &dir : APP_DIRS) {
		for (auto & p : fs::directory_iterator(dir)) {
			Application app = {};
			app.id = p.path();
			ifstream infile(app.id);
			string line;
			while (getline(infile, line)) {
				if (app.name == "" && line.find("Name=") == 0) {
					app.name = line.substr(5);
				}
				if (app.genericName == "" && line.find("GenericName=") == 0) {
					app.genericName = line.substr(12);
				}
				if (app.comment == "" && line.find("Comment=") == 0) {
					app.comment = line.substr(8);
				}
				if (app.cmd == "" && line.find("Exec=") == 0 && line.substr(5) != "") {
					app.cmd = line.substr(5);
				}
			}
			
			stringstream ss(app.name);
			string word;
			while (getline(ss, word, ' ')) {
				app.keywords.push_back({ lowercase(word), 1000 });
			}

			stringstream ss2(app.genericName + ' ' + app.comment);
			while (getline(ss2, word, ' ')) {
				app.keywords.push_back({ lowercase(word), 1 });
			}

			applications.push_back(app);
		}
	}
}

void setProperty (const char *property, const char *value) {
	const Atom propertyAtom = XInternAtom(display, property, False);
	const long valueAtom = XInternAtom(display, value, False);
	XChangeProperty(display, window, propertyAtom, XA_ATOM, 32, PropModeReplace, (unsigned char *) &valueAtom, 1);
}

void onKeyPress (XEvent &event) {
	char text[128] = {0};
	KeySym keysym;
	int textlength = XLookupString(&event.xkey, text, sizeof text, &keysym, NULL);
	
	switch (keysym) {
		case XK_Escape:
			exit(0);
			break;

		case XK_Return:
			// spawn process
			exit(0);
			break;

		case XK_Up:
			selected--;
			break;
		
		case XK_Down:
			selected++;
			break;

		case XK_Left:
			if (cursor > 0) {
				cursor--;
			}
			break;

		case XK_Right:
			if (cursor < query.length()) {
				cursor++;
			}
			break;

		case XK_Home:
			cursor = 0;
			break;

		case XK_End:
			cursor = query.length();
			break;

		case XK_BackSpace:
			if (cursor > 0) {
				query = query.substr(0, cursor - 1) + query.substr(cursor, query.length());
				cursor--;
			}
			break;

		case XK_Delete:
			if (cursor < query.length()) {
				query = query.substr(0, cursor) + query.substr(cursor + 1, query.length());
			}
			break;

		default:
			if (textlength == 1) {
				query = query.substr(0, cursor) + text + query.substr(cursor, query.length());
				cursor++;
			}
	}
	
	render();
}

void defineColor (Visual *visual, Colormap &colormap, Color &color, unsigned short red, unsigned short green, unsigned short blue, unsigned short alpha) {
	red *= 255;
	green *= 255;
	blue *= 255;
	alpha *= 255;
	XRenderColor xrcolor = { .red = red, .green = green, .blue = blue, .alpha = alpha };
	XftColorAllocValue(display, visual, colormap, &xrcolor, &color.xft);
	XColor xcolor = { .red = red, .green = green, .blue = blue };
	XAllocColor(display, colormap, &xcolor);
	color.x = xcolor.pixel;
}

int main () {
	getApplications();
	display = XOpenDisplay(NULL);
	screen = DefaultScreen(display);

	Visual *visual = DefaultVisual(display, screen);
	Colormap colormap = DefaultColormap(display, screen);
	int depth = DefaultDepth(display, screen);

	XSetWindowAttributes attributes;
	attributes.background_pixel = WhitePixel(display, screen);

	window = XCreateWindow(display, XRootWindow(display, screen),
		0, 0, 100, 100,
		5, depth, InputOutput, visual, CWBackPixel, &attributes);
	XSelectInput(display, window, ExposureMask | KeyPressMask | FocusChangeMask);

	XGCValues gr_values;
	gc = XCreateGC(display, window, 0, &gr_values);

	setProperty("_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_DOCK");
	setProperty("_NET_WM_STATE", "_NET_WM_STATE_ABOVE");
	setProperty("_NET_WM_STATE", "_NET_WM_STATE_MODAL");

	XMapWindow(display, window);

	defineColor(visual, colormap, style.black, 80, 0, 0, 255);
	defineColor(visual, colormap, style.gray, 153, 153, 153, 255);
	defineColor(visual, colormap, style.white, 255, 255, 255, 255);
	defineColor(visual, colormap, style.highlight, 248, 194, 145, 255);

	style.regular = XftFontOpenName(display, screen, "Ubuntu,sans-11");
	style.bold = XftFontOpenName(display, screen, "Ubuntu,sans-11:bold");
	style.regularSmall = XftFontOpenName(display, screen, "Ubuntu,sans-10");
	style.boldSmall = XftFontOpenName(display, screen, "Ubuntu,sans-10:bold");

	xftdraw = XftDrawCreate(display, window, visual, colormap);

	XEvent event;
	while (1) {
		while (XCheckMaskEvent(display, ExposureMask | KeyPressMask | FocusChangeMask, &event)) {
			if (event.type == Expose) { render(); }
			if (event.type == KeyPress) { onKeyPress(event); }
			if (event.type == FocusOut) { exit(0); }
		}
		cursorBlink();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
