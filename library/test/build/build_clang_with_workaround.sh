clang++-7 ../../src/safe_ptr.cpp ../test_safe_pointers.cpp ../../src/iibmalloc/src/foundation/src/std_error.cpp ../../src/iibmalloc/src/foundation/src/safe_memory_error.cpp ../../src/iibmalloc/src/foundation/src/log.cpp ../../src/iibmalloc/src/foundation/src/tagged_ptr_impl.cpp ../../src/iibmalloc/src/foundation/3rdparty/fmt/src/format.cc ../../src/iibmalloc/src/page_allocator_linux.cpp ../../src/iibmalloc/src/iibmalloc.cpp -I../../src/iibmalloc/src -I../../src -I../../src/iibmalloc/src/foundation/include -I../../src/iibmalloc/src/foundation/3rdparty/fmt/include -L/root/tests/safe-memory/test/build -lgcc_lto_workaround -std=c++17 -DUSING_T_SOCKETS -g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-empty-body -DNDEBUG -O2 -flto -lpthread -o test.bin