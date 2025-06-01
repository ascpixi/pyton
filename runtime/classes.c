#include "classes.h"

#include "sys/core.h"

void py_verify_self_arg(pyobj_t* self, pyobj_t* type) {
    if (self == NULL)
        sys_panic("The 'self' argument was NULL.");
    
    if (self->type == type)
        return;

    pyobj_t* current_base = self->type->as_type.base;
    while (current_base != type && current_base != NULL) {
        current_base = current_base->as_type.base;
    }

    if (current_base == NULL) {
        // Oops... we've walked the entire inheritance chain but didn't find the expected type.
        sys_panic("The 'self' argument was of an invalid type.");
    }
}