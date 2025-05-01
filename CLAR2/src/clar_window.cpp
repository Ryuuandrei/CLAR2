#include "clar_window.h"
#include <stdexcept>

namespace CLAR {

	Window::Window(int w, int h, const char* name) : m_Name(name), width(w), height(h) {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_Window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

        glfwSetWindowUserPointer(m_Window, this);

        glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
            auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            app->m_FrameBufferResized = true;
            app->width = width;
            app->height = height;

            app->onResize();
        });

        glfwSetErrorCallback([](int error, const char* description) {
            fprintf(stderr, "GLFW Error %d: %s\n", error, description);
		});

        this->GetCursorPos(&xpos, &ypos);

        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xpos, double ypos) {
			auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
			{
                app->x = 0;
                app->y = 0;
			}
            else
            {
                app->x = xpos - app->xpos;
                app->y = ypos - app->ypos;
            }
            app->xpos = xpos;
            app->ypos = ypos;
		});

        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));

            static auto lastState = glfwGetKey(window, GLFW_KEY_ESCAPE);
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && lastState == GLFW_RELEASE)
                app->ToggleCursor();

            lastState = GLFW_RELEASE;
            app->x = 0;
            app->y = 0;
        });

        glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xoffset, double yoffset) {
			auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            app->scroll += yoffset;
		});
	}

	Window::~Window() {
        glfwDestroyWindow(m_Window);

        glfwTerminate();
	}

    void Window::getFramebufferSize(int* w, int* h) const {
        glfwGetFramebufferSize(m_Window, w, h);
    }

    void Window::createSurfaceWindow(VkInstance instance, VkSurfaceKHR* surface)
    {
        if (glfwCreateWindowSurface(instance, m_Window, nullptr, surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    int Window::KeyState(int key) const
    {
        return glfwGetKey(m_Window, key);
    }
}