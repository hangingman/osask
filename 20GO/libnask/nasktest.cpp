#include <iostream>
#include <memory>
#include <sstream>
#include "libnask.hpp"

template <class T> void address_cout(T t);
template <class T> std::string address_get(T t);

template <class T> void address_cout(T t) {
  std::cout << std::hex << reinterpret_cast<void *>(t) << std::endl;
}

template <class T> std::string address_get(T t) {
  std::stringstream buf;
  buf << std::hex << reinterpret_cast<void *>(t) << std::endl;
  return buf.str();
}

void test_skipspace();
void test_putimm();
void test_nask();
void test_nask_buffer();

int main(int args, char** argv) {
  puts ("Start nask unit tests...");
  test_skipspace();
  test_putimm();
  test_nask();
  test_nask_buffer();
  puts ("Finish nask unit tests...");
  return 0;
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

void test_putimm() {
}

void test_nask() {
  puts("** It would be better creating child elements with smart-pointer");
  std::unique_ptr<STR_STATUS> status(new STR_STATUS());
  std::unique_ptr<STR_TERM> expression(new STR_TERM());
  status->expression = expression.get();
  address_cout(&status->expression);
  puts("=== [SUCCESS] ===");
}

void test_nask_buffer() {
  puts("** buf can be defined with std::unique_ptr<UCHAR[]>");
  std::unique_ptr<UCHAR[]> buf(new UCHAR[2 * 8]);
  nask32bitInt* bp;
  address_cout(buf.get());
  address_cout(&buf[0]);
  if (address_get(buf.get()) != address_get(&buf[0])) {
    exit(EXIT_FAILURE);
  } else {
    puts("=== [SUCCESS] ===");
  }

  puts("** bp can be set from std::unique_ptr<UCHAR[]>");
  bp = ucharToNask32bitIntPtr(&buf[0]);
  address_cout(bp);
  puts("=== [SUCCESS] ===");

  puts("** bp can increment");
  address_cout(bp);
  bp++;
  address_cout(bp);
  bp++;
  address_cout(bp);
  puts("=== [SUCCESS] ===");

}
