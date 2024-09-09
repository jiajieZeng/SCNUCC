CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
add_compile_definitions(ANTLR4CPP_STATIC)

# Configure CCache if available
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

include_directories(frontend/include)
include_directories(backend/include)
include_directories(dependencies/antlr4-runtime)

file(GLOB_RECURSE FRONTEND_SRC CONFIGURE_DEPENDS frontend/src/*.cpp)
file(GLOB_RECURSE BACKEND_SRC CONFIGURE_DEPENDS backend/src/*.cpp)
file(GLOB_RECURSE ANTLR4_RUNTIME dependencies/antlr4-runtime/*.cpp)

add_executable(${PROJECT_NAME} ${FRONTEND_SRC} ${BACKEND_SRC} ${ANTLR4_RUNTIME}
                backend/compiler.cpp)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    # Ensure target is a console application
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE OFF)
    # Linker flags
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -Wl,-subsystem,console")
else()
    # Linker flags
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-subsystem")
endif()

# Add necessary libraries
target_link_libraries(${PROJECT_NAME} Threads::Threads)
