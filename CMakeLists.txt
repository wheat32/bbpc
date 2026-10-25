# bbpc: a native Linux port of Bloodborne. One CMake project for all of it: the GPU library
# (gpu/), the loader and its PS4 runtime (bb-probe), the GPU and input helper
# (bb-gpu-capabilities), the Qt launcher (launcher/) and the tests. build.sh, the CI and CLion all
# configure it through CMakePresets.json; run.sh expects the release preset's out/ layout:
# out/bb-probe, out/bb-gpu-capabilities, out/gpu/libbbgpu.so, out/gpu/libbbnet20.so.
cmake_minimum_required(VERSION 3.25)

# version.json is the single source of truth for the version (the launcher, the releases).
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.json" BBPC_VERSION_JSON)
string(JSON BBPC_VERSION GET "${BBPC_VERSION_JSON}" app_version)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/version.json")

project(bbpc VERSION ${BBPC_VERSION} LANGUAGES C CXX)

# C++23 throughout (the GPU library, the launcher and the DLSS bridge set it for their own targets
# too); the port's C code is C23.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(BBPC_BUILD_PORT "Build the game: the GPU library, the loader and its runtime" ON)
option(BBPC_BUILD_LAUNCHER "Build the Qt launcher" ON)
option(BBPC_BUILD_TESTS "Build the tests and register them with CTest" OFF)
option(BBPC_WERROR "Treat warnings in the port's C code as errors" ON)
set(BBPC_PACKAGE_RPATH "" CACHE STRING
    "Extra RUNPATH entries of the port's binaries, for packages that keep the libraries elsewhere")
set(DLSS_SDK_ROOT "" CACHE PATH "NVIDIA DLSS SDK checkout (github.com/NVIDIA/DLSS): builds the DLSS bridge")

if (BBPC_BUILD_TESTS)
    enable_testing()
endif()

if (BBPC_BUILD_PORT)
    include(cmake/Submodules.cmake)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(BBPC_VULKAN REQUIRED IMPORTED_TARGET vulkan)
    pkg_check_modules(BBPC_SDL3 REQUIRED IMPORTED_TARGET sdl3)

    add_subdirectory(gpu)

    # The port's own C code: C23 without extensions (nullptr, bool), every warning, and
    # assertions kept in every build type.
    function(bbpc_port_c_target target)
        set_target_properties(${target} PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
        target_compile_options(${target} PRIVATE -Wall -Wextra -UNDEBUG $<$<BOOL:${BBPC_WERROR}>:-Werror>)
        target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
        # -pthread itself, not CMake's Threads (which leaves it out where glibc has pthreads built
        # in): it defines _REENTRANT, which exposes the POSIX functions the strict C23 sources use.
        target_compile_options(${target} PRIVATE -pthread)
        target_link_options(${target} PRIVATE -pthread)
    endfunction()

    # Links a target to the runtime's GPU library the way the loader is: against libbbgpu.so
    # itself, not its usage requirements (its static libraries must not be linked twice), with
    # the runtime's symbols exported for it (it is linked with --allow-shlib-undefined).
    function(bbpc_link_gpu_library target)
        add_dependencies(${target} bbgpu)
        target_link_libraries(${target} PRIVATE "$<TARGET_LINKER_FILE:bbgpu>")
        target_link_options(${target} PRIVATE -rdynamic)
        # $ORIGIN/gpu: packaged copies keep the library next to the binary without patching it.
        set(rpath "\$ORIGIN/gpu" "${CMAKE_BINARY_DIR}/gpu" ${BBPC_PACKAGE_RPATH})
        set_target_properties(${target} PROPERTIES BUILD_RPATH "${rpath}")
    endfunction()

    # LibAtrac9 (MIT): the game's ATRAC9 audio decoder, built without this project's warnings.
    file(GLOB BBPC_ATRAC9_SOURCES CONFIGURE_DEPENDS third_party/LibAtrac9/C/src/*.c)
    add_library(atrac9 STATIC ${BBPC_ATRAC9_SOURCES})
    set_target_properties(atrac9 PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
    target_compile_options(atrac9 PRIVATE -w -UNDEBUG)

    file(GLOB BBPC_RUNTIME_SOURCES CONFIGURE_DEPENDS src/runtime*.c)

    # The loader: maps the prepared game image (scripts/) and jumps into it. Not position
    # independent: the image's addresses are fixed.
    add_executable(bb-probe src/probe.c ${BBPC_RUNTIME_SOURCES} src/vulkan_smoke.c)
    bbpc_port_c_target(bb-probe)
    bbpc_link_gpu_library(bb-probe)
    target_link_libraries(bb-probe PRIVATE atrac9 m PkgConfig::BBPC_VULKAN PkgConfig::BBPC_SDL3)
    target_link_options(bb-probe PRIVATE -no-pie)
    set_target_properties(bb-probe PROPERTIES POSITION_INDEPENDENT_CODE OFF)

    # GPU, display and gamepad queries for run.sh and the launcher (--gpus, --displays,
    # --gamepads, --read-input, --live-resolution, --device).
    add_executable(bb-gpu-capabilities tools/gpu_capabilities.c)
    bbpc_port_c_target(bb-gpu-capabilities)
    target_link_libraries(bb-gpu-capabilities PRIVATE PkgConfig::BBPC_VULKAN PkgConfig::BBPC_SDL3)
    if (BBPC_PACKAGE_RPATH)
        set_target_properties(bb-gpu-capabilities PROPERTIES BUILD_RPATH "${BBPC_PACKAGE_RPATH}")
    endif()

    # Optional DLSS (NVIDIA RTX): the bridge (gpu/dlss_bridge, the only code using NVIDIA's SDK)
    # next to bb-probe, with NVIDIA's libnvidia-ngx-dlss.so. Without them DLSS is unavailable.
    if (DLSS_SDK_ROOT)
        add_subdirectory(gpu/dlss_bridge)
        set_target_properties(bbport_dlss PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
        # A target of its own: a POST_BUILD step must be added where bbport_dlss is defined, and
        # gpu/dlss_bridge also builds standalone (MSVC).
        add_custom_target(bbport_dlss_runtime ALL
            COMMAND "${CMAKE_COMMAND}" "-DSDK=${DLSS_SDK_ROOT}" "-DOUT=${CMAKE_BINARY_DIR}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyDlssRuntime.cmake"
            DEPENDS bbport_dlss
            COMMENT "NVIDIA's DLSS library next to bb-probe")
        message(STATUS "DLSS bridge: on (${DLSS_SDK_ROOT})")
    endif()
endif()

if (BBPC_BUILD_LAUNCHER)
    add_subdirectory(launcher)
endif()

# The installed port (the AppImage and the release tarball): <prefix>/bin/bbpc (the launcher) and
# <prefix>/share/bbpc with run.sh, its scripts and the prebuilt binaries in share/bbpc/bin, which
# run.sh plays with BB_PREBUILT=1. The launcher finds share/bbpc next to its bin/.
if (BBPC_BUILD_PORT)
    include(GNUInstallDirs)
    set(BBPC_PORT_DESTINATION "${CMAKE_INSTALL_DATADIR}/bbpc")
    install(PROGRAMS run.sh DESTINATION "${BBPC_PORT_DESTINATION}")
    install(DIRECTORY scripts patches DESTINATION "${BBPC_PORT_DESTINATION}" PATTERN __pycache__ EXCLUDE)
    install(DIRECTORY tools/fsr4cap DESTINATION "${BBPC_PORT_DESTINATION}/tools"
            PATTERN __pycache__ EXCLUDE PATTERN "*.exe" EXCLUDE PATTERN bin EXCLUDE)
    # FSR 4 v07 (MIT, tools/fetch_fsr4_assets.sh) when it was downloaded; FSR 4.1.1 never (it is
    # built from the player's own AMD DLL).
    install(DIRECTORY fsr4_shaders DESTINATION "${BBPC_PORT_DESTINATION}" OPTIONAL)

    set(BBPC_INSTALL_PROBE_RPATH "\$ORIGIN/gpu" ${BBPC_PACKAGE_RPATH})
    set_target_properties(bb-probe PROPERTIES INSTALL_RPATH "${BBPC_INSTALL_PROBE_RPATH}")
    set_target_properties(bb-gpu-capabilities bbgpu PROPERTIES INSTALL_RPATH "${BBPC_PACKAGE_RPATH}")
    install(TARGETS bb-probe bb-gpu-capabilities RUNTIME DESTINATION "${BBPC_PORT_DESTINATION}/bin")
    install(TARGETS bbgpu LIBRARY DESTINATION "${BBPC_PORT_DESTINATION}/bin/gpu")
    foreach (module IN ITEMS bbnet bbnet20)
        if (TARGET ${module})
            set(BBPC_MODULE_RPATH "\$ORIGIN" ${BBPC_PACKAGE_RPATH})
            set_target_properties(${module} PROPERTIES INSTALL_RPATH "${BBPC_MODULE_RPATH}")
            install(TARGETS ${module} LIBRARY DESTINATION "${BBPC_PORT_DESTINATION}/bin/gpu")
        endif()
    endforeach()
    if (TARGET bbport_dlss)
        # NVIDIA's RTX SDK license allows the runtime library in an application; its text goes along.
        install(TARGETS bbport_dlss LIBRARY DESTINATION "${BBPC_PORT_DESTINATION}/bin")
        # file(INSTALL) adds DESTDIR itself, as the rules install() writes do.
        install(CODE "file(GLOB BBPC_NGX_LIBRARIES \"${CMAKE_BINARY_DIR}/libnvidia-ngx-dlss.so.*\")
                      file(INSTALL \${BBPC_NGX_LIBRARIES}
                           DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${BBPC_PORT_DESTINATION}/bin\")")
        install(FILES "${CMAKE_BINARY_DIR}/NVIDIA-DLSS-LICENSE.txt" DESTINATION "${BBPC_PORT_DESTINATION}")
    endif()
    install(FILES LICENSE README.md DESTINATION "${BBPC_PORT_DESTINATION}")
endif()

if (BBPC_BUILD_TESTS AND BBPC_BUILD_PORT)
    # The runtime's tests (build.sh --test ran them before).
    add_executable(pad-test tests/test_pad.c)
    bbpc_port_c_target(pad-test)
    target_link_libraries(pad-test PRIVATE m PkgConfig::BBPC_VULKAN PkgConfig::BBPC_SDL3)

    add_executable(file-mods-test tests/test_file_mods.c)
    bbpc_port_c_target(file-mods-test)

    add_executable(content-test tests/test_content.c src/runtime_content.c)
    bbpc_port_c_target(content-test)
    target_compile_definitions(content-test PRIVATE _GNU_SOURCE)

    foreach (test IN ITEMS runtime sema online)
        add_executable(${test}-test tests/test_${test}.c ${BBPC_RUNTIME_SOURCES})
        bbpc_port_c_target(${test}-test)
        bbpc_link_gpu_library(${test}-test)
        target_link_libraries(${test}-test PRIVATE atrac9 m PkgConfig::BBPC_VULKAN PkgConfig::BBPC_SDL3)
    endforeach()

    foreach (test IN ITEMS pad runtime file-mods sema online content)
        add_test(NAME runtime.${test} COMMAND ${test}-test WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endforeach()

    # The GPU library's tests: neither launches the game nor needs a Vulkan device.
    set(BBPC_GPU_TESTS motion-history-test ui-composition-test cp-decoder-test fx-init-clear-test
        effect-flags-test dlss-generation-test pending-reads-test settings-test upscaler-support-test scene-resolution-test
        motion-shader-test taa-shader-test camera-motion-test translation-memory-test
        srt-walker-fault-test)
    foreach (test IN LISTS BBPC_GPU_TESTS)
        set_target_properties(${test} PROPERTIES EXCLUDE_FROM_ALL OFF)
        string(REGEX REPLACE "-test$" "" name ${test})
        add_test(NAME gpu.${name} COMMAND ${test} WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endforeach()

    # The preparation scripts, the patch compiler, mods, online and run.sh.
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME python COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests
             WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
