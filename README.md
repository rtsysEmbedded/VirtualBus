
# C++ Project

This is a reorganized C++ project structure with the following layout:

- `src/` - Source files.
- `include/` - Header files.
- `libs/` - External libraries.
- `tests/` - Unit tests.
- `cmake/` - CMake modules.
- `build/` - Build output (ignored in version control).


- Prerequisites for Each Platform
Linux: Install OpenSSL via your package manager:
bash
Copy code
sudo apt-get install libssl-dev
macOS: Install OpenSSL using Homebrew:
bash
Copy code
brew install openssl
Windows:
Install OpenSSL using a package like vcpkg.
Example:
bash
Copy code
vcpkg install openssl:x64-windows
Add vcpkg to your CMake toolchain:
bash
Copy code
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake