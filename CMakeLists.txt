cmake_minimum_required (VERSION 3.8)

project ("chess")

find_package(SFML 2.5.1 COMPONENTS graphics audio window network system REQUIRED)

if ((NOT ${CMAKE_VERSION} VERSION_LESS 3.11) AND (NOT OpenGL_GL_PREFERENCE))
    set(OpenGL_GL_PREFERENCE "LEGACY")
endif()

find_package(OpenGL REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/imgui)

set(SOURCES 
    src/main.cpp
    src/constants.cpp
    src/types.cpp
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui-SFML.cpp
)

add_executable (chess ${SOURCES})
target_link_libraries(chess PUBLIC sfml-graphics sfml-window sfml-audio sfml-system ${OPENGL_LIBRARIES})

add_custom_command(
    TARGET chess POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/libs/$<CONFIGURATION>"
    $<TARGET_FILE_DIR:chess>
)

add_custom_command(
    TARGET chess POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/textures"
    "$<TARGET_FILE_DIR:chess>/textures"
)

add_custom_command(
    TARGET chess POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/fonts"
    "$<TARGET_FILE_DIR:chess>/fonts"
)