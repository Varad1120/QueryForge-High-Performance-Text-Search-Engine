cmake_minimum_required(VERSION 3.16)
project(queryforge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find Catch2 for unit tests
find_package(Catch2 3 QUIET)
if (NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# Find libpqxx for repository persistence
if (APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/opt/homebrew/opt/libpqxx" "/opt/homebrew/opt/libpq")
endif()

find_package(PkgConfig QUIET)
find_package(libpqxx QUIET)
if (NOT TARGET libpqxx::pqxx)
    find_path(LIBPQXX_INCLUDE_DIR pqxx/pqxx PATHS /opt/homebrew/include /opt/homebrew/opt/libpqxx/include /usr/local/include)
    find_library(LIBPQXX_LIBRARY NAMES pqxx PATHS /opt/homebrew/lib /opt/homebrew/opt/libpqxx/lib /usr/local/lib)
    find_path(LIBPQ_INCLUDE_DIR libpq-fe.h PATHS /opt/homebrew/include /opt/homebrew/opt/libpq/include /usr/local/include)
    find_library(LIBPQ_LIBRARY NAMES pq PATHS /opt/homebrew/lib /opt/homebrew/opt/libpq/lib /usr/local/lib)
    if (LIBPQXX_INCLUDE_DIR AND LIBPQXX_LIBRARY)
        add_library(libpqxx::pqxx UNKNOWN IMPORTED)
        set_target_properties(libpqxx::pqxx PROPERTIES
            IMPORTED_LOCATION "${LIBPQXX_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBPQXX_INCLUDE_DIR}"
        )
        if (LIBPQ_INCLUDE_DIR)
            set_property(TARGET libpqxx::pqxx APPEND PROPERTY
                INTERFACE_INCLUDE_DIRECTORIES "${LIBPQ_INCLUDE_DIR}")
        endif()
        if (LIBPQ_LIBRARY)
            set_property(TARGET libpqxx::pqxx APPEND PROPERTY
                INTERFACE_LINK_LIBRARIES "${LIBPQ_LIBRARY}")
        endif()
    endif()
endif()

# Library: Normalizer
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/normalizer.cpp)
    add_library(normalizer_lib ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/normalizer.cpp)
    target_include_directories(normalizer_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

# Library: Trie
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/structures/trie.cpp)
    add_library(trie_lib ${CMAKE_CURRENT_SOURCE_DIR}/src/structures/trie.cpp)
    target_include_directories(trie_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    if (TARGET normalizer_lib)
        target_link_libraries(trie_lib PUBLIC normalizer_lib)
    endif()
endif()

# Library: Suffix Array
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/structures/suffix_array.cpp)
    add_library(suffix_array_lib ${CMAKE_CURRENT_SOURCE_DIR}/src/structures/suffix_array.cpp)
    target_include_directories(suffix_array_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

# Library: AVL Tree
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/structures/avl_tree.cpp)
    add_library(avl_tree_lib ${CMAKE_CURRENT_SOURCE_DIR}/src/structures/avl_tree.cpp)
    target_include_directories(avl_tree_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

# Library: Max-Heap
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/structures/max_heap.cpp)
    add_library(max_heap_lib ${CMAKE_CURRENT_SOURCE_DIR}/src/structures/max_heap.cpp)
    target_include_directories(max_heap_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

# Library: Document Repository
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/repository/document_repository.cpp)
    add_library(document_repository_lib ${CMAKE_CURRENT_SOURCE_DIR}/src/repository/document_repository.cpp)
    target_include_directories(document_repository_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    if (TARGET libpqxx::pqxx)
        target_link_libraries(document_repository_lib PUBLIC libpqxx::pqxx)
    elseif (LIBPQXX_FOUND)
        target_include_directories(document_repository_lib PUBLIC ${LIBPQXX_INCLUDE_DIRS})
        target_link_libraries(document_repository_lib PUBLIC ${LIBPQXX_LIBRARIES})
    endif()
endif()

# Library: Search Engine
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/search_engine.cpp)
    add_library(search_engine_lib ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/search_engine.cpp)
    target_include_directories(search_engine_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    if (TARGET trie_lib)
        target_link_libraries(search_engine_lib PUBLIC trie_lib)
    endif()
    if (TARGET suffix_array_lib)
        target_link_libraries(search_engine_lib PUBLIC suffix_array_lib)
    endif()
    if (TARGET avl_tree_lib)
        target_link_libraries(search_engine_lib PUBLIC avl_tree_lib)
    endif()
    if (TARGET max_heap_lib)
        target_link_libraries(search_engine_lib PUBLIC max_heap_lib)
    endif()
    if (TARGET document_repository_lib)
        target_link_libraries(search_engine_lib PUBLIC document_repository_lib)
    endif()
endif()

find_package(Threads REQUIRED)

# Main executable
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
    add_executable(queryforge ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
    target_include_directories(queryforge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(queryforge PRIVATE Threads::Threads)
    if (TARGET search_engine_lib)
        target_link_libraries(queryforge PRIVATE search_engine_lib)
    endif()
    if (TARGET trie_lib)
        target_link_libraries(queryforge PRIVATE trie_lib)
    endif()
    if (TARGET normalizer_lib)
        target_link_libraries(queryforge PRIVATE normalizer_lib)
    endif()
    if (TARGET document_repository_lib)
        target_link_libraries(queryforge PRIVATE document_repository_lib)
    endif()
endif()

# Enable Testing
enable_testing()

# Normalizer Tests
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/normalizer_tests.cpp AND TARGET normalizer_lib)
    add_executable(normalizer_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/normalizer_tests.cpp)
    target_link_libraries(normalizer_tests PRIVATE normalizer_lib Catch2::Catch2WithMain)
    add_test(NAME normalizer_tests COMMAND normalizer_tests)
endif()

# Trie Tests
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/trie_tests.cpp AND TARGET trie_lib)
    add_executable(trie_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/trie_tests.cpp)
    target_link_libraries(trie_tests PRIVATE trie_lib Catch2::Catch2WithMain)
    add_test(NAME trie_tests COMMAND trie_tests)
endif()

# Suffix Array Tests
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/suffix_array_tests.cpp AND TARGET suffix_array_lib)
    add_executable(suffix_array_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/suffix_array_tests.cpp)
    target_link_libraries(suffix_array_tests PRIVATE suffix_array_lib Catch2::Catch2WithMain)
    add_test(NAME suffix_array_tests COMMAND suffix_array_tests)
endif()

# AVL Tree Tests
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/avl_tree_tests.cpp AND TARGET avl_tree_lib)
    add_executable(avl_tree_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/avl_tree_tests.cpp)
    target_link_libraries(avl_tree_tests PRIVATE avl_tree_lib Catch2::Catch2WithMain)
    add_test(NAME avl_tree_tests COMMAND avl_tree_tests)
endif()

# Max Heap Tests
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/max_heap_tests.cpp AND TARGET max_heap_lib)
    add_executable(max_heap_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/max_heap_tests.cpp)
    target_link_libraries(max_heap_tests PRIVATE max_heap_lib Catch2::Catch2WithMain)
    add_test(NAME max_heap_tests COMMAND max_heap_tests)
endif()

# DB Repository Tests
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/db_repository_tests.cpp AND TARGET document_repository_lib)
    add_executable(db_repository_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/db_repository_tests.cpp)
    target_link_libraries(db_repository_tests PRIVATE document_repository_lib Catch2::Catch2WithMain)
    add_test(NAME db_repository_tests COMMAND db_repository_tests)
    set_tests_properties(db_repository_tests PROPERTIES
        LABELS "database;integration"
    )
    if (NOT DEFINED ENV{QUERYFORGE_DB_URL} OR "$ENV{QUERYFORGE_DB_URL}" STREQUAL "")
        # Avoid running the libpqxx-linked process when no database was
        # configured; connection failures in this environment can abort in
        # libpqxx teardown. Reconfigure after setting QUERYFORGE_DB_URL.
        set_tests_properties(db_repository_tests PROPERTIES DISABLED TRUE)
    endif()
endif()

# Integration Tests
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_tests.cpp AND TARGET search_engine_lib)
    add_executable(integration_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_tests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/search_engine_test_main.cpp
    )
    target_link_libraries(integration_tests PRIVATE search_engine_lib Catch2::Catch2)
    add_test(NAME integration_tests COMMAND integration_tests)
endif()

# Search Engine Unit Tests
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/search_engine_tests.cpp AND TARGET search_engine_lib)
    add_executable(search_engine_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/search_engine_tests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/search_engine_test_main.cpp
    )
    target_link_libraries(search_engine_tests PRIVATE search_engine_lib Catch2::Catch2)
    add_test(NAME search_engine_tests COMMAND search_engine_tests)
endif()

# Benchmarks are deliberately built as a separate executable and are not
# registered with CTest. Execution requires an explicit test-database URL.
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/benchmark_main.cpp AND TARGET search_engine_lib)
    add_executable(queryforge_benchmarks ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/benchmark_main.cpp)
    target_link_libraries(queryforge_benchmarks PRIVATE search_engine_lib)
endif()
