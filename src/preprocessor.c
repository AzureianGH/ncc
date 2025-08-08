#include "preprocessor_simple.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Simple macro structure
struct MacroData {
    char** parameters;
    int paramCount;
    int isVariadic;
};

// Global preprocessor state
static struct {
    Macro* macros;
    int macroCount;
    int maxMacros;
    char** includePaths;
    int includePathCount;
    FILE* input;
    int line;
    char* currentFile;
} state;

Preprocessor preprocessor; // Export interface

// Helper: find macro by name
static Macro* pp_find_macro(const char* name) {
    for (int i = 0; i < state.macroCount; i++) {
        if (strcmp(state.macros[i].name, name) == 0) return &state.macros[i];
    }
    return NULL;
}

// Helper: is identifier character
static int pp_is_ident_char(char c) {
    return (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
}

// Helper: naive object-like macro expansion for a single line, skipping inside strings
static char* pp_expand_macros_line(const char* line) {
    if (!line) return NULL;
    size_t cap = strlen(line) + 1;
    char* buf = (char*)malloc(cap);
    size_t len = 0;
    int in_string = 0;
    for (size_t i = 0; line[i] != '\0'; ) {
        char c = line[i];
        if (c == '"') {
            in_string = !in_string;
            if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
            buf[len++] = line[i++];
            continue;
        }
        if (!in_string && ((c == '_') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
            size_t start = i;
            i++;
            while (pp_is_ident_char(line[i])) i++;
            size_t idLen = i - start;
            char* ident = (char*)malloc(idLen + 1);
            memcpy(ident, &line[start], idLen);
            ident[idLen] = '\0';
            Macro* m = pp_find_macro(ident);
            if (m) {
                size_t repLen = strlen(m->replacement);
                if (len + repLen + 1 >= cap) { cap = (len + repLen + 1) * 2; buf = (char*)realloc(buf, cap); }
                memcpy(&buf[len], m->replacement, repLen);
                len += repLen;
            } else {
                if (len + idLen + 1 >= cap) { cap = (len + idLen + 1) * 2; buf = (char*)realloc(buf, cap); }
                memcpy(&buf[len], &line[start], idLen);
                len += idLen;
            }
            free(ident);
            continue;
        }
        if (len + 2 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
        buf[len++] = c;
        i++;
    }
    buf[len] = '\0';
    return buf;
}

void preprocessorError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    fprintf(stderr, "Preprocessor error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    
    va_end(args);
}

Macro* createMacro(const char* name, const char* replacement, int isFunction) {
    Macro* macro = malloc(sizeof(Macro));
    if (!macro) return NULL;
    
    macro->name = strdup(name);
    macro->replacement = strdup(replacement);
    macro->isFunction = isFunction;
    
    if (isFunction) {
        macro->data = malloc(sizeof(MacroData));
        macro->data->parameters = NULL;
        macro->data->paramCount = 0;
        macro->data->isVariadic = 0;
    } else {
        macro->data = NULL;
    }
    
    return macro;
}

void addMacro(Macro* macro) {
    if (state.macroCount >= state.maxMacros) {
        state.maxMacros = state.maxMacros ? state.maxMacros * 2 : 16;
        state.macros = realloc(state.macros, sizeof(Macro) * state.maxMacros);
    }
    
    state.macros[state.macroCount] = *macro;
    state.macroCount++;
    free(macro); // We copied the contents
}

void removeMacro(const char* name) {
    for (int i = 0; i < state.macroCount; i++) {
        if (strcmp(state.macros[i].name, name) == 0) {
            // Free macro data
            free(state.macros[i].name);
            free(state.macros[i].replacement);
            if (state.macros[i].data) {
                if (state.macros[i].data->parameters) {
                    for (int j = 0; j < state.macros[i].data->paramCount; j++) {
                        free(state.macros[i].data->parameters[j]);
                    }
                    free(state.macros[i].data->parameters);
                }
                free(state.macros[i].data);
            }
            
            // Shift remaining macros
            for (int j = i; j < state.macroCount - 1; j++) {
                state.macros[j] = state.macros[j + 1];
            }
            state.macroCount--;
            return;
        }
    }
}

void addIncludePath(const char* path) {
    state.includePaths = realloc(state.includePaths, 
                               sizeof(char*) * (state.includePathCount + 1));
    state.includePaths[state.includePathCount] = strdup(path);
    state.includePathCount++;
}

void clearIncludePaths(void) {
    for (int i = 0; i < state.includePathCount; i++) {
        free(state.includePaths[i]);
    }
    free(state.includePaths);
    state.includePaths = NULL;
    state.includePathCount = 0;
}

void initPreprocessor(void) {
    memset(&state, 0, sizeof(state));
    memset(&preprocessor, 0, sizeof(preprocessor));
    
    // Set up exported interface
    preprocessor.macros = NULL;
    preprocessor.macroCount = 0;
    preprocessor.maxMacros = 0;
    preprocessor.includePaths = NULL;
    preprocessor.fileStackDepth = 0;
    preprocessor.currentFile = NULL;
    
    state.line = 1;
}

void cleanupPreprocessor(void) {
    // Free macros
    for (int i = 0; i < state.macroCount; i++) {
        free(state.macros[i].name);
        free(state.macros[i].replacement);
        if (state.macros[i].data) {
            if (state.macros[i].data->parameters) {
                for (int j = 0; j < state.macros[i].data->paramCount; j++) {
                    free(state.macros[i].data->parameters[j]);
                }
                free(state.macros[i].data->parameters);
            }
            free(state.macros[i].data);
        }
    }
    free(state.macros);
    
    // Free include paths
    clearIncludePaths();
    
    // Close input file if open
    if (state.input && state.input != stdin) {
        fclose(state.input);
    }
    
    free(state.currentFile);
    memset(&state, 0, sizeof(state));
}

char* preprocessSource(const char* source) {
    // Minimal preprocessor supporting:
    // - #include "path"
    // - #define name replacement (object-like)
    // - #undef name
    // - #ifdef/#ifndef/#else/#endif (no #elif, no expression #if)
    // - simple object-like macro expansion on non-directive lines (skip inside strings)

    if (!source) return NULL;

    size_t outCap = strlen(source) + 256;
    size_t outLen = 0;
    char* out = (char*)malloc(outCap);
    if (!out) return NULL;
    out[0] = '\0';

    // Conditional include stack
    int condStackCap = 16;
    int condTop = -1;
    int* condStack = (int*)malloc(sizeof(int) * condStackCap);
    int currentInclude = 1; // by default include text

    // Iterate lines
    const char* p = source;
    while (*p) {
        // Read one line
        const char* lineStart = p;
        while (*p && *p != '\n') p++;
        size_t lineLen = (size_t)(p - lineStart);
        if (*p == '\n') p++;

        // Copy line into a temporary null-terminated buffer
        char* line = (char*)malloc(lineLen + 1);
        memcpy(line, lineStart, lineLen);
        line[lineLen] = '\0';

        // Trim leading whitespace
        char* cur = line;
        while (*cur == ' ' || *cur == '\t' || *cur == '\r' || *cur == '\f' || *cur == '\v') cur++;

        if (*cur == '#') {
            // Preprocessor directive
            cur++; // skip '#'
            while (*cur == ' ' || *cur == '\t') cur++;

            // Extract directive keyword
            char dir[32] = {0};
            int di = 0;
            while (*cur && (( *cur >= 'a' && *cur <= 'z') || (*cur >= 'A' && *cur <= 'Z') || *cur == '_')) {
                if (di < (int)sizeof(dir) - 1) dir[di++] = *cur;
                cur++;
            }
            while (*cur == ' ' || *cur == '\t') cur++;

            if (strcmp(dir, "include") == 0) {
                // Only support #include "path"
                if (*cur == '"') {
                    cur++;
                    const char* pathStart = cur;
                    while (*cur && *cur != '"') cur++;
                    size_t plen = (size_t)(cur - pathStart);
                    char* incPath = (char*)malloc(plen + 1);
                    memcpy(incPath, pathStart, plen); incPath[plen] = '\0';

                    // Try to open directly or via include paths
                    char fullPath[1024];
                    FILE* inc = fopen(incPath, "r");
                    if (!inc) {
                        for (int i = 0; i < state.includePathCount && !inc; i++) {
                            snprintf(fullPath, sizeof(fullPath), "%s/%s", state.includePaths[i], incPath);
                            inc = fopen(fullPath, "r");
                        }
                    }
                    if (inc) fclose(inc);

                    char* included = preprocessFile(incPath);
                    if (!included) {
                        // Try included via includePaths if direct failed in preprocessFile
                        for (int i = 0; i < state.includePathCount && !included; i++) {
                            snprintf(fullPath, sizeof(fullPath), "%s/%s", state.includePaths[i], incPath);
                            included = preprocessFile(fullPath);
                        }
                    }
                    if (included && currentInclude) {
                        size_t need = strlen(included);
                        if (outLen + need + 1 > outCap) { outCap = (outLen + need + 1) * 2; out = (char*)realloc(out, outCap); }
                        memcpy(&out[outLen], included, need);
                        outLen += need;
                        out[outLen] = '\0';
                    }
                    free(included);
                    free(incPath);
                }
            } else if (strcmp(dir, "define") == 0) {
                // #define NAME REPLACEMENT
                // Parse name
                char name[256] = {0};
                int ni = 0;
                while (*cur == ' ' || *cur == '\t') cur++;
                while (*cur && ( (*cur == '_') || (*cur >= 'a' && *cur <= 'z') || (*cur >= 'A' && *cur <= 'Z') || (*cur >= '0' && *cur <= '9') )) {
                    if (ni < (int)sizeof(name)-1) name[ni++] = *cur;
                    cur++;
                }
                while (*cur == ' ' || *cur == '\t') cur++;
                const char* replStart = cur;
                // Trim trailing spaces
                const char* replEnd = line + lineLen;
                while (replEnd > replStart && (replEnd[-1] == ' ' || replEnd[-1] == '\t' || replEnd[-1] == '\r')) replEnd--;
                size_t rlen = (size_t)(replEnd - replStart);
                char* repl = (char*)malloc(rlen + 1);
                memcpy(repl, replStart, rlen); repl[rlen] = '\0';
                Macro* m = createMacro(name, repl, 0);
                addMacro(m);
                free(repl);
            } else if (strcmp(dir, "undef") == 0) {
                // #undef NAME
                char name[256] = {0};
                int ni = 0;
                while (*cur == ' ' || *cur == '\t') cur++;
                while (*cur && ( (*cur == '_') || (*cur >= 'a' && *cur <= 'z') || (*cur >= 'A' && *cur <= 'Z') || (*cur >= '0' && *cur <= '9') )) {
                    if (ni < (int)sizeof(name)-1) name[ni++] = *cur;
                    cur++;
                }
                removeMacro(name);
            } else if (strcmp(dir, "ifdef") == 0) {
                char name[256] = {0};
                int ni = 0;
                while (*cur == ' ' || *cur == '\t') cur++;
                while (*cur && ( (*cur == '_') || (*cur >= 'a' && *cur <= 'z') || (*cur >= 'A' && *cur <= 'Z') || (*cur >= '0' && *cur <= '9') )) {
                    if (ni < (int)sizeof(name)-1) name[ni++] = *cur;
                    cur++;
                }
                if (condTop + 1 >= condStackCap) { condStackCap *= 2; condStack = (int*)realloc(condStack, sizeof(int)*condStackCap); }
                int isdef = pp_find_macro(name) != NULL;
                condStack[++condTop] = currentInclude;
                currentInclude = currentInclude && isdef;
            } else if (strcmp(dir, "ifndef") == 0) {
                char name[256] = {0};
                int ni = 0;
                while (*cur == ' ' || *cur == '\t') cur++;
                while (*cur && ( (*cur == '_') || (*cur >= 'a' && *cur <= 'z') || (*cur >= 'A' && *cur <= 'Z') || (*cur >= '0' && *cur <= '9') )) {
                    if (ni < (int)sizeof(name)-1) name[ni++] = *cur;
                    cur++;
                }
                if (condTop + 1 >= condStackCap) { condStackCap *= 2; condStack = (int*)realloc(condStack, sizeof(int)*condStackCap); }
                int isdef = pp_find_macro(name) != NULL;
                condStack[++condTop] = currentInclude;
                currentInclude = currentInclude && (!isdef);
            } else if (strcmp(dir, "else") == 0) {
                if (condTop >= 0) {
                    // Parent include state is condStack[condTop]
                    int parent = condStack[condTop];
                    // Flip within parent's active region
                    currentInclude = parent && !currentInclude;
                }
            } else if (strcmp(dir, "endif") == 0) {
                if (condTop >= 0) {
                    int parent = condStack[condTop--];
                    currentInclude = parent;
                }
            } else {
                // Unknown directive: ignore
            }
            free(line);
            continue;
        }

        // Non-directive line
        if (currentInclude) {
            char* expanded = pp_expand_macros_line(line);
            size_t need = strlen(expanded) + 1; // include the newline we'll append
            if (outLen + need + 1 > outCap) { outCap = (outLen + need + 1) * 2; out = (char*)realloc(out, outCap); }
            memcpy(&out[outLen], expanded, need - 1);
            outLen += need - 1;
            out[outLen++] = '\n';
            out[outLen] = '\0';
            free(expanded);
        }

        free(line);
    }

    free(condStack);
    return out;
}

char* preprocessFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        preprocessorError("Cannot open file: %s", filename);
        return NULL;
    }
    
    // Simple implementation - just read the file
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    size_t bytesRead = fread(content, 1, size, file);
    content[bytesRead] = '\0';  // Properly null-terminate using actual bytes read
    fclose(file);
    
    // Update current file tracking
    free(state.currentFile);
    state.currentFile = strdup(filename);
    state.line = 1;  // Reset line counter for new file
    
    // Also update the exported interface
    preprocessor.currentFile = state.currentFile;
    preprocessor.fileStackDepth = 1;  // Indicate we're in a file

    // Run preprocessSource on file content to process directives within included files
    char* processed = preprocessSource(content);
    free(content);
    return processed;
}
