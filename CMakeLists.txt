cmake_minimum_required(VERSION 3.16)
project(racr C)

find_package(SDL3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME}
  main.c
  engine/shader.c
  engine/mesh.c
  engine/obj_loader.c
  engine/scene.c
  engine/scene_io.c
  engine/camera.c
)

target_link_libraries(${PROJECT_NAME}
  SDL3::SDL3
  ${OPENGL_LIBRARIES}
  m
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${SDL3_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIRS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED YES
)

# Copy shaders and assets to build directory so relative paths work
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/scenes ${CMAKE_BINARY_DIR}/scenes
)
