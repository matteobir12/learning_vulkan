#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <tuple>
#include <optional>
#include <stdexcept>
#include <vector>

namespace VulkanUtils {

// Little bit messy that this is both the window and the surface. Could split.
class WindowAndSurfaceManager {
 public:
  int windowShouldClose() const { return glfwWindowShouldClose(window); }
  void pollEvents() { glfwPollEvents(); }
  // can be null
  VkSurfaceKHR getSurface() const { return surface; }
  std::tuple<int, int> getFramebufferSize() const {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    return std::make_tuple(width, height);
  }

  WindowAndSurfaceManager() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }
  
  ~WindowAndSurfaceManager() {
    if (surface && instance)
      vkDestroySurfaceKHR(instance, surface, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void createSurface(VkInstance _instance) {
    instance = _instance;
    if (surface)
      std::cerr << "Surface already existed?" << std::endl;

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
      throw std::runtime_error("failed to create window surface!");
  }

  std::vector<const char*> getRequiredInstanceExtensions() const {
    uint32_t extensionCount;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    return {extensions, extensions + extensionCount};
  }

  WindowAndSurfaceManager(const WindowAndSurfaceManager&) = delete;
  private:
  GLFWwindow* window;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkInstance instance = VK_NULL_HANDLE;

  const int WIDTH = 800;
  const int HEIGHT = 600;
};

}
