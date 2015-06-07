#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "value.h"
#include "talloc.h"
#include "linkedlist.h"

/* Justin Lim and Elliot Mawby. CS 251, Dave Musicant. Tokenizer Assignment.
 * Due May 6, 2015.
 */

// Read all of the input from stdin, and return a linked list consisting of the
// tokens.
Value *tokenize() {
    char charRead;
    Value *list = makeNull();
    charRead = fgetc(stdin);
    
    // Reads input.
    while (charRead != EOF) {
    
        // Tokenizes '('.
        if (charRead == '(') {
            Value *token = talloc(sizeof(Value));
            (*token).type = OPEN_TYPE;
            (*token).s = &charRead;
            list = cons(token, list);
            charRead = fgetc(stdin);
        }
           
        // Tokenizes ')'.
        else if (charRead == ')') {
           
            Value *token = talloc(sizeof(Value));
            (*token).type = CLOSE_TYPE;
            (*token).s = &charRead;
            list = cons(token, list);
            charRead = fgetc(stdin);
        }
        
        // Tokenizes comments.
        else if (charRead == ';') {
            // Counts chars added to the string and keeps track of capacity.
            int counter = 0;
            int capacity = 20;
            // Creates new char array and moves to next character.
            char *string = talloc(sizeof(char)*capacity);
            string[counter] = charRead;
            counter ++;
            charRead = fgetc(stdin);
            // Adds chars to char array until string ends.
            while (charRead != '\n' && charRead != EOF) {
               
                // Ensures capacity of string.
                if (counter == (capacity-3)) {
                    string[counter] = charRead;
                    counter ++;
                    capacity = capacity*2;
                    int new_counter = 0;
                    char *new_string = talloc(sizeof(char)*capacity);
                    // Copies everything from old string to new string.
                    while (new_counter != counter) {
                        new_string[new_counter] = string[new_counter];
                        new_counter ++;
                    }
                    // Sets new string to old string.
                    string = new_string;
                }
                
                else {
                    string[counter] = charRead;
                    counter ++;
                }
                charRead = fgetc(stdin);
            }
            counter ++;
            string[counter] = '\0';
            
            Value *token = talloc(sizeof(Value));
            (*token).type = STR_TYPE;
            (*token).s = string;
            list = cons(token, list);
            charRead = fgetc(stdin);
        }
        
        // Tokenizes strings.
        else if (charRead == '"') {
            // Counts chars added to the string and keeps track of capacity.
            int counter = 0;
            int capacity = 20;
            // Creates new char array and moves to next character.
            char *string = talloc(sizeof(char)*capacity);
            string[counter] = charRead;
            counter ++;
            charRead = fgetc(stdin);
            // Adds chars to char array until string ends.
            while (charRead != '"') {
                //Test for syntax error (beginning quote but no end quote).
                if (charRead == EOF) { 
                    printf("Error: Your Code is Untokenizable.\n");
                    texit(0);
                }
                //Test to see if there is are escaped characters
                else if (charRead == '\\') {
                    charRead = fgetc(stdin);
                    // If they want the string to include ', ", or "\".
                    if (charRead == '\\' || charRead == '\'' || charRead == '\"'){
                        string[counter] = charRead;
                        counter ++;
                    }
                    else if (charRead == 'n') {
                        string[counter] = '\n';
                        counter ++;
                    }
                    else if (charRead == 't') {
                        string[counter] = '\t';
                        counter++;
                    }   
                }
                
                // Ensures capacity of string.
                else if (counter == (capacity-3)) {
                    string[counter] = charRead;
                    counter ++;
                    capacity = capacity*2;
                    int new_counter = 0;
                    char *new_string = talloc(sizeof(char)*capacity);
                    // Copies everything from old string to new string.
                    while (new_counter != counter) {
                        new_string[new_counter] = string[new_counter];
                        new_counter ++;
                    }
                    // Sets new string to old string.
                    string = new_string;
                }
                
                else {
                    string[counter] = charRead;
                    counter ++;
                }
                charRead = fgetc(stdin);
            }
            string[counter] = charRead;
            counter ++;
            string[counter] = '\0';
            
            Value *token = talloc(sizeof(Value));
            (*token).type = STR_TYPE;
            (*token).s = string;
            list = cons(token, list);
            charRead = fgetc(stdin);
        }
        
        // Tokenizes symbols, ints, floats, and bools.    
        else if ((33 <= ((int) charRead)) && (((int) charRead) <= 126)) {
            // Overall charRead counter.
            int counter = 0;
            // Capacity for all arrays.
            int capacity = 300;
            
            // Counters to test type.
            int int_counter = 0;
            int bool_counter = 0;
            int sign_counter = 0;
            int float_counter = 0;
            
            // If sign is 0, number is positive, else negative.
            int sign = 0;
            
            // Booleans to confirm type.
            bool is_int = 0;
            bool is_bool = 0;
            bool is_float = 0;
            
            // Vals to cons various types.
            int val = 0;
            int bool_val = 0;
            double float_val = 0;
            
            // Creates new char array and int array.
            char *string = talloc(sizeof(char)*capacity);
            int *int_array = talloc(sizeof(int)*capacity);
            // Adds chars to char array until string ends.
            while (charRead != ' ' && charRead != '\n' && charRead != ')' && charRead != '\t') {
                int val = (int) charRead;
                string[counter] = charRead;
                counter ++;
                // Keeps track of which symbols are bools.
                if (charRead == '#') {
                    bool_counter = 1;
                }
                // If previous character was a '#' and the
                // current character is 't' or 'f' it is a bool.
                else if (bool_counter == 1) {
                    if (charRead == 't') {
                        bool_counter = 2;
                        bool_val = 1;
                    }
                    else if (charRead == 'f') {
                        bool_counter = 2;
                        bool_val = 0;
                    }
                }
                // Tests to see if the token is a float.
                else if (charRead == '.') {
                    float_counter ++;
                    int_array[int_counter] = (int) charRead;
                    int_counter ++;
                }
                else if(float_counter == 1) {
                    // if current char is a number and the token is a float.
                    if ((48 <= ((int) charRead)) && (((int) charRead) <= 57)) {
                        int_array[int_counter] = ((int)charRead - (int) '0');
                        int_counter++;
                    }
                }
                // Checking for signed ints/floats.
                else if((45 == (int) charRead) || (43 == (int) charRead)){
                    sign_counter = 1;
                    if ((45 == (int) charRead)) {
                        sign = 1;
                    }    
                }
                else if (sign_counter == 1) {
                    // If current char is a number and token is signed value.
                    if ((48 <= ((int) charRead)) && (((int) charRead) <= 57)) { 
                        sign_counter = 2;
                        int_array[int_counter] = ((int)charRead - (int) '0');
                        int_counter++;
                    }
                }
                // Keeps track of which symbols are ints/floats. 
                else if ((48 <= ((int) charRead)) && (((int) charRead) <= 57)) {
                    int_array[int_counter] = ((int)charRead - (int) '0');
                    int_counter ++;
                   
                }
                
                // Ensures capacity of string.
                if (counter == (capacity-1)) {
                    capacity = capacity*2;
                    int new_counter = 0;
                    char *new_string = talloc(sizeof(char)*capacity);
                    // Copies everything from old string to new string.
                    while (new_counter != counter) {
                        new_string[new_counter] = string[new_counter];
                        new_counter ++;
                    }
                    // Sets new string to old string.
                    string = new_string;
                }
                charRead = fgetc(stdin);
            }
            string[counter] = '\0';
            int_array[counter] = '\0';
            
            // If it is a signed value, don't count the sign in the length of the number.
            if (sign_counter == 2){
                counter = counter - 1;
            }
            // If all symbols are ints, then item is an int.
            if (int_counter == counter && float_counter < 1) {
                
                is_int = 1;
                int i;
                double result = 0;
                // Converts the list of ints to the resulting number.
                for (i=0; i < counter; i++) {
                  result = result + (int_array[i] * pow((double) 10, ((double) (int_counter - 1) - (double) i)));
                }
                // If negative make negative.
                if (sign == 1) {
                    result = result * -1;
                }
                val = (int) result;
                
            }
            // If float:
            if (int_counter == counter && float_counter == 1) {
                is_float = 1;
                int i = 0;
                int j = 0;
                int decimal = 0;
                double result = 0;
                for (j=0; j < counter; j++) {
                    if (int_array[j] == '.') {
                        decimal = j;
                    }
                }
                // Calculates the result before the decimal.
                while (int_array[i] != '.') {
                    result = result + (int_array[i] * pow((double) 10, ((double) (decimal - 1) - (double) i)));
                    i ++;
                }
                decimal = 0;
                for (i = i; i < counter; i++) { // Calculates the result after the decimal.
                    
                    if (int_array[i] == 46) {
                        decimal ++;
                    }
                    else {
                        result = result + (int_array[i] / pow((double) 10, (double) decimal));
                        decimal ++;
                    }
                }
                if (sign == 1) {
                    result = result * -1;
                }
                float_val = result;
            }
            // If token is a boolean:
            if (bool_counter == counter) { 
                is_bool = 1;
                val = bool_val;
            }
            // Adds either symbol or int to token.
            if (is_int == 1) {
                Value *token = talloc(sizeof(Value));
                (*token).type = INT_TYPE;
                (*token).i = val;
                list = cons(token, list);
                if (charRead != ')' && charRead != '\n' && charRead != '\t') {
                   charRead = fgetc(stdin);
                }
            }
            else if (is_float == 1) {
                Value *token = talloc(sizeof(Value));
                (*token).type = DOUBLE_TYPE;
                (*token).d = float_val;
                list = cons(token, list);
                if (charRead != ')' && charRead != '\n' && charRead != '\t') {
                   charRead = fgetc(stdin);
                }
            }
            else if (is_bool == 1) {
                Value *token = talloc(sizeof(Value));
                (*token).type = BOOL_TYPE;
                (*token).i = val;
                list = cons(token, list);
                if (charRead != ')' && charRead != '\n' && charRead != '\t') {
                    charRead = fgetc(stdin);
                }
            }
            else {
                Value *token = talloc(sizeof(Value));
                (*token).type = SYMBOL_TYPE;
                (*token).s = string;
                list = cons(token, list);
                if (charRead != ')' && charRead != '\n' && charRead != '\t') {
                    charRead = fgetc(stdin);
                }
            }
        }
        
        else if (charRead == ' ') {
            charRead = fgetc(stdin);
        }
        else if (charRead == '\n') {
            charRead = fgetc(stdin);
        }
        else if (charRead == '\t') {
            charRead = fgetc(stdin);
        }
        else {
            printf("Syntax error: untokenizable\n");
            texit(0);
        }
    }
    
    Value *revList = reverse(list);
    return revList;
}

// Displays the contents of the linked list as tokens, with type information
void displayTokens(Value *list) {
   Value *val = list;
    
    // If null, the end of the list has been reached.
    if ((*val).type == NULL_TYPE) {
        printf("End of list.\n");
    }
    // Otherwise, print the item in the list, or if CONS_TYPE,
    // display the car go to next item (cdr).
    else {
        switch ((*val).type) {
        case INT_TYPE:
        printf("%i:int\n", (*val).i);
        break;
        case DOUBLE_TYPE:
        printf("%f:double\n", (*val).d);
        break;
        case STR_TYPE:
        printf("%s:string\n", (*val).s);
        break;
        case CONS_TYPE:
        displayTokens((*val).c.car);
        displayTokens((*val).c.cdr); 
        break;
        case NULL_TYPE:
        printf("\n");
        break;
        // Part 2
        case PTR_TYPE:
        printf("\n");
        break;
        // Part 3
        case OPEN_TYPE:
        printf("%c:open\n", '(');
        break;
        case CLOSE_TYPE:
        printf("%c:close\n", ')');
        break;
        case BOOL_TYPE:
        printf("%i:bool\n", (*val).i);
        break;
        case SYMBOL_TYPE:
        printf("%s:symbol\n", (*val).s);
        break;
        case VOID_TYPE:
        break;
        case CLOSURE_TYPE:
        break;
        case PRIMITIVE_TYPE:
        break;
        }
    }  
}
