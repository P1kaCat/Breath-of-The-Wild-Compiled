if(NOT BUILD_TESTING)
    return()
endif()
foreach(test vertex_texture latte_pixel latte_execute postprocess present_without_clear memory_layout)
    add_executable(botw_${test}_test tools/${test}_test.c)
    target_link_libraries(botw_${test}_test PRIVATE botw_graphics dolrecomp_cpu)
    add_test(NAME botw_${test} COMMAND botw_${test}_test)
    set_tests_properties(botw_${test} PROPERTIES
        ENVIRONMENT "BOTW_HEADLESS=1;BOTW_MUTE=1"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:botw_graphics>")
endforeach()
add_executable(botw_cemu_layout_test tools/cemu_layout_test.c src/wiiu_gx2_cemu_layout.c)
target_include_directories(botw_cemu_layout_test PRIVATE src "${DOLRECOMP_DIR}/src")
add_test(NAME botw_cemu_layout COMMAND botw_cemu_layout_test)
add_executable(botw_audio_buffer_test tools/audio_buffer_test.c src/wiiu_audio_buffer.c)
target_include_directories(botw_audio_buffer_test PRIVATE src)
add_test(NAME botw_audio_buffer COMMAND botw_audio_buffer_test)
add_executable(botw_audio_muted_test tools/audio_muted_test.c)
target_link_libraries(botw_audio_muted_test PRIVATE botw_audio)
add_test(NAME botw_audio_muted COMMAND botw_audio_muted_test)
set_tests_properties(botw_audio_muted PROPERTIES ENVIRONMENT "BOTW_MUTE=1"
    PASS_REGULAR_EXPRESSION "host output muted"
    FAIL_REGULAR_EXPRESSION "native Windows output opened"
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:botw_audio>")
