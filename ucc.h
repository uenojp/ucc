#pragma once

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Tokenizer
//

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

bool consume(Token** tok, const char* op);
int expect_number();
Token* new_token(TokenKind kind, int val, char* str, int len);
Token* tokenize();
void error(const char* fmt, ...);
void error_at(const char* loc, const char* fmt, ...);
void print_token_chain(Token* tok);

//
// Parser
//

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

Node* parse(Token* tok);

//
// Code generator
//

void codegen_expr(const Node* node);
void codegen(const Node* node);
