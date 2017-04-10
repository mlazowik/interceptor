#ifndef INTERCEPT_PROGRAM_HEADERS_H
#define INTERCEPT_PROGRAM_HEADERS_H

struct program_headers {
    void *(*get_function_address)(const char*);
};

extern const struct program_headers ProgramHeaders;

#endif //INTERCEPT_PROGRAM_HEADERS_H
