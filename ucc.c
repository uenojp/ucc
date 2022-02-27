#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Kind of token */
typedef enum {
    TK_RESERVED,  // Symbol
    TK_NUM,       // Number token
    TK_EOF,       // Token for end of file
} TokenKind;

/* Token */
typedef struct Token Token;
struct Token {
    TokenKind kind;
    Token* next;  // Pointer to next token
    int val;      // Value for number token
    char* str;    // Token string
};

void error(const char* fmt, ...);
void error_at(const char* loc, const char* fmt, ...);

void print_token_chain(Token* tok);

/* Input program */
char* user_input;

/* Current Token */
Token* token;

/* Transit the next token if the current token is an operator
 * and return whether it's possible to transit or not. */
bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        return false;
    /* Transit next token if it's   */
    token = token->next;

    return true;
}

/* Transit the next token and return a value of the current token if it's a number token.
 * If not, report an error. */
int expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "expected a number");
    int val = token->val;
    token = token->next;

    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

/* Create new tokne and return it. */
Token* new_token(TokenKind kind, int val, char* str) {
    Token* tok = calloc(1, sizeof(Token));
    if (tok == NULL) {
        perror("new_token: calloc");
        return NULL;
    }
    tok->kind = kind;
    tok->val = val;
    tok->str = str;

    return tok;
}

/* Tokenize 'user_input' and return the head of token chain. */
Token* tokenize() {
    char* p = user_input;
    Token head;  // temporary head
    head.next = NULL;
    Token* cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-') {
            cur->next = new_token(TK_RESERVED, -1, p++);
            if (cur->next == NULL)
                goto err;
            cur = cur->next;
            continue;
        }

        if (isdigit(*p)) {
            /* Token.str is the string after p
             * 1 + 12 - 4
             *     ↑
             *     p
             * str is "12 - 4" (NOT "12") */
            char* tmp = p;  // Because the order of evaluation of the arguments is unspecified.
            cur->next = new_token(TK_NUM, strtol(p, &p, 10), tmp);
            if (cur->next == NULL)
                goto err;
            cur = cur->next;
            continue;
        }
    err:
        error_at(p, "failed to tokenize");
    }
    cur->next = new_token(TK_EOF, -1, p);

    return head.next;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("invalid arguments count\n");
        return 1;
    }

    user_input = argv[1];
    token = tokenize();
    // print_token_chain(token); // For debug

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("\n");
    printf("main:\n");
    printf("    mov rax, %d\n", expect_number());
    while (!at_eof()) {
        if (consume('+')) {
            printf("    add rax, %d\n", expect_number());
            continue;
        }
        if (consume('-')) {
            printf("    sub rax, %d\n", expect_number());
            continue;
        }
    }
    printf("    ret\n");

    return 0;
}

/* Report an error and exit. */
void error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

/* Report an error location and exit. */
void error_at(const char* loc, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

/* Print token chain from tok. */
void print_token_chain(Token* tok) {
    for (Token* cur = tok; cur->kind != TK_EOF; cur = cur->next) {
        char* kind;
        if (cur->kind == TK_RESERVED)
            kind = "RESERVED";
        else if (cur->kind == TK_NUM)
            kind = "NUM";
        else if (cur->kind == TK_EOF)
            kind = "EOF";
        else
            kind = "UNKNOWN";
        printf("(%s, %d, \'%s\') => ", kind, cur->val, cur->str);
    }
    printf("EOF\n");
}