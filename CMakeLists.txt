cmake_minimum_required(VERSION 3.16)
project(CPU_SIM_GUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_GUI "Build Qt6 GUI (cpusim_gui)" ON)

set(LOADER_SOURCES
    ElfLoader.cpp
    HexLoader.cpp
)

set(CPU_SOURCES
    CPU.cpp
    ALU.cpp
    CSRFile.cpp
    MMU.cpp
)

if(BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    set(GUI_SOURCES
        gui/main.cpp
        gui/MainWindow.cpp
        gui/PipelineWidget.cpp
        gui/StatsWidget.cpp
        gui/RegisterWidget.cpp
        gui/MemoryWidget.cpp
        gui/DependencyWidget.cpp
        gui/SimulatorController.cpp
    )

    set(GUI_HEADERS
        gui/MainWindow.h
        gui/PipelineWidget.h
        gui/StatsWidget.h
        gui/RegisterWidget.h
        gui/MemoryWidget.h
        gui/DependencyWidget.h
        gui/SimulatorController.h
    )

    add_executable(cpusim_gui
        ${CPU_SOURCES}
        ${LOADER_SOURCES}
        ${GUI_SOURCES}
        ${GUI_HEADERS}
    )

    target_link_libraries(cpusim_gui
        Qt6::Core
        Qt6::Widgets
        Qt6::Charts
    )
    target_include_directories(cpusim_gui PRIVATE .)
endif()

add_executable(cpusim
    ${CPU_SOURCES}
    ${LOADER_SOURCES}
    cpusim.cpp
)
target_include_directories(cpusim PRIVATE .)

function(add_cpu_test target source)
    add_executable(${target} ${source} ${CPU_SOURCES} ${LOADER_SOURCES})
    target_include_directories(${target} PRIVATE . ${CMAKE_SOURCE_DIR}/tests)
endfunction()

add_cpu_test(rvc_expand_test tests/rvc_expand_test.cpp)
add_cpu_test(golden_kernel_test tests/golden_kernel_test.cpp)
add_cpu_test(hello_elf_test tests/hello_elf_test.cpp)
add_cpu_test(load_use_test tests/load_use_test.cpp)
add_cpu_test(forwarding_test tests/forwarding_test.cpp)
add_cpu_test(fpu_decode_test tests/fpu_decode_test.cpp)
add_cpu_test(call_ret_elf_test tests/call_ret_elf_test.cpp)
add_cpu_test(executable_mode_test tests/executable_mode_test.cpp)
add_cpu_test(trap_test tests/trap_test.cpp)
add_cpu_test(mmu_test tests/mmu_test.cpp)

find_program(RISCV_GCC NAMES riscv64-elf-gcc riscv32-unknown-elf-gcc
    PATHS /opt/homebrew/bin /usr/local/bin)

enable_testing()
add_test(NAME rvc_expand_test COMMAND $<TARGET_FILE:rvc_expand_test>)
add_test(NAME m_ext_syscall COMMAND $<TARGET_FILE:cpusim> ${CMAKE_SOURCE_DIR}/instruction_memory/instMem-m-ext.txt)
set_tests_properties(m_ext_syscall PROPERTIES PASS_REGULAR_EXPRESSION "Syscall exit code: 42")
add_test(NAME golden_kernel_test COMMAND $<TARGET_FILE:golden_kernel_test>
    ${CMAKE_SOURCE_DIR}/instruction_memory/instMem-m-ext.txt)
add_test(NAME load_use_test COMMAND $<TARGET_FILE:load_use_test>
    ${CMAKE_SOURCE_DIR}/instruction_memory/instMem-load-use.txt)
add_test(NAME forwarding_test COMMAND $<TARGET_FILE:forwarding_test>
    ${CMAKE_SOURCE_DIR}/instruction_memory/instMem-forward.txt)
add_test(NAME fpu_decode_test COMMAND $<TARGET_FILE:fpu_decode_test>)
add_test(NAME executable_mode_test COMMAND $<TARGET_FILE:executable_mode_test>)
add_test(NAME trap_test COMMAND $<TARGET_FILE:trap_test>)
add_test(NAME mmu_test COMMAND $<TARGET_FILE:mmu_test>)

if(RISCV_GCC)
  add_test(
    NAME hello_elf_test
    COMMAND bash -c
      "\"${CMAKE_SOURCE_DIR}/scripts/build_example_elf.sh\" && \
       \"$<TARGET_FILE:hello_elf_test>\" \"${CMAKE_BINARY_DIR}/hello.elf\"")
  add_test(
    NAME hello_elf_cpusim
    COMMAND bash -c
      "\"${CMAKE_SOURCE_DIR}/scripts/build_example_elf.sh\" && \
       \"$<TARGET_FILE:cpusim>\" \"${CMAKE_BINARY_DIR}/hello.elf\" --max-cycles 50000")
  set_tests_properties(hello_elf_cpusim PROPERTIES PASS_REGULAR_EXPRESSION "Syscall exit code: 42")
  add_test(
    NAME call_ret_elf_test
    COMMAND bash -c
      "\"${CMAKE_SOURCE_DIR}/scripts/build_example_elf.sh\" && \
       \"$<TARGET_FILE:call_ret_elf_test>\" \"${CMAKE_BINARY_DIR}/call_ret.elf\"")
  set_tests_properties(call_ret_elf_test PROPERTIES PASS_REGULAR_EXPRESSION "call_ret_elf_test ok")
  add_test(
    NAME fib_print_cpusim
    COMMAND bash -c
      "\"${CMAKE_SOURCE_DIR}/scripts/build_example_elf.sh\" && \
       \"$<TARGET_FILE:cpusim>\" \"${CMAKE_BINARY_DIR}/fib_print.elf\" --max-cycles 200000")
  set_tests_properties(fib_print_cpusim PROPERTIES PASS_REGULAR_EXPRESSION "Syscall exit code: 0")
  add_test(
    NAME count_primes_cpusim
    COMMAND bash -c
      "\"${CMAKE_SOURCE_DIR}/scripts/build_example_elf.sh\" && \
       \"$<TARGET_FILE:cpusim>\" \"${CMAKE_BINARY_DIR}/count_primes.elf\" --max-cycles 300000")
  set_tests_properties(count_primes_cpusim PROPERTIES PASS_REGULAR_EXPRESSION "Syscall exit code: 0")
  add_test(
    NAME bench_smoke_test
    COMMAND bash -c
      "\"${CMAKE_SOURCE_DIR}/scripts/build_example_elf.sh\" && \
       \"$<TARGET_FILE:cpusim>\" \"${CMAKE_BINARY_DIR}/hello.elf\" --bench --json --max-cycles 50000")
  set_tests_properties(bench_smoke_test PROPERTIES PASS_REGULAR_EXPRESSION "\"syscall_exit_code\":42")
endif()
