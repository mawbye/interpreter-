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
 * Modified May 20, 2015.
 * and May 26, 2015.
 *
 */


// ***********************************************************
// ******************** HELPER FUNCTIONS: ********************
// ***********************************************************

// Finds the assigned value for a symbol.
Value *lookUpSymbol(Value *expr, Frame *frame) {
    Frame *current_frame = frame;
    // While we aren't looking at the top frame:
    while (current_frame != NULL) {
        Value *list = (*current_frame).bindings;
        // Go through the list of variables for that frame:
        while ((*list).type != NULL_TYPE) {
            if (!strcmp((*expr).s,(*car(car(list))).s)) {
                return cdr(car(list));
            }
            list = cdr(list);
        }
        // Move to the previous frame:
        current_frame = (*current_frame).parent;
    }
    if (current_frame == NULL) {
        printf("Evaluation Error: unassigned variable.\n");
        texit(0);
    }
    return 0;
}

// Binds a variable and value together.
// Returns Value pointing to linked list.
Value *bind(Value *bind_var, Value *bind_val, Value *return_list) {
    // Create cons cells for binding list.
    Value *pair = talloc(sizeof(Value));
    (*pair).type = CONS_TYPE;
    // Add variable and associated value to pair.
    (*pair).c.car = bind_var;
    (*pair).c.cdr = bind_val;
    // Add pair to return list.
    return_list = cons(pair, return_list);
    return return_list;
}

// Binds primitive functions to top frame.
void primBind(char *name, Value *(*function)(struct Value *), Frame *frame) {
    // creates a primitive type Value
    Value *value = talloc(sizeof(Value));
    (*value).type = PRIMITIVE_TYPE;
    (*value).pf = function;
    // Binds value to top frame
    Value *name_val = talloc(sizeof(Value));
    (*name_val).type = SYMBOL_TYPE;
    (*name_val).s = name;
    (*frame).bindings = bind(name_val,value,(*frame).bindings);
}

// Helper evaluates a list of arguments.
Value *evalEach(Value *args, Frame *frame) {
    Value *result = talloc(sizeof(Value));
    result = makeNull();
    while ((*args).type != NULL_TYPE) {
        if((*args).type == CONS_TYPE) {
            Value *evaled_args = eval(car(args),frame);
            result = cons(evaled_args,result);
        }
        args = cdr(args);
    }
    result = reverse(result);
    return result;
}

// ********************************************************
// ******************** Special Forms: ********************
// ********************************************************

// ******************** Evaluates if statements. ********************
Value *evalIf(Value *args, Frame *frame) {
    Frame *current_frame = frame;
    Value *return_value = makeNull();
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
    // Evaluates conditions that are not booleans.
    while ((*condition).type != BOOL_TYPE) {
        if ((*condition).type == NULL_TYPE || (*condition).type == INT_TYPE
                                           || (*condition).type == DOUBLE_TYPE
                                           || (*condition).type == STR_TYPE) {
            printf("Evaluation Error: condition doesn't evaluate to boolean.\n");
            texit(0);
        }
        else if ((*condition).type == SYMBOL_TYPE) {
            condition = eval(condition,current_frame);
        }
        else if ((*condition).type == CONS_TYPE) {
            // Deals with commands/cases that return a CONS_TYPE when evaluated.
            Value *sub = eval(condition,current_frame);
            if ((*sub).type == CONS_TYPE) {
                condition = car(sub);
            }
            else {
                condition = sub;
            }
        }
    }
    
    // Evaluates conditions that are booleans.
    if ((*condition).type == BOOL_TYPE) {
        if ((*condition).i == 1) {
            
            return_value = car(cdr(args));
        }
        else {
            return_value = car(cdr(cdr(args)));
        }
    }
    
    // Determines whether to print symbol or not.
    if ((*return_value).type == SYMBOL_TYPE) {
        return_value = eval(return_value,current_frame);
    }
    else if ((*return_value).type == CONS_TYPE) {
        return_value = eval(return_value,current_frame);
    }
    return return_value;
}

// ******************** Evaluates Let commands. ********************
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
        
        // Evaluates commands in variable assignment.
        if ((*bind_val).type == CONS_TYPE) {
            bind_val = eval(bind_val,frame);
        }
        
        // Bind pair.
        current_binding = bind(bind_var,bind_val,current_binding);
        
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
    
    // Evaluate body in proper frame.
    Value *return_value = eval(body, new_frame);
    return return_value;
}

// ******************** Evaluates quote commands ********************
Value *evalQuote(Value *args, Frame *frame) {
    Value *result = args;
    if ((*car(result)).type != CONS_TYPE && (*car(result)).type != NULL_TYPE) {
        result = car(args);
    }
    return result;
}

// ******************** Evaluates Define commands. ********************
Value *evalDefine(Value *args, Frame *frame) {
    // Error checks for define.
    if ((*car(args)).type != SYMBOL_TYPE) {
        printf("Evaluation Error: incorrect define syntax.\n");
        texit(0);
    }
    // Checks if body of define exists.
    if ((*cdr(args)).type != CONS_TYPE) {
        printf("Evaluation Error: incorrect define syntax.\n");
        texit(0);
    }
    // CHecks if body of define exists.
    if ((*car(cdr(args))).type == NULL_TYPE) {
        printf("Evaluation Error: incorrect define syntax.\n");
        texit(0);
    }
    // Creates top frame and void return Value.
    Frame *top_frame = frame;
    Value *return_value = talloc(sizeof(Value));
    (*return_value).type = VOID_TYPE;
    
    // Bind command to top frame.
    Value *var = car(args);
    Value *val = eval(car(cdr(args)),frame);
    (*top_frame).bindings = bind(var,val,(*top_frame).bindings);
    
    return return_value;
}

// ******************** Evaluates Lambda commands. ********************
Value *evalLambda(Value *args, Frame *frame) {
    if ((*args).type != CONS_TYPE) {
        printf("Evaluation Error: incorrect lambda syntax.\n");
        texit(0);
    } 
    if ((*cdr(args)).type != CONS_TYPE) {
        printf("Evaluation Error: incorrect lambda syntax.\n");
        texit(0);
    }
    if ((*car(cdr(args))).type == NULL_TYPE) {
        printf("Evaluation Error: incorrect lambda syntax.\n");
        texit(0);
    }
    if ((*car(cdr(args))).type == CONS_TYPE) {
        if ((*car(car(cdr(args)))).type == NULL_TYPE) {
            printf("Evaluation Error: incorrect lambda syntax.\n");
            texit(0);
        }
    }
    
    // Creates closure and adds necessary attributes to closure.
    Value *closure = talloc(sizeof(Value));
    (*closure).type = CLOSURE_TYPE;
    (*closure).cl.paramNames = car(args);
    (*closure).cl.functionCode = cdr(args);

    (*closure).cl.frame = frame;
    
    return closure;
}

// **********************************************************
// ******************** Primitive Types: ********************
// **********************************************************

// ******************** Evaluates + commands. ********************
Value *evalPlus(Value *args) {
    // Final return value.
    double return_integer = 0;
    // Tracks whether return value should be int or double.
    bool return_real = 0;
    Value *parameters = args;
    
    // Goes through arguments.
    while ((*parameters).type != NULL_TYPE) {
        // Tracks individual argument type.
        bool real_number = 0;
        double new_integer = 0;
      
        // Checks current argument's type.
        if ((*car(parameters)).type == DOUBLE_TYPE) {
            real_number = 1;
            return_real = 1;
        }
        // Error check for non-number arguments.
        if ((*car(parameters)).type != INT_TYPE && (*car(parameters)).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in + command.\n");
            texit(0);
        }
        // Places value into correct type.
        if (real_number) {
            new_integer = (*car(parameters)).d;
        }
        else {
            new_integer = (*car(parameters)).i;
        }
        // Does math.
        return_integer = return_integer + new_integer;
        // Moves to next argument.
        parameters = cdr(parameters);
    }
    // Creates result value.
    Value *result = talloc(sizeof(Value));
    // Extra: tracks final return value type.
    if (return_real) {
        (*result).type = DOUBLE_TYPE;
        (*result).d = return_integer;
    }
    else {
        (*result).type = INT_TYPE;
        (*result).i = (int) return_integer;
    }
    return result;
}
// Returns fuction evalPlus().
Value *primPlus(Value *args) {
    return evalPlus(args); 
}

// ******************** Evaluates - commands. ********************
Value *evalMinus(Value *args) {
    // Tracks whether return value should be int or double.
    bool return_real = 0;
    Value *parameters = args;
    
    // Final return value.
    double return_integer;
    if ((*car(parameters)).type == DOUBLE_TYPE) {
        return_real = 1;
        return_integer = (*car(parameters)).d;
    }
    else {
        return_integer = (*car(parameters)).i;
    }
    parameters = cdr(parameters);
    
    // Goes through arguments.
    while ((*parameters).type != NULL_TYPE) {
        // Tracks individual argument type.
        bool real_number = 0;
        double new_integer = 0;
        
        // Checks current argument's type.
        if ((*car(parameters)).type == DOUBLE_TYPE) {
            real_number = 1;
            return_real = 1;
        }
        
        // Error check for non-number arguments.
        if ((*car(parameters)).type != INT_TYPE && (*car(parameters)).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in - command.\n");
            texit(0);
        }
        // Places value into correct type.
        if (real_number) {
            new_integer = (*car(parameters)).d;
        }
        else {
            new_integer = (*car(parameters)).i;
        }
        // Does math.
        return_integer = return_integer - new_integer;
        // Moves to next argument.
        parameters = cdr(parameters);
    }
    // Creates result value.
    Value *result = talloc(sizeof(Value));
    // Extra: tracks final return value type.
    if (return_real) {
        (*result).type = DOUBLE_TYPE;
        (*result).d = return_integer;
    }
    else {
        (*result).type = INT_TYPE;
        (*result).i = (int) return_integer;
    }
    return result;
}
// Returns function evalMinus().
Value *primMinus(Value *args) {
    return evalMinus(args); 
}

// ******************** Evaluates * commands. ********************
Value *evalMult(Value *args) {
    // Final return value.
    double return_integer = 1;
    // Tracks whether return value should be int or double.
    bool return_real = 0;
    Value *parameters = args;
    
    // Goes through arguments.
    while ((*parameters).type != NULL_TYPE) {
        // Tracks individual argument type.
        bool real_number = 0;
        double new_integer = 0;
       
        // Checks current argument's type.
        if ((*car(parameters)).type == DOUBLE_TYPE) {
            real_number = 1;
            return_real = 1;
        }
        // Error check for non-number arguments.
        if ((*car(parameters)).type != INT_TYPE && (*car(parameters)).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in * command.\n");
            texit(0);
        }
        // Places value into correct type.
        if (real_number) {
            new_integer = (*car(parameters)).d;
        }
        else {
            new_integer = (*car(parameters)).i;
        }
        // Does math.
        return_integer = return_integer * new_integer;
        // Moves to next argument.
        parameters = cdr(parameters);
    }
    // Creates result value.
    Value *result = talloc(sizeof(Value));
    // Extra: tracks final return value type.
    if (return_real) {
        (*result).type = DOUBLE_TYPE;
        (*result).d = return_integer;
    }
    else {
        (*result).type = INT_TYPE;
        (*result).i = (int) return_integer;
    }
    return result;
}
// Returns function evalMult().
Value *primMult(Value *args) {
    return evalMult(args); 
}

// ******************** Evaluates / commands. ********************
Value *evalDiv(Value *args) {
    // Tracks whether return value should be int or double.
    bool return_real = 0;
    Value *parameters = args;
    
    // Final return value.
    double return_integer;
    if ((*car(parameters)).type == DOUBLE_TYPE) {
        return_real = 1;
        return_integer = (*car(parameters)).d;
    }
    else {
        return_integer = (*car(parameters)).i;
    }
    parameters = cdr(parameters);
    
    // Goes through arguments.
    while ((*parameters).type != NULL_TYPE) {
        // Tracks individual argument type.
        bool real_number = 0;
        double new_integer = 0;
        
        // Checks current argument's type.
        if ((*car(parameters)).type == DOUBLE_TYPE) {
            real_number = 1;
            return_real = 1;
        }
        // Error check for non-number arguments.
        if ((*car(parameters)).type != INT_TYPE && (*car(parameters)).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in / command.\n");
            texit(0);
        }
        // Places value into correct type.
        if (real_number) {
            new_integer = (*car(parameters)).d;
        }
        else {
            new_integer = (*car(parameters)).i;
        }
        // Does math.
        return_integer = return_integer / new_integer;
        // Moves to next argument.
        parameters = cdr(parameters);
    }
    // Creates result value.
    Value *result = talloc(sizeof(Value));
    // Extra: tracks final return value type.
    if (return_real) {
        (*result).type = DOUBLE_TYPE;
        (*result).d = return_integer;
    }
    else {
        (*result).type = INT_TYPE;
        (*result).i = (int) return_integer;
    }
    return result;
}
// Returns function evalDiv().
Value *primDiv(Value *args) {
    return evalDiv(args); 
}

// ******************** Evaluates modulo commands. ********************
Value *evalMod(Value *args) {
    // Final return value.
    int return_integer = 0;
    Value *parameters = args;
        
    // Checks current argument's type.
    if ((*car(parameters)).type != INT_TYPE) {
        printf("Evaluation Error: arguments must be integers yung blud.\n");
        texit(0);
    }
    if ((*car(cdr(parameters))).type != INT_TYPE) {
        printf("Evaluation Error: arguments must be integers yung blud.\n");
        texit(0);
    }
    if ((*car(parameters)).type != INT_TYPE && (*car(parameters)).type != DOUBLE_TYPE) {
        printf("Evaluation Error: incorrect argument in modulo command.\n");
        texit(0);
    }
     
    // Does math.
    return_integer = (*car(parameters)).i % (*car(cdr(parameters))).i;
    
    // Creates result value.
    Value *result = talloc(sizeof(Value));
    (*result).type = INT_TYPE;
    (*result).i = return_integer;
    return result;
}
// Returns function evalMod().
Value *primMod(Value *args) {
    // Error checks for mod input.
    if((*args).type != CONS_TYPE) {
        printf("Evaluation error: modulo requires 2 arguments\n");
        texit(0);
    }
    else if((*cdr(args)).type == NULL_TYPE){
        printf("Evaluation error: modulo requires 2 arguments\n");
        texit(0);
    }
    else if((*cdr(cdr(args))).type != NULL_TYPE){
        printf("Evaluation error: modulo requires 2 arguments\n");
        texit(0);
    }
    return evalMod(args); 
}

// ******************** Evaluates < commands. ********************
Value *evalLT(Value *args) {
    // Final return bool.
    bool return_bool = 0;
    // Tracks whether return value should be int or double.
    bool return_real = 0;
    Value *parameters = args;
    
    // Goes through arguments.
    while ((*cdr(parameters)).type != NULL_TYPE) {
        // Tracks individual argument type.
        bool real_number = 0;
        bool next_real_number = 0;
        double new_integer = 0;
        double next_integer = 0;
        
        // Checks current and next arguments' type.
        if ((*car(parameters)).type == DOUBLE_TYPE) {
            real_number = 1;
        }
        if ((*car(cdr(parameters))).type == DOUBLE_TYPE) {
                next_real_number = 1;
        }
        
        // Error check for non-number arguments.
        if ((*car(parameters)).type != INT_TYPE && (*car(parameters)).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in < command.\n");
            texit(0);
        }
        if ((*car(cdr(parameters))).type != INT_TYPE && (*car(cdr(parameters))).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in < command.\n");
            texit(0);
        }
        
        // Places current and next values into correct type.
        if (real_number) {
            new_integer = (*car(parameters)).d;
        }
        else {
            new_integer = (*car(parameters)).i;
        }
        if (next_real_number) {
            next_integer = (*car(cdr(parameters))).d;
        }
        else {
            next_integer = (*car(cdr(parameters))).i;
        }
       
        if (new_integer < next_integer) {
            return_bool = 1;
        }
        else {
            return_bool = 0;
            break;
        }
        // Moves to next argument.
        parameters = cdr(parameters);
    }
    Value *result = talloc(sizeof(Value));
    (*result).type = BOOL_TYPE;
    (*result).i = return_bool;
    
    return result;
}
Value *primLT(Value *args) {
    if((*args).type != CONS_TYPE) {
        printf("Evaluation error: < requires 2 arguments\n");
        texit(0);
    }
    else if((*cdr(args)).type == NULL_TYPE){
        printf("Evaluation error: < requires 2 arguments\n");
        texit(0);
    }
    return evalLT(args); 
}

// ******************** Evaluates > commands. ********************

Value *evalGT(Value *args) {
    // Final return bool.
    bool return_bool = 0;
    // Tracks whether return value should be int or double.
    bool return_real = 0;
    Value *parameters = args;
    
    // Goes through arguments.
    while ((*cdr(parameters)).type != NULL_TYPE) {
        // Tracks individual argument type.
        bool real_number = 0;
        bool next_real_number = 0;
        double new_integer = 0;
        double next_integer = 0;
        
        // Checks current and next arguments' type.
        if ((*car(parameters)).type == DOUBLE_TYPE) {
            real_number = 1;
        }
        if ((*car(cdr(parameters))).type == DOUBLE_TYPE) {
                next_real_number = 1;
        }
        
        // Error check for non-number arguments.
        if ((*car(parameters)).type != INT_TYPE && (*car(parameters)).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in > command.\n");
            texit(0);
        }
        if ((*car(cdr(parameters))).type != INT_TYPE && (*car(cdr(parameters))).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in > command.\n");
            texit(0);
        }
        
        // Places current and next values into correct type.
        if (real_number) {
            new_integer = (*car(parameters)).d;
        }
        else {
            new_integer = (*car(parameters)).i;
        }
        if (next_real_number) {
            next_integer = (*car(cdr(parameters))).d;
        }
        else {
            next_integer = (*car(cdr(parameters))).i;
        }
       
        if (new_integer > next_integer) {
            return_bool = 1;
        }
        else {
            return_bool = 0;
            break;
        }
        // Moves to next argument.
        parameters = cdr(parameters);
    }
    Value *result = talloc(sizeof(Value));
    (*result).type = BOOL_TYPE;
    (*result).i = return_bool;
    
    return result;
}
Value *primGT(Value *args) {
    if((*args).type != CONS_TYPE) {
        printf("Evaluation error: > requires 2 arguments\n");
        texit(0);
    }
    else if((*cdr(args)).type == NULL_TYPE){
        printf("Evaluation error: > requires 2 arguments\n");
        texit(0);
    }
    return evalGT(args); 
}

// ******************** Evaluates = commands. ********************

Value *evalEQ(Value *args) {
    // Final return bool.
    bool return_bool = 0;
    // Tracks whether return value should be int or double.
    bool return_real = 0;
    Value *parameters = args;
    
    // Goes through arguments.
    while ((*cdr(parameters)).type != NULL_TYPE) {
        // Tracks individual argument type.
        bool real_number = 0;
        bool next_real_number = 0;
        double new_integer = 0;
        double next_integer = 0;
        
        // Checks current and next arguments' type.
        if ((*car(parameters)).type == DOUBLE_TYPE) {
            real_number = 1;
        }
        if ((*car(cdr(parameters))).type == DOUBLE_TYPE) {
                next_real_number = 1;
        }
        
        // Error check for non-number arguments.
        if ((*car(parameters)).type != INT_TYPE && (*car(parameters)).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in = command.\n");
            texit(0);
        }
        if ((*car(cdr(parameters))).type != INT_TYPE && (*car(cdr(parameters))).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in = command.\n");
            texit(0);
        }
        
        // Places current and next values into correct type.
        if (real_number) {
            new_integer = (*car(parameters)).d;
        }
        else {
            new_integer = (*car(parameters)).i;
        }
        if (next_real_number) {
            next_integer = (*car(cdr(parameters))).d;
        }
        else {
            next_integer = (*car(cdr(parameters))).i;
        }
       
        if (new_integer == next_integer) {
            return_bool = 1;
        }
        else {
            return_bool = 0;
            break;
        }
        // Moves to next argument.
        parameters = cdr(parameters);
    }
    Value *result = talloc(sizeof(Value));
    (*result).type = BOOL_TYPE;
    (*result).i = return_bool;
    
    return result;
}
Value *primEQ(Value *args) {
    if((*args).type != CONS_TYPE) {
        printf("Evaluation error: = requires 2 arguments.\n");
        texit(0);
    }
    else if((*cdr(args)).type == NULL_TYPE){
        printf("Evaluation error: = requires 2 arguments.\n");
        texit(0);
    }
    return evalEQ(args); 
}

// ******************** Evaluates null? commands. ********************
Value *evalNull(Value *args) {
    bool is_null = 0;
    
    if ((*args).type == NULL_TYPE) {
        is_null = 1;
    }
    else if ((*args).type == CONS_TYPE) {
        if((*car(args)).type == CONS_TYPE) {
            if((*car(car(args))).type == NULL_TYPE) {
                is_null = 1;
            }
        }
        else if ((*car(args)).type == NULL_TYPE) {
            is_null = 1;
        }
        
    }
    else if (is_null != 1 && (*args).type == CONS_TYPE){
        printf("Evaluation Error: incorrect arguments in null?.\n");
        texit(0);
    }
    
    Value *result = talloc(sizeof(Value));
    (*result).type = BOOL_TYPE;
    (*result).i = is_null;
    return result;
}
// Returns function evalNull().
Value *primNull(Value *args) {
    if((*cdr(args)).type != NULL_TYPE) {
        printf("Evaluation error: null only takes 1 argument.\n");
        texit(0);
    }
    Value *return_val = evalNull(args);
    return return_val;
}

// ******************** Evaluate the Car command ********************
Value *evalCar(Value *args) {
    if((*args).type != CONS_TYPE) {
        printf("Evaluation Error: incorrect Car arguments.\n");
        texit(0);
    }
    Value *return_val = talloc(sizeof(Value));
    (*return_val).type = CONS_TYPE;
    (*return_val).c.cdr = makeNull();
    
    (*return_val).c.car = car(car(car(args)));
    

    return return_val;
}
Value *primCar(Value *args) {
    if((*args).type != CONS_TYPE){
        printf("Evaluation error: car only takes lists.\n");
        texit(0);
    }
    else if((*cdr(args)).type != NULL_TYPE) {
        printf("Evaluation error: car only takes 1 argument.\n");
        texit(0);
    }
    else if((*car(args)).type != CONS_TYPE){
        printf("Evaluation error: car only takes lists.\n");
        texit(0);
    }
    else if((*car(car(args))).type != CONS_TYPE){
        printf("Evaluation error: car only takes lists.\n");
        texit(0);
    }
    return evalCar(args);
}

// ******************* Evaluates CDR command ******************
Value *evalCdr(Value *args) {
    if((*args).type != CONS_TYPE) {
        printf("Evaluation Error: incorrect Cdr arguments.\n");
        texit(0);
    }
    Value *return_val = talloc(sizeof(Value));
    (*return_val).type = CONS_TYPE;
    
    (*return_val).c.cdr = makeNull();
    (*return_val).c.car = cdr(car(car(args)));
    return return_val;
}
Value *primCdr(Value *args) {
    if((*args).type != CONS_TYPE){
        printf("Evaluation error: cdr only takes lists.\n");
        texit(0);
    }
    else if((*cdr(args)).type != NULL_TYPE) {
        printf("Evaluation error: cdr only takes 1 argument.\n");
        texit(0);
    }
    else if((*car(args)).type != CONS_TYPE){
        printf("Evaluation error: cdr only takes lists.\n");
        texit(0);
    }
    else if((*car(car(args))).type != CONS_TYPE){
        printf("Evaluation error: cdr only takes lists.\n");
        texit(0);
    }
    return evalCdr(args);
}

// ******************* Evaluates cons command ****************** 
Value *evalCons(Value *vehicle, Value *kudur) {
    Value *Cons_cell = talloc(sizeof(Value));
    (*Cons_cell).type = CONS_TYPE;
    
    if((*vehicle).type == CONS_TYPE) {
        vehicle = car(vehicle);
    }
    
    Value *list = makeNull();
    Value *dot = talloc(sizeof(Value));
    (*dot).type = SYMBOL_TYPE;
    (*dot).s = ".";
    
    //printf("VEHICLE:\n");
    //display(vehicle);
    //printf("-----\n");
    //printf("KUDUR:\n");
    //printTree(kudur);
    //printf("\n");
    //printf("-----\n");
    
    if((*car(kudur)).type != CONS_TYPE) {
        if ((*car(kudur)).type != NULL_TYPE) {
            list = cons(dot, list);
            list = cons(car(kudur), list);
        }
    }
    else if ((*car(car(kudur))).type != CONS_TYPE) {
        if ((*car(car(kudur))).type != NULL_TYPE) {
            if ((*cdr(car(kudur))).type != NULL_TYPE) {
                printf("Evaluation Error: cons requires 2 arguments.\n");
                texit(0);
            }
            list = cons(dot, list);
            list = cons(car(car(kudur)), list);
        }
    }
    else {
        kudur = car(car(kudur));
        
        while((*kudur).type != NULL_TYPE) {
    
            list = cons(car(kudur), list);
            kudur = cdr(kudur);
        }
    }  
    list = reverse(list);
    list = cons(vehicle, list);
    
    (*Cons_cell).c.car = list;
    (*Cons_cell).c.cdr = makeNull();
    
    //printf("Result:\n");
    //printTree(list);
    //printf("\n");
    //printf("+++++\n");
    return Cons_cell;
}

Value *primCons(Value *args) {
    if((*args).type != CONS_TYPE) {
        printf("Evaluation error: cons requires 2 arguments.\n");
        texit(0);
    }
    else if((*cdr(args)).type == NULL_TYPE){
        printf("Evaluation error: cons requires 2 arguments.\n");
        texit(0);
    }
    else if((*cdr(cdr(args))).type != NULL_TYPE){
        printf("Evaluation error: cons requires 2 arguments.\n");
        texit(0);
    }
    
    Value *vehicle = car(args);
    Value *kudur = cdr(args);
    return evalCons(vehicle, kudur);
}
// ******************* Applies arguments to commands ******************
Value *apply(Value *function, Value *args) {
    Value *result;
    
    if ((*function).type == PRIMITIVE_TYPE) {
        result = (*function).pf(args);
    }
    else {
        // Creates new frame.
        Value *parameters = (*function).cl.paramNames;
        Frame *new_frame = talloc(sizeof(Frame));
        (*new_frame).parent = (*function).cl.frame;
        Value *current_binding = makeNull();
    
        while ((*parameters).type != NULL_TYPE) {
            // Error checks for nested assignments in let.
            if ((*parameters).type != CONS_TYPE) {
                printf("Evaluation Error: incorrect lambda parameters.\n");
                texit(0);
            }
            if ((*car(parameters)).type == NULL_TYPE) {
                printf("Evaluation Error: incorrect variable assignment syntax.\n");
                texit(0);
            }   
            
            // Find variable and associated value.
            Value *bind_var = car(parameters);
            Value *bind_val;
            // Deals with weird input.
            if ((*args).type != CONS_TYPE) {
                bind_val = args;
            }
            else {
                bind_val = car(args);
                args = cdr(args);
            }
       
            // Check to see if commands are correct syntax.
            if ((*bind_var).type != SYMBOL_TYPE) {
                printf("Evaluation Error: variable in apply not available for assignment.\n");
                texit(0);
            }
        
            // Bind command.
            current_binding = bind(bind_var,bind_val,current_binding);
        
            parameters = cdr(parameters);
        }
        // Reverse to put into correct order.
        current_binding = reverse(current_binding);
        // Set binding list to new frame.
        (*new_frame).bindings = current_binding;
    
        // Evaluates given code with arguments added to binding.
        Value *fc = (*function).cl.functionCode;
    
        while ((*fc).type != NULL_TYPE) {
            if((*fc).type == CONS_TYPE) {
                result = eval(car(fc), new_frame);
                fc = cdr(fc);
            }
        }  
    }
    return result;
}
// Shell function that calls eval() on first object.
void interpret(Value *tree) {
    Value *val = reverse(tree);
    
    // Creates top frame.
    Frame *current_frame = talloc(sizeof(Frame));
    Value *current_binding = makeNull();
    (*current_frame).parent = NULL;
    (*current_frame).bindings = current_binding;
    
    // Binds primitives to frames.
    primBind("+", primPlus ,current_frame);
    primBind("-", primMinus ,current_frame);
    primBind("*", primMult ,current_frame);
    primBind("/", primDiv ,current_frame);
    primBind("modulo", primMod ,current_frame);
    primBind("<", primLT ,current_frame);
    primBind(">", primGT ,current_frame);
    primBind("=", primEQ ,current_frame);
    primBind("cdr", primCdr,current_frame);
    primBind("car", primCar,current_frame);
    primBind("null?", primNull,current_frame);
    primBind("cons", primCons, current_frame);
    
    Value *result;
    while ((*val).type != NULL_TYPE) {
        if((*val).type == CONS_TYPE) {
            result = eval(car(val),current_frame);
            if((*result).type == CONS_TYPE) { 
                printTree(reverse(result));
                printf("\n");
            }
            else {
                display(result);
            }
        }
        else {
            result = eval(val,current_frame);
            display(result);
        }
        val = cdr(val);
    }
}

// Evaluates things.
Value *eval(Value *expr, Frame *frame) {
    Value *result = expr;
    
    switch (expr->type) {
    case INT_TYPE:
    break;
    case DOUBLE_TYPE:
    break;
    case STR_TYPE:
    break;
    case BOOL_TYPE:
    break;
    case SYMBOL_TYPE:
        return lookUpSymbol(expr, frame);
    break;
    case CONS_TYPE:
        if ((*car(expr)).type == SYMBOL_TYPE) {
            if (!strcmp((*car(expr)).s,"if")) {
                result = evalIf(cdr(expr),frame);
            }
            else if (!strcmp((*car(expr)).s,"let")) {
                result = evalLet(cdr(expr),frame);
            }
            else if (!strcmp((*car(expr)).s,"quote")) {
                result = evalQuote(cdr(expr),frame);
            }
            else if (!strcmp((*car(expr)).s,"define")) {
                result = evalDefine(cdr(expr),frame);
            }
            else if (!strcmp((*car(expr)).s,"lambda")) {
                result = evalLambda(cdr(expr),frame);
            }
            else {
                Value *evaledOperator = eval(car(expr), frame);
                Value *evaledArgs = evalEach(cdr(expr), frame);
                result = apply(evaledOperator,evaledArgs);
            }
        }
        else {
            printf("Evaluation Error: expected a procedure that can be applied to arguments.\n");
            texit(0);
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
    case VOID_TYPE:
    break;
    case CLOSURE_TYPE:
    break;
    case PRIMITIVE_TYPE:
    break;
    }
    return result;
}
