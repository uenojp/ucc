#include "ucc.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("invalid arguments count\n");
        return 1;
    }

    Token* token = tokenize(argv[1]);
    // print_token_chain(token);  // For debug
    Node* node = parse(token);
    codegen(node);

    return 0;
}
