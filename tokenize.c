#include "ucc.h"

static char* input_program;

/* Transit the next token if the current token is an operator
 * and return whether it's possible to transit or not. */
bool consume(Token** tok, const char* op) {
    if ((*tok)->kind != TK_RESERVED || (*tok)->len != strlen(op) ||
        strncmp((*tok)->str, op, (*tok)->len))
        return false;
    (*tok) = (*tok)->next;

    return true;
}

/* Transit the next token and return a value of the current token if it's a
 * number token. If not, report an error. */
int expect_number(Token** tok) {
    if ((*tok)->kind != TK_NUM)
        error_at((*tok)->str, "expected a number");
    int val = (*tok)->val;
    (*tok) = (*tok)->next;

    return val;
}

/* Create new tokne and return it. */
Token* new_token(TokenKind kind, int val, char* str, int len) {
    Token* tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->val = val;
    tok->str = str;
    tok->len = len;

    return tok;
}

/* Tokenize 'p' and return the head of token chain. */
Token* tokenize(char* p) {
    input_program = p;
    Token head;  // temporary head
    head.next = NULL;
    Token* cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        int len = 2;  // Token length like "<=" and "=="
        if (!strncmp("==", p, len) || !strncmp("!=", p, len) || !strncmp("<=", p, len) ||
            !strncmp(">=", p, len)) {
            cur->next = new_token(TK_RESERVED, -1, p, len);
            p += len;
            if (cur->next == NULL)
                goto err;
            cur = cur->next;
            continue;
        }

        len = 1;  // Token length like "<", "+" and "("
        if (strchr("+-*/()<>", *p)) {
            cur->next = new_token(TK_RESERVED, -1, p++, len);
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
            char* tmp = p;
            int val = strtol(p, &p, 10);
            int len = p - tmp;
            cur->next = new_token(TK_NUM, val, tmp, len);
            if (cur->next == NULL)
                goto err;
            cur = cur->next;
            continue;
        }
    err:
        error_at(p, "failed to tokenize");
    }
    cur->next = new_token(TK_EOF, -1, p, -1);

    return head.next;
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

    int pos = loc - input_program;
    fprintf(stderr, "%s\n", input_program);
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
        printf("(%s, %d, \"%.*s\") => ", kind, cur->val, cur->len, cur->str);
    }
    printf("EOF\n");
}
