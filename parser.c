#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "value.h"
#include "talloc.h"
#include "linkedlist.h"
#include "tokenizer.h"

/* Justin Lim and Elliot Mawby. CS251, Dave Musicant. Parser Assignment.
 * Due May 11, 2015.
 *
 * Modified printTree() on May 17, 2015.
 *
 */

// Global variables to test whether correct parentheses syntax was used.
// Global variable to help with printing correctly.
int open_paren_count = 0;
int close_paren_count = 0;
// Global variable to check for parentheses syntax errors.
int depth = 0;

bool special_case = 1;

// Helper fuction: adds value to parse tree.
Value *addToTree(Value *list, Value *token) {
    list = cons(car(token),list);
    return list;
}

// Helper function: Recursively goes through parse tree and prints corrently
// formatted Racket code on a single line.
void recursePrint(Value *tree) {
    Value *val = tree;
    switch ((*val).type) {
    case INT_TYPE:
    printf("%i", (*val).i);
    break;
    case DOUBLE_TYPE:
    printf("%f", (*val).d);
    break;
    case STR_TYPE:
    printf("%s", (*val).s);
    break;
    case CONS_TYPE:
    // Prints empty lists.
    if ((*car(val)).type == NULL_TYPE) {
        printf("()");
        open_paren_count++;
        close_paren_count++;
        recursePrint((*val).c.cdr);
    }
    // Tests that print parentheses in the correct places.
    else if ((*car(val)).type == CONS_TYPE) {
        printf("(");
        open_paren_count++;                         
        if ((*cdr(val)).type == NULL_TYPE && (open_paren_count > close_paren_count)) {
            //printf("open paren = %i\n", open_paren_count);
            //printf("close paren = %i\n", close_paren_count);
            close_paren_count++;
            recursePrint((*val).c.car);
            printf(")");
        	
        }
        else {
        	recursePrint((*val).c.car);
        	if ((*cdr(val)).type != NULL_TYPE) {
        	    printf(" ");
        	}
        	recursePrint((*val).c.cdr);
       	}
    }
    else if ((*cdr(val)).type == NULL_TYPE && (open_paren_count > close_paren_count)) {
        //printf("open paren = %i\n", open_paren_count);
        //printf("close paren = %i\n", close_paren_count);
        close_paren_count++;
        recursePrint((*val).c.car);
        printf(")");
        
    }
    else {
        recursePrint((*val).c.car);
        if ((*cdr(val)).type != NULL_TYPE) {
        	printf(" ");
        }
        recursePrint((*val).c.cdr);
    }
    break;
    case NULL_TYPE:
    break;
    case PTR_TYPE:
    break;
    case OPEN_TYPE:
    break;
    case CLOSE_TYPE:
    break;
    case BOOL_TYPE:
    printf("%i", (*val).i);
    break;
    case SYMBOL_TYPE:
    printf("%s", (*val).s);
    break;
    case VOID_TYPE:
    break;
    case CLOSURE_TYPE:
    break;
    }
}

// Prints the tree to the screen in a readable fashion. It should look just like
// Racket code; use parentheses to indicate subtrees.
void printTree(Value *tree) {
    Value *reverse_tree = reverse(tree);
    recursePrint(reverse_tree);
}
    
// Helper fuction: Takes a list of tokens from a Racket program, and returns a pointer to a
// parse tree representing that program.
Value *makeParseTree(Value *tokens) {
    // Initialize the output tree.
    Value *tree = makeNull();
    // Initialize an intermediate tree used when you hit a close parenthese.    
    Value *concat_list_head = makeNull();
    Value *cur_token = tokens;
    if ((*cur_token).type == NULL_TYPE) {
        return tree;
    }

    while ((*cur_token).type != NULL_TYPE) {
        // When an open paren is found:
        if ((*car(cur_token)).type == OPEN_TYPE) {
            //Add one to the depth counter when we hit an open paren.
            depth++;
            // Create a CONS cell.
            Value *item = talloc(sizeof(Value));
            (*item).type = CONS_TYPE;
            // Recurse CDR of current token list.
            Value *return_item = makeParseTree((cdr(cur_token)));
            Value *rev_item = reverse(return_item);
            (*item).c.car = rev_item;
            
            // Add returned recursive list to tree.
            tree = addToTree(tree,item);
            // Move to next token.
            cur_token = cdr(cur_token);
            
            // Deal with skipping tokens that have already been recursed over.
            int paren = 0;
            // Loop that finds the correct token to start at after 
            while ((*car(cur_token)).type != CLOSE_TYPE || paren >= 1) {
                // A close paren has been found.
                //display(car(cur_token));
                if ((*car(cur_token)).type == OPEN_TYPE) {
                    paren++;
                }
                else if ((*car(cur_token)).type == CLOSE_TYPE) {
                    paren--;

                }
                if ((*cdr(cur_token)).type != NULL_TYPE) {
                	
                	cur_token = cdr(cur_token);
               	}
               	else {
               		printf("Syntax error: Too Many Open Parentheses.\n");
    				texit(0);
    			}
            }
            if ((*cdr(cur_token)).type != NULL_TYPE) {
                cur_token = cdr(cur_token);
            }
            else {
            	return tree;
           	}
        }
        // Concatinates tokens to list once a close paren has been reached.
        else if ((*car(cur_token)).type == CLOSE_TYPE) {
            // If there are more close parens than open parens then exit.
            if (depth < 1) {
            	printf("Syntax error: Too Many Close Parentheses.\n");
            	texit(0);
           	}
           	// Decrement depth when we find a valid close paren.
           	depth--;
            while ((*tree).type == CONS_TYPE) {
                // Create the block of code to be returned.
                concat_list_head = addToTree(concat_list_head,tree);
                tree = cdr(tree);
            }
            // Reverse the list to put it into correct order.
            concat_list_head = reverse(concat_list_head); 
            return concat_list_head;
        } 
        else {
            // Add the token to tree and move to the next token.
            tree = addToTree(tree,cur_token);
            cur_token = cdr(cur_token);
        }
    }
    
    // If there are more open parens than close parens at the end of the file.
    if (depth != 0) {
    	printf("Syntax error: Too Many Open Parentheses.\n");
    	texit(0);
   	} 
   	else {
    	return tree;
    }
    
    return 0;
}

// Main parse function. Tests whether input is NULL, then calls makeParseTree(),
// which is a recursive subroutine that creates a parse tree from given tokens.
Value *parse(Value *tokens) {
    Value *return_tree = tokens;
    if ((*return_tree).type == NULL_TYPE) {
        printf("Syntax Error: Empty File.\n");
        texit(0);
    }
    else {
    	return_tree = makeParseTree(return_tree);
    }
    return return_tree;
}


