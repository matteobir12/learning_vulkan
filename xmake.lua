set_project("SimpleVulkanRenderer")

set_config("buildir", "build")

add_rules("mode.release", "mode.debug")

target("SimpleVulkanRenderer")
    set_kind("binary")
    set_languages("c++17")
    add_files("src/*.cpp")
    add_includedirs("include")
    add_syslinks("glfw", "vulkan", "dl", "pthread", "X11", "Xxf86vm", "Xrandr", "Xi")

task("shaders")
    on_run(function()
        os.exec("cd shaders && ./compile.sh")
    end)
    set_menu {
        usage = "xmake shaders",
        description = "Compile Vulkan shaders"
    }
