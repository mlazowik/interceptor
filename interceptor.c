#include "interceptor.h"

#include "program_headers.h"

void *intercept_function(const char *name, void *new_func) {
    return ProgramHeaders.get_function_address(name);
}