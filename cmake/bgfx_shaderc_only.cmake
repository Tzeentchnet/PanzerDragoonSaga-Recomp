# bgfx.cmake ties shaderc to its aggregate tools option. PDS needs shaderc but
# none of the other tools or their example-common support library.
set(BGFX_BUILD_TOOLS_SHADER ON CACHE BOOL "Build bgfx shader tools." FORCE)
