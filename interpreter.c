#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "tokenizer.h"
#include "value.h"
#include "linkedlist.h"
#include "parser.h"
#include "talloc.h"
#include "interpreter.h"

/* Justin Lim and Elliot Mawby. CS 251, Dave Musicant. Let/If Assignment.
 * Due May 18, 2015.
 *
 */

int depth;

// Finds the assigned value for a symbol in a specified let frame.
Value *lookUpSymbol(Value *expr, Frame *frame) {
    Frame *current_frame = frame;
    while ((*current_frame).parent != NULL) {
        Value *list = (*current_frame).bindings;
        while ((*list).type != NULL_TYPE) {
            if (!strcmp((*expr).s,(*car(car(list))).s)) {
                return cdr(car(list));
            }
            list = cdr(list);
        }
        current_frame = (*current_frame).parent;
    }
    if ((*current_frame).parent == NULL) {
        printf("Evaluation Error: unassigned variable.\n");
        texit(0);
    }
    return 0;
}

// Helper function: returns true if command
// is available in our list of implemented racket commands and
// throws an error otherwise.
bool commandCheck(Value *expr) {
    bool return_bool = 0;
    if ((*expr).type == CONS_TYPE) {
        if ((*car(expr)).type == SYMBOL_TYPE) {
            if (!strcmp((*car(expr)).s,"if")) {
                return_bool = 1;
            }
            else if (!strcmp((*car(expr)).s,"let")) {
                return_bool = 1;
            }
            else {
                printf("Evaluation Error: command not found.\n");
                texit(0);
            }
        }
        else {
            printf("Evaluation Error: command not found.\n");
            texit(0);
        }
    }   
    else {
        printf("Programmer Error: improper use of commandCheck().\n");
        texit(0);
    }
    return return_bool;
}

// Evaluates if statements (can't evaluate expressions as conditions).
Value *evalIf(Value *args, Frame *frame) {
    Value *list = (*frame).bindings;
    
    // Checks for correct number of arguments in if statment.
    if ((*args).type != CONS_TYPE) {
        printf("Evaluation Error: if command requires 3 arguments.\n");
        texit(0);
    }
    else if ((*car(args)).type == NULL_TYPE || (*cdr(args)).type == NULL_TYPE) {
        printf("Evaluation Error: if command requires 3 arguments.\n");
        texit(0);
    }
    else if ((*car(cdr(args))).type == NULL_TYPE || (*cdr(cdr(args))).type == NULL_TYPE) {
        printf("Evaluation Error: if command requires 3 arguments.\n");
        texit(0);
    }
    // Checks if there are too many arguments.
    if ((*cdr(cdr(cdr(args)))).type != NULL_TYPE) {
        printf("Evaluation Error: if command contains too many arguments.\n");
        texit(0);
    }
    
    Value *condition = car(args);
    
    // Deals with boolean arguments.
    if ((*condition).type == BOOL_TYPE) {
        if ((*condition).i == 1) {
            depth--;
            return eval(car(cdr(args)),frame);
        }
        else {
            depth--;
            return eval(car(cdr(cdr(args))),frame);
        }
    }
    // Deals with variables that are assigned boolean variables.
    else if ((*condition).type == SYMBOL_TYPE) {
        Value *symbol = lookUpSymbol(condition,frame);
        if ((*symbol).type == BOOL_TYPE) {
            if ((*condition).i == 1) {
            return eval(car(cdr(args)),frame);
            }
            else {
                depth--;
                return eval(car(cdr(cdr(args))),frame);
            }
        }
        else {
            printf("Evaluation Error: condition doesn't evaluate to boolean.\n");
            texit(0);
        }
    }
    // Deals with S-expressions as conditions.
    else if ((*condition).type == CONS_TYPE) {
        if (commandCheck(condition)) {
            eval(condition,frame);
        }
    }
    else {
        printf("Evaluation Error: condition doesn't evaluate to boolean.\n");
        texit(0);
    }
    return 0;
}

Value *evalLet(Value *args, Frame *frame) {
    // Create a new pointer, e.
    Value *e = talloc(sizeof(Value));
    (*e).type = PTR_TYPE;
    //Create a new frame.
    Frame *new_frame = talloc(sizeof(Frame));
    
    // Point e towards the current frame.
    (*e).p = frame;
    (*new_frame).parent = (*e).p;
    
    // Error checks for let.
    if ((*args).type != CONS_TYPE) {
        printf("Evaluation Error: let requires a variable assignment and body.\n");
        texit(0);
    }
    
    Value *parameters = car(args);
    // Put contents of let into bindings.
    Value *current_binding = makeNull();
    while ((*parameters).type != NULL_TYPE) {
    
        // Error checks for nested assignments in let.
        if ((*parameters).type != CONS_TYPE || (*car(parameters)).type != CONS_TYPE) {
            printf("Evaluation Error: let requires nested list.\n");
            texit(0);
        }
        if ((*car(car(parameters))).type == NULL_TYPE ||
            (*cdr(car(parameters))).type == NULL_TYPE ||
            (*car(cdr(car(parameters)))).type == NULL_TYPE ||
            (*cdr(cdr(car(parameters)))).type != NULL_TYPE) {
            printf("Evaluation Error: incorrect variable assignment syntax.\n");
            texit(0);
        }
        
        // Find variable and associated value.
        Value *bind_var = car(car(parameters));
        Value *bind_val = car(cdr(car(parameters)));
        
        // Check to see if commands are correct syntax.
        if ((*bind_var).type != SYMBOL_TYPE) {
            printf("Evaluation Error: variable in let not available for assignment.\n");
            texit(0);
        }
        
        // Create cons cells for binding list.
        Value *pair = talloc(sizeof(Value));
        (*pair).type = CONS_TYPE;
        // Add variable and associated value to pair.
        (*pair).c.car = bind_var;
        (*pair).c.cdr = bind_val;
        // Add pair to binding list.
        current_binding = cons(pair, current_binding);
        
        parameters = cdr(parameters);
    }
    // Reverse to put into correct order.
    current_binding = reverse(current_binding);
    // Set binding list to new frame.
    (*new_frame).bindings = current_binding;
    
    // Error check if body of let command exists.
    if ((*cdr(args)).type != CONS_TYPE) {
        printf("Evaluation Error: no body in let.\n");
        texit(0);
    }
    if ((*cdr(cdr(args))).type != NULL_TYPE) {
        printf("Evaluation Error: let command contains too many arguments.\n");
        texit(0);
    }
    
    // Set up body of let frame.
    Value *body = car(cdr(args));
    
    // Error checks contents of body.
    if ((*body).type == CONS_TYPE) {
        commandCheck(body);
    }
    
    // Evaluate body in proper frame.
    Value *return_value = eval(body, new_frame);
    depth--;
    return return_value;
}

void recurseInterpret(Value *tree) {
    Value *val = tree;
    Frame *current_frame = talloc(sizeof(Frame));
    (*current_frame).parent = NULL;
    
    Value *current_binding = makeNull();
    (*current_frame).bindings = current_binding;
    
    Value *return_val = eval(val, current_frame);
}

void interpret(Value *tree) {
    depth = -1;
    if ((*car(tree)).type == CONS_TYPE) {
        commandCheck(car(tree));
    }
    tree = reverse(tree);
    recurseInterpret(tree);
}

Value *eval(Value *expr, Frame *frame) {
    depth++;
    switch (expr->type) {
    case INT_TYPE:
        if (depth == 0) {
            printf("%i\n", (*expr).i);
        }
        break;
    case DOUBLE_TYPE:
        if (depth == 0) {
            printf("%f\n", (*expr).d);
        }
        break;
    case STR_TYPE:
        if (depth == 0) {
            printf("%s\n", (*expr).s);
        }
        break;
    case BOOL_TYPE:
        if (depth == 0) {
            printf("%i\n", (*expr).i);
        }
        break;
    case SYMBOL_TYPE:
        return lookUpSymbol(expr, frame);
        break;
    case CONS_TYPE:
        if ((*car(expr)).type == SYMBOL_TYPE) {
            if (!strcmp((*car(expr)).s,"if")) {
                Value *result = evalIf(cdr(expr),frame);
                display(result);
            }
            else if (!strcmp((*car(expr)).s,"let")) {
                Value *result = evalLet(cdr(expr),frame);
                if ((*result).type != CONS_TYPE) {
                    display(result);
                }
            }
            // Other commands
            else {
                printf("Evaluation Error: command not found.\n");
                texit(0);
            }
        }
        else {
            depth--;
            eval(car(expr),frame);
            depth--;
            eval(cdr(expr),frame);
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
    }
    return expr;
}
