
#define LSP_MAX_DIAGS 100
#define LSP_MAX_MSG_LEN 256
#define LSP_BUFFER_SIZE 16384
#define LSP_JSON_TOKEN_MAX 128
#define LSP_MAX_COMPLETIONS 100
#define LSP_URI_MAX 256
#define LSP_PATH_MAX 256
#define LSP_HOVER_MAX 2048

typedef struct {
    int line;
    int startCol;
    int endCol;
    int severity;
    char message[LSP_MAX_MSG_LEN];
} LspDiagnostic;

typedef struct {
    char label[256];
    char detail[256];
    char insertText[256];
    int kind;
    bool selected;
} CompletionItem;

typedef struct {
    bool active;
    int count;
    int selectedIndex;
    int startLine;
    int startCol;
    CompletionItem items[LSP_MAX_COMPLETIONS];
    char filter[256];
} CompletionList;

typedef struct {
    char contents[LSP_HOVER_MAX];
    bool active;
    int line;
    int col;
    int width;
    int height;
} HoverInfo;

typedef struct {
    char uri[LSP_URI_MAX];
    int line;
    int startCol;
    int endCol;
} DefinitionLocation;

typedef struct {
    bool active;
    pid_t pid;
    int inPipe[2];
    int outPipe[2];
    int requestId;
    char langId[32];
    char uri[LSP_URI_MAX];
    LspDiagnostic diagnostics[LSP_MAX_DIAGS];
    int diagnosticCount;
    CompletionList completions;
    double definitionRequestTime;
    char configPath[PATH_MAX];
    HoverInfo hoverInfo;
    DefinitionLocation definitionLocation;
    bool isDefinitionAvailable;
    double lastHoverTime;
    int hoverLine;
    int hoverCol;
    char buffer[LSP_BUFFER_SIZE];
    int bufferLen;
} LspClient;

extern LspClient lsp;

void LspInit(void);
void LspStop(void);
void LspCleanup(void);
void UpdateLsp(void);
void EditorLspKeyHandler(int key, int action, int mods);
void EditorLspCharHandler(unsigned int c);