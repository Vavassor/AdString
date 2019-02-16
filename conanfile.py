from conans import ConanFile, CMake


class AftstringConan(ConanFile):
    name = "AftString"
    version = "1.0.0"
    license = "CC0-1.0"
    author = "Andrew Dawson dawso.andrew@gmail.com"
    url = "https://github.com/Vavassor/AftString/"
    description = "Growable string library"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = "cmake"
    exports_sources = "Source/*"

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="Source")
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["AftString"]
