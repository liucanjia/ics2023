#include <klibtest.h>

void (*entry)() = NULL;

int main(const char *args) {
    switch (args[0]) {
        CASE('m', memset_test);
        default:
            break;
    }
    return 0;
}