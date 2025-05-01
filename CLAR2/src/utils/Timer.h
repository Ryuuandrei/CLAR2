#pragma once

#include <iostream>
#include <chrono>

class Timer {
public:
	Timer() {
		m_StartTimepoint = std::chrono::high_resolution_clock::now();
	}

	~Timer() {
		Stop();
	}

	void Stop() {
		auto currentTime = std::chrono::high_resolution_clock::now();
		auto dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - m_StartTimepoint).count();

		std::cout << "Timer took " << dt << "ms" << std::endl;
	}

private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
};
