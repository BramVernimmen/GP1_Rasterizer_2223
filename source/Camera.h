#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
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

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		Matrix projectionMatrix{};

		float nearPlane{ 0.1f };
		float farPlane{ 100.f };

		float aspectRatio{ 0.f };

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f}, float _aspectRatio = 1.7f, float _nearPlane = 0.1f, float _farPlane = 100.f)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);
			aspectRatio = _aspectRatio;
			nearPlane = _nearPlane;
			farPlane = _farPlane;

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			// same as cameraToWorld from Raytracer
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			//ONB => invViewMatrix
			invViewMatrix = { {right},{up},{forward},{origin} };
			//Inverse(ONB) => ViewMatrix
			viewMatrix = invViewMatrix.Inverse();

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W2
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			//...
			float movementSpeed{ 25.f };
			float rotationSpeed{ 80.f * TO_RADIANS };
			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);


			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);



			const float speedMultiplier{ 4.0f };

			// Speeding up all movement
			movementSpeed += movementSpeed * speedMultiplier * (pKeyboardState[SDL_SCANCODE_LSHIFT] || pKeyboardState[SDL_SCANCODE_RSHIFT]);
			rotationSpeed += rotationSpeed * speedMultiplier * (pKeyboardState[SDL_SCANCODE_LSHIFT] || pKeyboardState[SDL_SCANCODE_RSHIFT]);

			// In/decreasing FOV -> might not really be optimal, yet nesting if statements might be the easiest
			// currently disabled since not needed for assignment

			/*if (pKeyboardState[SDL_SCANCODE_O] || pKeyboardState[SDL_SCANCODE_P])
			{
				if (pKeyboardState[SDL_SCANCODE_O])
				{
					if (fovAngle > 0.5f)
					{
						fovAngle -= movementSpeed * deltaTime;
						UpdateFOV();
					}
				}
				else
				{
					if (fovAngle < 179.5f)
					{
						fovAngle += movementSpeed * deltaTime;
						UpdateFOV();
					}
				}
			}*/


			// Moving origin with "WASD"
			origin += forward * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP]);
			origin -= forward * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN]);
			origin += right * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT]);
			origin -= right * (movementSpeed * deltaTime) * (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT]);



			switch (mouseState)
			{
			case SDL_BUTTON_LMASK:
				origin += forward * -(static_cast<float>(mouseY) * movementSpeed * deltaTime);
				totalYaw += static_cast<float>(mouseX) * rotationSpeed * deltaTime;
				break;
			case SDL_BUTTON_RMASK:
				totalYaw += static_cast<float>(mouseX) * rotationSpeed * deltaTime;
				totalPitch += -(static_cast<float>(mouseY)) * rotationSpeed * deltaTime;
				break;
			case SDL_BUTTON_LMASK | SDL_BUTTON_RMASK:
				origin += up * -((movementSpeed / 2.f) * static_cast<float>(mouseY) * deltaTime);
				break;
			}


			const Matrix finalRotation{ Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw) };

			forward = finalRotation.TransformVector(Vector3::UnitZ);
			forward.Normalize();


			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
