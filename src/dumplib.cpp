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
        throw runtime_error("bfd_openr failed");

    if (!bfd_check_format(b.get(), bfd_archive))
        throw runtime_error("not an archive");

    // FIXME
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
