cmake_minimum_required(VERSION 3.14)
project(ArenaNet VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# FetchContent for header-only libraries
include(FetchContent)

# Asio (Standalone)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-28-0
)
FetchContent_MakeAvailable(asio)

# Nlohmann JSON
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# jwt-cpp
FetchContent_Declare(
    jwt_cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG        v0.7.0
)
FetchContent_MakeAvailable(jwt_cpp)

# redis-plus-plus
FetchContent_Declare(
    redis_plus_plus
    GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
    GIT_TAG        1.3.10
)
FetchContent_MakeAvailable(redis_plus_plus)

# System libraries (to be installed via package manager or Docker)
find_package(PostgreSQL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${asio_SOURCE_DIR}/asio/include
    ${PostgreSQL_INCLUDE_DIRS}
)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Executable
add_executable(ArenaNet ${SOURCES})

# Definitions
target_compile_definitions(ArenaNet PRIVATE ASIO_STANDALONE)

# Linking
target_link_directories(ArenaNet PRIVATE /opt/homebrew/lib)
target_link_libraries(ArenaNet
    PRIVATE
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
    pqxx
    ${PostgreSQL_LIBRARIES}
    redis++
    hiredis
    pthread
)

# Testing (optional for MVP)
# enable_testing()
# add_subdirectory(tests)
