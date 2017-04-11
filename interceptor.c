#include "interceptor.h"

#include "program_headers.h"

void *intercept_function(const char *name, void *new_func) {
    ProgramHeaders.replace_got_entries(name, new_func);
    return ProgramHeaders.get_function_address(name);
}

void unintercept_function(const char *name) {
    void *address = ProgramHeaders.get_function_address(name);
    ProgramHeaders.replace_got_entries(name, address);
}