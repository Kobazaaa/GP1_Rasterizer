#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle, float _aspectRatio, float _near, float _far):
			origin{_origin},
			fovAngle{_fovAngle},
			aspect{_aspectRatio},
			near{ _near },
			far{ _far }
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) * 0.5f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		const float MOVEMENT_SPEED = 20.f;
		const float ROTATION_SPEED = 30.f;

		Matrix viewMatrix{};
		Matrix invViewMatrix{};
		Matrix projectionMatrix{};

		float near	{ 0.1f };
		float far	{ 100.f };
		float aspect{ 1.f };

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f }, float _aspectRatio = { 1.3333f }, float nearDist = {0.1f}, float farDist = {100.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) * 0.5f);

			origin = _origin;

			aspect = _aspectRatio;

			near = nearDist;
			far = farDist;

			CalculateProjectionMatrix();
		}

		
		void CalculateViewMatrix()
		{
			// Calculate the viewMatrix, aka the matrix to transform world space to camera space
			viewMatrix = Matrix::CreateLookAtLH(origin, origin + forward, Vector3::UnitY);
			Matrix temp = viewMatrix; // create a temp copy because .Inverse() changes the matrix itself, which we don't want

			// The inverse of the viewMatrix is the invViewMatrix matrix
			invViewMatrix = temp.Inverse();

			// Get the axis' for the camera out of the invViewMatrix matrix
			right	= invViewMatrix.GetAxisX();
			up		= invViewMatrix.GetAxisY();
			forward = invViewMatrix.GetAxisZ();
		}

		void CalculateProjectionMatrix()
		{
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspect, near, far);
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();
			const float displacement = MOVEMENT_SPEED * deltaTime;
			const float pitchLockAngle = 80 * TO_RADIANS;

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			// FORWARD
			if (pKeyboardState[SDL_SCANCODE_Z]
				or pKeyboardState[SDL_SCANCODE_W]
				or pKeyboardState[SDL_SCANCODE_UP])		origin += forward * displacement;
			// LEFT
			if (pKeyboardState[SDL_SCANCODE_Q]
				or pKeyboardState[SDL_SCANCODE_A]
				or pKeyboardState[SDL_SCANCODE_LEFT])	origin -= right * displacement;
			// BACKWARDS
			if (pKeyboardState[SDL_SCANCODE_S]
				or pKeyboardState[SDL_SCANCODE_DOWN])	origin -= forward * displacement;
			// RIGHT
			if (pKeyboardState[SDL_SCANCODE_D]
				or pKeyboardState[SDL_SCANCODE_RIGHT])	origin += right * displacement;


			// MOUSE MOVEMENTS AND ROTATIONS
			const bool LMB = mouseState & SDL_BUTTON(SDL_BUTTON_LEFT);
			const bool RMB = mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT);
			if (!LMB and RMB)
			{
				totalPitch  -= SignOf(mouseY) * ROTATION_SPEED * deltaTime * TO_RADIANS;
				totalPitch   = std::clamp(totalPitch, -pitchLockAngle, pitchLockAngle); // locks the up/down camera rotation so you don't overshoot
				totalYaw	+= SignOf(mouseX) * ROTATION_SPEED * deltaTime * TO_RADIANS;
			}
			else if (LMB and !RMB)
			{
				origin		-= SignOf(mouseY) * forward * displacement;
				totalYaw	+= SignOf(mouseX) * ROTATION_SPEED * deltaTime * TO_RADIANS;
			}
			else if (LMB and RMB)
			{
				origin -= SignOf(mouseY) * up * displacement;
			}

			// Apply the rotations
			Matrix totalRotation = Matrix::CreateRotation(Vector3(totalPitch, totalYaw, 0));
			forward = totalRotation.TransformVector(Vector3::UnitZ);
			Camera::forward.Normalize();

			//Update Matrices
			CalculateViewMatrix();
		}
	};
}
