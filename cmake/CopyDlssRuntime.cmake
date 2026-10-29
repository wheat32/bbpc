# cmake -DSDK=<DLSS SDK> -DOUT=<build dir> -P CopyDlssRuntime.cmake
# Puts NVIDIA's libnvidia-ngx-dlss.so.<version> (Super Resolution), libnvidia-ngx-dlssg.so.<version>
# (Frame Generation, when the SDK has it: 310.7 or newer) and their license next to bb-probe,
# replacing those of an earlier SDK (vk_dlss.cpp takes the newest it finds there).
file(GLOB old_libraries "${OUT}/libnvidia-ngx-dlss.so.*" "${OUT}/libnvidia-ngx-dlssg.so.*")
if (old_libraries)
    file(REMOVE ${old_libraries})
endif()
file(GLOB libraries "${SDK}/lib/Linux_x86_64/rel/libnvidia-ngx-dlss.so.*")
if (NOT libraries)
    message(FATAL_ERROR "No libnvidia-ngx-dlss.so.* in ${SDK}/lib/Linux_x86_64/rel")
endif()
file(COPY ${libraries} DESTINATION "${OUT}")
file(GLOB frame_generation "${SDK}/lib/Linux_x86_64/rel/libnvidia-ngx-dlssg.so.*")
if (frame_generation)
    file(COPY ${frame_generation} DESTINATION "${OUT}")
else()
    message(STATUS "DLSS Frame Generation library not in this SDK (lib/Linux_x86_64/rel/libnvidia-ngx-dlssg.so.*): "
                   "DLSS Super Resolution only")
endif()
file(COPY_FILE "${SDK}/LICENSE.txt" "${OUT}/NVIDIA-DLSS-LICENSE.txt")
