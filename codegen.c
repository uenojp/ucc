#include "ucc.h"

/* Generate code emulating a stack machine. */
void codegen_expr(const Node* node) {
    if (node == NULL)
        return;

    codegen_expr(node->lhs);
    codegen_expr(node->rhs);

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

void codegen(const Node* node) {
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("\n");
    printf("main:\n");
    codegen_expr(node);
    printf("    pop rax\n");
    printf("    ret\n");
}