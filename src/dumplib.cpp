#define PACKAGE
#include <bfd.h>
#include <memory>
#include <iostream>

using namespace std;

class bfd_closer {
public:
    using pointer = bfd*;

    void operator()(bfd* b) {
        bfd_close(b);
    }
};

using bfd_ptr = unique_ptr<bfd*, bfd_closer>;

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

        // FIXME - print symbols
    }
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        try {
            do_file(argv[i]);
        } catch (const exception& e) {
            cerr << "Failed to parse " << argv[i] << ": " << e.what() << endl;
        }
    }

    return 0;
}
