cmake_minimum_required(VERSION 3.16)
project(CPU_SIM_GUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Enable Qt6 MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Source files (exclude cpusim.cpp as it has its own main())
set(LOADER_SOURCES
    ElfLoader.cpp
    HexLoader.cpp
)

set(CPU_SOURCES
    CPU.cpp
    ALU.cpp
)

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

# Executable for GUI
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

# Include directories
target_include_directories(cpusim_gui PRIVATE .)

# Executable for command-line version
add_executable(cpusim
    ${CPU_SOURCES}
    ${LOADER_SOURCES}
    cpusim.cpp
)

# Unit tests: RVC expansion (no Qt)
add_executable(rvc_expand_test
    tests/rvc_expand_test.cpp
    ${CPU_SOURCES}
)
target_include_directories(rvc_expand_test PRIVATE .)

add_executable(golden_kernel_test
    tests/golden_kernel_test.cpp
    ${CPU_SOURCES}
    ${LOADER_SOURCES}
)
target_include_directories(golden_kernel_test PRIVATE .)

add_executable(hello_elf_test
    tests/hello_elf_test.cpp
    ${CPU_SOURCES}
    ${LOADER_SOURCES}
)
target_include_directories(hello_elf_test PRIVATE .)

# Optional: build examples/hello.elf when a RISC-V cross-compiler is available.
find_program(RISCV_GCC NAMES riscv64-elf-gcc riscv32-unknown-elf-gcc
    PATHS /opt/homebrew/bin /usr/local/bin)
set(HELLO_ELF "${CMAKE_BINARY_DIR}/hello.elf")
if(RISCV_GCC)
  add_custom_command(
    OUTPUT "${HELLO_ELF}"
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/build_example_elf.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Building hello.elf for CPU_SIM"
    VERBATIM)
  add_custom_target(hello_elf DEPENDS "${HELLO_ELF}")
endif()

enable_testing()
add_test(NAME rvc_expand_test COMMAND $<TARGET_FILE:rvc_expand_test>)
add_test(NAME m_ext_syscall COMMAND $<TARGET_FILE:cpusim> ${CMAKE_SOURCE_DIR}/instruction_memory/instMem-m-ext.txt)
set_tests_properties(m_ext_syscall PROPERTIES PASS_REGULAR_EXPRESSION "Syscall exit code: 42")
add_test(NAME golden_kernel_test COMMAND $<TARGET_FILE:golden_kernel_test>
    ${CMAKE_SOURCE_DIR}/instruction_memory/instMem-m-ext.txt)
if(RISCV_GCC)
  add_test(
    NAME hello_elf_test
    COMMAND bash -c
      "\"${CMAKE_SOURCE_DIR}/scripts/build_example_elf.sh\" && \
       \"$<TARGET_FILE:hello_elf_test>\" \"${HELLO_ELF}\"")
  add_test(
    NAME hello_elf_cpusim
    COMMAND bash -c
      "\"${CMAKE_SOURCE_DIR}/scripts/build_example_elf.sh\" && \
       \"$<TARGET_FILE:cpusim>\" \"${HELLO_ELF}\" --max-cycles 50000")
  set_tests_properties(hello_elf_cpusim PROPERTIES PASS_REGULAR_EXPRESSION "Syscall exit code: 42")
endif()
