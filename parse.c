#include "ucc.h"

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

/* Grammer
 * expr     := equality
 * equality := relation ("==" relation | "!=" relation)*
 * relation := add ("<" add | "<=" add | ">" add | ">=" add)*
 * add      := mul ("+" mul | "-" mul)*
 * mul      := unary ("*" unary | "/" unary)*
 * unary    := ("+" | "-")? primary
 * primary  := num | "(" expr ")" */
Node* expr(Token** tok);
Node* equality(Token** tok);
Node* relation(Token** tok);
Node* add(Token** tok);
Node* mul(Token** tok);
Node* unary(Token** tok);
Node* primary(Token** tok);

/* expr := equality */
Node* expr(Token** tok) {
    return equality(tok);
}

/* equality := relation ("==" relation | "!=" relation)* */
Node* equality(Token** tok) {
    Node* node = relation(tok);
    for (;;) {
        if (consume(tok, "=="))
            node = new_node(ND_EQ, node, relation(tok));
        else if (consume(tok, "!="))
            node = new_node(ND_NE, node, relation(tok));
        else
            return node;
    }
}

/* relation := add ("<" add | "<=" add | ">" add | ">=" add)* */
Node* relation(Token** tok) {
    Node* node = add(tok);
    for (;;) {
        if (consume(tok, "<"))
            node = new_node(ND_LT, node, add(tok));
        else if (consume(tok, "<="))
            node = new_node(ND_LE, node, add(tok));
        else if (consume(tok, ">"))
            node = new_node(ND_LT, add(tok), node);
        else if (consume(tok, ">="))
            node = new_node(ND_LE, add(tok), node);
        else
            return node;
    }
}

/* add := mul ("+" mul | "-" mul)* */
Node* add(Token** tok) {
    Node* node = mul(tok);
    for (;;) {
        if (consume(tok, "+"))
            node = new_node(ND_ADD, node, mul(tok));
        else if (consume(tok, "-"))
            node = new_node(ND_SUB, node, mul(tok));
        else
            return node;
    }
}

/* mul := unary ("*" unary | "/" unary)* */
Node* mul(Token** tok) {
    Node* node = unary(tok);
    for (;;) {
        if (consume(tok, "*"))
            node = new_node(ND_MUL, node, unary(tok));
        else if (consume(tok, "/"))
            node = new_node(ND_DIV, node, unary(tok));
        else
            return node;
    }
}

/* unary := ("+" | "-")? primary */
Node* unary(Token** tok) {
    if (consume(tok, "+"))
        return primary(tok);
    if (consume(tok, "-"))
        return new_node(ND_SUB, new_node_num(0), primary(tok));

    return primary(tok);
}

/* primary := "(" expr ")" | num */
Node* primary(Token** tok) {
    if (consume(tok, "(")) {
        Node* node = expr(tok);
        consume(tok, ")");
        return node;
    }

    return new_node_num(expect_number(tok));
}

/* Psrse the toknes and return the AST root. */
Node* parse(Token* tok) {
    Node* node = expr(&tok);
    if (tok->kind != TK_EOF)
        error("failed to parse");
    return node;
}
