#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "value.h"
#include "linkedlist.h"

/* Justin Lim and Elliot Mawby. CS251, Dave Musicant. Talloc Assignment.
 * Due May 1, 2015.
 */

// Global Variables
Value *tList;
bool init = 0;

// Replacement for malloc that stores the pointers allocated. It should store
// the pointers in some kind of list; a linked list would do fine, but insert
// here whatever code you'll need to do so; don't call functions in the
// pre-existing linkedlist.h. Otherwise you'll end up with circular
// dependencies, since you're going to modify the linked list to use talloc.
void *talloc(size_t size) {
    // Initializes tList
    if (init == 0) {
        tList = malloc(size);
        (*tList).type = NULL_TYPE;
        init = 1;
    }
    // Cons the new item to the list.
    Value *v = malloc(sizeof(Value));
    (*v).type = CONS_TYPE;
    Value *new_node = malloc(size);
    (*v).c.car = new_node;
    (*v).c.cdr = tList;
    tList = v;
    return new_node;
}

// Free all pointers allocated by talloc, as well as whatever memory you
// allocated in lists to hold those pointers.
void tfree() {
    Value *val = tList;
    // Test for when we reach the end of the list.
    while ((*val).type != NULL_TYPE) {
        // Free the car of val.
        free((*val).c.car);
        // Save val to a temporary val, and free val.
        Value *temp = (*val).c.cdr;
        free(val);
        // Reassign val to the cdr of val.           
        val = temp;
    }
    free(val);
    
    // Sets initialize boolean back to 0 so tList can be initialized again.
    init = 0;
}

// Replacement for the C function "exit", that consists of two lines: it calls
// tfree before calling exit. It's useful to have later on; if an error happens,
// you can exit your program, and all memory is automatically cleaned up.
void texit(int status) {
    tfree();
    exit(status);
}
