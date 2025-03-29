#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <libgen.h>
LspClient lsp = {0};
static bool lspInitialized = false;
double lastTypingTime = 0;
bool autoCompletionEnabled = true;
int autoCompletionDelayMs = 250;
int minCompletionChars = 2;
bool preventUiReopen = false;
double preventionStartTime = 0.0;
const double preventionDuration = 2.0;
static int prevLine = -1;
static int prevCol = -1;
static char prevFilename[MAX_FILENAME] = {0};
static bool isDefinitionJumped = false;
char* LspEscapeJson(const char* text) {
    if (!text) return NULL;
    size_t len = strlen(text);
    char* escaped = malloc(len * 2 + 1);
    if (!escaped) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\\' || text[i] == '"' || text[i] == '\n' || text[i] == '\r') {
            escaped[j++] = '\\';
            if (text[i] == '\n') { escaped[j++] = 'n'; continue; } 
            else if (text[i] == '\r') { escaped[j++] = 'r'; continue; }
        }
        escaped[j++] = text[i];
    }
    escaped[j] = '\0';
    return escaped;
}
char* LspFindJsonValue(const char* json, const char* key) {
    if (!json || !key) return NULL;
    char searchKey[LSP_JSON_TOKEN_MAX];
    snprintf(searchKey, sizeof(searchKey), "\"%s\"", key);
    const char* keyPos = strstr(json, searchKey);
    if (!keyPos) return NULL;
    keyPos += strlen(searchKey);
    const char* colon = strchr(keyPos, ':');
    if (!colon) return NULL;
    const char* valueStart = colon + 1;
    while (*valueStart && isspace(*valueStart)) valueStart++;
    if (!*valueStart) return NULL;
    return (char*)valueStart;
}
char* LspFindJsonArray(const char* json, const char* key) {
    char* value = LspFindJsonValue(json, key);
    if (!value || *value != '[') return NULL;
    return value;
}
int LspJsonArrayLength(const char* array) {
    if (!array || *array != '[') return 0;
    int count = 0, depth = 0;
    bool inString = false, itemStarted = false;
    for (const char* p = array + 1; *p; p++) {
        if (inString && p[-1] == '\\') continue;
        switch (*p) {
            case '"': inString = !inString; break;
            case '[': case '{': if (!inString) depth++; break;
            case ']': case '}': if (!inString) { depth--; if (depth < 0) return count; } break;
            case ',': if (!inString && depth == 0) { count++; itemStarted = false; } break;
            default: if (!inString && depth == 0 && !isspace(*p) && !itemStarted) itemStarted = true; break;
        }
    }
    return count + (itemStarted ? 1 : 0);
}
char* LspJsonArrayItem(const char* array, int index) {
    if (!array || *array != '[' || index < 0) return NULL;
    const char* p = array + 1;
    while (isspace(*p)) p++;
    int currentIndex = 0, depth = 0;
    bool inString = false;
    while (*p) {
        if (currentIndex == index) return (char*)p;
        if (inString && p[-1] == '\\') { p++; continue; }
        switch (*p) {
            case '"': inString = !inString; break;
            case '[': case '{': if (!inString) depth++; break;
            case ']': case '}': if (!inString) depth--; break;
            case ',': if (!inString && depth == 0) { currentIndex++; p++; while (isspace(*p)) p++; continue; } break;
        }
        if (*p == ']' && !inString && depth < 0) break;
        p++;
    }
    return NULL;
}
char* LspJsonExtractString(const char* json, char* buffer, size_t bufferSize) {
    if (!json || !buffer || bufferSize == 0) return NULL;
    buffer[0] = '\0';
    const char* start = strchr(json, '"');
    if (!start) return NULL;
    start++;
    size_t i = 0;
    while (*start && *start != '"' && i < bufferSize - 1) {
        if (*start == '\\' && *(start + 1)) {
            start++;
            if (i >= bufferSize - 1) break;
            switch (*start) {
                case 'n': buffer[i++] = '\n'; break;
                case 'r': buffer[i++] = '\r'; break;
                case 't': buffer[i++] = '\t'; break;
                case '\\': buffer[i++] = '\\'; break;
                case '"': buffer[i++] = '"'; break;
                default: buffer[i++] = *start; break;
            }
        } else {
            buffer[i++] = *start;
        }
        start++;
    }
    buffer[i] = '\0';
    return buffer;
}
int LspJsonExtractInt(const char* json, int defaultValue) {
    if (!json) return defaultValue;
    while (*json && isspace(*json)) json++;
    if (!*json || !isdigit(*json)) return defaultValue;
    return atoi(json);
}
void LspCreateUri(const char* filepath, char* uri, size_t maxLen) {
    char absPath[PATH_MAX];
    if (filepath[0] != '/') {
        char cwd[PATH_MAX];
        getcwd(cwd, sizeof(cwd));
        snprintf(absPath, sizeof(absPath), "%s/%s", cwd, filepath);
    } else {
        strncpy(absPath, filepath, sizeof(absPath) - 1);
    }
    snprintf(uri, maxLen, "file://%s", absPath);
}
void LspSend(const char* message, int length) {
    if (!lsp.active) return;
    fd_set write_fds;
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    FD_ZERO(&write_fds);
    FD_SET(lsp.inPipe[1], &write_fds);
    if (select(lsp.inPipe[1] + 1, NULL, &write_fds, NULL, &tv) <= 0) {
        snprintf(statusMsg, sizeof(statusMsg), "LSP write timeout");
        return;
    }
    ssize_t written = 0;
    while (written < length) {
        ssize_t result = write(lsp.inPipe[1], message + written, length - written);
        if (result < 0) {
            if (errno == EINTR) continue;
            snprintf(statusMsg, sizeof(statusMsg), "LSP write error: %s", strerror(errno));
            return;
        }
        written += result;
    }
}
char* LspReceive(void) {
    if (!lsp.active) return NULL;
    if (lsp.bufferLen >= LSP_BUFFER_SIZE - 1) {
        snprintf(statusMsg, sizeof(statusMsg), "LSP buffer overflow");
        return NULL;
    }
    ssize_t bytesRead = read(lsp.outPipe[0], lsp.buffer + lsp.bufferLen, LSP_BUFFER_SIZE - 1 - lsp.bufferLen);
    if (bytesRead > 0) {
        lsp.bufferLen += bytesRead;
        lsp.buffer[lsp.bufferLen] = '\0';
    } else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        snprintf(statusMsg, sizeof(statusMsg), "LSP read error: %s", strerror(errno));
        return NULL;
    }
    char* headersEnd = strstr(lsp.buffer, "\r\n\r\n");
    if (!headersEnd) return NULL;
    int contentLength = 0;
    if (sscanf(lsp.buffer, "Content-Length: %d", &contentLength) != 1) {
        lsp.bufferLen = 0;
        return NULL;
    }
    headersEnd += 4;
    int headerLength = headersEnd - lsp.buffer;
    int messageLength = headerLength + contentLength;
    if (lsp.bufferLen < messageLength) return NULL;
    char* message = malloc(contentLength + 1);
    if (!message) {
        snprintf(statusMsg, sizeof(statusMsg), "LSP memory allocation failed");
        return NULL;
    }
    memcpy(message, headersEnd, contentLength);
    message[contentLength] = '\0';
    memmove(lsp.buffer, lsp.buffer + messageLength, lsp.bufferLen - messageLength);
    lsp.bufferLen -= messageLength;
    lsp.buffer[lsp.bufferLen] = '\0';
    return message;
}
bool LspInitialize(void) {
    char initRequest[1024];
    int reqLen = snprintf(initRequest, sizeof(initRequest), 
        "{\"jsonrpc\":\"2.0\",\"id\":%d,\"method\":\"initialize\","
        "\"params\":{"
        "\"processId\":%d,"
        "\"clientInfo\":{\"name\":\"TextEditor\",\"version\":\"0.1\"},"
        "\"rootUri\":null,"
        "\"capabilities\":{"
        "\"textDocument\":{"
        "\"synchronization\":{\"didSave\":true,\"dynamicRegistration\":true},"
        "\"publishDiagnostics\":{\"relatedInformation\":false},"
        "\"completion\":{\"dynamicRegistration\":true,\"contextSupport\":true},"
        "\"hover\":{\"dynamicRegistration\":true,\"contentFormat\":[\"plaintext\",\"markdown\"]},"
        "\"definition\":{\"dynamicRegistration\":true}"
        "}"
        "}"
        "}}",
        lsp.requestId++, getpid());
    if (reqLen < 0 || reqLen >= sizeof(initRequest)) {
        snprintf(statusMsg, sizeof(statusMsg), "LSP init request too large");
        return false;
    }
    char header[64];
    snprintf(header, sizeof(header), "Content-Length: %d\r\n\r\n", reqLen);
    LspSend(header, strlen(header));
    LspSend(initRequest, reqLen);
    char* response = NULL;
    time_t startTime = time(NULL);
    while (time(NULL) - startTime < 5) {
        response = LspReceive();
        if (response) break;
        usleep(10000);
    }
    if (!response) {
        snprintf(statusMsg, sizeof(statusMsg), "LSP initialize timeout");
        return false;
    }
    bool success = strstr(response, "\"result\"") != NULL;
    free(response);
    return success;
}
void CreateCompilationDatabase() {
    const char* dbPath = "compile_commands.json";
    if (access(dbPath, F_OK) == 0) {
        return;
    }
    FILE* fp = fopen(dbPath, "w");
    if (!fp) {
        fprintf(stderr, "Failed to create compilation database\n");
        return;
    }
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        fclose(fp);
        return;
    }
    fprintf(fp, "[\n");
    fprintf(fp, "  {\n");
    fprintf(fp, "    \"directory\": \"%s\",\n", cwd);
    fprintf(fp, "    \"command\": \"clang -Wall -Wextra -I./deps -I./projects -I./src\",\n");
    fprintf(fp, "    \"file\": \"%s\"\n", filename);
    fprintf(fp, "  }\n");
    fprintf(fp, "]\n");
    fclose(fp);
    fprintf(stderr, "Created compilation database at %s/compile_commands.json\n", cwd);
}
void SetupLspWorkspace() {
    if (!lsp.active || !lspInitialized) return;
    char workspaceRoot[PATH_MAX];
    getcwd(workspaceRoot, sizeof(workspaceRoot));
    char rootUri[PATH_MAX + 10];
    snprintf(rootUri, sizeof(rootUri), "file://%s", workspaceRoot);
    char workspaceFolders[2048];
    snprintf(workspaceFolders, sizeof(workspaceFolders),
             "{\"jsonrpc\":\"2.0\",\"method\":\"workspace/didChangeWorkspaceFolders\","
             "\"params\":{"
             "\"event\":{"
             "\"added\":[{\"uri\":\"%s\",\"name\":\"project\"}],"
             "\"removed\":[]"
             "}"
             "}}",
             rootUri);
    int len = strlen(workspaceFolders);
    char header[64];
    sprintf(header, "Content-Length: %d\r\n\r\n", len);
    LspSend(header, strlen(header));
    LspSend(workspaceFolders, len);
}
bool FindProjectConfig(char* configPath, size_t maxLen) {
    const char* configLocations[] = {
        ".clangd", 
        ".clangd.yaml", 
        ".clangd.yml",
        "clangd/.config", 
        "clangd/config.yaml", 
        "clangd/config.yml",
    };
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) return false;
    char searchPath[PATH_MAX];
    char currentDir[PATH_MAX];
    strncpy(currentDir, cwd, sizeof(currentDir));
    while (strlen(currentDir) > 1) {
        for (size_t i = 0; i < sizeof(configLocations)/sizeof(configLocations[0]); i++) {
            snprintf(searchPath, sizeof(searchPath), "%s/%s", currentDir, configLocations[i]);
            if (access(searchPath, F_OK) == 0) {
                strncpy(configPath, searchPath, maxLen - 1);
                fprintf(stderr, "Found clangd config at: %s\n", configPath);
                return true;
            }
        }
        const char* home = getenv("HOME");
        if (home) {
            snprintf(searchPath, sizeof(searchPath), "%s/.config/clangd/config.yaml", home);
            if (access(searchPath, F_OK) == 0) {
                strncpy(configPath, searchPath, maxLen - 1);
                fprintf(stderr, "Found global clangd config in home directory: %s\n", configPath);
                return true;
            }
        }
        char* lastSlash = strrchr(currentDir, '/');
        if (!lastSlash || lastSlash == currentDir) break;
        *lastSlash = '\0';
    }
    return false;
}
bool LspStart(const char* server, const char* langId) {
    if (lsp.active) LspStop();
    memset(&lsp, 0, sizeof(lsp));
    strncpy(lsp.langId, langId, sizeof(lsp.langId) - 1);
    if (pipe(lsp.inPipe) != 0 || pipe(lsp.outPipe) != 0) {
        snprintf(statusMsg, sizeof(statusMsg), "Failed to create pipes: %s", strerror(errno));
        return false;
    }
    lsp.pid = fork();
    if (lsp.pid < 0) {
        snprintf(statusMsg, sizeof(statusMsg), "Failed to fork: %s", strerror(errno));
        close(lsp.inPipe[0]); close(lsp.inPipe[1]);
        close(lsp.outPipe[0]); close(lsp.outPipe[1]);
        return false;
    }
    if (lsp.pid == 0) {
        close(lsp.inPipe[1]); close(lsp.outPipe[0]);
        dup2(lsp.inPipe[0], STDIN_FILENO); close(lsp.inPipe[0]);
        dup2(lsp.outPipe[1], STDOUT_FILENO); close(lsp.outPipe[1]);
        if (strcmp(langId, "c") == 0 || strcmp(langId, "cpp") == 0) {
            // Attempt to find an existing config first
            char configPath[PATH_MAX] = ".clangd";
            
            // If .clangd doesn't exist, create it
            if (access(configPath, F_OK) != 0) {
                FILE* config = fopen(configPath, "w");
                if (config) {
                    fprintf(config, 
                        "CompileFlags:\n"
                        "  Add: \n"
                        "    - -I.\n"
                        "    - -I./deps\n"
                        "    - -I./projects\n"
                        "    - -I./src\n"
                        "    - -I/usr/local/include\n"
                        "    - -I../src\n"
                        "    - -I../include\n"
                    );
                    fclose(config);
                }
            }

            execl(server, server, 
                  "--log=verbose", 
                  "--background-index", 
                  "--clang-tidy",
                  "--header-insertion=iwyu",
                  "--query-driver=/usr/bin/clang,/usr/bin/gcc",
                  "--compile-commands-dir=.",
                  NULL);
        } else {
            execl(server, server, NULL);
        }
        fprintf(stderr, "Failed to execute %s: %s\n", server, strerror(errno));
        exit(1);
    }
    close(lsp.inPipe[0]); close(lsp.outPipe[1]);
    int flags = fcntl(lsp.outPipe[0], F_GETFL, 0);
    fcntl(lsp.outPipe[0], F_SETFL, flags | O_NONBLOCK);
    lsp.active = true;
    lsp.requestId = 1;
    lsp.completions.active = false;
    lsp.completions.count = 0;
    lsp.hoverInfo.active = false;
    lsp.lastHoverTime = 0;
    lsp.isDefinitionAvailable = false;
    lsp.definitionRequestTime = 0;
    if (!LspInitialize()) {
        fprintf(stderr, "Failed to initialize LSP server\n");
        LspStop();
        return false;
    }
    SetupLspWorkspace();
    char msg[] = "{\"jsonrpc\":\"2.0\",\"method\":\"initialized\",\"params\":{}}";
    int len = strlen(msg);
    char header[64];
    sprintf(header, "Content-Length: %d\r\n\r\n", len);
    LspSend(header, strlen(header));
    LspSend(msg, len);
    lspInitialized = true;
    snprintf(statusMsg, sizeof(statusMsg), "LSP: Connected to %s", langId);
    return true;
}
void LspStop(void) {
    if (!lsp.active) return;
    char req[128];
    int reqLen = snprintf(req, sizeof(req), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"method\":\"shutdown\",\"params\":{}}", lsp.requestId++);
    if (reqLen > 0 && reqLen < sizeof(req)) {
        char header[64];
        snprintf(header, sizeof(header), "Content-Length: %d\r\n\r\n", reqLen);
        LspSend(header, strlen(header));
        LspSend(req, reqLen);
        char exit[] = "{\"jsonrpc\":\"2.0\",\"method\":\"exit\",\"params\":{}}";
        reqLen = strlen(exit);
        snprintf(header, sizeof(header), "Content-Length: %d\r\n\r\n", reqLen);
        LspSend(header, strlen(header));
        LspSend(exit, reqLen);
    }
    if (lsp.inPipe[1] >= 0) close(lsp.inPipe[1]);
    if (lsp.outPipe[0] >= 0) close(lsp.outPipe[0]);
    if (lsp.pid > 0) {
        int status;
        kill(lsp.pid, SIGTERM);
        waitpid(lsp.pid, &status, 0);
    }
    lsp.active = false;
    lspInitialized = false;
    lsp.diagnosticCount = 0;
    lsp.bufferLen = 0;
    lsp.completions.active = false;
    lsp.hoverInfo.active = false;
}
void LspNotifyDidOpen(const char* filepath, const char* text) {
    if (!lsp.active || !lspInitialized) return;
    LspCreateUri(filepath, lsp.uri, sizeof(lsp.uri));
    char* escapedText = LspEscapeJson(text);
    if (!escapedText) return;
    char* didOpenNotification = malloc(strlen(escapedText) + 512);
    if (!didOpenNotification) { free(escapedText); return; }
    sprintf(didOpenNotification, 
            "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didOpen\","
            "\"params\":{"
            "\"textDocument\":{"
            "\"uri\":\"%s\","
            "\"languageId\":\"%s\","
            "\"version\":1,"
            "\"text\":\"%s\""
            "}"
            "}}",
            lsp.uri, lsp.langId, escapedText);
    int len = strlen(didOpenNotification);
    char header[64];
    sprintf(header, "Content-Length: %d\r\n\r\n", len);
    LspSend(header, strlen(header));
    LspSend(didOpenNotification, len);
    free(escapedText);
    free(didOpenNotification);
}
void LspNotifyDidChange(const char* filepath, const char* text) {
    if (!lsp.active || !lspInitialized) return;
    LspCreateUri(filepath, lsp.uri, sizeof(lsp.uri));
    char* escapedText = LspEscapeJson(text);
    if (!escapedText) return;
    static int version = 2;
    char* didChangeNotification = malloc(strlen(escapedText) + 512);
    if (!didChangeNotification) { free(escapedText); return; }
    sprintf(didChangeNotification, 
            "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didChange\","
            "\"params\":{"
            "\"textDocument\":{"
            "\"uri\":\"%s\","
            "\"version\":%d"
            "},"
            "\"contentChanges\":[{"
            "\"text\":\"%s\""
            "}]"
            "}}",
            lsp.uri, version++, escapedText);
    int len = strlen(didChangeNotification);
    char header[64];
    sprintf(header, "Content-Length: %d\r\n\r\n", len);
    LspSend(header, strlen(header));
    LspSend(didChangeNotification, len);
    free(escapedText);
    free(didChangeNotification);
}
void LspNotifyDidSave(const char* filepath) {
    if (!lsp.active || !lspInitialized) return;
    LspCreateUri(filepath, lsp.uri, sizeof(lsp.uri));
    char didSaveNotification[512];
    sprintf(didSaveNotification, 
            "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/didSave\","
            "\"params\":{"
            "\"textDocument\":{"
            "\"uri\":\"%s\""
            "}"
            "}}",
            lsp.uri);
    int len = strlen(didSaveNotification);
    char header[64];
    sprintf(header, "Content-Length: %d\r\n\r\n", len);
    LspSend(header, strlen(header));
    LspSend(didSaveNotification, len);
}
void LspProcessCompletionResponse(const char* json) {
    char* resultValue = LspFindJsonValue(json, "result");
    if (!resultValue) return;
    lsp.completions.count = 0;
    lsp.completions.active = true;
    lsp.completions.selectedIndex = 0;
    char* itemsArray = NULL;
    if (resultValue[0] == '[') {
        itemsArray = resultValue;
    } else {
        itemsArray = LspFindJsonArray(resultValue, "items");
    }
    if (!itemsArray) return;
    int numItems = LspJsonArrayLength(itemsArray);
    for (int i = 0; i < numItems && i < LSP_MAX_COMPLETIONS; i++) {
        char* item = LspJsonArrayItem(itemsArray, i);
        if (!item) continue;
        CompletionItem* completion = &lsp.completions.items[lsp.completions.count];
        char* label = LspFindJsonValue(item, "label");
        if (!label) continue;
        LspJsonExtractString(label, completion->label, sizeof(completion->label));
        char* detail = LspFindJsonValue(item, "detail");
        if (detail) {
            LspJsonExtractString(detail, completion->detail, sizeof(completion->detail));
        } else {
            completion->detail[0] = '\0';
        }
        char* insertText = LspFindJsonValue(item, "insertText");
        if (insertText) {
            LspJsonExtractString(insertText, completion->insertText, sizeof(completion->insertText));
        } else {
            strncpy(completion->insertText, completion->label, sizeof(completion->insertText) - 1);
        }
        char* kind = LspFindJsonValue(item, "kind");
        if (kind) {
            completion->kind = LspJsonExtractInt(kind, 0);
        } else {
            completion->kind = 0;
        }
        completion->selected = (i == 0);
        lsp.completions.count++;
    }
}
void LspProcessDiagnostics(const char* json) {
    lsp.diagnosticCount = 0;
    char* paramsValue = LspFindJsonValue(json, "params");
    if (!paramsValue) return;
    char* diagnosticsArray = LspFindJsonArray(paramsValue, "diagnostics");
    if (!diagnosticsArray) return;
    int numDiagnostics = LspJsonArrayLength(diagnosticsArray);
    for (int i = 0; i < numDiagnostics && i < LSP_MAX_DIAGS; i++) {
        char* diagItem = LspJsonArrayItem(diagnosticsArray, i);
        if (!diagItem) continue;
        char* range = LspFindJsonValue(diagItem, "range");
        if (!range) continue;
        char* start = LspFindJsonValue(range, "start");
        if (!start) continue;
        char* line = LspFindJsonValue(start, "line");
        char* startChar = LspFindJsonValue(start, "character");
        if (!line || !startChar) continue;
        char* end = LspFindJsonValue(range, "end");
        if (!end) continue;
        char* endChar = LspFindJsonValue(end, "character");
        if (!endChar) continue;
        char* severity = LspFindJsonValue(diagItem, "severity");
        char* message = LspFindJsonValue(diagItem, "message");
        if (!severity || !message) continue;
        LspDiagnostic* diagnostic = &lsp.diagnostics[lsp.diagnosticCount++];
        diagnostic->line = LspJsonExtractInt(line, 0);
        diagnostic->startCol = LspJsonExtractInt(startChar, 0);
        diagnostic->endCol = LspJsonExtractInt(endChar, 0);
        diagnostic->severity = LspJsonExtractInt(severity, 0);
        LspJsonExtractString(message, diagnostic->message, LSP_MAX_MSG_LEN);
    }
}
void LspRequestCompletion(int line, int character) {
    if (!lsp.active || !lspInitialized) return;
    char request[1024];
    sprintf(request, 
            "{\"jsonrpc\":\"2.0\",\"id\":%d,\"method\":\"textDocument/completion\","
            "\"params\":{"
            "\"textDocument\":{\"uri\":\"%s\"},"
            "\"position\":{\"line\":%d,\"character\":%d},"
            "\"context\":{\"triggerKind\":1}"
            "}}",
            lsp.requestId++, lsp.uri, line, character);
    int len = strlen(request);
    char header[64];
    sprintf(header, "Content-Length: %d\r\n\r\n", len);
    LspSend(header, strlen(header));
    LspSend(request, len);
    lsp.completions.startLine = line;
    lsp.completions.startCol = character;
}
void LspRequestHover(int line, int character) {
    if (!lsp.active || !lspInitialized) return;
    char request[1024];
    sprintf(request, 
            "{\"jsonrpc\":\"2.0\",\"id\":%d,\"method\":\"textDocument/hover\","
            "\"params\":{"
            "\"textDocument\":{\"uri\":\"%s\"},"
            "\"position\":{\"line\":%d,\"character\":%d}"
            "}}",
            lsp.requestId++, lsp.uri, line, character);
    int len = strlen(request);
    char header[64];
    sprintf(header, "Content-Length: %d\r\n\r\n", len);
    LspSend(header, strlen(header));
    LspSend(request, len);
    lsp.hoverLine = line;
    lsp.hoverCol = character;
}
#define LSP_HOVER_MAX 16384
#define LSP_MAX_HOVER_LINES 500
typedef enum {
    COLOR_DEFAULT,
    COLOR_KEYWORD,
    COLOR_TYPE,
    COLOR_FUNCTION,
    COLOR_VARIABLE,
    COLOR_STRING,
    COLOR_COMMENT,
    COLOR_EMPHASIS,
    COLOR_HEADER,
    COLOR_CODE,
    COLOR_PARAMETER,
    COLOR_MARKDOWN_LINK
} ContentType;
Color GetContentTypeColor(ContentType type) {
    switch (type) {
        case COLOR_KEYWORD:       return Color01;
        case COLOR_TYPE:          return Color02;
        case COLOR_FUNCTION:      return Color16;
        case COLOR_VARIABLE:      return Color23;
        case COLOR_STRING:        return Color28;
        case COLOR_COMMENT:       return Color08;
        case COLOR_EMPHASIS:      return Color14;
        case COLOR_HEADER:        return Color22;
        case COLOR_CODE:          return Color10;
        case COLOR_PARAMETER:     return Color24;
        case COLOR_MARKDOWN_LINK: return Color12;
        default:                  return Color07;
    }
}
ContentType CategorizeContent(const char* text) {
    const char* keywords[] = {
        "class", "struct", "enum", "interface", "typedef", 
        "public", "private", "protected", "static", "const", 
        "virtual", "inline", "namespace", "template"
    };
    const char* types[] = {
        "int", "char", "float", "double", "void", "bool", 
        "size_t", "unsigned", "signed", "long", "short"
    };
    const char* functions[] = {
        "function", "method", "constructor", "destructor"
    };
    for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
        if (strstr(text, keywords[i])) return COLOR_KEYWORD;
    }
    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        if (strstr(text, types[i])) return COLOR_TYPE;
    }
    for (size_t i = 0; i < sizeof(functions)/sizeof(functions[0]); i++) {
        if (strstr(text, functions[i])) return COLOR_FUNCTION;
    }
    if (strstr(text, "@param") || strstr(text, "parameter")) return COLOR_PARAMETER;
    if (strstr(text, "//") || strstr(text, "/*")) return COLOR_COMMENT;
    if (strstr(text, "\"")) return COLOR_STRING;
    if (strstr(text, "**") || strstr(text, "__")) return COLOR_EMPHASIS;
    if (strstr(text, "#")) return COLOR_HEADER;
    if (strstr(text, "`")) return COLOR_CODE;
    if (strstr(text, "[[") || strstr(text, "]]")) return COLOR_MARKDOWN_LINK;
    return COLOR_DEFAULT;
}
void LspProcessHoverResponse(const char* json) {
    if (!json) {
        lsp.hoverInfo.active = false;
        return;
    }
    char* resultValue = LspFindJsonValue(json, "result");
    if (!resultValue || strcmp(resultValue, "null") == 0) {
        lsp.hoverInfo.active = false;
        return;
    }
    lsp.hoverInfo.active = true;
    lsp.hoverInfo.line = lsp.hoverLine;
    lsp.hoverInfo.col = lsp.hoverCol;
    lsp.hoverInfo.contents[0] = '\0';
    char* contents = NULL;
    if (resultValue[0] == '{') {
        contents = LspFindJsonValue(resultValue, "contents");
    } else {
        contents = resultValue;
    }
    if (!contents) {
        lsp.hoverInfo.active = false;
        return;
    }
    size_t totalLen = 0;
    char buffer[LSP_HOVER_MAX] = {0};
    bool isMarkdown = strstr(json, "\"kind\":\"markdown\"") != NULL;
    if (isMarkdown) {
        strncpy(lsp.hoverInfo.contents, "// Markdown Documentation\n\n", 
                sizeof(lsp.hoverInfo.contents) - 1);
        totalLen = strlen(lsp.hoverInfo.contents);
    }
    char extractedContent[LSP_HOVER_MAX] = {0};
    bool contentExtracted = false;
    if (contents[0] == '"') {
        contentExtracted = LspJsonExtractString(contents, extractedContent, sizeof(extractedContent));
    } else if (contents[0] == '{') {
        char* value = LspFindJsonValue(contents, "value");
        if (value) {
            contentExtracted = LspJsonExtractString(value, extractedContent, sizeof(extractedContent));
        }
    } else if (contents[0] == '[') {
        char* firstItem = LspJsonArrayItem(contents, 0);
        if (firstItem) {
            contentExtracted = LspJsonExtractString(firstItem, extractedContent, sizeof(extractedContent));
        }
    }
    if (!contentExtracted) {
        strncpy(extractedContent, "// No hover information available", sizeof(extractedContent) - 1);
    }
    strncat(lsp.hoverInfo.contents + totalLen, extractedContent, 
            sizeof(lsp.hoverInfo.contents) - totalLen - 1);
    char* output = lsp.hoverInfo.contents;
    char* input = lsp.hoverInfo.contents;
    while (*input) {
        if (input[0] == '\\' && input[1]) {
            switch (input[1]) {
                case 'n': *output++ = '\n'; break;
                case 'r': *output++ = '\r'; break;
                case 't': *output++ = '\t'; break;
                case '\\': *output++ = '\\'; break;
                case '"': *output++ = '"'; break;
                default: *output++ = input[1]; break;
            }
            input += 2;
        } else {
            *output++ = *input++;
        }
    }
    *output = '\0';
    for (char* p = lsp.hoverInfo.contents; *p; p++) {
        if ((*p < 32 && *p != '\n' && *p != '\t' && *p != '\r') || *p > 126) {
            *p = ' ';
        }
    }
    if (strlen(lsp.hoverInfo.contents) == 0) {
        strncpy(lsp.hoverInfo.contents, "// No hover information available", 
                sizeof(lsp.hoverInfo.contents) - 1);
    }
    if (lsp.hoverInfo.active) {
        lsp.hoverInfo.width = 0;
        lsp.hoverInfo.height = 0;
    }
}
void LspRequestDefinition(int line, int character) {
    if (!lsp.active || !lspInitialized) {
        snprintf(statusMsg, sizeof(statusMsg), "LSP not ready");
        return;
    }
    static double lastDefinitionRequestTime = 0;
    if (window.time - lastDefinitionRequestTime < 0.2) {
        return;
    }
    lastDefinitionRequestTime = window.time;
    char request[1024];
    int reqLen = snprintf(request, sizeof(request),
        "{\"jsonrpc\":\"2.0\",\"id\":%d,\"method\":\"textDocument/definition\","
        "\"params\":{"
        "\"textDocument\":{\"uri\":\"%s\"},"
        "\"position\":{\"line\":%d,\"character\":%d}"
        "}}",
        lsp.requestId++, lsp.uri, line, character);
    if (reqLen < 0 || reqLen >= sizeof(request)) {
        snprintf(statusMsg, sizeof(statusMsg), "Definition request too large");
        return;
    }
    char header[64];
    snprintf(header, sizeof(header), "Content-Length: %d\r\n\r\n", reqLen);
    LspSend(header, strlen(header));
    LspSend(request, reqLen);
    lsp.isDefinitionAvailable = false;
    lsp.definitionRequestTime = window.time;
    fprintf(stderr, "Requesting definition at line %d, character %d\n", line, character);
}
void UriDecode(char *dst, const char *src, size_t maxLen) {
    if (!dst || !src || maxLen == 0) return;
    size_t i = 0, j = 0;
    if (strncmp(src, "file://", 7) == 0) {
        src += 7;
    }
    if (src[0] == '/' && src[1] && src[2] == ':') {
        src++;
    }
    fprintf(stderr, "Decoding URI: %s\n", src);
    while (src[i] && j < maxLen - 1) {
        if (src[i] == '%' && src[i+1] && src[i+2] && 
            isxdigit((unsigned char)src[i+1]) && isxdigit((unsigned char)src[i+2])) {
            char hex[3] = {src[i+1], src[i+2], '\0'};
            int value;
            sscanf(hex, "%x", &value);
            dst[j++] = (char)value;
            i += 3;
        } else if (src[i] == '+') {
            dst[j++] = ' ';
            i++;
        } else {
            dst[j++] = src[i++];
        }
    }
    dst[j] = '\0';
    fprintf(stderr, "Decoded path: %s\n", dst);
}
bool NormalizeFilePath(const char* origPath, char* normPath, size_t maxLen) {
    if (!origPath || !normPath || maxLen == 0) return false;
    char tempPath[PATH_MAX] = {0};
    strncpy(tempPath, origPath, PATH_MAX - 1);
    if (access(tempPath, F_OK) == 0) {
        char realPath[PATH_MAX] = {0};
        if (realpath(tempPath, realPath)) {
            strncpy(normPath, realPath, maxLen - 1);
            return true;
        }
        strncpy(normPath, tempPath, maxLen - 1);
        return true;
    }
    char cwd[PATH_MAX] = {0};
    if (getcwd(cwd, sizeof(cwd))) {
        char projectRoot[PATH_MAX] = {0};
        strncpy(projectRoot, cwd, sizeof(projectRoot) - 1);
        while (strlen(projectRoot) > 1) {
            char checkPath[PATH_MAX] = {0};
            snprintf(checkPath, sizeof(checkPath), "%s/.git", projectRoot);
            if (access(checkPath, F_OK) == 0) break;
            snprintf(checkPath, sizeof(checkPath), "%s/Makefile", projectRoot);
            if (access(checkPath, F_OK) == 0) break;
            snprintf(checkPath, sizeof(checkPath), "%s/CMakeLists.txt", projectRoot);
            if (access(checkPath, F_OK) == 0) break;
            char* lastSlash = strrchr(projectRoot, '/');
            if (!lastSlash || lastSlash == projectRoot) {
                strncpy(projectRoot, "/", sizeof(projectRoot) - 1);
                break;
            }
            *lastSlash = '\0';
        }
        const char* relLocations[] = {
            ".",           
            "src",         
            "include",     
            "../src",      
            "../include",  
            "../../src",   
            "../../include"
        };
        char* fileName = basename((char*)origPath);
        for (size_t i = 0; i < sizeof(relLocations) / sizeof(relLocations[0]); i++) {
            char testPath[PATH_MAX] = {0};
            snprintf(testPath, sizeof(testPath), "%s/%s/%s", 
                     projectRoot, relLocations[i], fileName);
            fprintf(stderr, "Trying path: %s\n", testPath);
            if (access(testPath, F_OK) == 0) {
                char realPath[PATH_MAX] = {0};
                if (realpath(testPath, realPath)) {
                    strncpy(normPath, realPath, maxLen - 1);
                    return true;
                }
                strncpy(normPath, testPath, maxLen - 1);
                return true;
            }
        }
    }
    strncpy(normPath, origPath, maxLen - 1);
    return false;
}
char* FindRootDirectory(void) {
    static char rootDir[PATH_MAX] = {0};
    if (rootDir[0] != '\0') return rootDir;
    char cwd[PATH_MAX] = {0};
    if (!getcwd(cwd, sizeof(cwd))) return NULL;
    strncpy(rootDir, cwd, sizeof(rootDir) - 1);
    while (strlen(rootDir) > 1) {
        char checkPath[PATH_MAX] = {0};
        const char* markers[] = {
            "/.git", "/Makefile", "/CMakeLists.txt", 
            "/compile_commands.json", "/src", "/include"
        };
        for (size_t i = 0; i < sizeof(markers)/sizeof(markers[0]); i++) {
            snprintf(checkPath, sizeof(checkPath), "%s%s", rootDir, markers[i]);
            if (access(checkPath, F_OK) == 0) {
                fprintf(stderr, "Found project marker %s at %s\n", markers[i], rootDir);
                return rootDir;
            }
        }
        char* lastSlash = strrchr(rootDir, '/');
        if (!lastSlash || lastSlash == rootDir) {
            return rootDir;
        }
        *lastSlash = '\0';
    }
    strncpy(rootDir, cwd, sizeof(rootDir) - 1);
    return rootDir;
}
bool FindFile(const char* filename, char* foundPath, size_t maxLen) {
    if (filename[0] == '/' && access(filename, F_OK) == 0) {
        strncpy(foundPath, filename, maxLen - 1);
        fprintf(stderr, "Found file at absolute path: %s\n", foundPath);
        return true;
    }
    char cwdPath[PATH_MAX] = {0};
    char cwd[PATH_MAX] = {0};
    if (getcwd(cwd, sizeof(cwd))) {
        snprintf(cwdPath, sizeof(cwdPath), "%s/%s", cwd, filename);
        if (access(cwdPath, F_OK) == 0) {
            strncpy(foundPath, cwdPath, maxLen - 1);
            fprintf(stderr, "Found file in current directory: %s\n", foundPath);
            return true;
        }
    }
    char* rootDir = FindRootDirectory();
    if (rootDir) {
        const char* patterns[] = {
            "%s/%s", "%s/src/%s", "%s/include/%s", "%s/deps/%s", "%s/libs/%s", "%s/extern/%s", "%s/external/%s", "%s/third_party/%s", "%s/third-party/%s", "%s/vendor/%s", "%s/../%s", "%s/../src/%s", "%s/../include/%s",
        };
        for (size_t i = 0; i < sizeof(patterns)/sizeof(patterns[0]); i++) {
            char testPath[PATH_MAX] = {0};
            snprintf(testPath, sizeof(testPath), patterns[i], rootDir, filename);
            fprintf(stderr, "Trying path: %s\n", testPath);
            if (access(testPath, F_OK) == 0) {
                strncpy(foundPath, testPath, maxLen - 1);
                fprintf(stderr, "Found file using pattern %s: %s\n", patterns[i], foundPath);
                return true;
            }
        }
    }
    const char* systemPaths[] = {
        "/usr/include", "/usr/local/include", "/usr/include/x86_64-linux-gnu"
    };
    for (size_t i = 0; i < sizeof(systemPaths)/sizeof(systemPaths[0]); i++) {
        char testPath[PATH_MAX] = {0};
        snprintf(testPath, sizeof(testPath), "%s/%s", systemPaths[i], filename);
        if (access(testPath, F_OK) == 0) {
            strncpy(foundPath, testPath, maxLen - 1);
            fprintf(stderr, "Found file in system path: %s\n", foundPath);
            return true;
        }
    }
    return false;
}
bool OpenFileFromDefinition(const char* filePath, int line, int column) {
    char decodedPath[PATH_MAX] = {0};
    UriDecode(decodedPath, filePath, sizeof(decodedPath));
    fprintf(stderr, "Attempting to open file: %s\n", decodedPath);
    char resolvedPath[PATH_MAX] = {0};
    bool foundFile = false;
    if (access(decodedPath, F_OK) == 0) {
        strncpy(resolvedPath, decodedPath, sizeof(resolvedPath) - 1);
        foundFile = true;
        fprintf(stderr, "Found file with direct decoded path: %s\n", resolvedPath);
    }
    else {
        char baseNameBuf[PATH_MAX] = {0};
        strncpy(baseNameBuf, decodedPath, sizeof(baseNameBuf) - 1);
        char* baseName = basename(baseNameBuf);
        if (FindFile(baseName, resolvedPath, sizeof(resolvedPath))) {
            foundFile = true;
        }
        else {
            char* cleanName = strrchr(baseName, '/');
            if (cleanName) {
                cleanName++;
            } else {
                cleanName = baseName;
            }
            if (FindFile(cleanName, resolvedPath, sizeof(resolvedPath))) {
                foundFile = true;
            }
        }
    }
    if (!foundFile || resolvedPath[0] == '\0') {
        char baseNameBuf[PATH_MAX] = {0};
        strncpy(baseNameBuf, decodedPath, sizeof(baseNameBuf) - 1);
        char* baseName = basename(baseNameBuf);
        fprintf(stderr, "Trying with basename: %s\n", baseName);
        if (FindFile(baseName, resolvedPath, sizeof(resolvedPath))) {
            foundFile = true;
        }
    }
    if (!foundFile || resolvedPath[0] == '\0') {
        fprintf(stderr, "Could not find definition file: %s\n", decodedPath);
        snprintf(statusMsg, sizeof(statusMsg), "Definition file not found");
        return false;
    }
    fprintf(stderr, "Loading file: %s\n", resolvedPath);
    if (!LoadFile(resolvedPath)) {
        fprintf(stderr, "Failed to load file: %s\n", resolvedPath);
        snprintf(statusMsg, sizeof(statusMsg), "Failed to load definition file");
        return false;
    }
    strncpy(filename, resolvedPath, sizeof(filename) - 1);
    cursorLine = fmax(0, fmin(line, numLines - 1));
    cursorCol = fmax(0, fmin(column, lines[cursorLine].length));
    selStartLine = cursorLine;
    selStartCol = cursorCol;
    selEndLine = cursorLine;
    selEndCol = cursorCol;
    isSelecting = true;
    isDefinitionJumped = true;
    int visibleHeight = window.screen_height - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0);
    scroll.targetY = (cursorLine * lineHeight) - (visibleHeight / 2) + (lineHeight / 2);
    scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY));
    if (!wordWrap) {
        float cursorX = GetCursorXPosition();
        int visibleWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
        scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), cursorX - (visibleWidth / 2)));
    } else {
        scroll.targetX = 0;
    }
    if (wordWrap) {
        RecalculateWrappedLines();
    }
    Color defaultTextColor = {textColor.r, textColor.g, textColor.b, textColor.a};
    char* initialText = ConstructTextFromLines();
    int textLen = initialText ? strlen(initialText) : 0;
    SyntaxInitWithLanguageDetection(
        defaultTextColor, 
        SyntaxDrawWrapper, 
        &font, 
        fontSize, 
        NULL, 
        filename, 
        initialText, 
        textLen, 
        statusMsg, 
        sizeof(statusMsg)
    );
    if (initialText) free(initialText);
    LspInit();
    char baseNameBuf[PATH_MAX] = {0};
    strncpy(baseNameBuf, resolvedPath, sizeof(baseNameBuf) - 1);
    char* baseName = basename(baseNameBuf);
    snprintf(statusMsg, sizeof(statusMsg), "Navigated to %s:%d", baseName, line + 1);
    return true;
}
void LspDrawDiagnostics() {
    if (!lsp.active || lsp.diagnosticCount == 0) return;
    int startLine = (int)(scroll.currentY / lineHeight);
    int endLine = startLine + (window.screen_height / lineHeight) + 1;
    int textX = showLineNumbers ? gutterWidth : 0;
    for (int i = 0; i < lsp.diagnosticCount; i++) {
        LspDiagnostic* diag = &lsp.diagnostics[i];
        if (diag->line < startLine || diag->line >= endLine || diag->line >= numLines) 
            continue;
        const char* lineText = lines[diag->line].text;
        int lineLen = lines[diag->line].length;
        int diagLineY = (diag->line * lineHeight) - (int)scroll.currentY;
        int startCol = fmax(0, fmin(diag->startCol, lineLen));
        int endCol = fmax(startCol, fmin(diag->endCol, lineLen));
        int startX = textX;
        for (int c = 0; c < startCol; c++) {
            char charStr[2] = {lineText[c], '\0'};
            startX += GetTextSizeCached(font, fontSize, charStr).width;
        }
        int endX = startX;
        for (int c = startCol; c < endCol; c++) {
            char charStr[2] = {lineText[c], '\0'};
            endX += GetTextSizeCached(font, fontSize, charStr).width;
        }
        if (startCol == endCol) {
            endX = startX + GetTextSizeCached(font, fontSize, "m").width;
        }
        startX -= (int)scroll.currentX;
        endX -= (int)scroll.currentX;
        Color underlineColor;
        switch (diag->severity) {
            case 1: underlineColor = Color19; break; 
            case 2: underlineColor = Color18; break; 
            case 3: underlineColor = Color20; break; 
            case 4: underlineColor = Color03; break; 
            default: underlineColor = Color08; break;
        }
        float lineThickness = fmax(1.0f, (fontSize / 20.0f));
        float yPos = diagLineY + lineHeight + lineThickness - 1;
        float dashLength = fmax(3.0f, fontSize / 6.0f);
        float gapLength = fmax(2.0f, fontSize / 10.0f);
        float currentX = startX;
        while (currentX < endX) {
            float segmentEnd = fmin(currentX + dashLength, endX);
            DrawLine(currentX, yPos, segmentEnd, yPos, lineThickness, underlineColor);
            currentX = segmentEnd + gapLength;
        }
    }
    FlushRectBatch();
    FlushTextBatch();
    for (int i = 0; i < lsp.diagnosticCount; i++) {
        LspDiagnostic* diag = &lsp.diagnostics[i];
        if (diag->line < startLine || diag->line >= endLine || diag->line >= numLines) continue;
        const char* lineText = lines[diag->line].text;
        int lineLen = lines[diag->line].length;
        int diagLineY = (diag->line * lineHeight) - (int)scroll.currentY;
        int startCol = fmax(0, fmin(diag->startCol, lineLen));
        int endCol = fmax(startCol, fmin(diag->endCol, lineLen));
        int startX = textX;
        for (int c = 0; c < startCol; c++) {
            char charStr[2] = {lineText[c], '\0'};
            startX += GetTextSizeCached(font, fontSize, charStr).width;
        }
        int endX = startX;
        if (startCol == endCol) {
            endX = startX + GetTextSizeCached(font, fontSize, "m").width;
        } else {
            for (int c = startCol; c < endCol; c++) {
                char charStr[2] = {lineText[c], '\0'};
                endX += GetTextSizeCached(font, fontSize, charStr).width;
            }
        }
        startX -= (int)scroll.currentX;
        endX -= (int)scroll.currentX;
        bool isHovered = (
            (mouse.x >= startX && mouse.x <= endX && 
             mouse.y >= diagLineY && mouse.y <= diagLineY + lineHeight) ||
            (mouse.x <= textX && mouse.x >= textX - 20 && 
             mouse.y >= diagLineY && mouse.y <= diagLineY + lineHeight)
        );
        if (isHovered) {
            Color diagColor;
            const char* sevLabel;
            switch (diag->severity) {
                case 1: diagColor = Color19; sevLabel = "Error"; break;
                case 2: diagColor = Color18; sevLabel = "Warning"; break;
                case 3: diagColor = Color20; sevLabel = "Info"; break;
                default: diagColor = Color08; sevLabel = "Note"; break;
            }
            float tooltipFontSize = fontSize * 0.78f;
            int padding = (int)(fontSize * 0.3f);
            char headerText[256];
            snprintf(headerText, sizeof(headerText), "%s at Line %d", 
                     sevLabel, diag->line + 1);
            const char* msg = diag->message ? diag->message : "No description available";
            int msgLen = strlen(msg);
            TextSize headerSize = GetTextSizeCached(font, tooltipFontSize, headerText);
            int maxLineWidth = headerSize.width;
            int tempPos = 0;
            int charWidth = (int)(tooltipFontSize * 0.5f);
            int maxCharsPerLine = (int)(fontSize * 25.0f) / charWidth;
            while (tempPos < msgLen) {
                int lineEnd = tempPos;
                int lineLen = 0;
                while (lineEnd < msgLen && lineLen < maxCharsPerLine && msg[lineEnd] != '\n') {
                    lineEnd++;
                    lineLen++;
                }
                char tempBuf[512] = {0};
                strncpy(tempBuf, msg + tempPos, lineLen);
                tempBuf[lineLen] = '\0';
                TextSize lineSize = GetTextSizeCached(font, tooltipFontSize, tempBuf);
                if (lineSize.width > maxLineWidth) {
                    maxLineWidth = lineSize.width;
                }
                tempPos = lineEnd;
                if (tempPos < msgLen && msg[tempPos] == '\n') {
                    tempPos++;
                }
            }
            int tooltipWidth = maxLineWidth + padding * 2 + (int)(fontSize * 2.0f);
            tooltipWidth = fmax(tooltipWidth, (int)(fontSize * 18.0f));
            tooltipWidth = fmin(tooltipWidth, (int)(fontSize * 35.0f));
            tooltipWidth = fmin(tooltipWidth, window.screen_width - 20);
            int contentWidth = tooltipWidth - padding * 2;
            int charsPerLine = contentWidth / charWidth;
            int lineCount = 0;
            tempPos = 0;
            while (tempPos < msgLen) {
                int lineLen = fmin(msgLen - tempPos, charsPerLine);
                for (int i = 0; i < lineLen; i++) {
                    if (msg[tempPos + i] == '\n') {
                        lineLen = i + 1;
                        break;
                    }
                }
                if (tempPos + lineLen < msgLen && lineLen == charsPerLine) {
                    int j = lineLen;
                    while (j > 0 && msg[tempPos + j] != ' ' && msg[tempPos + j] != '\n') {
                        j--;
                    }
                    if (j > 0) lineLen = j + 1;
                }
                lineCount++;
                tempPos += lineLen;
                while (tempPos < msgLen && (msg[tempPos] == ' ' || msg[tempPos] == '\n')) {
                    tempPos++;
                }
            }
            int headerHeight = (int)(tooltipFontSize * 1.3f);
            int lineHeight = (int)(tooltipFontSize * 1.05f);
            int tooltipHeight = headerHeight + (lineHeight * lineCount) + padding * 2;
            int maxHeightAllowed = window.screen_height - 20;
            if (tooltipHeight > maxHeightAllowed) {
                tooltipHeight = maxHeightAllowed;
                lineCount = (tooltipHeight - headerHeight - padding * 2) / lineHeight;
            }
            int tooltipX = mouse.x + (int)(fontSize * 0.3f);
            int tooltipY;
            if (mouse.y < window.screen_height / 2) {
                tooltipY = diagLineY + lineHeight + 2;
            } else {
                tooltipY = diagLineY - tooltipHeight - 2;
            }
            if (tooltipX + tooltipWidth > window.screen_width)
                tooltipX = window.screen_width - tooltipWidth - 5;
            if (tooltipX < 5)
                tooltipX = 5;
            if (tooltipY + tooltipHeight > window.screen_height)
                tooltipY = window.screen_height - tooltipHeight - 5;
            if (tooltipY < 5)
                tooltipY = 5;
            DrawRectBatch(tooltipX, tooltipY, tooltipWidth, tooltipHeight, Color30);
            DrawRectBorder(tooltipX, tooltipY, tooltipWidth, tooltipHeight, 1, diagColor);
            DrawRectBatch(tooltipX, tooltipY, tooltipWidth, headerHeight, diagColor);
            DrawTextBatch(
                tooltipX + padding,
                tooltipY + (headerHeight - tooltipFontSize) / 2,
                font, 
                tooltipFontSize, 
                headerText, 
                Color15
            );
            int contentX = tooltipX + padding;
            int contentY = tooltipY + headerHeight + padding/2;
            int linesDrawn = 0;
            int pos = 0;
            while (pos < msgLen && linesDrawn < lineCount) {
                while (pos < msgLen && (msg[pos] == ' ' || msg[pos] == '\n')) {
                    pos++;
                }
                if (pos >= msgLen) break;
                int lineLen = fmin(msgLen - pos, charsPerLine);
                for (int i = 0; i < lineLen; i++) {
                    if (msg[pos + i] == '\n') {
                        lineLen = i;
                        break;
                    }
                }
                if (pos + lineLen < msgLen && lineLen == charsPerLine) {
                    int j = lineLen;
                    while (j > 0 && msg[pos + j] != ' ') {
                        j--;
                    }
                    if (j > 0) {
                        lineLen = j;
                    }
                }
                if (lineLen > 0) {
                    char lineBuf[512] = {0};
                    strncpy(lineBuf, msg + pos, lineLen);
                    lineBuf[lineLen] = '\0';
                    int end = lineLen - 1;
                    while (end >= 0 && (lineBuf[end] == ' ' || lineBuf[end] == '\t')) {
                        lineBuf[end--] = '\0';
                    }
                    DrawTextBatch(
                        contentX,
                        contentY + (linesDrawn * lineHeight),
                        font,
                        tooltipFontSize,
                        lineBuf,
                        Color15
                    );
                    linesDrawn++;
                }
                pos += lineLen;
                if (pos < msgLen && msg[pos] == '\n') {
                    pos++;
                }
                while (pos < msgLen && msg[pos] == ' ') {
                    pos++;
                }
                if (lineLen <= 0) break;
            }
        }
    }
    FlushRectBatch();
    FlushTextBatch();
}
void LspDrawCompletions() {
    if (!lsp.completions.active || lsp.completions.count == 0) return;
    static double completionStartTime = 0;
    static bool isFirstShow = true;
    if (isFirstShow) {
        completionStartTime = window.time;
        isFirstShow = false;
        return;
    }
    if (window.time - completionStartTime < 0.1) return;
    float completionFontSize = fontSize * 0.95f;
    float detailFontSize = fontSize * 0.85f;
    int itemHeight = (int)(fontSize * 1.3f);
    int padding = (int)(fontSize * 0.5f);
    int iconWidth = (int)(fontSize * 1.5f);
    int maxWidth = (int)(window.screen_width * 0.4f);
    int cursorX, cursorY;
    if (wordWrap) {
        int wl, wc;
        OriginalToWrapped(lsp.completions.startLine, lsp.completions.startCol, &wl, &wc);
        cursorY = wl * lineHeight - scroll.currentY;
    } else {
        cursorY = lsp.completions.startLine * lineHeight - scroll.currentY;
    }
    int textX = showLineNumbers ? gutterWidth : 0;
    cursorX = textX;
    if (lsp.completions.startLine < numLines) {
        for (int i = 0; i < lsp.completions.startCol && i < lines[lsp.completions.startLine].length; i++) {
            char charStr[2] = {lines[lsp.completions.startLine].text[i], '\0'};
            TextSize size = GetTextSizeCached(font, fontSize, charStr);
            cursorX += size.width;
        }
    }
    cursorX -= scroll.currentX;
    int popupWidth = 0;
    for (int i = 0; i < lsp.completions.count; i++) {
        TextSize labelSize = GetTextSizeCached(font, completionFontSize, lsp.completions.items[i].label);
        int itemWidth = labelSize.width + iconWidth + padding * 3;
        if (lsp.completions.items[i].detail[0] != '\0') {
            TextSize detailSize = GetTextSizeCached(font, detailFontSize, lsp.completions.items[i].detail);
            itemWidth += detailSize.width + padding;
        }
        if (itemWidth > popupWidth) popupWidth = itemWidth;
    }
    popupWidth = fmax(200, fmin(maxWidth, popupWidth));
    int maxVisibleItems = 10;
    int visibleItems = fmin(maxVisibleItems, lsp.completions.count);
    int popupHeight = visibleItems * itemHeight + padding * 2;
    int hintHeight = (int)(fontSize * 1.2f);
    popupHeight += hintHeight;
    int popupX = cursorX;
    int popupY = cursorY + lineHeight + 2;
    if (popupX + popupWidth > window.screen_width) 
        popupX = window.screen_width - popupWidth - 5;
    if (popupX < 0) 
        popupX = 0;
    if (popupY + popupHeight > window.screen_height) 
        popupY = cursorY - popupHeight - 2;
    if (popupY < 0) 
        popupY = 0;
    DrawRectBatch(popupX, popupY, popupWidth, popupHeight, Color30);
    DrawRectBorder(popupX, popupY, popupWidth, popupHeight, 1, Color13);
    DrawRectBatch(popupX, popupY, popupWidth, (int)(fontSize * 0.8f) + padding/2, Color04);
    DrawTextBatch(
        popupX + padding, 
        popupY + padding/4, 
        font, 
        fontSize * 0.7f, 
        "Completions", 
        Color15
    );
    int titleBarHeight = (int)(fontSize * 0.8f) + padding/2;
    int visibleStart = 0;
    if (lsp.completions.count > visibleItems) {
        if (lsp.completions.selectedIndex >= visibleItems) 
            visibleStart = lsp.completions.selectedIndex - visibleItems/2;
        if (lsp.completions.selectedIndex < visibleStart)
            visibleStart = lsp.completions.selectedIndex;
        visibleStart = fmax(0, fmin(lsp.completions.count - visibleItems, visibleStart));
    }
    for (int i = 0; i < visibleItems && i + visibleStart < lsp.completions.count; i++) {
        int itemIndex = i + visibleStart;
        CompletionItem* item = &lsp.completions.items[itemIndex];
        int itemY = popupY + titleBarHeight + i * itemHeight;
        if (itemIndex == lsp.completions.selectedIndex) {
            DrawRectBatch(popupX + 1, itemY, popupWidth - 2, itemHeight, Color00);
            DrawRectBorder(popupX + 1, itemY, popupWidth - 2, itemHeight, 1, Color13);
        }
        Color iconColor;
        char iconChar = ' ';
        switch (item->kind) {
            case 1:  iconChar = 'T'; iconColor = Color23; break;
            case 2:  iconChar = 'M'; iconColor = Color03; break;
            case 3:  iconChar = 'F'; iconColor = Color03; break;
            case 4:  iconChar = 'C'; iconColor = Color02; break;
            case 5:  iconChar = 'F'; iconColor = Color16; break;
            case 6:  iconChar = 'V'; iconColor = Color16; break;
            case 7:  iconChar = 'C'; iconColor = Color01; break;
            case 8:  iconChar = 'I'; iconColor = Color01; break;
            case 9:  iconChar = 'M'; iconColor = Color22; break;
            case 10: iconChar = 'P'; iconColor = Color16; break;
            case 11: iconChar = 'U'; iconColor = Color18; break;
            case 12: iconChar = 'V'; iconColor = Color16; break;
            case 13: iconChar = 'E'; iconColor = Color01; break;
            case 14: iconChar = 'K'; iconColor = Color19; break;
            case 15: iconChar = 'S'; iconColor = Color28; break;
            case 16: iconChar = 'C'; iconColor = Color08; break;
            case 17: iconChar = 'F'; iconColor = Color08; break;
            case 18: iconChar = 'R'; iconColor = Color08; break;
            case 19: iconChar = 'F'; iconColor = Color08; break;
            case 20: iconChar = 'E'; iconColor = Color01; break;
            case 21: iconChar = 'C'; iconColor = Color08; break;
            case 22: iconChar = 'S'; iconColor = Color01; break;
            case 23: iconChar = 'E'; iconColor = Color08; break;
            case 24: iconChar = 'O'; iconColor = Color08; break;
            case 25: iconChar = 'T'; iconColor = Color06; break;
            default: iconChar = '?'; iconColor = Color07; break;
        }
        char iconStr[2] = {iconChar, '\0'};
        int iconX = popupX + padding;
        int iconY = itemY + (itemHeight - fontSize) / 2;
        if (itemIndex == lsp.completions.selectedIndex) {
            DrawRectBatch(
                iconX - padding/2, 
                iconY - padding/4, 
                fontSize + padding, 
                fontSize + padding/2, 
                Color05
            );
        }
        DrawTextBatch(iconX, iconY, font, fontSize, iconStr, iconColor);
        Color labelColor = (itemIndex == lsp.completions.selectedIndex) ? Color15 : Color07;
        DrawTextBatch(
            iconX + iconWidth, 
            iconY, 
            font, 
            completionFontSize, 
            item->label, 
            labelColor
        );
        if (item->detail[0] != '\0') {
            TextSize labelSize = GetTextSizeCached(font, completionFontSize, item->label);
            Color detailColor = (itemIndex == lsp.completions.selectedIndex) ? Color02 : Color08;
            char detailStr[256];
            strncpy(detailStr, item->detail, sizeof(detailStr) - 1);
            detailStr[sizeof(detailStr) - 1] = '\0';
            int maxDetailWidth = popupWidth - iconWidth - labelSize.width - padding * 4;
            TextSize detailSize = GetTextSizeCached(font, detailFontSize, detailStr);
            if (detailSize.width > maxDetailWidth) {
                int len = strlen(detailStr);
                while (len > 3 && detailSize.width > maxDetailWidth) {
                    detailStr[len-3] = '.';
                    detailStr[len-2] = '.';
                    detailStr[len-1] = '.';
                    detailStr[len] = '\0';
                    len--;
                    detailSize = GetTextSizeCached(font, detailFontSize, detailStr);
                }
            }
            DrawTextBatch(
                iconX + iconWidth + labelSize.width + padding, 
                iconY + (fontSize - detailFontSize) / 2, 
                font, 
                detailFontSize, 
                detailStr, 
                detailColor
            );
        }
    }
    if (lsp.completions.count > visibleItems) {
        int scrollbarWidth = 4;
        int scrollbarHeight = (visibleItems * (popupHeight - titleBarHeight - hintHeight)) / lsp.completions.count;
        scrollbarHeight = fmax(20, scrollbarHeight);
        float scrollRatio = (float)visibleStart / (lsp.completions.count - visibleItems);
        int scrollbarY = popupY + titleBarHeight + scrollRatio * (popupHeight - titleBarHeight - hintHeight - scrollbarHeight);
        DrawRectBatch(
            popupX + popupWidth - scrollbarWidth - 2, 
            popupY + titleBarHeight, 
            scrollbarWidth, 
            popupHeight - titleBarHeight - hintHeight, 
            Color29
        );
        DrawRectBatch(
            popupX + popupWidth - scrollbarWidth - 2, 
            scrollbarY, 
            scrollbarWidth, 
            scrollbarHeight, 
            Color13
        );
    }
    int hintY = popupY + popupHeight - hintHeight;
    DrawRectBatch(popupX, hintY, popupWidth, hintHeight, Color30);
    DrawRectBorder(popupX, hintY, popupWidth, 1, 1, Color05);
    char hintText[] = "↑/↓: Navigate  Tab: Select  Esc: Cancel";
    TextSize hintSize = GetTextSizeCached(font, fontSize * 0.75, hintText);
    int hintX = popupX + (popupWidth - hintSize.width) / 2;
    DrawTextBatch(hintX, hintY + (hintHeight - hintSize.height) / 2, font, fontSize * 0.75, hintText, Color02);
    if (!lsp.completions.active) isFirstShow = true;
}
void LspDrawHoverInfo() {
    if (!lsp.hoverInfo.active || !lsp.hoverInfo.contents[0]) return;
    float baseHoverFontSize = fontSize * 0.9f;
    int horizontalPadding = (int)(fontSize * 0.6f);
    int verticalPadding = (int)(fontSize * 0.3f);
    float lineSpacingFactor = 1.1f;
    int cursorX, cursorY;
    if (wordWrap) {
        int wl, wc;
        OriginalToWrapped(lsp.hoverInfo.line, lsp.hoverInfo.col, &wl, &wc);
        cursorY = wl * lineHeight - scroll.currentY;
    } else {
        cursorY = lsp.hoverInfo.line * lineHeight - scroll.currentY;
    }
    int textX = showLineNumbers ? gutterWidth : 0;
    cursorX = textX;
    if (lsp.hoverInfo.line < numLines) {
        for (int i = 0; i < lsp.hoverInfo.col && i < lines[lsp.hoverInfo.line].length; i++) {
            char charStr[2] = {lines[lsp.hoverInfo.line].text[i], '\0'};
            TextSize size = GetTextSizeCached(font, fontSize, charStr);
            cursorX += size.width;
        }
    }
    cursorX -= scroll.currentX;
    typedef struct {
        const char* pattern;
        Color color;
        bool isPrefix;
    } SyntaxPattern;
    SyntaxPattern syntaxPatterns[] = {
        {"class", Color01, false}, {"struct", Color01, false}, {"enum", Color01, false},
        {"typedef", Color01, false}, {"static", Color09, false}, {"const", Color09, false},
        {"int", Color06, false}, {"char", Color06, false}, {"bool", Color06, false},
        {"void", Color06, false}, {"size_t", Color06, false}, {"//", Color08, true},
        {"/*", Color08, true}, {"#", Color08, true}, {"\"", Color28, false},
        {"'", Color28, false}, {"@param", Color03, true}, {"@return", Color03, true},
        {"Parameters:", Color22, true}, {"Returns:", Color22, true}, {"**", Color14, false},
        {"`", Color10, false},
    };
    char* tempCopy = strdup(lsp.hoverInfo.contents);
    if (!tempCopy) return;
    char* tmpContext = NULL;
    char* tmpLine = strtok_r(tempCopy, "\n", &tmpContext);
    int maxRawLineWidth = 0;
    while (tmpLine) {
        TextSize lineSize = GetTextSizeCached(font, baseHoverFontSize, tmpLine);
        maxRawLineWidth = fmax(maxRawLineWidth, lineSize.width);
        tmpLine = strtok_r(NULL, "\n", &tmpContext);
    }
    free(tempCopy);
    bool isDefinition = (strstr(lsp.hoverInfo.contents, "DEFINITION DETAILS") != NULL);
    int tooltipWidth = maxRawLineWidth + (horizontalPadding * 2);
    tooltipWidth += (int)(fontSize * 2.0f);
    if (isDefinition) {
        tooltipWidth = fmax(tooltipWidth, (int)(fontSize * 35.0f));
    } else {
        tooltipWidth = fmax(tooltipWidth, (int)(fontSize * 25.0f));
    }
    tooltipWidth = fmin(tooltipWidth, window.screen_width - 10);
    int usableWidth = tooltipWidth - (horizontalPadding * 2);
    char* lineCopy = strdup(lsp.hoverInfo.contents);
    if (!lineCopy) return;
    char* context = NULL;
    char* line = strtok_r(lineCopy, "\n", &context);
    char** wrappedLines = malloc(1000 * sizeof(char*));
    if (!wrappedLines) {
        free(lineCopy);
        return;
    }
    int numWrappedLines = 0;
    int totalHeight = 0;
    while (line) {
        bool isSpecialLine = (strstr(line, "```") == line || 
                             strstr(line, "@param") == line || 
                             strstr(line, "//") == line);
        if (isSpecialLine || strlen(line) == 0) {
            wrappedLines[numWrappedLines] = strdup(line);
            TextSize lineSize = GetTextSizeCached(font, baseHoverFontSize, line);
            totalHeight += (int)(lineSize.height * lineSpacingFactor);
            numWrappedLines++;
        } else {
            int len = strlen(line);
            int startPos = 0;
            while (startPos < len) {
                int charsInLine = 0;
                int lineWidth = 0;
                int lastSpace = -1;
                for (int i = startPos; i <= len; i++) {
                    if (i == len || line[i] == ' ') {
                        int wordEnd = i;
                        int wordStart = startPos + charsInLine;
                        char wordBuf[1024] = {0};
                        int wordLen = wordEnd - wordStart;
                        if (wordLen > 0 && wordLen < 1024) {
                            strncpy(wordBuf, line + wordStart, wordLen);
                            wordBuf[wordLen] = '\0';
                            TextSize wordSize = GetTextSizeCached(font, baseHoverFontSize, wordBuf);
                            if (lineWidth + wordSize.width > usableWidth && lineWidth > 0) {
                                break;
                            }
                            lineWidth += wordSize.width;
                            charsInLine = i - startPos;
                            if (i < len && line[i] == ' ') {
                                lastSpace = i;
                                charsInLine++;
                            }
                        }
                    }
                }
                if (lastSpace == -1 && charsInLine == 0 && len - startPos > 0) {
                    charsInLine = 1;
                    while (startPos + charsInLine < len) {
                        char testBuf[1024] = {0};
                        strncpy(testBuf, line + startPos, charsInLine);
                        testBuf[charsInLine] = '\0';
                        TextSize testSize = GetTextSizeCached(font, baseHoverFontSize, testBuf);
                        if (testSize.width > usableWidth) break;
                        charsInLine++;
                    }
                    charsInLine = fmax(1, charsInLine - 1);
                }
                if (charsInLine > 0) {
                    char* wrappedLine = malloc(charsInLine + 1);
                    if (wrappedLine) {
                        strncpy(wrappedLine, line + startPos, charsInLine);
                        wrappedLine[charsInLine] = '\0';
                        wrappedLines[numWrappedLines++] = wrappedLine;
                        TextSize lineSize = GetTextSizeCached(font, baseHoverFontSize, wrappedLine);
                        totalHeight += (int)(lineSize.height * lineSpacingFactor);
                    }
                }
                startPos += charsInLine;
                while (startPos < len && line[startPos] == ' ') startPos++;
                if (startPos >= len) break;
            }
        }
        line = strtok_r(NULL, "\n", &context);
    }
    free(lineCopy);
    totalHeight += (int)(baseHoverFontSize);
    int tooltipHeight = totalHeight + (verticalPadding * 2);
    tooltipHeight = fmin(tooltipHeight, window.screen_height - 10);
    int tooltipX = cursorX + (int)(fontSize * 0.5f);
    int tooltipY = cursorY + lineHeight + 2;
    if (tooltipX + tooltipWidth > window.screen_width - 5) {
        tooltipX = window.screen_width - tooltipWidth - 5;
    }
    if (tooltipY + tooltipHeight > window.screen_height - 5) {
        tooltipY = cursorY - tooltipHeight - 2;
    }
    DrawRectBatch(tooltipX, tooltipY, tooltipWidth, tooltipHeight, Color30);
    DrawRectBorder(tooltipX, tooltipY, tooltipWidth, tooltipHeight, 1, Color05);
    DrawRectBatch(tooltipX, tooltipY, tooltipWidth, 2, Color04);
    FlushRectBatch();
    int lineY = tooltipY + verticalPadding;
    bool inCodeBlock = false;
    int visibleLines = numWrappedLines;
    for (int i = 0; i < visibleLines; i++) {
        if (!wrappedLines[i] || !wrappedLines[i][0]) {
            lineY += (int)(baseHoverFontSize * 0.6f);
            continue;
        }
        if (strncmp(wrappedLines[i], "```", 3) == 0) {
            inCodeBlock = !inCodeBlock;
            DrawTextBatch(tooltipX + horizontalPadding, lineY, font, baseHoverFontSize, wrappedLines[i], Color10);
            lineY += (int)(baseHoverFontSize * lineSpacingFactor);
            continue;
        }
        Color textColor = inCodeBlock ? Color10 : Color07;
        if (!inCodeBlock) {
            for (size_t p = 0; p < sizeof(syntaxPatterns)/sizeof(syntaxPatterns[0]); p++) {
                const char* pattern = syntaxPatterns[p].pattern;
                bool isPrefix = syntaxPatterns[p].isPrefix;
                bool matched = (isPrefix && strncmp(wrappedLines[i], pattern, strlen(pattern)) == 0) || 
                              (!isPrefix && strstr(wrappedLines[i], pattern));
                if (matched) {
                    textColor = syntaxPatterns[p].color;
                    break;
                }
            }
        }
        DrawTextBatch(tooltipX + horizontalPadding, lineY, font, baseHoverFontSize, wrappedLines[i], textColor);
        lineY += (int)(baseHoverFontSize * lineSpacingFactor);
    }
    char* closeHint = "Esc to close";
    float hintFontSize = baseHoverFontSize * 0.65f;
    TextSize hintSize = GetTextSizeCached(font, hintFontSize, closeHint);
    int hintX = tooltipX + tooltipWidth - hintSize.width - horizontalPadding;
    int hintY = tooltipY + tooltipHeight - (int)(baseHoverFontSize * 0.9f);
    DrawTextBatch(hintX, hintY, font, hintFontSize, closeHint, Color02);
    for (int i = 0; i < numWrappedLines; i++) {
        free(wrappedLines[i]);
    }
    free(wrappedLines);
    FlushTextBatch();
}
bool LspApplySelectedCompletion() {
    if (!lsp.completions.active || 
        lsp.completions.selectedIndex < 0 || 
        lsp.completions.selectedIndex >= lsp.completions.count) 
        return false;
    CompletionItem* item = &lsp.completions.items[lsp.completions.selectedIndex];
    int textToDelete = cursorCol - lsp.completions.startCol;
    if (textToDelete < 0) textToDelete = 0;
    for (int i = 0; i < textToDelete; i++) {
        DeleteChar();
    }
    for (int i = 0; i < strlen(item->insertText); i++) {
        InsertChar(item->insertText[i]);
    }
    lsp.completions.active = false;
    return true;
}
void CheckAutoCompletion() {
    if (!autoCompletionEnabled || !lsp.active || preventUiReopen || lsp.completions.active) {
        return;
    }
    double timeSinceTyping = window.time - lastTypingTime;
    if (timeSinceTyping > (autoCompletionDelayMs / 1000.0) && 
        timeSinceTyping < 2.0 && cursorCol > 0) {
        char currentWord[64] = {0};
        int wordLen = 0;
        int startCol = cursorCol;
        while (startCol > 0 && wordLen < sizeof(currentWord) - 1) {
            char c = lines[cursorLine].text[startCol - 1];
            if (!isalnum(c) && c != '_') break;
            startCol--;
        }
        for (int i = startCol; i < cursorCol && wordLen < sizeof(currentWord) - 1; i++) {
            currentWord[wordLen++] = lines[cursorLine].text[i];
        }
        currentWord[wordLen] = '\0';
        if (wordLen >= minCompletionChars) {
            LspRequestCompletion(cursorLine, startCol);
        }
    }
}
void HandleAsyncLspResponses() {
    static double requestStartTime = 0;
    static bool wasRequestInProgress = false;
    if (lsp.definitionRequestTime > 0 && 
        window.time - lsp.definitionRequestTime > 2.0) {
        fprintf(stderr, "Definition request timed out\n");
        snprintf(statusMsg, sizeof(statusMsg), "Definition request timed out");
        lsp.definitionRequestTime = 0;
        wasRequestInProgress = false;
    }
    if (lsp.isDefinitionAvailable && lsp.definitionRequestTime > 0) {
        requestStartTime = lsp.definitionRequestTime;
        wasRequestInProgress = true;
        lsp.definitionRequestTime = 0;
        char decodedDefinitionPath[PATH_MAX] = {0};
        UriDecode(decodedDefinitionPath, lsp.definitionLocation.uri, sizeof(decodedDefinitionPath));
        char baseNameBuf[PATH_MAX] = {0};
        strncpy(baseNameBuf, decodedDefinitionPath, sizeof(baseNameBuf) - 1);
        char* baseName = basename(baseNameBuf);
        char currentUri[LSP_URI_MAX];
        LspCreateUri(filename, currentUri, sizeof(currentUri));
        char decodedCurrentPath[PATH_MAX] = {0};
        UriDecode(decodedCurrentPath, currentUri, sizeof(decodedCurrentPath));
        if (strcmp(decodedDefinitionPath, decodedCurrentPath) == 0) {
            snprintf(statusMsg, sizeof(statusMsg), "Definition found at line %d (press F12 to go)", 
                     lsp.definitionLocation.line + 1);
        } else {
            snprintf(statusMsg, sizeof(statusMsg), "Definition found in %s (press F12 to go)", 
                     baseName);
        }
    }
}
void LspProcessDefinitionResponse(const char* json) {
    if (!json) {
        lsp.isDefinitionAvailable = false;
        return;
    }
    char* resultValue = LspFindJsonValue(json, "result");
    if (!resultValue || strcmp(resultValue, "null") == 0) {
        lsp.isDefinitionAvailable = false;
        snprintf(statusMsg, sizeof(statusMsg), "No definition found");
        return;
    }
    if (resultValue[0] == '[') {
        char* firstItem = LspJsonArrayItem(resultValue, 0);
        if (!firstItem) {
            lsp.isDefinitionAvailable = false;
            snprintf(statusMsg, sizeof(statusMsg), "Empty definition results");
            return;
        }
        fprintf(stderr, "Processing definition array result, using first item\n");
        resultValue = firstItem;
    }
    char* uriValue = LspFindJsonValue(resultValue, "uri");
    if (!uriValue) {
        uriValue = LspFindJsonValue(resultValue, "targetUri");
        if (!uriValue) {
            lsp.isDefinitionAvailable = false;
            snprintf(statusMsg, sizeof(statusMsg), "Invalid definition format (no URI)");
            return;
        }
    }
    char uri[LSP_URI_MAX] = {0};
    LspJsonExtractString(uriValue, uri, sizeof(uri));
    fprintf(stderr, "Definition URI: %s\n", uri);
    int line = 0, startChar = 0, endChar = 0;
    char* rangeValue = LspFindJsonValue(resultValue, "range");
    if (rangeValue) {
        char* startValue = LspFindJsonValue(rangeValue, "start");
        if (startValue) {
            char* lineValue = LspFindJsonValue(startValue, "line");
            char* startCharValue = LspFindJsonValue(startValue, "character");
            if (lineValue && startCharValue) {
                line = LspJsonExtractInt(lineValue, 0);
                startChar = LspJsonExtractInt(startCharValue, 0);
                char* endValue = LspFindJsonValue(rangeValue, "end");
                if (endValue) {
                    char* endCharValue = LspFindJsonValue(endValue, "character");
                    if (endCharValue) {
                        endChar = LspJsonExtractInt(endCharValue, startChar);
                    }
                }
            }
        }
    }
    else {
        char* targetRangeValue = LspFindJsonValue(resultValue, "targetSelectionRange");
        if (targetRangeValue) {
            char* startValue = LspFindJsonValue(targetRangeValue, "start");
            if (startValue) {
                char* lineValue = LspFindJsonValue(startValue, "line");
                char* startCharValue = LspFindJsonValue(startValue, "character");
                if (lineValue && startCharValue) {
                    line = LspJsonExtractInt(lineValue, 0);
                    startChar = LspJsonExtractInt(startCharValue, 0);
                    char* endValue = LspFindJsonValue(targetRangeValue, "end");
                    if (endValue) {
                        char* endCharValue = LspFindJsonValue(endValue, "character");
                        if (endCharValue) {
                            endChar = LspJsonExtractInt(endCharValue, startChar);
                        }
                    }
                }
            }
        }
    }
    if (line == 0 && startChar == 0) {
        char* lineValue = LspFindJsonValue(resultValue, "line");
        char* startCharValue = LspFindJsonValue(resultValue, "character");
        if (lineValue && startCharValue) {
            line = LspJsonExtractInt(lineValue, 0);
            startChar = LspJsonExtractInt(startCharValue, 0);
        }
    }
    if (endChar == 0) {
        endChar = startChar + 1;
    }
    strncpy(lsp.definitionLocation.uri, uri, sizeof(lsp.definitionLocation.uri) - 1);
    lsp.definitionLocation.line = line;
    lsp.definitionLocation.startCol = startChar;
    lsp.definitionLocation.endCol = endChar;
    lsp.isDefinitionAvailable = true;
    lsp.definitionRequestTime = 0;
    fprintf(stderr, "Definition found at %s:%d:%d-%d\n", 
            uri, line, startChar, endChar);
    char currentUri[LSP_URI_MAX];
    LspCreateUri(filename, currentUri, sizeof(currentUri));
    if (strcmp(uri, currentUri) == 0) {
        snprintf(statusMsg, sizeof(statusMsg), "Definition found at line %d", 
                 line + 1);
    } else {
        char decodedPath[PATH_MAX] = {0};
        UriDecode(decodedPath, uri, sizeof(decodedPath));
        char baseNameBuf[PATH_MAX] = {0};
        strncpy(baseNameBuf, decodedPath, sizeof(baseNameBuf) - 1);
        char* baseName = basename(baseNameBuf);
        snprintf(statusMsg, sizeof(statusMsg), "Definition found in %s", baseName);
    }
}
void LspUpdate() {
    if (!lsp.active) return;
    char* message;
    while ((message = LspReceive()) != NULL) {
        char* idValue = LspFindJsonValue(message, "id");
        if (idValue) {
            char* resultValue = LspFindJsonValue(message, "result");
            if (resultValue) {
                if (strstr(message, "\"textDocument/definition\"") ||
                    (lsp.completions.startLine >= 0 && resultValue[0] == '[')) {
                    LspProcessDefinitionResponse(message);
                } else if (strstr(message, "\"textDocument/hover\"") ||
                           (lsp.hoverInfo.line >= 0 && strstr(message, "\"contents\""))) {
                    if (!preventUiReopen) {
                        LspProcessHoverResponse(message);
                    }
                }
            }
        } else {
            char* methodValue = LspFindJsonValue(message, "method");
            if (methodValue) {
                char methodName[64] = {0};
                LspJsonExtractString(methodValue, methodName, sizeof(methodName));
                if (strcmp(methodName, "textDocument/publishDiagnostics") == 0)
                    LspProcessDiagnostics(message);
            }
        }
        free(message);
    }
    HandleAsyncLspResponses();
    if (preventUiReopen && (window.time - preventionStartTime) > preventionDuration) {
        preventUiReopen = false;
    }
    if (preventUiReopen) {
        lsp.completions.active = false;
        lsp.hoverInfo.active = false;
        return;
    }
    CheckAutoCompletion();
    static double lastCursorUpdate = 0;
    static int lastCursorLine = -1, lastCursorCol = -1;
    static double lastHoverRequest = 0;
    const double HOVER_COOLDOWN = 2.0;
    const double HOVER_DELAY = 1.0;
    if (cursorLine != lastCursorLine || cursorCol != lastCursorCol) {
        lsp.hoverInfo.active = false;
        lastCursorLine = cursorLine;
        lastCursorCol = cursorCol;
        lastCursorUpdate = window.time;
    } else if (!lsp.hoverInfo.active &&
               !preventUiReopen &&
               window.time - lastCursorUpdate > HOVER_DELAY &&
               window.time - lastHoverRequest > HOVER_COOLDOWN) {
        LspRequestHover(cursorLine, cursorCol);
        lastHoverRequest = window.time;
    }
}
void ToggleAutoCompletion() {
    autoCompletionEnabled = !autoCompletionEnabled;
    snprintf(statusMsg, sizeof(statusMsg), "Auto-completion %s", 
             autoCompletionEnabled ? "enabled" : "disabled");
}
void UpdateLsp() {
    static bool lastDirty = false;
    if (lsp.active && isFileDirty && !lastDirty) {
        char* text = ConstructTextFromLines();
        if (text) {
            LspNotifyDidChange(filename, text);
            free(text);
        }
    }
    lastDirty = isFileDirty;
    LspUpdate();
    if (lsp.diagnosticCount > 0) {
        LspDrawDiagnostics();
    }
    if (lsp.hoverInfo.active) {
        LspDrawHoverInfo();
    }
    if (lsp.completions.active && lsp.completions.count > 0) {
        LspDrawCompletions();
    }
}
void LspInit() {
    const struct {
        const char* extension;
        const char* language;
        const char* serverPaths[3];
    } serverMap[] = {
        {".c", "c", {"/usr/bin/clangd", "/usr/local/bin/clangd", NULL}},
        {".cpp", "cpp", {"/usr/bin/clangd", "/usr/local/bin/clangd", NULL}},
        {".h", "c", {"/usr/bin/clangd", "/usr/local/bin/clangd", NULL}},
        {".py", "python", {"/usr/bin/pylsp", "/usr/local/bin/pylsp", NULL}},
        {".js", "javascript", {"/usr/bin/typescript-language-server", "/usr/local/bin/typescript-language-server", NULL}},
        {".ts", "typescript", {"/usr/bin/typescript-language-server", "/usr/local/bin/typescript-language-server", NULL}},
        {".go", "go", {"/usr/bin/gopls", "/usr/local/bin/gopls", NULL}},
        {".rs", "rust", {"/usr/bin/rust-analyzer", "/usr/local/bin/rust-analyzer", NULL}}
    };
    const char* ext = strrchr(filename, '.');
    if (!ext) return;
    for (size_t i = 0; i < sizeof(serverMap)/sizeof(serverMap[0]); i++) {
        if (strcasecmp(ext, serverMap[i].extension) == 0) {
            for (int j = 0; serverMap[i].serverPaths[j] != NULL; j++) {
                if (access(serverMap[i].serverPaths[j], X_OK) == 0) {
                    fprintf(stderr, "Found LSP server: %s\n", serverMap[i].serverPaths[j]);
                    if (LspStart(serverMap[i].serverPaths[j], serverMap[i].language)) {
                        char* text = ConstructTextFromLines();
                        if (text) {
                            LspNotifyDidOpen(filename, text);
                            free(text);
                            snprintf(statusMsg, sizeof(statusMsg), "LSP: Connected to %s", serverMap[i].serverPaths[j]);
                        }
                        return;
                    } else {
                        fprintf(stderr, "Failed to start LSP server: %s\n", serverMap[i].serverPaths[j]);
                    }
                } else {
                    fprintf(stderr, "Server not executable or not found: %s\n", serverMap[i].serverPaths[j]);
                }
            }
            snprintf(statusMsg, sizeof(statusMsg), "LSP: No server found for %s", serverMap[i].language);
            return;
        }
    }
}
void LspRestart() {
    if (!lsp.active) return;
    char langId[32];
    strcpy(langId, lsp.langId);
    LspStop();
    usleep(100000);
    const char* serverPath = NULL;
    if (strcmp(langId, "c") == 0 || strcmp(langId, "cpp") == 0) {
        serverPath = access("/usr/local/bin/clangd", X_OK) == 0 ? "/usr/local/bin/clangd" : "/usr/bin/clangd";
    } else if (strcmp(langId, "python") == 0) {
        serverPath = access("/usr/local/bin/pylsp", X_OK) == 0 ? "/usr/local/bin/pylsp" : "/usr/bin/pylsp";
    } else if (strcmp(langId, "javascript") == 0 || strcmp(langId, "typescript") == 0) {
        serverPath = access("/usr/local/bin/typescript-language-server", X_OK) == 0 ? 
                    "/usr/local/bin/typescript-language-server" : "/usr/bin/typescript-language-server";
    } else if (strcmp(langId, "go") == 0) {
        serverPath = access("/usr/local/bin/gopls", X_OK) == 0 ? "/usr/local/bin/gopls" : "/usr/bin/gopls";
    } else if (strcmp(langId, "rust") == 0) {
        serverPath = access("/usr/local/bin/rust-analyzer", X_OK) == 0 ? "/usr/local/bin/rust-analyzer" : "/usr/bin/rust-analyzer";
    }
    if (serverPath && access(serverPath, X_OK) == 0) {
        if (LspStart(serverPath, langId)) {
            char* text = ConstructTextFromLines();
            if (text) {
                LspNotifyDidOpen(filename, text);
                free(text);
            }
            snprintf(statusMsg, sizeof(statusMsg), "LSP server restarted");
        }
    }
}
void LspCleanup() {
    if (lsp.active) LspStop();
}
void GoToDefinition() {
    if (isDefinitionJumped) {
        if (prevLine != -1 && strlen(prevFilename) > 0) {
            fprintf(stderr, "Returning to previous location: %s:%d:%d\n", 
                    prevFilename, prevLine, prevCol);
            if (strcmp(filename, prevFilename) != 0) {
                if (SyntaxIsEnabled()) {
                    SyntaxCleanup();
                }
                if (!LoadFile(prevFilename)) {
                    snprintf(statusMsg, sizeof(statusMsg), "Failed to load previous file");
                    return;
                }
                strncpy(filename, prevFilename, sizeof(filename) - 1);
                Color defaultTextColor = {textColor.r, textColor.g, textColor.b, textColor.a};
                char* initialText = ConstructTextFromLines();
                int textLen = initialText ? strlen(initialText) : 0;
                SyntaxInitWithLanguageDetection(
                    defaultTextColor, 
                    SyntaxDrawWrapper, 
                    &font, 
                    fontSize, 
                    NULL, 
                    filename, 
                    initialText, 
                    textLen, 
                    statusMsg, 
                    sizeof(statusMsg)
                );
                if (initialText) free(initialText);
                LspInit();
            }
            cursorLine = prevLine;
            cursorCol = prevCol;
            selStartLine = cursorLine;
            selStartCol = cursorCol;
            selEndLine = cursorLine;
            selEndCol = cursorCol;
            isSelecting = true;
            prevLine = -1;
            prevCol = -1;
            prevFilename[0] = '\0';
            isDefinitionJumped = false;
            int visibleHeight = window.screen_height - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0);
            scroll.targetY = (cursorLine * lineHeight) - (visibleHeight / 2) + (lineHeight / 2);
            scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY));
            if (!wordWrap) {
                float cursorX = GetCursorXPosition();
                int visibleWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
                scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), cursorX - (visibleWidth / 2)));
            }
            if (wordWrap) {
                RecalculateWrappedLines();
            }
            snprintf(statusMsg, sizeof(statusMsg), "Returned to previous location");
            return;
        }
    }
    if (!lsp.active) {
        snprintf(statusMsg, sizeof(statusMsg), "LSP not active");
        return;
    }
    static bool isRequestInProgress = false;
    static double lastRequestTime = 0;
    if (isRequestInProgress || 
        (window.time - lastRequestTime < 0.5)) {
        return;
    }
    isRequestInProgress = true;
    lastRequestTime = window.time;
    if (lsp.isDefinitionAvailable) {
        char currentUri[LSP_URI_MAX];
        LspCreateUri(filename, currentUri, sizeof(currentUri));
        char decodedCurrentPath[PATH_MAX] = {0};
        char decodedDefinitionPath[PATH_MAX] = {0};
        UriDecode(decodedCurrentPath, currentUri, sizeof(decodedCurrentPath));
        UriDecode(decodedDefinitionPath, lsp.definitionLocation.uri, sizeof(decodedDefinitionPath));
        fprintf(stderr, "Current path: %s\nDefinition path: %s\n", 
                decodedCurrentPath, decodedDefinitionPath);
        prevLine = cursorLine;
        prevCol = cursorCol;
        strncpy(prevFilename, filename, sizeof(prevFilename) - 1);
        if (strcmp(decodedDefinitionPath, decodedCurrentPath) == 0) {
            fprintf(stderr, "Navigating within same file to line %d\n", lsp.definitionLocation.line);
            cursorLine = lsp.definitionLocation.line;
            cursorCol = lsp.definitionLocation.startCol;
            selStartLine = cursorLine;
            selStartCol = lsp.definitionLocation.startCol;
            selEndLine = cursorLine;
            selEndCol = lsp.definitionLocation.endCol;
            isSelecting = true;
            isDefinitionJumped = true;
            int visibleHeight = window.screen_height - (showStatusBar ? statusBarHeight : 0) - (showModeline ? modeLineHeight : 0);
            scroll.targetY = (cursorLine * lineHeight) - (visibleHeight / 2) + (lineHeight / 2);
            scroll.targetY = fmax(0, fmin(MaxScroll(), scroll.targetY));
            if (!wordWrap) {
                float cursorX = GetCursorXPosition();
                int visibleWidth = window.screen_width - (showLineNumbers ? gutterWidth : 0) - (showScrollbar ? scrollbarWidth : 0);
                scroll.targetX = fmax(0, fmin(MaxHorizontalScroll(), cursorX - (visibleWidth / 2)));
            }
            if (wordWrap) {
                RecalculateWrappedLines();
            }
            snprintf(statusMsg, sizeof(statusMsg), "Definition at line %d", cursorLine + 1);
            lsp.isDefinitionAvailable = false;
            isRequestInProgress = false;
            return;
        } 
        else {
            fprintf(stderr, "Navigating to different file: %s\n", lsp.definitionLocation.uri);
            if (OpenFileFromDefinition(lsp.definitionLocation.uri, 
                                       lsp.definitionLocation.line,
                                       lsp.definitionLocation.startCol)) {
                lsp.isDefinitionAvailable = false;
                lsp.hoverInfo.active = false;
                lsp.completions.active = false;
                isDefinitionJumped = true;
                isRequestInProgress = false;
                return;
            } else {
                snprintf(statusMsg, sizeof(statusMsg), "Failed to open definition file");
                isRequestInProgress = false;
                return;
            }
        }
    }
    snprintf(statusMsg, sizeof(statusMsg), "Finding definition...");
    lsp.hoverInfo.active = false;
    lsp.completions.active = false;
    lsp.definitionRequestTime = window.time;
    char wordUnderCursor[64] = {0};
    int wordLen = 0;
    int startCol = cursorCol;
    int endCol = cursorCol;
    if (cursorLine < numLines) {
        while (startCol > 0 && (isalnum(lines[cursorLine].text[startCol - 1]) || lines[cursorLine].text[startCol - 1] == '_')) 
            startCol--;
        while (endCol < lines[cursorLine].length && (isalnum(lines[cursorLine].text[endCol]) || lines[cursorLine].text[endCol] == '_')) 
            endCol++;
        wordLen = endCol - startCol;
        if (wordLen > 0 && wordLen < sizeof(wordUnderCursor) - 1) {
            strncpy(wordUnderCursor, &lines[cursorLine].text[startCol], wordLen);
            wordUnderCursor[wordLen] = '\0';
        }
    }
    fprintf(stderr, "Requesting definition for position %d:%d\n", cursorLine, cursorCol);
    LspRequestDefinition(cursorLine, cursorCol);
    if (wordLen > 0) {
        snprintf(statusMsg, sizeof(statusMsg), "Finding definition for '%s'...", wordUnderCursor);
    }
    isRequestInProgress = false;
}
void EditorLspKeyHandler(int key, int action, int mods) {
    if (action != GLFW_PRESS) return;
    bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    if (key == GLFW_KEY_F12 || (key == GLFW_KEY_G && ctrl)) {
        fprintf(stderr, "F12 or Ctrl+G pressed - Go To Definition\n");
        preventUiReopen = false;
        lsp.hoverInfo.active = false;
        GoToDefinition();
        return;
    }
    if (ctrl && shift && key == GLFW_KEY_SPACE) {
        ToggleAutoCompletion();
        return;
    }
    if (key == GLFW_KEY_ESCAPE) {
        if (lsp.completions.active || lsp.hoverInfo.active) {
            lsp.completions.active = false;
            lsp.completions.count = 0;
            lsp.completions.selectedIndex = 0;
            lsp.hoverInfo.active = false;
            preventUiReopen = true;
            preventionStartTime = window.time;
            return;
        }
    }
    if (key == GLFW_KEY_SPACE && ctrl && !shift) {
        preventUiReopen = false;
        LspRequestCompletion(cursorLine, cursorCol);
        return;
    }
    if (key == GLFW_KEY_I && ctrl) {
        preventUiReopen = false;
        LspRequestHover(cursorLine, cursorCol);
        return;
    }
    if (lsp.completions.active) {
        switch (key) {
            case GLFW_KEY_DOWN:
                lsp.completions.selectedIndex = 
                    (lsp.completions.selectedIndex + 1) % lsp.completions.count;
                return;
            case GLFW_KEY_UP:
                lsp.completions.selectedIndex = 
                    (lsp.completions.selectedIndex - 1 + lsp.completions.count) % 
                    lsp.completions.count;
                return;
            case GLFW_KEY_TAB:
            case GLFW_KEY_ENTER:
                if (LspApplySelectedCompletion()) {
                    if (wordWrap) RecalculateWrappedLines();
                    return;
                }
                break;
        }
    }
    if (ctrl && shift) {
        switch (key) {
            case GLFW_KEY_R:
                if (lsp.active) LspRestart();
                return;
            case GLFW_KEY_P:
                preventUiReopen = !preventUiReopen;
                if (preventUiReopen) {
                    preventionStartTime = window.time;
                    lsp.completions.active = false;
                    lsp.hoverInfo.active = false;
                    snprintf(statusMsg, sizeof(statusMsg), "LSP popups disabled");
                } else {
                    snprintf(statusMsg, sizeof(statusMsg), "LSP popups enabled");
                }
                return;
        }
    }
}
void EditorLspCharHandler(unsigned int c) {
    if (preventUiReopen) {
        return;
    }
    lastTypingTime = window.time;
    if (c == '.' || c == '>' || c == ':' || c == '(' || c == '[') {
        LspRequestCompletion(cursorLine, cursorCol);
    }
}