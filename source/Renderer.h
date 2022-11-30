#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void ToggleDepth() { m_ShowDepth = !m_ShowDepth; }
		void ToggleCanRotate() { m_CanRotate = !m_CanRotate; }
		void ToggleNormalMapping() { m_DisplayNormalMapping = !m_DisplayNormalMapping; }

		void CycleRenderMode();

	private:

		enum class RenderMode
		{
			ObservedArea = 0,
			Diffuse = 1,
			Specular = 2,
			Combined = 3
		};

		RenderMode m_CurrentRenderMode{ RenderMode::Combined };
		bool m_ShowDepth{ false };
		bool m_CanRotate{ true };
		bool m_DisplayNormalMapping{ true };



		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		float m_AspectRatio{};
		
		// currently just using 1, will probably use more later
		Texture* m_pTexture{};
		Texture* m_pTextureTukTuk{};
		Texture* m_pTextureVehicleDiffuse{};
		Texture* m_pTextureVehicleNormal{};
		Texture* m_pTextureVehicleGloss{};
		Texture* m_pTextureVehicleSpecular{};

		Mesh TukTuk{ {},{}, PrimitiveTopology::TriangleList };
		Mesh Vehicle{ {},{}, PrimitiveTopology::TriangleList };
		Mesh TestPlane{};

		Matrix m_TranslateObjectPosition{};
		float m_CurrentRotation{};
		const float m_RotationSpeed{  1.f }; //  * TO_RADIANS
		const Vector3 m_LightDirection{ 0.577f, -0.577f, 0.577f };
		const float m_LightIntensity{ 7.f };
		const float m_Shininess{ 25.f };
		const ColorRGB m_Ambient{ 0.025f, 0.025f, 0.025f };




		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const; //W1 Version
		void VertexTransformationFunction(Mesh& currentMesh) const; //W3 Version

		void Render_W1_Part1();
		void Render_W1_Part2();
		void Render_W1_Part3();
		void Render_W1_Part4();
		void Render_W1_Part5();

		void Render_W2_Part1();
		void Render_W2_Part2();
		void Render_W2_Part3();
		void Render_W2_Part4();

		void Render_W3_Part1();

		void Render_W4_Part1();


		ColorRGB PixelShading(const Vertex_Out& v);

		bool CheckPositionInFrustrum(const Vector3& position);

	};
}
