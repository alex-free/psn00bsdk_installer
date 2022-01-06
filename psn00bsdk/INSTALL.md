
# Getting started with PSn00bSDK

## Building and installing

The instructions below are for Windows and Linux. Building on macOS hasn't been
tested extensively yet, however it should work once the GCC toolchain is built
and installed properly.

1. Install prerequisites and a host compiler toolchain. On Linux (most distros)
   install the following packages from your distro's package manager:

   - `git`
   - `build-essential`, `base-devel` or similar
   - `make` or `ninja-build`
   - `cmake` (3.20+ is required, download it from
     [here](https://cmake.org/download) if your package manager only provides
     older versions)

   On Windows install [MSys2](https://www.msys2.org), then open the "MSys2
   MSYS" shell and run this command:

   ```bash
   pacman -Syu git mingw-w64-x86_64-make mingw-w64-x86_64-ninja mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc
   ```

   If you are prompted to close the shell, you may have to reopen it afterwards
   and rerun the command to finish installation.
   **Do not use the MSys2 shell for the next steps**, use a regular command
   prompt or PowerShell instead.

   Add these directories (replace `C:\msys64` if you installed MSys2 to a
   different location) to the `PATH` environment variable using System
   Properties:

   - `C:\msys64\mingw64\bin`
   - `C:\msys64\usr\bin`

2. Build and install a GCC toolchain for `mipsel-unknown-elf`, as detailed in
   [TOOLCHAIN.md](TOOLCHAIN.md). On Windows, you may download a precompiled
   version from [Lameguy64's website](http://lameguy64.net?page=psn00bsdk) and
   extract it into one of the directories listed below instead.

   **NOTE**: PSn00bSDK is also compatible with toolchains that target
   `mipsel-none-elf`. If you already have such a toolchain (e.g. because you
   have another PS1 SDK installed) you can skip this step. Make sure you pass
   `-DPSN00BSDK_TARGET=mipsel-none-elf` to CMake when configuring the SDK
   though (see step 5).

3. If you chose a non-standard install location for the toolchain, add the
   `bin` subfolder (inside the top-level toolchain directory) to the `PATH`
   environment variable. This step is unnecessary if you installed/extracted
   the toolchain into any of these directories:

   - `C:\Program Files\mipsel-unknown-elf`
   - `C:\Program Files (x86)\mipsel-unknown-elf`
   - `C:\mipsel-unknown-elf`
   - `/usr/local/mipsel-unknown-elf`
   - `/usr/mipsel-unknown-elf`
   - `/opt/mipsel-unknown-elf`

4. Clone the PSn00bSDK repo, then run the following command from the PSn00bSDK
   repository to download additional dependencies:

   ```bash
   git submodule update --init --recursive --remote
   ```

5. Compile the libraries, tools and examples using CMake:

   ```bash
   cmake --preset default .
   cmake --build ./build
   ```

   If you want to install the SDK to a custom location rather than the default
   one (`C:\Program Files\PSn00bSDK` or `/usr/local` depending on your OS), add
   `--install-prefix <INSTALL_PATH>` to the first command. Add
   `-DPSN00BSDK_TARGET=mipsel-none-elf` if your toolchain targets
   `mipsel-none-elf` rather than `mipsel-unknown-elf`.

   **NOTE**: Ninja is used by default to build the SDK. If you can't get it to
   work or don't have it installed, pass `-G "Unix Makefiles"` (or
   `-G "MSYS Makefiles"` on Windows) to the first command to build using `make`
   instead.

6. Install the SDK to the path you chose (add `sudo` or run it from a command
   prompt with admin privileges if necessary):

   ```bash
   cmake --install ./build
   ```

   This will create and populate the following directories:

   - `<INSTALL_PATH>/bin`
   - `<INSTALL_PATH>/lib/libpsn00b`
   - `<INSTALL_PATH>/share/psn00bsdk`

7. Set the `PSN00BSDK_LIBS` environment variable to point to the `lib/libpsn00b`
   subfolder inside the install directory. You might also want to add the `bin`
   folder to `PATH` if it's not listed already.

Although not strictly required, you'll probably want to install a PS1 emulator
with debugging capabilities such as [no$psx](https://problemkaputt.de/psx.htm)
(Windows only), [DuckStation](https://github.com/stenzek/duckstation) or
[pcsx-redux](https://github.com/grumpycoders/pcsx-redux).
**Avoid ePSXe and anything based on MAME** as they are inaccurate.

## Building installer packages

CPack can be used to build NSIS-based installers, DEB/RPM packages and zipped
releases that include built SDK libraries, headers as well as the GCC toolchain.
Distributing prebuilt releases is however discouraged since PSn00bSDK is still
far from being feature-complete.

1. Follow steps 1-4 above to set up the toolchain, then install
   [NSIS](https://nsis.sourceforge.io/Download) on Windows or `dpkg` and `rpm`
   on Linux.

2. Run the following commands from the PSn00bSDK directory (pass the
   appropriate options to the first command if necessary):

   ```bash
   cmake --preset package .
   cmake --build ./build -t package
   ```

   All built packages will be copied to the `build/packages` folder.

## Creating a project

1. Copy the contents of `<INSTALL_PATH>/share/psn00bsdk/template` (or the
   `template` folder within the repo) to your new project's root directory.

2. Configure and build the template by running:

   ```bash
   cmake -S . -B ./build
   cmake --build ./build
   ```

   If you did everything correctly there should be a `template.bin` CD image in
   the `build` folder. Test it in an emulator to ensure it works.

Note that, even though the template relies on the `PSN00BSDK_LIBS` environment
variable to locate the SDK by default, you can also specify the path directly
on the CMake command line by adding
`-DCMAKE_TOOLCHAIN_FILE=<INSTALL_PATH>/lib/libpsn00b/cmake/sdk.cmake` to the
CMake command line.

The toolchain script defines a few CMake macros to create PS1 executables, DLLs
and CD images. See the [reference](doc/cmake_reference.md) for details.

-----------------------------------------
_Last updated on 2021-11-19 by spicyjpeg_
