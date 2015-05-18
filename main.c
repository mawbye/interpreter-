#include <stdio.h>
#include "tokenizer.h"
#include "value.h"
#include "linkedlist.h"
#include "parser.h"
#include "talloc.h"
#include "interpreter.h"

int main() {

    Value *list = tokenize(stdin);
    Value *tree = parse(list);
    
    printTree(tree);
    printf("\n\n");
    
    tree = reverse(tree);
    interpret(tree);

    tfree();
    return 0;
}