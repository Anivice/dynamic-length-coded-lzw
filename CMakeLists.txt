cmake_minimum_required(VERSION 3.20)
project(LZW C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)
include_directories(src/include)
add_compile_definitions(VERSION="0.0.1")

add_library(liblzw STATIC
        src/misc/utils.cpp                      src/include/utils.h
        src/misc/args.cpp                       src/include/args.h
        src/lzw/mmap.cpp                        src/include/mmap.h
        src/include/lzw.inl                     src/include/lzw.h
        src/include/numeric.inl                 src/include/numeric.h
        src/include/lzw6.h
)
target_link_libraries(liblzw PUBLIC atomic)

add_executable(lzw src/main.cpp)
target_link_libraries(lzw PUBLIC liblzw)

# add_unit_test EXECUTABLE_NAME [FILES...]
function(add_unit_test EXECUTABLE_NAME)
    add_executable(${EXECUTABLE_NAME} ${ARGN})
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE liblzw)
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        if ("${SANITIZER}" STREQUAL "address")
            target_compile_options(${EXECUTABLE_NAME} PRIVATE -fsanitize=address -fsanitize=undefined)
            target_link_libraries(${EXECUTABLE_NAME} PRIVATE asan ubsan atomic)
        elseif("${SANITIZER}" STREQUAL "thread")
            target_compile_options(${EXECUTABLE_NAME} PRIVATE -fsanitize=thread -fno-sanitize-recover=undefined -fno-sanitize=undefined)
            target_link_libraries(${EXECUTABLE_NAME} PRIVATE tsan atomic)
        else ()
            target_compile_options(${EXECUTABLE_NAME} PUBLIC -g -O0 -fprofile-arcs -ftest-coverage)
            target_link_options(${EXECUTABLE_NAME} PUBLIC -g -O0 -fprofile-arcs -ftest-coverage)
        endif()
    endif()

    add_test(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_NAME})
endfunction()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_definitions(liblzw PUBLIC DEBUG=1)
    target_compile_definitions(lzw PUBLIC DEBUG=1)
    if ("${SANITIZER}" STREQUAL "address")
        message(STATUS "Address sanitizer")
        target_compile_options(liblzw PRIVATE -fsanitize=address -fsanitize=undefined)
        target_link_libraries(liblzw PRIVATE asan ubsan atomic)
        target_compile_options(lzw PRIVATE -fsanitize=address -fsanitize=undefined)
        target_link_libraries(lzw PRIVATE asan ubsan atomic)
    elseif("${SANITIZER}" STREQUAL "thread")
        message(STATUS "Thread sanitizer")
        target_compile_options(liblzw PRIVATE -fsanitize=thread -fno-sanitize-recover=undefined -fno-sanitize=undefined)
        target_link_libraries(liblzw PRIVATE tsan atomic)
        target_compile_options(lzw PRIVATE -fsanitize=thread -fno-sanitize-recover=undefined -fno-sanitize=undefined)
        target_link_libraries(lzw PRIVATE tsan atomic)
    endif()

    enable_testing()
else ()
    target_compile_definitions(liblzw PUBLIC DEBUG=0)
    target_compile_definitions(lzw PUBLIC DEBUG=0)
    string(REPLACE " " ";" CC_ADDITIONAL_OPTIONS_LIST "${CC_ADDITIONAL_OPTIONS}")
    string(REPLACE " " ";" LD_ADDITIONAL_OPTIONS_LIST "${LD_ADDITIONAL_OPTIONS}")
    set(compile_args
                -O3
                -fomit-frame-pointer
                -ffast-math
                -fstrict-aliasing
                -fdata-sections
                -ffunction-sections
                -D_FORTIFY_SOURCE=2
                -fstack-protector-strong
                -Wl,-z,relro -Wl,-z,now
                ${CC_ADDITIONAL_OPTIONS_LIST}
    )
    set(link_args
                -O3
                -fomit-frame-pointer
                -ffast-math
                -fstrict-aliasing
                -fdata-sections
                -ffunction-sections
                -Wl,--gc-sections
                -D_FORTIFY_SOURCE=2
                -fstack-protector-strong
                -Wl,-z,relro -Wl,-z,now
                ${LD_ADDITIONAL_OPTIONS_LIST}
    )

    target_compile_options(lzw PUBLIC ${compile_args})
    target_link_options(lzw PUBLIC ${link_args})

    target_compile_options(liblzw PUBLIC ${compile_args})
    target_link_options(liblzw PUBLIC ${link_args})
endif ()

add_unit_test(numeric src/tests/numeric.cpp)
add_unit_test(lzw_test src/tests/lzw.cpp src/include/lzw6.h)
