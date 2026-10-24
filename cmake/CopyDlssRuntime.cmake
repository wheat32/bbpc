# cmake -DSDK=<DLSS SDK> -DOUT=<build dir> -P CopyDlssRuntime.cmake
# Puts NVIDIA's libnvidia-ngx-dlss.so.<version> and its license next to bb-probe, replacing the
# one of an earlier SDK (vk_dlss.cpp takes the newest it finds there).
file(GLOB old_libraries "${OUT}/libnvidia-ngx-dlss.so.*")
if (old_libraries)
    file(REMOVE ${old_libraries})
endif()
file(GLOB libraries "${SDK}/lib/Linux_x86_64/rel/libnvidia-ngx-dlss.so.*")
if (NOT libraries)
    message(FATAL_ERROR "No libnvidia-ngx-dlss.so.* in ${SDK}/lib/Linux_x86_64/rel")
endif()
file(COPY ${libraries} DESTINATION "${OUT}")
file(COPY_FILE "${SDK}/LICENSE.txt" "${OUT}/NVIDIA-DLSS-LICENSE.txt")
