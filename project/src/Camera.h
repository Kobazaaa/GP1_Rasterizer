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

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		const float MOVEMENT_SPEED = 15.f;
		const float ROTATION_SPEED = 30.f;

		Matrix worldToCamera{};
		Matrix cameraToWorld{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}


		void CalculateViewMatrix()
		{
			right	= Vector3::Cross(Vector3::UnitY, Camera::forward).Normalized();
			up		= Vector3::Cross(Camera::forward, right).Normalized();

			cameraToWorld = Matrix(
				right,
				up,
				Camera::forward,
				Camera::origin
			);

			worldToCamera = cameraToWorld.Inverse();
			
			//TODO W1
			//ONB => worldToCamera
			//Inverse(ONB) => ViewMatrix

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W3

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();
			const float displacement = MOVEMENT_SPEED * deltaTime;

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);


			if (pKeyboardState[SDL_SCANCODE_Z]
				or pKeyboardState[SDL_SCANCODE_W])		origin += Vector3(forward.x, forward.y, forward.z) * displacement;
			if (pKeyboardState[SDL_SCANCODE_Q]
				or pKeyboardState[SDL_SCANCODE_A])		origin -= Vector3(right.x, right.y, right.z) * displacement;
			if (pKeyboardState[SDL_SCANCODE_S])			origin -= Vector3(forward.x, forward.y, forward.z) * displacement;
			if (pKeyboardState[SDL_SCANCODE_D])			origin += Vector3(right.x, right.y, right.z) * displacement;
			if (pKeyboardState[SDL_SCANCODE_SPACE])		origin += Vector3::UnitY * displacement;
			if (pKeyboardState[SDL_SCANCODE_LCTRL])		origin -= Vector3::UnitY * displacement;


			// Rotate Camera when moving mouse and holding down LMB
			if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT) /*or mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)*/)
			{
				totalPitch	-= mouseY * ROTATION_SPEED * deltaTime * TO_RADIANS;
				totalPitch	= std::clamp(totalPitch, -80 * TO_RADIANS, 80 * TO_RADIANS); // locks the up/down camera rotation so you don't overshoot
				totalYaw	+= mouseX * ROTATION_SPEED * deltaTime * TO_RADIANS;
			}
			//// Move through the scene when holding down the mouse wheel button
			//if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
			//{
			//	int16_t sgnMouseX = 0;
			//	if (mouseX < -0.001f) sgnMouseX = -1;
			//	else if (mouseX > 0.001f) sgnMouseX = 1;
			//	else sgnMouseX = 0;

			//	int16_t sgnMouseY = 0;
			//	if (mouseY < -0.001f) sgnMouseY = 1;
			//	else if (mouseY > 0.001f) sgnMouseY = -1;
			//	else sgnMouseY = 0;

			//	origin += sgnMouseY * Vector3(forward.x, forward.y, forward.z) * MOVEMENT_SPEED * pTimer->GetElapsed();
			//	origin += sgnMouseX * Vector3(right.x  , right.y  , right.z  ) * MOVEMENT_SPEED * pTimer->GetElapsed();
			//}

			Matrix totalRotation = Matrix::CreateRotation(Vector3(totalPitch, totalYaw, 0));
			forward = totalRotation.TransformVector(Vector3::UnitZ);
			Camera::forward.Normalize();

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
