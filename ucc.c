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

        if (strchr("+-*/()", *p)) {
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

/* Grammer
 * expr    := mul ("+" mul | "-" mul)
 * mul     := primary ("*" primary | "/" primary)
 * primary := num | "(" expr ")" */

/* Kind of AST node */
typedef enum {
    ND_ADD,  // +
    ND_SUB,  // -
    ND_MUL,  // *
    ND_DIV,  // /
    ND_NUM,  // Integer
} NodeKind;

/* AST node */
typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node* lhs;
    Node* rhs;
    int val;  // Only if kind == ND_NUM
};

Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;

    return node;
}

Node* new_node_num(int val) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    node->lhs = NULL;
    node->rhs = NULL;

    return node;
}

Node* expr();
Node* mul();
Node* primary();

void print_AST(const Node* node);

/* expr := mul ("+" mul | "-" mul) */
Node* expr() {
    Node* node = mul();
    for (;;) {
        if (consume('+')) {
            node = new_node(ND_ADD, node, mul());
        } else if (consume('-')) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

/* mul := primary ("*" primary | "/" primary) */
Node* mul() {
    Node* node = primary();
    for (;;) {
        if (consume('*')) {
            node = new_node(ND_MUL, node, primary());
        } else if (consume('/')) {
            node = new_node(ND_DIV, node, primary());
        } else {
            return node;
        }
    }
}

/* primary := "(" expr ")" | num */
Node* primary() {
    if (consume('(')) {
        Node* node = expr();
        consume(')');
        return node;
    }

    return new_node_num(expect_number());
}

void gen(const Node* node) {
    if (node == NULL)
        return;

    gen(node->lhs);
    gen(node->rhs);

    if (node->kind == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    printf("    pop rdi\n");
    printf("    pop rax\n");
    if (node->kind == ND_ADD)
        printf("    add rax, rdi\n");
    if (node->kind == ND_SUB)
        printf("    sub rax, rdi\n");
    if (node->kind == ND_MUL)
        printf("    imul rax, rdi\n");
    if (node->kind == ND_DIV) {
        printf("    cqo\n");
        printf("    idiv rdi\n");
    }
    printf("    push rax\n");
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("invalid arguments count\n");
        return 1;
    }

    user_input = argv[1];
    token = tokenize();
    // print_token_chain(token);  // For debug

    Node* node = expr();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("\n");
    printf("main:\n");
    gen(node);
    printf("    pop rax\n");
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
        printf("(%s, %d, \'%c\') => ", kind, cur->val, cur->str[0]);
    }
    printf("EOF\n");
}
