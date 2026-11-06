include_guard()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/targets/android-ndk.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/qpm.cmake)

add_compile_definitions(QUEST)
add_compile_definitions(UNITY_6 UNITY_6_3)
add_compile_definitions(NEED_UNSAFE_CSHARP)