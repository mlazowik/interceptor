#include <stdio.h>

#include "lib.h"
#include "../interceptor.h"

int (*puts_orig) (const char *);

int my_puts(const char *s) {
    puts_orig("intercepted");
}

int main() {
    printf("puts address: %p\n", puts);
    puts_orig = intercept_function("puts", my_puts);
    puts("test intercepted");
    //puts_orig("test orig");
    printf("puts address: %p\n", puts);
    printf("orig address: %p\n", puts_orig);

    puts("test");
    dyn_puts();

    return 0;
}