#pragma once

#include <glm/glm.hpp>

namespace CLAR {
	struct Camera {
		glm::vec3 position;
		glm::vec3 pivot;
		glm::vec3 forward;
		glm::vec3 up;
		glm::vec3 right;

		float pitch = 0.0f;

		Camera() = default;
		Camera(const glm::vec3& position, const glm::vec3& pivot)
			: position(position), pivot(pivot)
		{
			forward = glm::normalize(pivot - position);
			up = glm::vec3(0.0f, 1.0f, 0.0f);
			right = glm::normalize(glm::cross(forward, up));
		}

		glm::mat4 LookAt() const {
			return glm::lookAt(position, pivot, up);
		}

		void MoveForward(float distance)
		{
			position += distance * glm::normalize(glm::vec3(forward.x, 0.f, forward.z));
			pivot += distance * glm::normalize(glm::vec3(forward.x, 0.f, forward.z));
		}

		void MoveRight(float distance)
		{
			position += distance * right;
			pivot += distance * right;
		}

		void MoveUp(float distance) {
			position += distance * up;
			pivot += distance * up;
		}

		void LookUp(float angle) {
			if (pitch + angle >= 1.5f || pitch + angle <= -1.5f) return;
			pitch += angle;
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, right);
			forward = glm::normalize(rotation * glm::vec4(forward, 0.0f));

			pivot = position + forward;
		}

		void LookRight(float angle) {
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, up);
			forward = glm::normalize(rotation * glm::vec4(forward, 0.0f));
			right = glm::normalize(glm::cross(forward, up));

			pivot = position + forward;
		}

		void RotateAlongPivotRight(float angle) {

			float length = glm::length(position - pivot);
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), -angle, up);

			position = pivot + length * glm::vec3(glm::normalize(rotation * glm::vec4(-forward, 0.0f)));
			forward = glm::normalize(pivot - position);
			right = glm::normalize(glm::cross(forward, up));
		}

		void RotateAlongPivotUp(float angle) {
			if (pitch + angle >= 1.5f || pitch + angle <= -1.5f) return;
			pitch += angle;

			float length = glm::length(position - pivot);
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), -angle, right);

			position = pivot + length * glm::vec3(glm::normalize(rotation * glm::vec4(-forward, 0.0f)));
			forward = glm::normalize(pivot - position);
		}

	};

	/*namespace CameraControler
	{
		Camera camera{ glm::vec3(0.0f, 2.0f, 2.0f), glm::vec3(0.0f, 2.0f, 0.0f) };

		void OnMouseMove(float dt, float dx, float dy)
		{
			camera.LookRight(-dx * dt);
			camera.LookUp(-dy * dt);
		}

		void OnInputUpdate(float dt, int key)
		{
			if (key == GLFW_KEY_W)
				camera.MoveForward(dt);
			if (key == GLFW_KEY_S)
				camera.MoveForward(-dt);
			if (key == GLFW_KEY_A)
				camera.MoveRight(-dt);
			if (key == GLFW_KEY_D)
				camera.MoveRight(dt);
			if (key == GLFW_KEY_SPACE)
				camera.MoveUp(dt);
			if (key == GLFW_KEY_LEFT_SHIFT)
				camera.MoveUp(-dt);
		}
	}*/
}
