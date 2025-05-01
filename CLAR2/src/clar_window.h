#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>

namespace CLAR {
	class Window {
	public:
		Window(int w, int h, const char* name);
		~Window();

		Window(const Window&) = delete;
		Window &operator=(const Window&) = delete;
		operator GLFWwindow*() const { return m_Window; }

		bool shouldClose() const { return glfwWindowShouldClose(m_Window); }
		void getFramebufferSize(int* w, int* h) const;

		void createSurfaceWindow(VkInstance instance, VkSurfaceKHR* surface);

		bool WasWindowResized() const { return m_FrameBufferResized; }
		void ResetWindowResizedFlag() { m_FrameBufferResized = false; }

		int KeyState(int key) const;
		void GetCursorPos(double* xpos, double* ypos) const { *xpos = this->xpos; *ypos = this->ypos; }
		void GetCursorDelta(double* x, double* y) { *x = this->x; *y = this->y; this->x = 0; this->y = 0; }
		void ToggleCursor() { glfwSetInputMode(m_Window, GLFW_CURSOR, glfwGetInputMode(m_Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED); }
		void GetScroll(double* scroll) const { *scroll = this->scroll; }

		void SetResizeCallback(const std::function<void()>& func) { this->onResize = onResize; }
	private:
		int width;
		int height;
		const char* m_Name;
		GLFWwindow* m_Window;
		bool m_FrameBufferResized = false;

		double xpos, ypos;
		double x, y;
		double scroll = 0;
		std::function<void()> onResize = []() {};
	};
}
