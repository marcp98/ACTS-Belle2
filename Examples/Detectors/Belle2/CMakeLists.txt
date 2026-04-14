acts_add_library(
    ExamplesDetectorBelle2
    src/Belle2.cpp
    src/Belle2Element.cpp
    src/LayerBuilder.cpp
    src/ProtoLayerCreator.cpp
    src/Belle2Builder.cpp
)

acts_compile_headers(ExamplesDetectorBelle2 GLOB "include/**/*.hpp")

target_include_directories(
    ActsExamplesDetectorBelle2
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesDetectorBelle2
    PUBLIC Acts::ExamplesDetectorsCommon
            Acts::PluginRoot
)
