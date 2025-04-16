
typedef struct { char* text; int length; } Line;
typedef struct { float targetY, currentY, targetX, currentX, velocity; bool smoothScroll; } ScrollState;
typedef struct { int originalLine; int startCol; int length; bool isWrapped; } WrappedLine;
typedef struct { int line; int col; char open; char close; bool wasAutoInserted; } AutoPair;

#define MAXTEXT 16384
#define MAX_FILENAME 256
#define MAX_AUTO_PAIRS 32

Line lines[MAXTEXT] = {0};
int numLines = 1, cursorLine = 0, cursorCol = 0, lineHeight = 0, gutterWidth = 0;
bool isSelecting = false, showLineNumbers = true, showStatusBar = true, showScrollbar = true, wordWrap = false;
bool isFileDirty = false, isScrollbarDragging = false, isHorizontalScrollbarDragging = false;
bool insertMode = true, showMinimap = true, isMinimapDragging = false, showModeline = true;
static bool autoParingEnabled = true;
int selStartLine = -1, selStartCol = -1, selEndLine = -1, selEndCol = -1;
int statusBarHeight = 25, scrollbarWidth = 12, minimapWidth = 100, modeLineHeight = 20;
double fontSize = 24.0f;
float minimapScale = 0.2f;
char fileEncoding[16] = "UTF-8", lineEnding[8] = "LF", filename[MAX_FILENAME] = "Untitled.txt", statusMsg[256] = "";
ScrollState scroll = {0, 0, 0, 0, 0, true};
static double lastVerticalScrollTime = 0, lastHorizontalScrollTime = 0, scrollbarFadeDelay = 1.5;
static double lastKeyTime[GLFW_KEY_LAST + 1] = {0};
static bool keyStates[GLFW_KEY_LAST + 1] = {false};
Color textColor = {220, 220, 220, 255};
WrappedLine wrappedLines[MAXTEXT * 2] = {0};
int numWrappedLines = 0;
static AutoPair autoPairs[MAX_AUTO_PAIRS] = {0};
static int numAutoPairs = 0;

int goalColumn = -1;
int consecutiveEOLMoves = 0;
bool wasAtEOL = false;
bool nowAtEOL = false;

static const struct {
    char open;
    char close;
} bracket_pairs[] = {
    {'(', ')'},
    {'[', ']'},
    {'{', '}'},
    {'<', '>'},
    {'"', '"'},
    {'\'', '\''},
    {'`', '`'},
};

static int targCol = -1;
