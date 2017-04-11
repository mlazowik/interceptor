#ifndef INTERCEPTOR_PROGRAM_HEADERS_H
#define INTERCEPTOR_PROGRAM_HEADERS_H

struct program_headers {
    void *(*get_function_address)(const char *);
    void (*replace_got_entries)(const char *, void *);
};

extern const struct program_headers ProgramHeaders;

#endif //INTERCEPTOR_PROGRAM_HEADERS_H
