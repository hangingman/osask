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
void test_ll_get_id();
void test_ll_calc_value();

int main(int args, char** argv) {
  puts ("Start nask unit tests...");
  test_skipspace();
  test_putimm();
  test_nask();
  test_nask_buffer();
  test_ll_get_id();
  //test_ll_calc_value();
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

void test_ll_get_id() {
  puts("** ll::get_id can return 4byte forwarded values");

  std::array<UCHAR, 4> buf1 = { 0x10, 0x00, 0x00, 0x00 };
  UCHAR* s1 = buf1.data();

  if (get_id(1, &s1, 0) != 16) {
    exit(EXIT_FAILURE);
  } else {
    puts("=== [SUCCESS] ===");
  }

  std::array<UCHAR, 4> buf2 = { 0x10, 0x10, 0x00, 0x00 };
  UCHAR* s2 = buf2.data();

  if (get_id(2, &s2, 0) != 4112) {
    exit(EXIT_FAILURE);
  } else {
    puts("=== [SUCCESS] ===");
  }

}

void test_ll_calc_value() {
  puts("** ll::calc_value can return legitimate values");

  std::unique_ptr<STR_VALUE> value(new STR_VALUE());
  std::array<UCHAR, 8> buf = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10 };
  UCHAR* s = buf.data();

  // 0x59 pattern
  s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
  s += 4;
  calc_value(value, &s);

  printf("STR_VALUE:len %s \n", value->to_string().c_str());

  puts("=== [SUCCESS] ===");
}
