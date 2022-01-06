
# PSn00bSDK CMake reference

## Setup

The only requirement to use the SDK in CMake is to set the
`CMAKE_TOOLCHAIN_FILE` variable to `INSTALL_PATH/lib/libpsn00b/cmake/sdk.cmake`
(where `INSTALL_PATH` is the install prefix PSn00bSDK is installed to). This
can be done on the command line (`-DCMAKE_TOOLCHAIN_FILE=...`), in
`CMakeLists.txt` (before calling `project()`) or using a
[preset](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html).

It's recommended to put this snippet in `CMakeLists.txt` to automatically set
the toolchain file according to the `PSN00BSDK_LIBS` environment variable:

```cmake
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{PSN00BSDK_LIBS})
  set(CMAKE_TOOLCHAIN_FILE $ENV{PSN00BSDK_LIBS}/cmake/sdk.cmake)
endif()
```

See the [template](../template/CMakeLists.txt) for an example CMake script
showing how to build a simple project.

## Targets

These targets are defined when using PSn00bSDK. There is no need to explicitly
link against any of these, as the helper commands (see below) handle linking
behind the scenes. To avoid conflicts, however, no target should be given any
of these names.

- `c`, `psxgpu`, `psxgte`, `psxspu`, `psxcd`, `psxsio`, `psxetc`, `psxapi`, `lzp`
- `psn00bsdk_common`, `psn00bsdk_object_lib`
- `psn00bsdk_static_exe`
- `psn00bsdk_dynamic_exe`
- `psn00bsdk_static_lib`
- `psn00bsdk_shared_lib`, `psn00bsdk_module_lib`

## Commands

- `psn00bsdk_add_executable(<name> <STATIC|DYNAMIC> [EXCLUDE_FROM_ALL] [sources...])`

  A wrapper around `add_executable()` to create PS1 executables. Three files
  will be generated for each call to this function:

  - `<name>.elf` (regular ELF executable)
  - `<name>.exe` (executable converted to the format expected by the PS1)
  - `<name>.map` (symbol map file for dynamic linking/introspection)

  The `.exe` and `.map` extensions can be customized by overriding
  `PSN00BSDK_EXECUTABLE_SUFFIX` and `PSN00BSDK_SYMBOL_MAP_SUFFIX` prior to
  creating the executable.

  The second argument (mandatory) specifies whether the executable is going to
  load DLLs at runtime. If set to `STATIC`, $gp-relative addressing (i.e.
  reusing the $gp register normally used for DLL addressing) will be enabled,
  slightly reducing executable size and RAM usage but breaking compatibility
  with the dynamic linker.

- `psn00bsdk_add_library(<name> <STATIC|OBJECT|SHARED|MODULE> [EXCLUDE_FROM_ALL] [sources...])`

  Wraps `add_library()` to create static libraries or dynamically-linked
  libraries (DLLs).

  The second argument (mandatory, unlike `add_library()`) specifies the type of
  library to create. `STATIC` will create a static library named `lib<name>.a`.
  `SHARED` and `MODULE` will compile a DLL, producing the following files (note
  that there is no `lib` prefix for DLLs):

  - `<name>.so` (regular ELF shared library)
  - `<name>.dll` (raw binary with some ELF headers prepended)

  As with executables, the `.dll` extension can be customized by setting
  `PSN00BSDK_SHARED_LIBRARY_SUFFIX`.

- `psn00bsdk_add_cd_image(<name> <image name> <config file> [...])`

  Creates a new target that will build a CD image using `mkpsxiso`.

  The first argument is the name of the target to create; next up is the name
  of the generated image file (`<image name>.bin` + `<image name>.cue`). The
  third argument is the path to the XML file passed to `mkpsxiso`.

  The XML file is "configured" by CMake, i.e. any `${var}` or `@var@`
  expressions are replaced with the values of the respective variables. In
  particular `${CD_IMAGE_NAME}` is replaced with the second argument passed to
  `psn00bsdk_add_cd_image()`; the file must properly set the output file names
  like this:

  ```xml
  <?xml version="1.0" encoding="utf-8"?>
  <iso_project
    image_name="${CD_IMAGE_NAME}.bin"
    cue_sheet="${CD_IMAGE_NAME}.cue"
  >
  ```

  Any additional argument is passed through to the underlying call to
  `add_custom_target()`, so most of the options supported by
  `add_custom_target()` are also supported here.

## Definitions

When compiling executables and libraries using the above commands the following
preprocessor macros are automatically `#define`'d:

- `PLAYSTATION`

  Always set to 1. Can be used to implement different options or code paths for
  libraries, so they can target both the host and PS1 (as it won't be defined
  when compiling outside of the SDK).

- `DEBUG`

  Defined and set to 1 in a debug configuration, i.e. when the
  `CMAKE_BUILD_TYPE` variable is set to `Debug`. This value is used by the
  PSn00bSDK libraries, and should be used in executables, to enable additional
  debug logging.

  Note that the default CMake configuration is usually debug, so it's
  recommended to specify `-DCMAKE_BUILD_TYPE=Release` to get rid of the logging
  overhead in release builds and reduce executable size.

## Cached settings

These variables are stored in CMake's cache and can be edited by the project's
build script, from the CMake command line when configuring the project
(`-Dname=value`) or using an editor such as the CMake GUI.

- `PSN00BSDK_TARGET` (`STRING`)

  The GCC toolchain's target triplet. PSn00bSDK assumes the toolchain targets
  `mipsel-unknown-elf` by default, however this can be changed to e.g. use a
  MIPS toolchain that was compiled for a slightly-different-but-equivalent
  target.

  The following GCC target triplets have been confirmed to work with PSn00bSDK:

  - `mipsel-unknown-elf`
  - `mipsel-none-elf`

- `PSN00BSDK_TC` (`PATH`)

  Path to the GCC toolchain's installation prefix/directory. By default this is
  initialized to the value of the `PSN00BSDK_TC` environment variable (if set).
  Note that modifying the environment variable after the project has been
  configured will *NOT* update this cache entry unless the project's cache is
  cleared manually.

  If not set, CMake will attempt to find the toolchain in the `PATH`
  environment variable and store its path in this variable (so the search does
  not have to be repeated).

  **IMPORTANT**: if the toolchain's target is not `mipsel-unknown-elf`,
  `PSN00BSDK_TARGET` must be set regardless of whether or not `PSN00BSDK_TC` is
  also set.

- `PSN00BSDK_LIBGCC` (`FILEPATH`)

  Path to the `libgcc.a` library bundled with the GCC toolchain. The contents
  of this library are merged into `libc` when building the SDK, so this
  variable is only actually needed when compiling `libpsn00b`. Setting this
  variable manually usually isn't necessary as CMake will locate `libgcc.a`
  automatically after finding the toolchain.

## Internal settings

These settings are not stored in CMake's cache and can only be set from within
the build script.

- `PSN00BSDK_LIBRARIES`

  List of libraries to link all created targets against. By default this
  includes all PSn00bSDK libraries.

- `PSN00BSDK_EXECUTABLE_SUFFIX`, `PSN00BSDK_SHARED_LIBRARY_SUFFIX`,
  `PSN00BSDK_SYMBOL_MAP_SUFFIX`

  File extensions to use for generated PS1 files. The default values are
  `.exe`, `.dll` and `.map` respectively. Note that file names and extensions
  can be changed anyway when building a CD image.

## Read-only variables

- `PSN00BSDK_VERSION`

  The SDK's version number (`major.minor.patch`).

- `PSN00BSDK_TOOLS`, `PSN00BSDK_INCLUDE`, `PSN00BSDK_LDSCRIPTS`

  Lists of paths used internally. Should not be set, manipulated or overridden
  by scripts.

- `TOOLCHAIN_NM`

  Path to the `nm` executable used to generate symbol maps. Although not used
  internally by CMake, this program is part of the GCC toolchain.

- `ELF2X`, `ELF2CPE`, `MKPSXISO`, `LZPACK`, `SMXLINK`

  Paths to the PSn00bSDK tools' executables. As no functions are currently
  provided for building assets, `LZPACK` and `SMXLINK` can be used with
  `add_custom_command()`/`add_custom_target()` to convert models and generate
  LZP archives as part of the build pipeline.

-----------------------------------------
_Last updated on 2021-09-27 by spicyjpeg_
