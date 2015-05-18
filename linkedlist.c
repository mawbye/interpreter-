#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "value.h"
#include "talloc.h"

/* Justin Lim and Elliot Mawby. CS251, Dave Musicant. Modified LinkedList.
 * Due April 29, 2015. Modified May 1, 2015.
 *
 * Changes: makeNull() and cons() now use the talloc function from talloc.c, instead of malloc.
 *          Reverse() function modified.
 */

// Create a new NULL_TYPE value node.
Value *makeNull() {
    Value *v = talloc(sizeof(Value));
    (*v).type = NULL_TYPE;
    return v;
}

// Create a new CONS_TYPE value node.
Value *cons(Value *car, Value *cdr) {
    Value *v = talloc(sizeof(Value));
    (*v).type = CONS_TYPE;
    (*v).c.car = car;
    (*v).c.cdr = cdr;
    return v;
}

// Display the contents of the linked list to the screen in some kind of
// readable format
void display(Value *list) {
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
        printf("%i\n", (*val).i);
        break;
        case DOUBLE_TYPE:
        printf("%f\n", (*val).d);
        break;
        case STR_TYPE:
        printf("%s\n", (*val).s);
        break;
        case CONS_TYPE:
        display((*val).c.car);
        display((*val).c.cdr); 
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
        printf("%c\n", '(');
        break;
        case CLOSE_TYPE:
        printf("%c\n", ')');
        break;
        case BOOL_TYPE:
        printf("%i\n", (*val).i);
        break;
        case SYMBOL_TYPE:
        printf("%s\n", (*val).s);
        break;
        }
    }  
}

// Return a new list that is the reverse of the one that is passed in. No stored
// data within the linked list should be duplicated; rather, a new linked list
// of CONS_TYPE nodes should be created, that point to items in the original
// list.
Value *reverse(Value *list) {
    Value *head = makeNull();
    Value *val = list;
    // Keeps track of the cdr of our list.
    Value *cdr_ptr = list;
    // Reverses the list.
    while ((*val).type == CONS_TYPE) {
        val = (*val).c.car;
        cdr_ptr = (*cdr_ptr).c.cdr;
        head = cons(val,head);
        val = cdr_ptr;
    }
    return head;
}


// Utility to make it less typing to get car value. Use assertions to make sure
// that this is a legitimate operation.
Value *car(Value *list) {
   return (*list).c.car;
}

// Utility to make it less typing to get cdr value. Use assertions to make sure
// that this is a legitimate operation.
Value *cdr(Value *list) {
    return (*list).c.cdr;
}

// Utility to check if pointing to a NULL_TYPE value. Use assertions to make sure
// that this is a legitimate operation.
bool isNull(Value *value) {
    if ((*value).type == NULL_TYPE) {
        return 1;
    } else {
        return 0;
    }
}

// Measure length of list. Use assertions to make sure that this is a legitimate
// operation.
int length(Value *value) {
    int counter = 0;
    if ((*value).type != CONS_TYPE) {
        if ((*value).type != NULL_TYPE) {
            return 1;
        }
    }
   
    while ((*value).type == CONS_TYPE) {
        counter ++;
        value = (*value).c.cdr;
    }
    return counter;
}
