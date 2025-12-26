#include "engine.hpp"
#include <exception>
#include <print>

int main() {
	try {
		sve::App{}.run();
	}
	catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	}
	catch (...) {
		std::println("PANIC!");
		return EXIT_FAILURE;
	}
}