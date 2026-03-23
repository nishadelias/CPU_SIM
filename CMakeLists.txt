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
    cpusim.cpp
)

# Unit tests: RVC expansion (no Qt)
add_executable(rvc_expand_test
    tests/rvc_expand_test.cpp
    ${CPU_SOURCES}
)
target_include_directories(rvc_expand_test PRIVATE .)

enable_testing()
add_test(NAME rvc_expand_test COMMAND $<TARGET_FILE:rvc_expand_test>)
add_test(NAME m_ext_syscall COMMAND $<TARGET_FILE:cpusim> ${CMAKE_SOURCE_DIR}/instruction_memory/instMem-m-ext.txt)
set_tests_properties(m_ext_syscall PROPERTIES PASS_REGULAR_EXPRESSION "Syscall exit code: 42")
