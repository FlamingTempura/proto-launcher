#include <iostream> // for log output
#include <unistd.h> // starting applications
#include <fstream> // for input file stream
#include <string> // string type
#include <vector> // flexible arrays
#include <map> // hashmaps
#include <algorithm> // for sorting
#include <chrono> // sleep and duration
#include <thread> // main loop management
#include <X11/Xlib.h> // X11 api
#include <X11/Xatom.h> // X11 Atoms (for preferences)
#include <X11/Xutil.h> // used to handle keyboard events
#include <X11/Xresource.h>
#include <X11/Xft/Xft.h> // fonts (requires libxft)
#include <filesystem> // used for scanning application dirs
#include <pwd.h> // used to get user home dir
#include <future>
#include <X11/extensions/Xrandr.h>

namespace fs = std::filesystem;
using std::string, std::map, std::vector, std::ifstream, std::ofstream, std::stringstream, std::thread, std::promise;

enum StyleAttribute {
	NAME,
	C_TITLE, C_COMMENT, C_BG, C_HIGHLIGHT, C_MATCH, // colors
	F_REGULAR, F_BOLD, F_SMALLREGULAR, F_SMALLBOLD, F_LARGE // fonts
};

struct Keyword {
	string word;
	int weight;
};

struct Application {
	string id, name, genericName, comment, cmd;
	vector<Keyword> keywords;
};

struct Result {
	Application *app;
	int score;
};

const int BASE_DPI = 96;
const int ROW_HEIGHT = 42;
const int INPUT_HEIGHT = 1.25 * ROW_HEIGHT;
const int TEXT_OFFSET = 0.63 * ROW_HEIGHT;
const int BORDER_WIDTH = 3;
const int INDENT = 14;
const int COMMENT_SPACE = 8;
const string HOME_DIR      = getenv("HOME")            != NULL ? getenv("HOME")            : getpwuid(getuid())->pw_dir;
const string CONFIG_DIR    = getenv("XDG_CONFIG_HOME") != NULL ? getenv("XDG_CONFIG_HOME") : HOME_DIR + "/.config";
const string DATA_DIR      = getenv("XDG_DATA_HOME")   != NULL ? getenv("XDG_DATA_HOME")   : HOME_DIR + "/.local/share";
const string CONFIG        = CONFIG_DIR + "/launcher.conf";
const string APP_DIRS[]    = { "/usr/share/applications", "/usr/local/share/applications", DATA_DIR + "/applications" };
const StyleAttribute COLORS[] = { C_TITLE, C_COMMENT, C_BG, C_HIGHLIGHT, C_MATCH };
const StyleAttribute FONTS[] = { F_REGULAR, F_BOLD, F_SMALLREGULAR, F_SMALLBOLD, F_LARGE };

map<string, int> launches = {};

map<StyleAttribute, const string> STYLE_ATTRIBUTES = {
	{ C_TITLE, "title" },
	{ C_COMMENT, "comment" },
	{ C_BG, "background" },
	{ C_HIGHLIGHT, "highlight" },
	{ C_MATCH, "match" },
	{ F_REGULAR, "regular" },
	{ F_BOLD, "bold" },
	{ F_SMALLREGULAR, "smallregular" },
	{ F_SMALLBOLD, "smallbold" },
	{ F_LARGE, "large" }
};

map<StyleAttribute, string> STYLE_DEFAULTS = {
	{ C_TITLE, "#111111" },
	{ C_COMMENT, "#999999" },
	{ C_BG, "#ffffff" },
	{ C_HIGHLIGHT, "#f8c291" },
	{ C_MATCH, "#111111" },
	{ F_REGULAR, "Ubuntu,sans-11" },
	{ F_BOLD, "Ubuntu,sans-11:bold" },
	{ F_SMALLREGULAR, "Ubuntu,sans-10" },
	{ F_SMALLBOLD, "Ubuntu,sans-10:bold" },
	{ F_LARGE, "Ubuntu,sans-20:light" }
};

vector<map<StyleAttribute, string>> THEMES = { // generated from http://daylerees.github.io/
	{ { NAME, "Default "} }, // Default
	{ { NAME, "Mellow Contrast" },     { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#7a7267" }, { C_BG, "#0b0a09" }, { C_HIGHLIGHT, "#13110f" }, { C_MATCH, "#1f8181" } },
	{ { NAME, "Legacy" },              { C_TITLE, "#aec2e0" }, { C_COMMENT, "#324357" }, { C_BG, "#14191f" }, { C_HIGHLIGHT, "#1b232c" }, { C_MATCH, "#ffffff" } },
	{ { NAME, "Slime Contrast" },      { C_TITLE, "#ffffff" }, { C_COMMENT, "#4f5a63" }, { C_BG, "#0b0c0d" }, { C_HIGHLIGHT, "#131516" }, { C_MATCH, "#6a9ec5" } },
	{ { NAME, "Peacock" },             { C_TITLE, "#ede0ce" }, { C_COMMENT, "#7a7267" }, { C_BG, "#2b2a27" }, { C_HIGHLIGHT, "#403c37" }, { C_MATCH, "#ff5d38" } },
	{ { NAME, "Earthsong Light" },     { C_TITLE, "#4d463e" }, { C_COMMENT, "#d6cab9" }, { C_BG, "#ffffff" }, { C_HIGHLIGHT, "#eeeeee" }, { C_MATCH, "#db784d" } },
	{ { NAME, "Zacks Contrast" },      { C_TITLE, "#f0f0f0" }, { C_COMMENT, "#777777" }, { C_BG, "#000000" }, { C_HIGHLIGHT, "#151515" }, { C_MATCH, "#ff6a38" } },
	{ { NAME, "Laravel Contrast" },    { C_TITLE, "#ffffff" }, { C_COMMENT, "#615953" }, { C_BG, "#000000" }, { C_HIGHLIGHT, "#151515" }, { C_MATCH, "#fc6b0a" } },
	{ { NAME, "Glowfish Contrast" },   { C_TITLE, "#6ea240" }, { C_COMMENT, "#3c4e2d" }, { C_BG, "#090b07" }, { C_HIGHLIGHT, "#12160d" }, { C_MATCH, "#db784d" } },
	{ { NAME, "Halflife" },            { C_TITLE, "#cccccc" }, { C_COMMENT, "#555555" }, { C_BG, "#222222" }, { C_HIGHLIGHT, "#282828" }, { C_MATCH, "#7d8991" } },
	{ { NAME, "Mud" },                 { C_TITLE, "#ffffff" }, { C_COMMENT, "#c3b8b7" }, { C_BG, "#403635" }, { C_HIGHLIGHT, "#322a29" }, { C_MATCH, "#ff9787" } },
	{ { NAME, "Keen" },                { C_TITLE, "#cccccc" }, { C_COMMENT, "#374c60" }, { C_BG, "#111111" }, { C_HIGHLIGHT, "#222222" }, { C_MATCH, "#8767b7" } },
	{ { NAME, "Patriot Contrast" },    { C_TITLE, "#cad9e3" }, { C_COMMENT, "#515e66" }, { C_BG, "#0d0e0f" }, { C_HIGHLIGHT, "#191b1c" }, { C_MATCH, "#2e6fd9" } },
	{ { NAME, "Glowfish" },            { C_TITLE, "#6ea240" }, { C_COMMENT, "#3c4e2d" }, { C_BG, "#191f13" }, { C_HIGHLIGHT, "#222a1a" }, { C_MATCH, "#db784d" } },
	{ { NAME, "Tribal" },              { C_TITLE, "#ffffff" }, { C_COMMENT, "#4a4a54" }, { C_BG, "#19191d" }, { C_HIGHLIGHT, "#33333c" }, { C_MATCH, "#5f5582" } },
	{ { NAME, "Peel" },                { C_TITLE, "#edebe6" }, { C_COMMENT, "#585146" }, { C_BG, "#23201c" }, { C_HIGHLIGHT, "#403c37" }, { C_MATCH, "#d3643b" } },
	{ { NAME, "FreshCut Contrast" },   { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#737b84" }, { C_BG, "#000000" }, { C_HIGHLIGHT, "#151515" }, { C_MATCH, "#00a8c6" } },
	{ { NAME, "Earthsong Contrast" },  { C_TITLE, "#ebd1b7" }, { C_COMMENT, "#7a7267" }, { C_BG, "#12100f" }, { C_HIGHLIGHT, "#282521" }, { C_MATCH, "#db784d" } },
	{ { NAME, "Gloom" },               { C_TITLE, "#d8ebe5" }, { C_COMMENT, "#4f6e64" }, { C_BG, "#2a332b" }, { C_HIGHLIGHT, "#3c4d3e" }, { C_MATCH, "#ff5d38" } },
	{ { NAME, "Yule" },                { C_TITLE, "#ede0ce" }, { C_COMMENT, "#7a7267" }, { C_BG, "#2b2a27" }, { C_HIGHLIGHT, "#52504b" }, { C_MATCH, "#d63131" } },
	{ { NAME, "Darkside Contrast" },   { C_TITLE, "#bababa" }, { C_COMMENT, "#494b4d" }, { C_BG, "#000000" }, { C_HIGHLIGHT, "#151515" }, { C_MATCH, "#e8341c" } },
	{ { NAME, "Mintchoc" },            { C_TITLE, "#bababa" }, { C_COMMENT, "#564439" }, { C_BG, "#2b221c" }, { C_HIGHLIGHT, "#3f322a" }, { C_MATCH, "#008d62" } },
	{ { NAME, "Potpourri" },           { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#696363" }, { C_BG, "#2e2b2c" }, { C_HIGHLIGHT, "#403c37" }, { C_MATCH, "#ed1153" } },
	{ { NAME, "Juicy" },               { C_TITLE, "#e3e2e0" }, { C_COMMENT, "#777777" }, { C_BG, "#222222" }, { C_HIGHLIGHT, "#282828" }, { C_MATCH, "#3bc7b8" } },
	{ { NAME, "Snappy Light" },        { C_TITLE, "#555555" }, { C_COMMENT, "#bbbbbb" }, { C_BG, "#ffffff" }, { C_HIGHLIGHT, "#eeeeee" }, { C_MATCH, "#f66153" } },
	{ { NAME, "Goldfish Contrast" },   { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#505c63" }, { C_BG, "#0c0d0e" }, { C_HIGHLIGHT, "#16181a" }, { C_MATCH, "#fa6900" } },
	{ { NAME, "Kiwi" },                { C_TITLE, "#edebe6" }, { C_COMMENT, "#354341" }, { C_BG, "#161a19" }, { C_HIGHLIGHT, "#282f2d" }, { C_MATCH, "#95c72a" } },
	{ { NAME, "Spearmint" },           { C_TITLE, "#719692" }, { C_COMMENT, "#93c7c0" }, { C_BG, "#e1f0ee" }, { C_HIGHLIGHT, "#ceebe7" }, { C_MATCH, "#25808a" } },
	{ { NAME, "Mud Contrast" },        { C_TITLE, "#ffffff" }, { C_COMMENT, "#524343" }, { C_BG, "#0d0b0b" }, { C_HIGHLIGHT, "#171413" }, { C_MATCH, "#f55239" } },
	{ { NAME, "Earthsong" },           { C_TITLE, "#ebd1b7" }, { C_COMMENT, "#7a7267" }, { C_BG, "#36312c" }, { C_HIGHLIGHT, "#45403b" }, { C_MATCH, "#db784d" } },
	{ { NAME, "Revelation Contrast" }, { C_TITLE, "#dedede" }, { C_COMMENT, "#7b726b" }, { C_BG, "#0c0b0b" }, { C_HIGHLIGHT, "#1a1918" }, { C_MATCH, "#617fa0" } },
	{ { NAME, "Github" },              { C_TITLE, "#555555" }, { C_COMMENT, "#b8b6b1" }, { C_BG, "#ffffff" }, { C_HIGHLIGHT, "#eeeeee" }, { C_MATCH, "#008080" } },
	{ { NAME, "Turnip Contrast" },     { C_TITLE, "#ede0ce" }, { C_COMMENT, "#7a7267" }, { C_BG, "#080809" }, { C_HIGHLIGHT, "#1c1d1f" }, { C_MATCH, "#487d76" } },
	{ { NAME, "Iceberg" },             { C_TITLE, "#bdd6db" }, { C_COMMENT, "#537178" }, { C_BG, "#323b3d" }, { C_HIGHLIGHT, "#3e4c4f" }, { C_MATCH, "#2d8da1" } },
	{ { NAME, "Zacks" },               { C_TITLE, "#f0f0f0" }, { C_COMMENT, "#777777" }, { C_BG, "#222222" }, { C_HIGHLIGHT, "#333333" }, { C_MATCH, "#ff6a38" } },
	{ { NAME, "Snappy" },              { C_TITLE, "#e3e2e0" }, { C_COMMENT, "#696969" }, { C_BG, "#393939" }, { C_HIGHLIGHT, "#282828" }, { C_MATCH, "#f66153" } },
	{ { NAME, "Slate" },               { C_TITLE, "#ebebf4" }, { C_COMMENT, "#515166" }, { C_BG, "#19191f" }, { C_HIGHLIGHT, "#2a2a33" }, { C_MATCH, "#89a7b1" } },
	{ { NAME, "Shrek" },               { C_TITLE, "#ffffff" }, { C_COMMENT, "#555555" }, { C_BG, "#222222" }, { C_HIGHLIGHT, "#333333" }, { C_MATCH, "#857a5e" } },
	{ { NAME, "Patriot" },             { C_TITLE, "#cad9e3" }, { C_COMMENT, "#515e66" }, { C_BG, "#2d3133" }, { C_HIGHLIGHT, "#40484d" }, { C_MATCH, "#2e6fd9" } },
	{ { NAME, "Chocolate" },           { C_TITLE, "#ffffff" }, { C_COMMENT, "#795431" }, { C_BG, "#150f08" }, { C_HIGHLIGHT, "#362715" }, { C_MATCH, "#ccb697" } },
	{ { NAME, "Peacock Contrast" },    { C_TITLE, "#ffffff" }, { C_COMMENT, "#555555" }, { C_BG, "#0c0c0b" }, { C_HIGHLIGHT, "#151513" }, { C_MATCH, "#ff5d38" } },
	{ { NAME, "Super" },               { C_TITLE, "#ffffff" }, { C_COMMENT, "#465360" }, { C_BG, "#15191d" }, { C_HIGHLIGHT, "#242b32" }, { C_MATCH, "#d60257" } },
	{ { NAME, "Peacocks In Space" },   { C_TITLE, "#dee3ec" }, { C_COMMENT, "#6e7a94" }, { C_BG, "#2b303b" }, { C_HIGHLIGHT, "#272b34" }, { C_MATCH, "#ff5d38" } },
	{ { NAME, "Lavender Contrast" },   { C_TITLE, "#e0ceed" }, { C_COMMENT, "#614e6e" }, { C_BG, "#080709" }, { C_HIGHLIGHT, "#110e13" }, { C_MATCH, "#b657ff" } },
	{ { NAME, "Arstotzka" },           { C_TITLE, "#edebe6" }, { C_COMMENT, "#3f3a36" }, { C_BG, "#211f1e" }, { C_HIGHLIGHT, "#292725" }, { C_MATCH, "#516b6b" } },
	{ { NAME, "Slime" },               { C_TITLE, "#ffffff" }, { C_COMMENT, "#4f5a63" }, { C_BG, "#292d30" }, { C_HIGHLIGHT, "#384147" }, { C_MATCH, "#9fb3c2" } },
	{ { NAME, "Sourlick Contrast" },   { C_TITLE, "#dedede" }, { C_COMMENT, "#615953" }, { C_BG, "#060606" }, { C_HIGHLIGHT, "#171615" }, { C_MATCH, "#8ac27a" } },
	{ { NAME, "Rainbow" },             { C_TITLE, "#c7d0d9" }, { C_COMMENT, "#424c55" }, { C_BG, "#16181a" }, { C_HIGHLIGHT, "#403c37" }, { C_MATCH, "#ef746f" } },
	{ { NAME, "Goldfish" },            { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#505c63" }, { C_BG, "#2e3336" }, { C_HIGHLIGHT, "#465459" }, { C_MATCH, "#fa6900" } },
	{ { NAME, "Snappy Contrast" },     { C_TITLE, "#e3e2e0" }, { C_COMMENT, "#696969" }, { C_BG, "#0c0c0c" }, { C_HIGHLIGHT, "#181818" }, { C_MATCH, "#f66153" } },
	{ { NAME, "Revelation" },          { C_TITLE, "#dedede" }, { C_COMMENT, "#7b726b" }, { C_BG, "#2e2c2b" }, { C_HIGHLIGHT, "#3b3633" }, { C_MATCH, "#617fa0" } },
	{ { NAME, "Frontier Contrast" },   { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#7a7267" }, { C_BG, "#110f0e" }, { C_HIGHLIGHT, "#221f1c" }, { C_MATCH, "#f23a3a" } },
	{ { NAME, "Stark Contrast" },      { C_TITLE, "#dedede" }, { C_COMMENT, "#615953" }, { C_BG, "#0b0a0a" }, { C_HIGHLIGHT, "#181716" }, { C_MATCH, "#f03113" } },
	{ { NAME, "Darkside" },            { C_TITLE, "#bababa" }, { C_COMMENT, "#494b4d" }, { C_BG, "#222324" }, { C_HIGHLIGHT, "#303333" }, { C_MATCH, "#e8341c" } },
	{ { NAME, "Solarflare Contrast" }, { C_TITLE, "#e3e2e0" }, { C_COMMENT, "#777777" }, { C_BG, "#000000" }, { C_HIGHLIGHT, "#151515" }, { C_MATCH, "#ff4e50" } },
	{ { NAME, "Frontier" },            { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#7a7267" }, { C_BG, "#36312c" }, { C_HIGHLIGHT, "#45403b" }, { C_MATCH, "#f23a3a" } },
	{ { NAME, "Tonic" },               { C_TITLE, "#eeeeee" }, { C_COMMENT, "#4a5356" }, { C_BG, "#2a2f31" }, { C_HIGHLIGHT, "#353b3e" }, { C_MATCH, "#b8cd44" } },
	{ { NAME, "Pastel" },              { C_TITLE, "#eeeeee" }, { C_COMMENT, "#444444" }, { C_BG, "#222222" }, { C_HIGHLIGHT, "#333333" }, { C_MATCH, "#04c4a5" } },
	{ { NAME, "Potpourri Contrast" },  { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#696363" }, { C_BG, "#0d0c0c" }, { C_HIGHLIGHT, "#141313" }, { C_MATCH, "#ed1153" } },
	{ { NAME, "Tron" },                { C_TITLE, "#aec2e0" }, { C_COMMENT, "#324357" }, { C_BG, "#14191f" }, { C_HIGHLIGHT, "#1b232c" }, { C_MATCH, "#ffffff" } },
	{ { NAME, "Grunge" },              { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#5c634f" }, { C_BG, "#31332c" }, { C_HIGHLIGHT, "#41453a" }, { C_MATCH, "#f56991" } },
	{ { NAME, "Bold" },                { C_TITLE, "#ffffff" }, { C_COMMENT, "#534b4b" }, { C_BG, "#2a2626" }, { C_HIGHLIGHT, "#393434" }, { C_MATCH, "#f0624b" } },
	{ { NAME, "Keen Contrast" },       { C_TITLE, "#ffffff" }, { C_COMMENT, "#374c60" }, { C_BG, "#000000" }, { C_HIGHLIGHT, "#151515" }, { C_MATCH, "#8767b7" } },
	{ { NAME, "Crisp" },               { C_TITLE, "#ffffff" }, { C_COMMENT, "#574457" }, { C_BG, "#221a22" }, { C_HIGHLIGHT, "#1c151c" }, { C_MATCH, "#765478" } },
	{ { NAME, "Otakon" },              { C_TITLE, "#f9f3f9" }, { C_COMMENT, "#515166" }, { C_BG, "#171417" }, { C_HIGHLIGHT, "#332d33" }, { C_MATCH, "#f6e6eb" } },
	{ { NAME, "Carbonight" },          { C_TITLE, "#b0b0b0" }, { C_COMMENT, "#423f3d" }, { C_BG, "#2e2c2b" }, { C_HIGHLIGHT, "#3b3633" }, { C_MATCH, "#8c8c8c" } },
	{ { NAME, "FreshCut" },            { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#737b84" }, { C_BG, "#2f3030" }, { C_HIGHLIGHT, "#383939" }, { C_MATCH, "#00a8c6" } },
	{ { NAME, "Juicy Contrast" },      { C_TITLE, "#e3e2e0" }, { C_COMMENT, "#777777" }, { C_BG, "#000000" }, { C_HIGHLIGHT, "#151515" }, { C_MATCH, "#3bc7b8" } },
	{ { NAME, "Box UK" },              { C_TITLE, "#414f5c" }, { C_COMMENT, "#b8b6b1" }, { C_BG, "#ffffff" }, { C_HIGHLIGHT, "#eeeeee" }, { C_MATCH, "#019d76" } },
	{ { NAME, "Grunge Contrast" },     { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#5c634f" }, { C_BG, "#0c0c0a" }, { C_HIGHLIGHT, "#1b1c18" }, { C_MATCH, "#f56991" } },
	{ { NAME, "Mellow" },              { C_TITLE, "#f8f8f2" }, { C_COMMENT, "#7a7267" }, { C_BG, "#36312c" }, { C_HIGHLIGHT, "#45403b" }, { C_MATCH, "#1f8181" } },
	{ { NAME, "Halflife Contrast" },   { C_TITLE, "#cccccc" }, { C_COMMENT, "#555555" }, { C_BG, "#000000" }, { C_HIGHLIGHT, "#151515" }, { C_MATCH, "#7d8991" } },
	{ { NAME, "Hyrule Contrast" },     { C_TITLE, "#c0d5c1" }, { C_COMMENT, "#716d6a" }, { C_BG, "#0c0c0c" }, { C_HIGHLIGHT, "#141413" }, { C_MATCH, "#569e16" } },
	{ { NAME, "Iceberg Contrast" },    { C_TITLE, "#bdd6db" }, { C_COMMENT, "#537178" }, { C_BG, "#0b0e0e" }, { C_HIGHLIGHT, "#141718" }, { C_MATCH, "#2d8da1" } },
	{ { NAME, "Piggy" },               { C_TITLE, "#edebe6" }, { C_COMMENT, "#3f3236" }, { C_BG, "#1c1618" }, { C_HIGHLIGHT, "#34282c" }, { C_MATCH, "#fd6a5d" } },
	{ { NAME, "Hyrule" },              { C_TITLE, "#c0d5c1" }, { C_COMMENT, "#716d6a" }, { C_BG, "#2d2c2b" }, { C_HIGHLIGHT, "#3d3934" }, { C_MATCH, "#569e16" } },
	{ { NAME, "Sourlick" },            { C_TITLE, "#dedede" }, { C_COMMENT, "#615953" }, { C_BG, "#2e2c2b" }, { C_HIGHLIGHT, "#3b3633" }, { C_MATCH, "#8ac27a" } },
	{ { NAME, "Laravel" },             { C_TITLE, "#dedede" }, { C_COMMENT, "#615953" }, { C_BG, "#2e2c2b" }, { C_HIGHLIGHT, "#3b3633" }, { C_MATCH, "#fc6b0a" } },
	{ { NAME, "Azure" },               { C_TITLE, "#ffffff" }, { C_COMMENT, "#414d62" }, { C_BG, "#181d26" }, { C_HIGHLIGHT, "#33333c" }, { C_MATCH, "#52708b" } },
	{ { NAME, "Tron Contrast" },       { C_TITLE, "#aec2e0" }, { C_COMMENT, "#324357" }, { C_BG, "#07090b" }, { C_HIGHLIGHT, "#101419" }, { C_MATCH, "#ffffff" } },
	{ { NAME, "Stark" },               { C_TITLE, "#dedede" }, { C_COMMENT, "#615953" }, { C_BG, "#2e2c2b" }, { C_HIGHLIGHT, "#3b3633" }, { C_MATCH, "#f03113" } },
	{ { NAME, "Userscape" },           { C_TITLE, "#879bb0" }, { C_COMMENT, "#bbbbbb" }, { C_BG, "#f5f8fc" }, { C_HIGHLIGHT, "#eeeeee" }, { C_MATCH, "#355b8c" } },
	{ { NAME, "Carbonight Contrast" }, { C_TITLE, "#ffffff" }, { C_COMMENT, "#423f3d" }, { C_BG, "#000000" }, { C_HIGHLIGHT, "#151515" }, { C_MATCH, "#8c8c8c" } },
	{ { NAME, "Gloom Contrast" },      { C_TITLE, "#d8ebe5" }, { C_COMMENT, "#4f6e64" }, { C_BG, "#0f120f" }, { C_HIGHLIGHT, "#1b211c" }, { C_MATCH, "#ff5d38" } },
	{ { NAME, "Lavender" },            { C_TITLE, "#e0ceed" }, { C_COMMENT, "#614e6e" }, { C_BG, "#29222e" }, { C_HIGHLIGHT, "#3a2f42" }, { C_MATCH, "#b657ff" } },
	{ { NAME, "Solarflare" },          { C_TITLE, "#e3e2e0" }, { C_COMMENT, "#777777" }, { C_BG, "#222222" }, { C_HIGHLIGHT, "#282828" }, { C_MATCH, "#ff4e50" } },
	{ { NAME, "Turnip" },              { C_TITLE, "#ede0ce" }, { C_COMMENT, "#7a7267" }, { C_BG, "#1a1b1d" }, { C_HIGHLIGHT, "#222222" }, { C_MATCH, "#487d76" } } 
};

map<StyleAttribute, string> STYLE_OVERRIDE = {};

Display *display;
int screen;
Window window, root;
GC gc;
XIC xic;
XftDraw *xftdraw;
string query = "";
string queryi = ""; // lower case
int selected = 0;
int cursor = 0;
bool cursorVisible = false;
int width;
float baseWidth = 0.3f; // width as percentage of screen width
int theme = 0;
float scaleFactor = 1.0f;
int inputHeight, rowHeight, textOffset, borderWidth, indent, commentSpace;
XSetWindowAttributes attributes;
vector<Application> applications;
vector<Result> results;
map<StyleAttribute, XftFont*> fonts;
map<StyleAttribute, XftColor> colors;

string lowercase (const string &str) {
	string out = str;
	transform(out.begin(), out.end(), out.begin(), ::tolower);
	return out;
};

int renderText(const int x, const int y, string text, XftFont &font, const XftColor &color) {
	XftDrawString8(xftdraw, &color, &font, x, y, (XftChar8 *) text.c_str(), text.length());
	if (text.back() == ' ') { // XftTextExtents appears to not count whitespace at the end of a string, so move it to the beginning
		text = " " + text;
	}
	XGlyphInfo extents;
	XftTextExtents8(display, &font, (FcChar8 *) text.c_str(), text.length(), &extents);
	return x + extents.width;
}

void search () {
	results = {};
	for (Application &app : applications) {
		int score = 0;
		int i = 0;
		for (const Keyword &keyword : app.keywords) {
			int matchIndex = keyword.word.find(queryi);
			if (matchIndex != string::npos) {
				// score determined by:
				// - apps whose names begin with the query string appear first
				// - apps whose names or descriptions contain the query string then appear
				// - apps which hav e been opened most frequently should be prioritised
				score = (100 - i) * keyword.weight * (matchIndex == 0 ? 10000 : 100) + launches[app.id];
				break;
			}
			i++;
		}
		if (score > 0) {
			results.push_back({ &app, score });
		}
	}
	sort(results.begin(), results.end(), [](const Result &a, const Result &b) {
		return b.score < a.score;
	});
	if (results.size() > 10) {
		results.resize(10); // limit to 10 results
	}
}

auto lastBlink = std::chrono::system_clock::now();
void renderTextInput (const bool showCursor) {
	lastBlink = std::chrono::system_clock::now();
	int ty = 0.66 * inputHeight;
	char buffer[256];
	time_t t = time(NULL);
	strftime(buffer, sizeof(buffer), "%a %e %b %H:%M", localtime(&t));
	int clockWidth = renderText(0, 0, buffer, *fonts[F_SMALLREGULAR], colors[C_COMMENT]);
	XClearArea(display, window, 0, 0, width, inputHeight, false); // clear input area
	XSetForeground(display, gc, colors[C_HIGHLIGHT].pixel); // input border color
	XSetLineAttributes(display, gc, borderWidth, LineSolid, CapButt, JoinRound); // input border style
	XDrawRectangle(display, window, gc, 0, 0, width - 1, inputHeight); // input border
	renderText(width - clockWidth - indent, ty * 0.92, buffer, *fonts[F_SMALLREGULAR], colors[C_TITLE]);
	if (showCursor) {
		int cursorX = renderText(indent * 1.3, ty, query.substr(0, cursor), *fonts[F_LARGE], colors[C_BG]); // invisible text just to figure out cursor position
		XSetForeground(display, gc, showCursor ? colors[C_TITLE].pixel : colors[C_BG].pixel); // cursor color
		XFillRectangle(display, window, gc, cursorX, inputHeight / 4, 3, inputHeight / 2); // cursor
	}
	renderText(indent, ty, query, *fonts[F_LARGE], colors[C_TITLE]); // visible input text
	cursorVisible = showCursor;
}

void cursorBlink () {
	auto now = std::chrono::system_clock::now();
	std::chrono::duration<double> delta = now - lastBlink;
	if (delta > (std::chrono::milliseconds) 700) { // seconds
		renderTextInput(!cursorVisible);
	}
}

void renderResults () {
	int resultCount = results.size();

	XClearArea(display, window, 0, inputHeight, width, resultCount * rowHeight, false); // clear results area
	XSetForeground(display, gc, colors[C_HIGHLIGHT].pixel); // results border color
	XSetLineAttributes(display, gc, borderWidth, LineSolid, CapButt, JoinRound); // results border style
	XDrawRectangle(display, window, gc, 0, inputHeight - 1, width - 1, resultCount * rowHeight - 1); // results border
	
	for (int i = 0; i < resultCount; i++) {
		const Result result = results[i];
		const int namei = lowercase(result.app->name).find(queryi);
		const int commenti = lowercase(result.app->comment).find(queryi);
		const int y = inputHeight + i * rowHeight;
		int x = indent;

		if (i == selected) {
			XSetForeground(display, gc, colors[C_HIGHLIGHT].pixel);
			XFillRectangle(display, window, gc, 0, y, width, rowHeight);
		}
		
		if (namei == string::npos) {
			x = renderText(x, y + textOffset, result.app->name.c_str(), *fonts[F_REGULAR], colors[C_TITLE]);
		} else {
			string str = result.app->name.substr(0, namei);
			x = renderText(x, y + textOffset, str.c_str(), *fonts[F_REGULAR], colors[C_TITLE]);
			str = result.app->name.substr(namei, query.length());
			x = renderText(x, y + textOffset, str.c_str(), *fonts[F_BOLD], colors[C_MATCH]);
			str = result.app->name.substr(namei + query.length());
			x = renderText(x, y + textOffset, str.c_str(), *fonts[F_REGULAR], colors[C_TITLE]);
		}

		if (commenti == string::npos) {
			renderText(x + commentSpace, y + textOffset, result.app->comment.c_str(), *fonts[F_SMALLREGULAR], colors[C_COMMENT]);
		} else {
			string str = result.app->comment.substr(0, commenti);
			x = renderText(x + commentSpace, y + textOffset, str.c_str(), *fonts[F_SMALLREGULAR], colors[C_COMMENT]);
			str = result.app->comment.substr(commenti, query.length());
			x = renderText(x, y + textOffset, str.c_str(), *fonts[F_SMALLBOLD], colors[C_COMMENT]);
			str = result.app->comment.substr(commenti + query.length());
			renderText(x, y + textOffset, str.c_str(), *fonts[F_SMALLREGULAR], colors[C_COMMENT]);
		}
	}
}

void readConfig () {
	struct stat info;
	if (stat(CONFIG.c_str(), &info) != 0) { return; }
	ifstream infile(CONFIG);
	string line;
	while (getline(infile, line)) {
		const int i = line.find_last_of("=");
		if (i == string::npos) { continue; }
		const string key = line.substr(0, i);
		const string val = line.substr(i + 1);
		if (key == "scale") {
			scaleFactor = stof(val);
		} else if (key == "width") {
			baseWidth = stof(val);
		} else if (key == "theme") {
			int j = 0;
			for (auto &t : THEMES) {
				if (t[NAME] == val) {
					theme = j;
					break;
				}
				j++;
			}
		} else if (key.find("/") == string::npos) {
			for (const auto &[type, attr] : STYLE_ATTRIBUTES) {
				if (key == attr) {
					STYLE_OVERRIDE[type] = val;
					break;
				}
			}
		} else {
			launches[key] = stoi(val);
		}
	}
}

void writeConfig () {
	ofstream outfile;
	outfile.open(CONFIG);
	outfile << "[Style]\n";
	outfile << "theme=" << THEMES[theme][NAME] << "\n";
	outfile << "scale=" << scaleFactor << "\n";
	outfile << "width=" << baseWidth << "\n";
	for (const auto &[type, attr] : STYLE_ATTRIBUTES) {
		if (STYLE_OVERRIDE.find(type) != STYLE_OVERRIDE.end()) {
			outfile << STYLE_ATTRIBUTES[type] << "=" << STYLE_OVERRIDE[type] << "\n";
		}
	}
	outfile << "\n[Launches]\n";

	for (const auto &[appid, count] : launches) {
		if (count > 0) {
			outfile << appid << "=" << count << "\n";
		}
	}
	outfile.close();
}

vector<Application> getApplications () {
	vector<Application> applications;
	for (const string &dir : APP_DIRS) {
		struct stat info;
		if (stat(dir.c_str(), &info) != 0) { continue; }
		for (const auto &entry : fs::directory_iterator(dir)) {
			Application app = {};
			app.id = entry.path();
			ifstream infile(app.id);
			string line, keywords;
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
				if (app.cmd == "" && line.find("Keywords=") == 0) {
					keywords = line.substr(9);
				}
			}
			
			stringstream ss = stringstream(lowercase(app.name));
			string word;
			while (getline(ss, word, ' ')) {
				app.keywords.push_back({ word, 1000 });
			}

			ss = stringstream(lowercase(keywords));
			while (getline(ss, word, ';')) {
				app.keywords.push_back({ word, 1 });
			}

			ss = stringstream(lowercase(app.genericName + ' ' + app.comment));
			while (getline(ss, word, ' ')) {
				app.keywords.push_back({ word, 1 });
			}

			applications.push_back(app);
		}
	}
	return applications;
}

void setProperty (const char *property, const char *value) {
	const Atom propertyAtom = XInternAtom(display, property, False);
	const long valueAtom = XInternAtom(display, value, False);
	XChangeProperty(display, window, propertyAtom, XA_ATOM, 32, PropModeReplace, (unsigned char *) &valueAtom, 1);
}

void launch (Application &app) {
	const int pid = fork(); // this duplicates the launcher process
	if (pid == 0) { // if this is the child process, replace it with the application
		chdir(HOME_DIR.c_str());
		stringstream ss(app.cmd);
		vector<char*> args;
		string arg;
		while (getline(ss, arg, ' ')) {
			if (arg.find('%') != 0) {
				string *a = new string(arg);
				args.push_back(const_cast<char *>(a->c_str()));
			}
		}
		args.push_back(NULL);
		char **command = &args[0];
		execvp(command[0], command);
	} else {
		launches[app.id]++;
		writeConfig();
	}
	exit(0);
}

Visual *visual;
Colormap colormap;
int windowX, windowY;

map<StyleAttribute, string> getStyle () {
	map<StyleAttribute, string> style = STYLE_DEFAULTS;
	for (const auto &[type, val] : THEMES[theme]) {
		style[type] = val;
	}
	for (const auto &[type, val] : STYLE_OVERRIDE) {
		style[type] = val;
	}
	return style;
};

void updateFonts () {
	map<StyleAttribute, string> style = getStyle();
	for (const StyleAttribute c : FONTS) {
		if (fonts.find(c) != fonts.end()) {
			XftFontClose(display, fonts[c]);
		}
		string name = style[c];
		int i = name.find("-");
		if (i > 0) {
			int j = name.find(":");
			string before = name.substr(0, i + 1);
			string number = name.substr(i + 1, j > 0 ? j - i : string::npos);
			string after = j > 0 ? name.substr(j, string::npos) : "";
			int size = stoi(number) * scaleFactor;
			name = before + std::to_string(size) + after;
		}
		fonts[c] = XftFontOpenName(display, screen, name.c_str());
	}
}

void updateStyle () {
	map<StyleAttribute, string> style = getStyle();

	for (const StyleAttribute c : COLORS) {
		unsigned short r = stol(style[c].substr(1, 2), nullptr, 16) * 256;
		unsigned short g = stol(style[c].substr(3, 2), nullptr, 16) * 256;
		unsigned short b = stol(style[c].substr(5, 2), nullptr, 16) * 256;
		XRenderColor xrcolor = { .red = r, .green = g, .blue = b, .alpha = 255 * 256 };
		XftColorAllocValue(display, visual, colormap, &xrcolor, &colors[c]);
	}

	updateFonts();

	XSetWindowBackground(display, window, colors[C_BG].pixel);
}

void updateScale () {
	int x, y, throwaway;
	unsigned m;
	Window w;
	XQueryPointer(display, root, &w, &w, &x, &y, &throwaway, &throwaway, &m); // get mouse position 
	XRRScreenResources *xrrr = XRRGetScreenResources(display, root);
	XRRCrtcInfo *monitor;
	int i;
	int ncrtc = xrrr->ncrtc;
	for (i = 0; i < ncrtc; ++i) {
		monitor = XRRGetCrtcInfo(display, xrrr, xrrr->crtcs[i]);
		if (x >= monitor->x && x < monitor->x + monitor->width &&
			y >= monitor->y && y < monitor->y + monitor->width) { // find monitor which mouse in on
			break;
		}
	}
	char *resourceString = XResourceManagerString(display);
	XrmInitialize(); /* Need to initialize the DB before calling Xrm* functions */
	XrmDatabase db = XrmGetStringDatabase(resourceString);
	char *type = NULL;
	XrmValue value;
	double dpi = 96.0;
	if (resourceString && 
			XrmGetResource(db, "Xft.dpi", "String", &type, &value) == True &&
			value.addr) {
		dpi = atof(value.addr);
	}
	float dpiScaleFactor = dpi / BASE_DPI;
	if (scaleFactor < 0.1) { scaleFactor = 0.1; }
	float sf = dpiScaleFactor * scaleFactor;
	inputHeight = sf * INPUT_HEIGHT;
	rowHeight = sf * ROW_HEIGHT;
	textOffset = sf * TEXT_OFFSET;
	borderWidth = sf * BORDER_WIDTH;
	indent = sf * INDENT;
	commentSpace = sf * COMMENT_SPACE;
	width = sf * monitor->width * baseWidth;
	if (width < 200) {
		if (monitor->width > 210) {
			width = 200;
		} else {
			width = monitor->width - 10;
		}
	}
	windowX = monitor->x + monitor->width / 2 - width / 2;
	windowY = monitor->y + 200;
	updateFonts();
}

void onKeyPress (XEvent &event) {
	char text[128] = {0};
	KeySym keysym;
	int textlength = Xutf8LookupString(xic, &event.xkey, text, sizeof text, &keysym, NULL);
	bool shift = event.xkey.state == 1;
	bool ctrl = event.xkey.state == 4;
	switch (keysym) {
		case XK_Escape:
			exit(0);
			break;
		case XK_Return:
			launch(*results[selected].app);
			break;
		case XK_Up:
			selected = selected > 0 ? selected - 1 : results.size() - 1;
			break;
		case XK_Down:
			selected = selected < results.size() - 1 ? selected + 1 : 0;
			break;
		case XK_Left:
			cursor = cursor > 0 ? cursor - 1 : 0;
			break;
		case XK_Right:
			cursor = cursor < query.length() ? cursor + 1 : query.length();
			break;
		case XK_Home:
			cursor = 0;
			break;
		case XK_End:
			cursor = query.length();
			break;
		case XK_BackSpace:
			if (cursor > 0) {
				if (ctrl) {
					query.erase(0, cursor);
					cursor = 0;
				} else {
					query.erase(cursor - 1, 1);
					cursor--;
				}
			}
			break;
		case XK_Delete:
			if (cursor < query.length()) {
				query.erase(cursor, ctrl ? query.length() - cursor : 1);
			}
			break;
		case XK_F4: // F4 and F5 for theme
		case XK_F5:
			if (keysym == XK_F4) {
				theme = theme > 0 ? theme - 1 : THEMES.size() - 1;
			} else {
				theme = theme < THEMES.size() - 1 ? theme + 1 : 0;
			}
			updateStyle();
			writeConfig();
			break;
		case XK_F6: // F6 and F7 for scaling/zoom
		case XK_F7:
			scaleFactor += keysym == XK_F6 ? -0.1f : 0.1f;
			if (scaleFactor > 6.0f) { scaleFactor = 6.0f; }
			if (scaleFactor < 0.1f) { scaleFactor = 0.1f; }
			updateScale();
			writeConfig();
			break;
		case XK_F8: // F8 and F9 for width
		case XK_F9:
			baseWidth += keysym == XK_F8 ? -0.01f : 0.01f;
			if (baseWidth > 1.0f) { baseWidth = 1.0f; }
			if (baseWidth < 0.05f) { baseWidth = 0.05f; }
			updateScale();
			writeConfig();
			break;
		default:
			if (textlength == 1 && !ctrl) { // check it's a character
				query = query.substr(0, cursor) + text + query.substr(cursor, query.length());
				cursor++;
			}
	}
	queryi = lowercase(query);
}

int main () {
	auto awaitApps = async(getApplications); // prepare list of apps in the background
	readConfig();

	display = XOpenDisplay(NULL);
	screen = DefaultScreen(display);
	visual = DefaultVisual(display, screen);
	colormap = DefaultColormap(display, screen);
	root = DefaultRootWindow(display);
	int depth = DefaultDepth(display, screen);
	bool applicationsLoaded = false;

	updateScale();
	
	window = XCreateWindow(display, root,
		windowX, windowY, width, inputHeight,
		5, depth, InputOutput, visual, CWBackPixel, &attributes);
	XSelectInput(display, window, ExposureMask | KeyPressMask | FocusChangeMask);
	XIM xim = XOpenIM(display, 0, 0, 0);
	xic = XCreateIC(xim, // input context
		XNInputStyle,   XIMPreeditNothing | XIMStatusNothing,
		XNClientWindow, window,
		XNFocusWindow,  window,
		NULL);

	XGCValues gr_values;
	gc = XCreateGC(display, window, 0, &gr_values);

	updateStyle();

	setProperty("_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_DOCK");
	setProperty("_NET_WM_STATE", "_NET_WM_STATE_ABOVE");
	setProperty("_NET_WM_STATE", "_NET_WM_STATE_MODAL");

	XMapWindow(display, window);

	xftdraw = XftDrawCreate(display, window, visual, colormap);

	XEvent event;
	while (1) {
		while (XCheckMaskEvent(display, ExposureMask | KeyPressMask | FocusChangeMask, &event)) {
			if (event.type == Expose) {
				renderTextInput(true);
				renderResults();
			}
			if (event.type == KeyPress) {
				onKeyPress(event);
				if (query.length() > 0) {
					if (!applicationsLoaded) {
						applications = awaitApps.get();
						applicationsLoaded = true;
					}
					search();
				} else {
					results = {};
				}
				if (selected >= results.size()) {
					selected = 0;
				}
				XMoveResizeWindow(display, window, windowX, windowY, width, inputHeight + results.size() * rowHeight);
				renderTextInput(true);
				renderResults();
			}
			if (event.type == FocusOut) { exit(0); }
		}
		cursorBlink();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
