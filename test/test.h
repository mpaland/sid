///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.design)
//             2026, PALANDesign, LLC
//
// \license The MIT License (MIT)
//
// This file is part of the sid library.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief sid tests: a tiny test harness, no framework needed
//        TEST_CASE(name) { ... } defines and registers a test case; inside it
//          CHECK(expr)             counts a failed check and goes on
//          CHECK_MSG(expr, text)   the same with a remark
//          CHECK_EQ(a, b)          a == b, shows both values when they differ
//          REQUIRE(expr)           ends the test case when it fails
//        test::run(argc, argv) in main() runs all cases, or those whose name starts with the text behind
//        --filter, and returns the exit code: 0 when no check failed.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SID_TEST_H_
#define _SID_TEST_H_

#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>


namespace test {

typedef struct tag_case_type {
  const char*   name;
  void          (*function)();
} case_type;

typedef struct tag_state_type {
  std::vector<case_type>  cases;
  const char*             running;      // name of the case that runs
  unsigned long           checks;
  unsigned long           failures;
} state_type;

inline state_type& state()
{
  static state_type s = { std::vector<case_type>(), "", 0UL, 0UL };
  return s;
}

// registers a test case before main() runs
class registrar
{
public:
  registrar(const char* name, void (*function)())
  {
    const case_type c = { name, function };
    state().cases.push_back(c);
  }
};

inline bool check(const char* file, int line, const char* expression, bool ok, const char* remark = nullptr)
{
  ++state().checks;
  if (!ok) {
    ++state().failures;
    std::printf("%s(%d): FAILED [%s]: %s%s%s\n", file, line, state().running, expression,
                (remark != nullptr) ? " - " : "", (remark != nullptr) ? remark : "");
  }
  return ok;
}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4389 4018)      // signed / unsigned comparison: the values of a test are what they are
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
template<typename A, typename B>
bool check_eq(const char* file, int line, const char* a_text, const char* b_text, const A& a, const B& b)
{
  const bool ok = (a == b);
  if (ok) {
    return check(file, line, a_text, true);
  }
  std::ostringstream values;
  values << a_text << " == " << b_text << " (" << +a << " != " << +b << ")";
  return check(file, line, values.str().c_str(), false);
}
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

inline int run(int argc, char** argv)
{
  const char* filter = "";
  for (int i = 1; (i + 1) < argc; ++i) {
    if (std::strcmp(argv[i], "--filter") == 0) {
      filter = argv[i + 1];
    }
  }
  unsigned long ran = 0UL;
  for (std::size_t i = 0U; i < state().cases.size(); ++i) {
    const case_type& c = state().cases[i];
    if (std::strncmp(c.name, filter, std::strlen(filter)) != 0) {
      continue;
    }
    const unsigned long before = state().failures;
    state().running = c.name;
    c.function();
    ++ran;
    if (state().failures != before) {
      std::printf("  [FAIL] %s\n", c.name);
    }
  }
  std::printf("\n%lu tests, %lu checks, %lu failures\n", ran, state().checks, state().failures);
  return ((state().failures == 0UL) && (ran != 0UL)) ? 0 : 1;
}

} // namespace test


#define TEST_CASE(name)                                                         \
  static void test_case_##name();                                               \
  static const ::test::registrar test_registrar_##name(#name, &test_case_##name);   \
  static void test_case_##name()

#define CHECK(expr)             ::test::check(__FILE__, __LINE__, #expr, !!(expr))
#define CHECK_MSG(expr, text)   ::test::check(__FILE__, __LINE__, #expr, !!(expr), (text))
#define CHECK_EQ(a, b)          ::test::check_eq(__FILE__, __LINE__, #a, #b, (a), (b))
#define REQUIRE(expr)           do { if (!::test::check(__FILE__, __LINE__, #expr, !!(expr), "REQUIRE failed, test case ended")) { return; } } while (0)

#endif  // _SID_TEST_H_
