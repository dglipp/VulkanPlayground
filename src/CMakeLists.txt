
add_executable(vkPlayground
    main.cpp
    vk_engine.cpp
    vk_engine.h
    vk_types.h
    vk_initializers.cpp
    vk_initializers.h
    vk_mesh.h
    vk_mesh.cpp)


set_property(TARGET vkPlayground PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vkPlayground>")

target_include_directories(vkPlayground PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vkPlayground vkbootstrap vma glm tinyobjloader imgui stb_image)
find_package(SDL2 REQUIRED CONFIG)

target_link_libraries(vkPlayground Vulkan::Vulkan SDL2 SDL2main)

add_dependencies(vkPlayground Shaders)
