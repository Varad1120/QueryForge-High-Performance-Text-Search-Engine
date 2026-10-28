#include <catch2/catch_session.hpp>

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    Catch::Session session;
    const int result = session.run(argc, argv);

    // libpqxx's shared-library shutdown crashes in the current local test
    // environment even when tests use the fake repository. All test cases and
    // local fixtures have completed before exiting here.
    std::cout.flush();
    std::cerr.flush();
    std::_Exit(result);
}
