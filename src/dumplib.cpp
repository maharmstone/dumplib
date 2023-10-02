#define PACKAGE
#include <bfd.h>
#include <memory>
#include <iostream>
#include <format>

using namespace std;

class bfd_exception : public exception {
public:
    bfd_exception(string_view func) {
        msg = string(func) + " failed (" + bfd_errmsg(bfd_get_error()) + ")";
    }

    const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    string msg;
};

class bfd_closer {
public:
    using pointer = bfd*;

    void operator()(bfd* b) {
        bfd_close(b);
    }
};

using bfd_ptr = unique_ptr<bfd*, bfd_closer>;

static void print_symbols(bfd* b, string_view archive) {
    uint8_t* ptr;
    unsigned int size;

    if (!(bfd_get_file_flags(b) & HAS_SYMS))
        return;

    auto count = bfd_read_minisymbols(b, 0, (void**)&ptr, &size);

    // FIXME - get DLL name from .idata?

    auto fn = bfd_get_filename(b);

    if (count <= 0)
        return;

    auto store = bfd_make_empty_symbol(b);
    if (!store)
        throw bfd_exception("bfd_make_empty_symbol");

    unique_ptr<uint8_t> v{ptr};

    for (unsigned int i = 0; i < count; i++) {
        symbol_info info;

        auto sym = bfd_minisymbol_to_symbol(b, false, ptr, store);
        if (!sym)
            throw bfd_exception("bfd_minisymbol_to_symbol");

        bfd_get_symbol_info(b, sym, &info);

        if (info.type == 'T' && strcmp(info.name, ".text"))
            cout << format("{}\t{}\t{}\n", archive, fn, info.name);

        ptr += size;
    }
}

static void do_file(const char* fn) {
    bfd_ptr b{bfd_openr(fn, nullptr)};

    if (!b)
        throw bfd_exception("bfd_openr");

    if (!bfd_check_format(b.get(), bfd_archive))
        throw runtime_error("not an archive");

    bfd_ptr ar;

    while (true) {
        char** matching;

        if (bfd* tmp = bfd_openr_next_archived_file(b.get(), ar.get()); tmp)
            ar.reset(tmp);
        else {
            if (bfd_get_error() == bfd_error_no_more_archived_files)
                break;

            throw bfd_exception("bfd_openr_next_archived_file");
        }

        if (!bfd_check_format_matches(ar.get(), bfd_object, &matching)) {
            bool pe_i386 = false;

            if (bfd_get_error() != bfd_error_file_ambiguously_recognized)
                throw bfd_exception("bfd_check_format_matches");

            if (matching) {
                unsigned int i = 0;
                while (matching[i]) {
                    if (!strcmp(matching[i], "pe-i386")) {
                        pe_i386 = true;
                        break;
                    }

                    i++;
                }

                free(matching);
            }

            if (!pe_i386)
                throw bfd_exception("bfd_check_format_matches");
        }

        print_symbols(ar.get(), fn);
    }
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        try {
            do_file(argv[i]);
        } catch (const exception& e) {
            cerr << format("Failed to parse {}: {}\n", argv[i], e.what());
        }
    }

    return 0;
}
