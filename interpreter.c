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

// Global variables.
// Paren keeps track of the number of parens.
// Quote_bool keeps track of printing quotes.
// Command_bool keeps track of variables vs. user made commands.
int paren;
int quote_bool;
int command_bool = 0;


// Finds the assigned value for a symbol in a specified let frame.
Value *lookUpSymbol(Value *expr, Frame *frame) {
    Frame *current_frame = frame;
    // While we aren't looking at the top frame:
    while (current_frame != NULL) {
        Value *list = (*current_frame).bindings;
        // Go through the list of variables for that frame:
        while ((*list).type != NULL_TYPE) {
            //display(list);
            if (!strcmp((*expr).s,(*car(car(list))).s)) {
              //  printf("found: ");
              //  printf("%s\n", (*expr).s);
              //  printf("returning: ");
              //  display(cdr(car(list)));
              //  printf("returned\n");
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

// Helper function: returns true if the given command
// is available in our list of implemented racket commands and
// false otherwise.
// (Only takes CONS_TYPE whose car is a possible command symbol).
bool commandCheck(Value *expr, Frame *frame) {
    bool return_bool = 0;
    Frame *current_frame = frame;
    if ((*expr).type == CONS_TYPE) {
        if ((*car(expr)).type == SYMBOL_TYPE) {
            if (!strcmp((*car(expr)).s,"if")) {
                return_bool = 1;
            }
            else if (!strcmp((*car(expr)).s,"let")) {
                return_bool = 1;
            }
            else if (!strcmp((*car(expr)).s,"quote")) {
                return_bool = 1;
            }
            else if (!strcmp((*car(expr)).s,"define")) {
                return_bool = 1;
            }
            else if (!strcmp((*car(expr)).s,"lambda")) {
                return_bool = 1;
            }
            else if (!strcmp((*car(expr)).s,"+")) {
                return_bool = 1;
            }
            else if (!strcmp((*car(expr)).s,"null?")) {
                return_bool = 1;
            }
            else if (!strcmp((*car(expr)).s,"car")) {
                return_bool = 1;
            }
            else if (!strcmp((*car(expr)).s,"cdr")) {
                return_bool = 1;
            }
            else if (!strcmp((*car(expr)).s,"cons")) {
                return_bool = 1;
            }
            else {
                // Checks user made commands.
                while (current_frame != NULL) {
                    Value *commands = (*current_frame).bindings;
                    while ((*commands).type != NULL_TYPE) {
                        if (!strcmp((*car(expr)).s,(*car(car(commands))).s)) {
                            return_bool = 1;
                        }
                        commands = cdr(commands);
                    }
                    current_frame = (*current_frame).parent;
                }
            }
        }
    }   
    else {
        printf("Programmer Error: improper use of commandCheck().\n");
        texit(0);
    }
    return return_bool;
}

// Helper Function: binds a variable and value together.
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

// ******************** Evaluates if statements. ********************
Value *evalIf(Value *args, Frame *frame) {
    paren++;
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
        if (commandCheck(return_value, frame)) {
            return_value = eval(return_value,current_frame);
        }
        else {
            printf("Evaluation Error: command not found.\n");
            texit(0);
       }
    }
    paren--;
    return return_value;
}

// ******************** Evaluates Let commands. ********************
Value *evalLet(Value *args, Frame *frame) {
    paren++;
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
            if (commandCheck(bind_val,frame)) {
                bind_val = eval(bind_val,frame);
            }
            else {
                printf("Evaluation Error: command not found.\n");
                texit(0);
            }
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
    
    // Error checks contents of body.
    if ((*body).type == CONS_TYPE) {
        if (!commandCheck(body,frame)) {
            printf("Evaluation Error: command not found.\n");
            texit(0);
        }
    }
    
    // Evaluate body in proper frame.
    Value *return_value = eval(body, new_frame);
    paren--;
    return return_value;
}

// ******************** Evaluates Quote commands. ********************
Value *evalQuote(Value *args, Frame *frame) {
   // printf("returning from quote$$$$$$$$$$$$$$$\n");
    //display(reverse(args));
    return car(args);
}

// ******************** Evaluates Define commands. ********************
Value *evalDefine(Value *args, Frame *frame) {
    paren++;
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
    
    paren--;
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
    paren++;
    Value *closure = talloc(sizeof(Value));
    (*closure).type = CLOSURE_TYPE;
    (*closure).cl.paramNames = car(args);
    (*closure).cl.functionCode = cdr(args);

    (*closure).cl.frame = frame;
    
    paren--;
    return closure;
}

// ******************** Evaluates + commands. ********************
Value *evalPlus(Value *args, Frame *frame) {
    paren++;
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
        // Evaluates current argument.
        Value *operand = eval(car(parameters),frame);
        
        // Checks current argument's type.
        if ((*operand).type == DOUBLE_TYPE) {
            real_number = 1;
            return_real = 1;
        }
        
        // Error check for non-number arguments.
        if ((*operand).type != INT_TYPE && (*operand).type != DOUBLE_TYPE) {
            printf("Evaluation Error: incorrect argument in command.\n");
            texit(0);
        }
        // Places value into correct type.
        if (real_number) {
            new_integer = (*operand).d;
        }
        else {
            new_integer = (*operand).i;
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
    paren--;
    return result;
}

// ******************** Evaluates null? commands. ********************
Value *evalNull(Value *args) {
    bool is_null = 0;
    //printf("LOOK AT ME NOW\n");
   // display(args);
    //printf("-----\n");
    // Error Check on arguments (must be a linked list).
    if ((*args).type == NULL_TYPE) {
        // printf("Evaluation Error: incorrect null? argument.\n");
        // texit(0);
        is_null = 1;
    }
    else if ((*args).type == CONS_TYPE) {
      //  printf("CDR: ");
       // display(cdr(args));
       // printf("ARGS: ");
       // display(args);
       // printf("car args\n");
      //  display(car(args));
        
        //printf("came here\n");
        //printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
      //  display(car(args));
        if((*car(args)).type == CONS_TYPE) {
           // printf("came here 2\n");
            if((*car(car(args))).type == NULL_TYPE) {
                //printf("came here 3\n");
                is_null = 1;
            }
        }
        else if ((*car(args)).type == NULL_TYPE) {
            //printf("car was null\n");
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
//Evaluate the Car function
Value *evalCar(Value *args) {
    paren++;
     
    if((*args).type != CONS_TYPE) {
        printf("Evaluation Error: incorrect Car arguments\n");
        texit(0);
    }

    paren--;
    return car(args);
}

Value *evalCdr(Value *args) {
    paren++;
    // printf("displaying args in EVALCDR##################\n");
    // display(args);
 
    if((*args).type != CONS_TYPE) {
        printf("Evaluation Error: incorrect Cdr arguments\n");
        texit(0);
    }
   
    paren--;
    return cdr(args);
}

Value *removeQuote(Value *value) {
    Value *return_val;
    // display(value);
    if((*value).type == CONS_TYPE) {
        if((*car(value)).type == CONS_TYPE) {
            if (!strcmp((*car(car(value))).s,"quote")) {
                return_val = cdr(car(value));
                return return_val;
            }
        }
    }
           
    return value;
}
Value *evalCons(Value *vehicle, Value *kudur) {
    paren ++;
    Value *Cons_cell = talloc(sizeof(Value));
    Value *quote_cell = talloc(sizeof(Value));
    Value *quote = talloc(sizeof(Value));
    Value *top_cell = talloc(sizeof(Value));

    (*Cons_cell).type = CONS_TYPE;
    (*quote_cell).type = CONS_TYPE;
    (*quote).type = SYMBOL_TYPE;
    (*top_cell).type = CONS_TYPE;

    //printf("display kudur\n");
    //display(kudur);

    //printf("lalalalla\n");

    //printf("car_cell car assigned\n");
    //display(vehicle);
    
    //printf("poooppopopop\n");
    vehicle = removeQuote(vehicle);
    kudur = removeQuote(kudur);
    


    
    (*Cons_cell).c.car = cons(vehicle,kudur);
    (*Cons_cell).c.cdr = makeNull();
    
    
    (*quote).s = "quote";
    
    (*quote_cell).c.car = quote;
    (*quote_cell).c.cdr = Cons_cell;
    (*top_cell).c.car = quote_cell;
    (*top_cell).c.cdr = makeNull();
    paren--;
    //printf("RETURNING\n");
   
    //display(top_cell);
    return top_cell;
}
// Given code and a list of arguments, applies arguments to code and returns result.
Value *apply(Value *function, Value *args) {
    // Creates new frame.
    // printf("displaying args,,,,,,,,,,\n");
    // display(args);
    Value *parameters = (*function).cl.paramNames;
    Frame *new_frame = talloc(sizeof(Frame));
    (*new_frame).parent = (*function).cl.frame;
    Value *current_binding = makeNull();
    bool error_check = 1;
    
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
        if (!error_check) {
            printf("Evaluation Error: incorrect number of parameters given.\n");
            texit(0);
        }
        
        // Find variable and associated value.
        Value *bind_var = car(parameters);
        Value *bind_val;
        // Deals with weird input.
        if ((*args).type != CONS_TYPE) {
            bind_val = args;
            error_check = 0;
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
    command_bool = 0;
    Value *result = talloc(sizeof(Value));
    //printf("ASDFASDF\n");
    if ((*car((*function).cl.functionCode)).type != CONS_TYPE) {
        result = car((*function).cl.functionCode);
    }
    else {
        result = eval((*function).cl.functionCode, new_frame);
    }
   
    
    paren--;
    
    return result;
}

// Shell function that calls eval() on first object.
void interpret(Value *tree) {
    Value *val = reverse(tree);
    paren = 0;
    quote_bool = 0;
    
    // Creates top frame.
    Frame *current_frame = talloc(sizeof(Frame));
    Value *current_binding = makeNull();
    (*current_frame).parent = NULL;
    (*current_frame).bindings = current_binding;
    
    Value *return_val = eval(val, current_frame);
}

Value *evalEach(Value *args, Frame *frame) {
    Value *result = talloc(sizeof(Value));
    result = makeNull();
    
    while ((*args).type != NULL_TYPE) {
       // printf("ARGS\n");
        //display(args);
        if((*args).type == CONS_TYPE) {
            Value *evaled_args = eval(car(args),frame);
           // printf("&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
            //display(evaled_args);
            result = cons(evaled_args,result);
        }
        args = cdr(args);
    }
   // printf("RETURNING FROM EVALEACHF222222\n");
   // display(result);
    result = reverse(result);
    return result;
}

// Evaluates things.
Value *eval(Value *expr, Frame *frame) {
   // printf("EVALUATING\n");
    Value *result = expr;
    
    switch (expr->type) {
    case INT_TYPE:
        if (paren == 0) {
            printf("%i\n", (*expr).i);
        }
        break;
    case DOUBLE_TYPE:
        if (paren == 0) {
            printf("%f\n", (*expr).d);
        }
        break;
    case STR_TYPE:
        if (paren == 0) {
            printf("%s\n", (*expr).s);
        }
        break;
    case BOOL_TYPE:
        if (paren == 0) {
            printf("%i\n", (*expr).i);
        }
        break;
    case SYMBOL_TYPE:
       // printf("looking for: ");
       // display(expr);
        result = lookUpSymbol(expr, frame);
        //printf("done\n");
       // display(result);
        //printf("done looking up symbol\n");
        break;
    case CONS_TYPE:
        if ((*car(expr)).type == SYMBOL_TYPE) {
            if (!strcmp((*car(expr)).s,"if")) {
                result = evalIf(cdr(expr),frame);
                if ((*result).type != CONS_TYPE && paren == 0) {
                    display(result);
                }
            }
            else if (!strcmp((*car(expr)).s,"let")) {
                result = evalLet(cdr(expr),frame);
                if ((*result).type != CONS_TYPE && paren == 0) {
                    display(result);
                }
            }
            else if (!strcmp((*car(expr)).s,"quote")) {
                result = evalQuote(cdr(expr),frame);
                if (paren == 0) {
                    //printTree(reverse(result));
                    //printf("\n");
                }
            }
            else if (!strcmp((*car(expr)).s,"define")) {
                result = evalDefine(cdr(expr),frame);
            }
            else if (!strcmp((*car(expr)).s,"lambda")) {
                result = evalLambda(cdr(expr),frame);
            }
            else if (!strcmp((*car(expr)).s,"+")) {
                result = evalPlus(cdr(expr), frame);
                if ((*result).type != CONS_TYPE && paren == 0) {
                    display(result);
                }
            }
            else if (!strcmp((*car(expr)).s,"null?")) {
               // printf("evaling args: ");
                //display(cdr(expr));
                if ((*cdr(expr)).type != CONS_TYPE) {
                     printf("Evaluation Error: no null? argument.\n");
                     texit(0);
                }
               /* if ((*cdr(cdr(expr))).type != NULL_TYPE) {
                    printf("Evaluation Error: too many null? arguements.\n");
                    texit(0);
                }*/
                Value *evaled = eval(car(cdr(expr)), frame);
               // printf("args: ");
                //display(evaled);
                result = evalNull(evaled);
                if ((*result).type != CONS_TYPE && paren == 0) {
                    display(result);
                }
            }
            else if (!strcmp((*car(expr)).s,"car") || !strcmp((*car(expr)).s,"cdr")) {
                //printf("in car\n");
                if ((*cdr(expr)).type != CONS_TYPE) {
                     printf("Evaluation Error: no null? argument.\n");
                     texit(0);
                }
                if ((*cdr(cdr(expr))).type != NULL_TYPE) {
                    printf("Evaluation Error: too many null? arguements.\n");
                    texit(0);
                }
                Value *evaled = eval(car(cdr(expr)), frame);
               // printf("args for car: ");
               // display(evaled);
                if(!strcmp((*car(expr)).s,"car")) {
                    result = evalCar(evaled);
                }
                else {
                    result = evalCdr(evaled);
                }
                if ((*result).type != CONS_TYPE && paren == 0) {
                    display(result);
                }
            }
            
            else if(!strcmp((*car(expr)).s,"cons")) {
                Value *evaled_car = eval(car(cdr(expr)), frame);
                Value *evaled_cdr = eval(car(cdr(cdr(expr))), frame);
                result = evalCons(evaled_car, evaled_cdr);
                printTree(result);
                printf("\n");
                
            }
            // Checks whether a symbol is a user made command or a variable.
            else if ((commandCheck(expr,frame)) && (command_bool == 1)){
                paren++;
                command_bool = 0;
                Value *evalOperator = eval(car(expr),frame);
                command_bool = 0;
                
                Value *evalArgs = evalEach(cdr(expr),frame);
                
           
                // Applies evaluated operator and arguments.
                result = apply(evalOperator,evalArgs); 
                
                if (paren == 0) {
                    display(result);
                }
            }
            // Don't really know why we need this.
            else if (commandCheck(expr, frame)){
                result = eval(car(expr), frame);
            }
            // Other commands go here.
            else {
                printf("Evaluation Error: undefined variable.\n");
                texit(0);
            }
        }
        // User made commands.
        else if ((*car(expr)).type == CONS_TYPE) {
            command_bool = 1;
            result = eval(car(expr),frame);
            eval(cdr(expr),frame);
        }
    
        else {
            eval(car(expr),frame);
            if ((*cdr(expr)).type != NULL_TYPE) {
                eval(cdr(expr),frame);
            }
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
   // printf("returning result\n");
    //display(result);
    return result;
}
