core_deps = [
    dependency('sdl2', required: true),
    dependency('SDL2_image', required: true),
    dependency('glew', required: true),
    dependency('opengl', required: true),
    dependency('glu', required: true),
    dependency('imgui', required: true),
    dependency('tinyobjloader', required: true),
]

core_source_files = [
    'src/App.cpp',
    'src/Buffer.cpp',
    'src/Camera.cpp',
    'src/CameraManipulator.cpp',
    'src/Framebuffer.cpp',
    'src/GLUtils.cpp',
    'src/Mesh.cpp',
    'src/ObjParser.cpp',
    'src/Octree.cpp',
    'src/Portal.cpp',
    'src/SDL_GLDebugMessageCallback.cpp',
    'src/Shader.cpp',
    'src/Skybox.cpp',
    'src/Texture2D.cpp',
]

core_inc = include_directories('include')

core_lib = static_library(
    'core',
    sources: core_source_files,
    include_directories: core_inc,
    dependencies: core_deps,
)