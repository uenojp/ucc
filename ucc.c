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
    int len;      // Token length
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
bool consume(const char* op) {
    if (token->kind != TK_RESERVED || token->len != strlen(op) ||
        strncmp(token->str, op, token->len))
        return false;
    token = token->next;

    return true;
}

/* Transit the next token and return a value of the current token if it's a
 * number token. If not, report an error. */
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
Token* new_token(TokenKind kind, int val, char* str, int len) {
    Token* tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->val = val;
    tok->str = str;
    tok->len = len;

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
            char* const tmp = p;
            const int val = strtol(p, &p, 10);
            const int len = p - tmp;
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

/* Grammer
 * expr     := equality
 * equality := relation ("==" relation | "!=" relation)*
 * relation := add ("<" add | "<=" add | ">" add | ">=" add)*
 * add      := mul ("+" mul | "-" mul)*
 * mul      := unary ("*" unary | "/" unary)*
 * unary    := ("+" | "-")? primary
 * primary  := num | "(" expr ")" */

/* Kind of AST node */
typedef enum {
    ND_ADD,  // +
    ND_SUB,  // -
    ND_MUL,  // *
    ND_DIV,  // /
    ND_EQ,   // ==
    ND_NE,   // !=
    ND_LT,   // <
    ND_LE,   // <=
    /* GT and GE are implemented by swapping both sides in parsing(func relation). */
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
Node* equality();
Node* relation();
Node* add();
Node* mul();
Node* unary();
Node* primary();

/* expr := equality */
Node* expr() {
    return equality();
}

/* equality := relation ("==" relation | "!=" relation)* */
Node* equality() {
    Node* node = relation();
    for (;;) {
        if (consume("=="))
            node = new_node(ND_EQ, node, relation());
        else if (consume("!="))
            node = new_node(ND_NE, node, relation());
        else
            return node;
    }
}

/* relation := add ("<" add | "<=" add | ">" add | ">=" add)* */
Node* relation() {
    Node* node = add();
    for (;;) {
        if (consume("<"))
            node = new_node(ND_LT, node, add());
        else if (consume("<="))
            node = new_node(ND_LE, node, add());
        else if (consume(">"))
            node = new_node(ND_LT, add(), node);
        else if (consume(">="))
            node = new_node(ND_LE, add(), node);
        else
            return node;
    }
}

/* add := mul ("+" mul | "-" mul)* */
Node* add() {
    Node* node = mul();
    for (;;) {
        if (consume("+"))
            node = new_node(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

/* mul := unary ("*" unary | "/" unary)* */
Node* mul() {
    Node* node = unary();
    for (;;) {
        if (consume("*"))
            node = new_node(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

/* unary := ("+" | "-")? primary */
Node* unary() {
    if (consume("+"))
        return primary();
    if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), primary());

    return primary();
}

/* primary := "(" expr ")" | num */
Node* primary() {
    if (consume("(")) {
        Node* node = expr();
        consume(")");
        return node;
    }

    return new_node_num(expect_number());
}

/* Generate code emulating a stack machine. */
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

    if (node->kind == ND_EQ) {
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
    }
    if (node->kind == ND_NE) {
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
    }
    if (node->kind == ND_LT) {
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
    }
    if (node->kind == ND_LE) {
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
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
        printf("(%s, %d, \"%.*s\") => ", kind, cur->val, cur->len, cur->str);
    }
    printf("EOF\n");
}
