add_library(dolrecomp_cpu STATIC
    "${DOLRECOMP_DIR}/src/cpu/cpu.c"
)
target_include_directories(dolrecomp_cpu PUBLIC "${DOLRECOMP_DIR}/src")

add_library(botw_audio SHARED
    src/wiiu_audio.c
    src/wiiu_audio_buffer.c
)
set_target_properties(botw_audio PROPERTIES
    OUTPUT_NAME "botw_audio"
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
)
target_include_directories(botw_audio PUBLIC
    src
    "${DOLRECOMP_DIR}/src"
)
if(WIN32)
    target_link_libraries(botw_audio PRIVATE winmm)
endif()

add_library(botw_graphics SHARED
    src/wiiu_latte_pixel.c
    src/wiiu_gx2.c
    src/wiiu_gx2_cemu_layout.c
    src/wiiu_gx2_software.c
    src/wiiu_latte_fetch.c
    src/wiiu_latte_vertex.c
    src/wiiu_latte_execute.c
    src/wiiu_memory.c
    src/botw_graphics_config.c
    src/botw_shader_override.c
    src/wiiu_window.c
)
set_target_properties(botw_graphics PROPERTIES
    OUTPUT_NAME "botw_graphics"
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
)
target_include_directories(botw_graphics PUBLIC
    src
    "${DOLRECOMP_DIR}/src"
)
target_link_libraries(botw_graphics PRIVATE dolrecomp_cpu)
if(WIN32)
    target_link_libraries(botw_graphics PRIVATE
        user32 gdi32 d3d11 dxgi d3dcompiler dxguid uuid
    )
endif()


if(BUILD_TESTING)
    add_executable(botw_latte_vertex_failure_replay tools/latte_vertex_failure_replay.c)
    target_link_libraries(botw_latte_vertex_failure_replay PRIVATE botw_graphics)
    add_test(NAME botw_latte_failure_context COMMAND botw_latte_vertex_failure_replay --self-test)
    set_tests_properties(botw_latte_failure_context PROPERTIES
        ENVIRONMENT "BOTW_HEADLESS=1;BOTW_MUTE=1"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:botw_graphics>")
    add_executable(botw_graphics_config_test tools/graphics_config_test.c src/botw_graphics_config.c)
    target_include_directories(botw_graphics_config_test PRIVATE src)
    add_test(NAME botw_graphics_config COMMAND botw_graphics_config_test)
    if(WIN32)
        add_executable(botw_shader_override_test tools/shader_override_test.c)
        target_link_libraries(botw_shader_override_test PRIVATE botw_graphics user32)
        add_test(NAME botw_shader_override COMMAND botw_shader_override_test)
        set_tests_properties(botw_shader_override PROPERTIES
            ENVIRONMENT "BOTW_HEADLESS=1;BOTW_MUTE=1"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:botw_graphics>")
        add_executable(botw_presentation_test tools/presentation_test.c)
        target_link_libraries(botw_presentation_test PRIVATE botw_graphics user32)
        add_test(NAME botw_presentation COMMAND botw_presentation_test)
        set_tests_properties(botw_presentation PROPERTIES
            ENVIRONMENT "BOTW_HEADLESS=1;BOTW_WINDOW_WIDTH=640;BOTW_WINDOW_HEIGHT=360;BOTW_VSYNC=1;BOTW_PRESENT_FILTER=nearest"
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:botw_graphics>")
    endif()
    foreach(tool botw_graphics_config_test botw_presentation_test botw_shader_override_test botw_latte_vertex_failure_replay)
        if(TARGET ${tool})
            set_target_properties(${tool} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${BOTW_OUTPUT_ROOT}/tools")
            foreach(configuration Debug Release RelWithDebInfo MinSizeRel)
                string(TOUPPER "${configuration}" configuration_upper)
                set_target_properties(${tool} PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY_${configuration_upper} "${BOTW_OUTPUT_ROOT}/tools")
            endforeach()
        endif()
    endforeach()
endif()
