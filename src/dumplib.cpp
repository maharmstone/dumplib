#define PACKAGE
#include <bfd.h>
#include <memory>
#include <iostream>
#include <format>

using namespace std;

class bfd_closer {
public:
    using pointer = bfd*;

    void operator()(bfd* b) {
        bfd_close(b);
    }
};

using bfd_ptr = unique_ptr<bfd*, bfd_closer>;

static void print_symbols(bfd* b) {
    uint8_t* ptr;
    unsigned int size;

    auto count = bfd_read_minisymbols(b, 0, (void**)&ptr, &size);

    cout << "---" << endl;
    // FIXME - print archive filename

    if (count <= 0)
        return;

    auto store = bfd_make_empty_symbol(b);
    if (!store)
        throw runtime_error("bfd_make_empty_symbol failed");

    unique_ptr<uint8_t> v{ptr};

    for (unsigned int i = 0; i < count; i++) {
        symbol_info info;

        auto sym = bfd_minisymbol_to_symbol(b, false, ptr, store);
        if (!sym)
            throw runtime_error("bfd_minisymbol_to_symbol failed");

        bfd_get_symbol_info(b, sym, &info);

        cout << format("{} {}\n", info.type, info.name);

        // FIXME - only T (not .text)

        ptr += size;
    }
}

static void do_file(const char* fn) {
    bfd_ptr b{bfd_openr(fn, nullptr)};

    if (!b)
        throw runtime_error("bfd_openr failed"); // FIXME - include error

    if (!bfd_check_format(b.get(), bfd_archive))
        throw runtime_error("not an archive");

    bfd_ptr ar;

    while (true) {
        if (bfd* tmp = bfd_openr_next_archived_file(b.get(), ar.get()); tmp)
            ar.reset(tmp);
        else {
            if (bfd_get_error() == bfd_error_no_more_archived_files)
                break;

            throw runtime_error("bfd_openr_next_archived_file failed"); // FIXME - include error
        }

        if (!bfd_check_format_matches(ar.get(), bfd_object, nullptr))
            throw runtime_error("file in archive was not an object");

        print_symbols(ar.get());
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
