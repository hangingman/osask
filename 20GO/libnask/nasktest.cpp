#include "nask.hpp"
#include <iostream>
#include <memory>

template <class T> void address_cout(T t);
void test_skipspace();

int main(int args, char** argv) {
     puts ("Start nask unit tests...");
     test_skipspace();
     puts ("Finish nask unit tests...");
     return 0;
}

template <class T> void address_cout(T t) {
     std::cout << std::hex << reinterpret_cast<void *>(t) << std::endl;
}

void test_skipspace() {
     // pointer will be skiped until next non-spaces
     puts("** pointer will be skiped until next non-spaces");
     UCHAR s[5] = { 0x20, 0x20, 0x20, 0x80, 0x20 };
     address_cout(&s[0]);
     address_cout(&s[1]);
     address_cout(&s[2]);
     address_cout(&s[3]);
     address_cout(&s[4]);
     UCHAR* t = &s[4];
     UCHAR* r = skipspace(&s[0], t);
     address_cout(r);

     if (r != &s[3]) {
	  exit(EXIT_FAILURE);
     } else {
	  puts("=== [SUCCESS] ===");
     }
}
