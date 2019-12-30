project(memcpy)

include(GNUInstallDirs)

add_definitions("-O3")
add_executable(memcpy_test  memcpy.c)
target_link_libraries(memcpy_test pthread)

enable_language(ASM)

add_executable(bstmbw  bstmbw.c memcpy.o)
target_link_libraries(bstmbw pthread)

add_executable(neon_test  neon.cpp)
target_link_libraries(neon_test pthread)

add_executable(neon_ding  main.cpp)
target_link_libraries(neon_ding pthread)