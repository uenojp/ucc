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

/* Input program */
char* user_input;

/* Current Token */
Token* token;

/* Report an error and exit. */
void error(char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

/* Report an error location and exit. */
void error_at(char* loc, char* fmt, ...) {
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

/* Transit the next token if the current token is an operator
 * and return whether it's possible to transit or not */
bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        return false;
    /* Transit next token if it's   */
    token = token->next;

    return true;
}

/* Transit the next token if the current token is an operator. */
void expect(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op)
        error_at(token->str, "invalid operator '%c", op);
    token = token->next;
}

/* Transit the next token and return a value of the current token if it's a number token.
 * If not, report an error. */
int expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "not a number");
    int val = token->val;
    token = token->next;

    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

/* New token with kind and str, and connect it to cur.
 * cur -> tok(new one with kind, str) */
Token* new_token(TokenKind kind, Token* cur, char* str) {
    Token* tok = calloc(1, sizeof(Token));
    if (tok == NULL) {
        perror("new_token: calloc:");
        return NULL;
    }
    tok->kind = kind;
    tok->str = str;  // should strdup??
    cur->next = tok;

    return tok;
}

/* Tokenize p and return the head.next token. */
Token* tokenize() {
    char* p = user_input;
    Token head;
    head.next = NULL;
    Token* cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        if (isdigit(*p)) {
            /* str of the number token is the string after p.
             * 1 + 12 - 4
             *     ↑
             *     p
             * token{..., val: 2, str: "12 - 4"}
             * NOT str: "12" */
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }
        error_at(p, "failed to tokenize");
    }
    new_token(TK_EOF, cur, p);

    return head.next;
}

/* Show token chain from tok. */
void show_token_chain(Token* tok) {
    Token* cur = tok;
    while (cur->kind != TK_EOF) {
        printf("val: %d str: %s\n", cur->val, cur->str);
        cur = cur->next;
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("invalid arguments count\n");
        return 1;
    }

    user_input = argv[1];
    token = tokenize();  // return head.next
    // show_token_chain(token);

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

        expect('-');  // For now, 'expect' can be replaced byconsume.
        printf("    sub rax, %d\n", expect_number());
    }
    printf("    ret\n");

    return 0;
}
