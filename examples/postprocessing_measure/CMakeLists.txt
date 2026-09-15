add_executable(postprocessing_measure
    postprocessing_measure.cpp
)

target_link_libraries(postprocessing_measure PRIVATE
    ViennaVulkanEngine::ViennaVulkanEngine
)

add_dependencies(postprocessing_measure
    ViennaVulkanEngine
    vve_simple_shaders
)

vve_set_output_dirs(postprocessing_measure)