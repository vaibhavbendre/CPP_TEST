"""Conan 2 recipe for the cinema booking library.

BONUS REQUIREMENT SATISFIED: "(Bonus) Conan package management" (design.md section 10).
No third-party dependencies: the library and its tests use only the C++ standard library.

Usage (from the repository root):

    conan profile detect --force            # once per machine
    conan create . --build=missing          # build, run tests, package into the local cache

Consumers of the package then write:

    find_package(cinema REQUIRED)
    target_link_libraries(app PRIVATE cinema::cinema)
"""
from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout


class CinemaConan(ConanFile):
    name = "cinema"
    version = "1.0.0"
    description = "In-memory, thread-safe movie ticket booking backend (C++17)"
    license = "MIT"
    package_type = "static-library"
    settings = "os", "compiler", "build_type", "arch"

    # Sources are exported with the recipe so `conan create .` works from a clean cache.
    exports_sources = ("CMakeLists.txt", "include/*", "src/*", "app/*", "tests/*")

    def validate(self):
        check_min_cppstd(self, 17)  # the code uses std::optional

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CINEMA_BUILD_CLI"] = False  # the demo CLI is not part of the package
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if not self.conf.get("tools.build:skip_test", default=False):
            cmake.test()

    def package(self):
        CMake(self).install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "cinema")
        self.cpp_info.set_property("cmake_target_name", "cinema::cinema")
        self.cpp_info.libs = ["cinema"]
        if self.settings.os in ("Linux", "FreeBSD"):
            self.cpp_info.system_libs.append("pthread")  # std::mutex / std::thread
