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

// Evaluates if statements (can't evaluate expressions as parameters).
Value *evalIf(Value *args, Frame *frame) {
    Value *list = (*frame).bindings;
    Value *test = car(args);
    if ((*test).type == BOOL_TYPE) {
        if ((*test).i == 1) {
            return eval(car(cdr(args)),frame);
        }
        else {
            return eval(car(cdr(cdr(args))),frame);
        }
    }
    
    // Will eventually deal with nested S-expressions.
    else if ((*test).type == CONS_TYPE) {
        return car(cdr(args));
    }
    return 0;
}

Value *evalLet(Value *args, Frame *frame) {
    Value *parameters = car(args);

    // Create a new pointer, e.
    Value *e = talloc(sizeof(Value));
    (*e).type = PTR_TYPE;
    //Create a new frame.
    Frame *new_frame = talloc(sizeof(Frame));
    
    // Point e towards the current frame.
    (*e).p = frame;
    (*new_frame).parent = (*e).p;
    
    // Put contents of let into bindings.
    Value *current_binding = makeNull();
    Value *new_binding = makeNull();
    while ((*parameters).type != NULL_TYPE) {
        // Find variable and associated value.
        Value *bind_var = car(car(parameters));
        Value *bind_val = car(cdr(car(parameters)));
        
        // Check to see if commands are correct syntax.
        if ((*bind_var).type != SYMBOL_TYPE) {
            printf("Incorrect Syntax in let.\n");
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

    // Set up body of let frame.
    Value *body = car(cdr(args));
    if ((*body).type == NULL_TYPE) {
        printf("Syntax Error: No body in let.\n");
        texit(0);
    }
    
    // Evaluate body in proper frame.
    Value *return_value = eval(body, new_frame);
    return return_value;
}
    
// Finds the assigned value for a symbol in a specified let frame.
Value *lookUpSymbol(Value *expr, Frame *frame) {
    Value *list = (*frame).bindings;
    while ((*list).type != NULL_TYPE) {
        if (!strcmp((*expr).s,(*car(car(list))).s)) {
            return cdr(car(list));
        }
        list = cdr(list);
    }
    return expr;
}

Value *evaluationError() {
    Value *return_val = talloc(sizeof(Value));
    (*return_val).type = NULL_TYPE;
    return return_val;
}

void interpret(Value *tree) {
    Value *val = tree;
    Frame *current_frame = talloc(sizeof(Frame));
    (*current_frame).parent = NULL;
    
    Value *current_binding = makeNull();
    (*current_frame).bindings = current_binding;
    
    Value *return_val = eval(val, current_frame);
}

Value *eval(Value *expr, Frame *frame) {
    switch (expr->type)  {
    case INT_TYPE:
        printf("%i\n", (*expr).i);
        break;
    case DOUBLE_TYPE:
        printf("%f\n", (*expr).d);
        break;
    case STR_TYPE:
        printf("%s\n", (*expr).s);
        break;
    case BOOL_TYPE:
        printf("%i\n", (*expr).i);
        break;
    case SYMBOL_TYPE:
        return lookUpSymbol(expr, frame);
        break;
    case CONS_TYPE:  
        
        // Sanity and error checking on first...
        
        if (!strcmp((*car(expr)).s,"if")) {
            Value *result = evalIf(cdr(expr),frame);
            display(result);
        }
        else if (!strcmp((*car(expr)).s,"let")) {
            Value *result = evalLet(cdr(expr),frame);
        }
        else {
            // not a recognized special form
            evaluationError();
            
            interpret(car(expr));
            interpret(cdr(expr));
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
