# Submodules (git clone --recursive, or git submodule update --init) and this port's changes to
# FSR-Vulkan (gpu/patches/fsr-vulkan), applied to its working tree once, at configure time, so
# that a fresh checkout configures the same way from build.sh and from CLion.

find_package(Git REQUIRED)

set(BBPC_FSR_VULKAN_DIR "${CMAKE_SOURCE_DIR}/gpu/third_party/fsr-vulkan")
if (NOT EXISTS "${BBPC_FSR_VULKAN_DIR}/CMakeLists.txt"
    OR NOT EXISTS "${CMAKE_SOURCE_DIR}/gpu/third_party/imgui/imgui.h"
    OR NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/LibAtrac9/C/src")
    message(STATUS "Submodules: git submodule update --init --recursive")
    execute_process(COMMAND "${GIT_EXECUTABLE}" submodule update --init --recursive
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" RESULT_VARIABLE bbpc_submodule_result)
    if (NOT bbpc_submodule_result EQUAL 0)
        message(FATAL_ERROR "Could not check out the submodules: run `git submodule update --init --recursive`")
    endif()
endif()

file(GLOB BBPC_FSR_PATCHES "${CMAKE_SOURCE_DIR}/gpu/patches/fsr-vulkan/*.patch")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BBPC_FSR_PATCHES})

# Applies every patch not applied yet; with CHECK_ONLY it only checks that they would apply.
# Sets ${result_var} to TRUE when all of them are (or would be) applied.
function(bbpc_apply_fsr_patches result_var check_only)
    set(${result_var} TRUE PARENT_SCOPE)
    foreach (patch IN LISTS BBPC_FSR_PATCHES)
        execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${BBPC_FSR_VULKAN_DIR}" apply --reverse --check "${patch}"
                        RESULT_VARIABLE applied OUTPUT_QUIET ERROR_QUIET)
        if (applied EQUAL 0)
            continue()
        endif()
        if (check_only)
            execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${BBPC_FSR_VULKAN_DIR}" apply --check "${patch}"
                            RESULT_VARIABLE status OUTPUT_QUIET ERROR_QUIET)
        else()
            execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${BBPC_FSR_VULKAN_DIR}" apply "${patch}"
                            RESULT_VARIABLE status)
        endif()
        if (NOT status EQUAL 0)
            set(${result_var} FALSE PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

# A checkout updated from an earlier version keeps that version's patch applied, and the new one
# applies neither way ("patch does not apply"): the submodule's tree holds nothing but these
# patches, so it is reset and they are applied again.
bbpc_apply_fsr_patches(bbpc_patches_ok TRUE)
if (NOT bbpc_patches_ok)
    message(STATUS "FSR-Vulkan: an earlier version of the port changed it; resetting it and patching again")
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${BBPC_FSR_VULKAN_DIR}" checkout -- .)
endif()
bbpc_apply_fsr_patches(bbpc_patches_ok FALSE)
if (NOT bbpc_patches_ok)
    message(FATAL_ERROR "FSR-Vulkan: the patches in gpu/patches/fsr-vulkan do not apply")
endif()
