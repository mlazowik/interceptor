#define _GNU_SOURCE

#include <elf.h>
#include <link.h>
#include <sys/auxv.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "program_headers.h"

struct address_query {
    const char *name;
    void *address;
};

struct dynamic_segment {
    Elf64_Sym *sym;
    char *strtab;
    Elf64_Xword relocation_records_size;
    Elf64_Xword relocation_records_type;
    char *relocation_records_address;
};

static bool is_vdso(struct dl_phdr_info *info) {
    Elf64_Ehdr *ehdr_vdso = (Elf64_Ehdr *) getauxval(AT_SYSINFO_EHDR);
    Elf64_Phdr *phdr_vdso = (Elf64_Phdr *) ((void *) ehdr_vdso +
                                            ehdr_vdso->e_phoff);

    return info->dlpi_phdr == phdr_vdso;
}

static bool is_dynamic_segment(const Elf64_Phdr program_header) {
    return program_header.p_type == PT_DYNAMIC;
}

Elf64_Dyn *get_dynamic_segment_address(struct dl_phdr_info *info,
                                       const Elf64_Phdr program_header) {
    return (Elf64_Dyn *) (info->dlpi_addr + program_header.p_vaddr);
}

static bool is_symbol_defined(Elf64_Sym *sym) {
    return sym->st_shndx != SHN_UNDEF;
}

static bool symbol_is_named(Elf64_Sym *sym, char *strtab, const char *name) {
    return strcmp(&strtab[sym->st_name], name) == 0;
}

static char *get_sym_name(struct dynamic_segment *dyn, Elf64_Sym *sym) {
    return &dyn->strtab[sym->st_name];
}

void *get_symbol_address(struct dl_phdr_info *info, Elf64_Sym *sym) {
    void *address = (void *) (info->dlpi_addr + sym->st_value);

    if (ELF64_ST_TYPE(sym->st_info) == STT_GNU_IFUNC) {
        void *(*fun)() = address;
        return fun();
    }

    return address;
}

const struct dynamic_segment parse_dynamic_segment(Elf64_Dyn *dyn) {
    struct dynamic_segment parsed;

    parsed.sym = NULL;
    parsed.strtab = NULL;
    parsed.relocation_records_size = 0;
    parsed.relocation_records_type = 0;
    parsed.relocation_records_address = NULL;

    while (dyn->d_tag != DT_NULL) {
        if (dyn->d_tag == DT_STRTAB) {
            parsed.strtab = (char *) dyn->d_un.d_ptr;
        } else if (dyn->d_tag == DT_SYMTAB) {
            parsed.sym = (Elf64_Sym *) dyn->d_un.d_ptr;
        } else if (dyn->d_tag == DT_PLTRELSZ) {
            parsed.relocation_records_size = dyn->d_un.d_val;
        } else if (dyn->d_tag == DT_PLTREL) {
            parsed.relocation_records_type = dyn->d_un.d_val;
        } else if (dyn->d_tag == DT_JMPREL) {
            parsed.relocation_records_address = (char *) dyn->d_un.d_ptr;
        }

        dyn++;
    }

    return parsed;
}

const struct dynamic_segment get_dynamic_segment(struct dl_phdr_info *info) {
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const Elf64_Phdr program_header = info->dlpi_phdr[i];

        if (!is_dynamic_segment(program_header)) {
            continue;
        }

        return parse_dynamic_segment(
                get_dynamic_segment_address(info, program_header));
    }
}

static int get_function_address_from_program_headers(struct dl_phdr_info *info,
                                                     size_t size, void *data) {
    struct address_query *query = (struct address_query *) data;

    if (is_vdso(info)) {
        return 0;
    }

    const struct dynamic_segment dyn = get_dynamic_segment(info);

    Elf64_Sym *current_sym = dyn.sym;
    while ((void *) current_sym < (void *) dyn.strtab) {
        if (is_symbol_defined(current_sym) &&
            symbol_is_named(current_sym, dyn.strtab, query->name)) {
            query->address = get_symbol_address(info, current_sym);

            // first object with given symbol wins
            return 1;
        }

        current_sym++;
    }

    return 0;
}

static int
replace_got_entry(struct dl_phdr_info *info, size_t size, void *data) {
    struct address_query *query = (struct address_query *) data;

    if (is_vdso(info)) {
        return 0;
    }

    struct dynamic_segment dyn = get_dynamic_segment(info);

    if (dyn.relocation_records_address == NULL) {
        return 0;
    }

    size_t relocation_record_size = dyn.relocation_records_type
                                    ? sizeof(Elf64_Rel *)
                                    : sizeof(Elf64_Rela *);

    size_t relocation_record_count =
            dyn.relocation_records_size / relocation_record_size;

    for (size_t j = 0; j < relocation_record_count; j++) {
        // Elf64_Rel is a prefix of Elf64_Rela, we do not care about the extra
        // field
        Elf64_Rel *rel = (Elf64_Rel *) (dyn.relocation_records_address +
                                        relocation_record_size * j);

        if (ELF64_R_TYPE(rel->r_info) != R_X86_64_JUMP_SLOT) {
            continue;
        }

        Elf64_Addr relocation_address = info->dlpi_addr + rel->r_offset;
        if (symbol_is_named(&dyn.sym[ELF64_R_SYM(rel->r_info)],
                            dyn.strtab, query->name)) {
            *((Elf64_Addr *) relocation_address) = (Elf64_Addr) query->address;
        }
    }

    return 0;
}

void *get_function_address(const char *name) {
    struct address_query query;
    query.name = name;
    query.address = NULL;

    dl_iterate_phdr(get_function_address_from_program_headers, &query);

    return query.address;
}

void replace_got_entries(const char *name, void *address) {
    struct address_query query;
    query.name = name;
    query.address = address;

    dl_iterate_phdr(replace_got_entry, &query);
}

const struct program_headers ProgramHeaders = {
        .get_function_address = get_function_address,
        .replace_got_entries = replace_got_entries
};