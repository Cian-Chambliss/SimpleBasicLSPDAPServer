cmake_minimum_required(VERSION 3.16)
project(BasicInterpreter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Try to find nlohmann_json from system/vcpkg first
find_package(nlohmann_json QUIET)

# If not found, use FetchContent to download it
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(INTERPRETER_SOURCES
    src/interpreter/basic_interpreter.cpp
    src/interpreter/parser.cpp
    src/interpreter/lexer.cpp
    src/interpreter/runtime.cpp
    src/interpreter/variables.cpp
    src/interpreter/functions.cpp
)

set(LSP_SOURCES
    src/lsp/lsp_server.cpp
)

set(DAP_SOURCES
    src/dap/dap_server.cpp
)

set(MAIN_SOURCES
    src/main.cpp
)

# Create the main interpreter executable
add_executable(basic_interpreter ${MAIN_SOURCES} ${INTERPRETER_SOURCES} ${LSP_SOURCES} ${DAP_SOURCES})

# Link libraries
if(nlohmann_json_FOUND)
    # System/vcpkg installation
    target_link_libraries(basic_interpreter 
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    else()
        # FetchContent installation
        target_link_libraries(basic_interpreter 
            nlohmann_json
            Threads::Threads
        )
    endif()

# Set compiler flags
if(MSVC)
    target_compile_options(basic_interpreter PRIVATE /W4)
else()
    target_compile_options(basic_interpreter PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install target
install(TARGETS basic_interpreter DESTINATION bin) 