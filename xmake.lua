set_project("SimpleVulkanRenderer")

set_config("buildir", "build")

add_rules("mode.release", "mode.debug")

target("SimpleVulkanRenderer")
    set_kind("binary")
    set_languages("c++17")
    add_files("src/*.cpp")
    add_includedirs("include")
    add_syslinks("glfw", "vulkan", "dl", "pthread", "X11", "Xxf86vm", "Xrandr", "Xi")
    if is_mode("debug") then
        add_cxxflags("-Og", "-g", "-ggdb",  "-Wall", "-Wextra", {force = true})
    elseif is_mode("release") then
        add_cxxflags("-O3")
    end

task("shaders")
    on_run(function()
        os.mkdir("shaders")
        os.exec("/usr/local/bin/glslc shaders/simple_shader.vert -o build/vert.spv")
        os.exec("/usr/local/bin/glslc shaders/simple_shader.frag -o build/frag.spv")
    end)
    set_menu {
        usage = "xmake shaders",
        description = "Compile Vulkan shaders"
    }
