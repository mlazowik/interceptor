#define _GNU_SOURCE

#include <elf.h>
#include <link.h>
#include <sys/auxv.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <stdio.h>

#include "program_headers.h"

struct function_address_query {
    const char *name;
    void *address;
};

static bool is_vdso(struct dl_phdr_info *info) {
    ElfW(Ehdr*) ehdr_vdso = (ElfW(Ehdr*)) getauxval(AT_SYSINFO_EHDR);
    ElfW(Phdr*) phdr_vdso = (ElfW(Phdr*)) ((void*)ehdr_vdso + ehdr_vdso->e_phoff);

    return info->dlpi_phdr == phdr_vdso;
}

static bool is_dynamic_section(const ElfW(Phdr) program_header) {
    return program_header.p_type == PT_DYNAMIC;
}

ElfW(Dyn*) get_dynamic_section_address(struct dl_phdr_info *info, const ElfW(Phdr) program_header) {
    return (ElfW(Dyn*))(info->dlpi_addr + program_header.p_vaddr);
}

static bool is_symbol_defined(ElfW(Sym*) sym) {
    return sym->st_shndx != SHN_UNDEF;
}

static bool symbol_is(ElfW(Sym*) sym, char *strtab, const char *name) {
    return strcmp(&strtab[sym->st_name], name) == 0;
}

void *get_symbol_address(struct dl_phdr_info *info, ElfW(Sym*) sym) {
    return (void *) (info->dlpi_addr + sym->st_value);
}

static int get_function_address_from_program_headers(struct dl_phdr_info *info,
                                                    size_t size, void *data) {
    struct function_address_query *query = (struct function_address_query*) data;

    if (is_vdso(info)) {
        return 0;
    }

    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr) program_header = info->dlpi_phdr[i];

        if (!is_dynamic_section(program_header)) {
            continue;
        }

        ElfW(Dyn*) dyn = get_dynamic_section_address(info, program_header);
        ElfW(Sym*) sym = NULL;
        char* strtab = 0;

        while (dyn->d_tag != DT_NULL) {
            if (dyn->d_tag == DT_STRTAB) {
                strtab = (char*)dyn->d_un.d_ptr;
            } else if (dyn->d_tag == DT_SYMTAB) {
                sym = (ElfW(Sym*)) dyn->d_un.d_ptr;
            }

            dyn++;
        }

        while ((void*)sym < (void*)strtab) {
            if (is_symbol_defined(sym) && symbol_is(sym, strtab, query->name)) {
                query->address = get_symbol_address(info, sym);
            }

            sym++;
        }
    }

    return 0;
}

void *get_function_address(const char *name) {
    struct function_address_query query;
    query.name = name;

    dl_iterate_phdr(get_function_address_from_program_headers, &query);

    return query.address;
}

const struct program_headers ProgramHeaders = {
        .get_function_address = get_function_address
};