//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	m_AspectRatio = m_Width / static_cast<float>(m_Height);

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//Render_W1_Part1(); // Rasterizer Stage Only
	//Render_W1_Part2(); // Projection Stage (Camera)
	Render_W1_Part3(); // Barycentric Coordinates
	//Render_W1_Part4(); // Depth Buffer
	//Render_W1_Part5(); // BoundingBox Optimization
	

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	// reserve the vertices_out so it's big enough
	vertices_out.reserve(vertices_in.size());

	for (Vertex currVertex: vertices_in) // we make copies, can't edit the original ones
	{
		// from world to view
		currVertex.position = m_Camera.invViewMatrix.TransformPoint(currVertex.position); 

		// from view to projection
		currVertex.position.x = (currVertex.position.x / currVertex.position.z) / (m_AspectRatio * m_Camera.fov);
		currVertex.position.y = (currVertex.position.y / currVertex.position.z) / m_Camera.fov;
		// currently the z component stays the same


		// from projection to screen
		currVertex.position.x = (currVertex.position.x + 1) / 2 * m_Width;
		currVertex.position.y = (1 - currVertex.position.y) / 2 * m_Height;

		// put it in the out vector
		vertices_out.emplace_back(currVertex);
	}
}

void dae::Renderer::Render_W1_Part1()
{
	// make triangle
	std::vector<Vector3> vertices_ndc
	{
		{0.f, 0.5f, 1.f},
		{0.5f, -0.5f, 1.f},
		{-0.5f, -0.5f, 1.f}
	};

	// convert from NDC to screen space / raster space
	for (auto& vector: vertices_ndc)
	{
		vector.x = (vector.x + 1) / 2 * m_Width;
		vector.y = (1 - vector.y) / 2 * m_Height;
	}

	// safe current vertices -> will later be done in a for loop, looping all triangles
	const Vector2 v0{ vertices_ndc[0].x, vertices_ndc[0].y };
	const Vector2 v1{ vertices_ndc[1].x, vertices_ndc[1].y };
	const Vector2 v2{ vertices_ndc[2].x, vertices_ndc[2].y };
	
	// edges to check using cross
	const Vector2 edge10{ v1 - v0 };
	const Vector2 edge21{ v2 - v1 };
	const Vector2 edge02{ v0 - v2 };


	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			// get current pixel
			const Vector2 currPixel{ static_cast<float>(px),static_cast<float>(py) };

			// get vector from current vertex to pixel
			const Vector2 v0toPixel{ v0 - currPixel };
			const Vector2 v1toPixel{ v1 - currPixel };
			const Vector2 v2toPixel{ v2 - currPixel };

			// calculate all cross products and store them for later -> used in barycentric coordinates
			const float edge10CrossPixel{ Vector2::Cross(edge10, v0toPixel) };
			const float edge21CrossPixel{ Vector2::Cross(edge21, v1toPixel) };
			const float edge02CrossPixel{ Vector2::Cross(edge02, v2toPixel) };

			// check if everything is clockwise -> <= 0
			// if true, it is in the triangle, if not , it isn't
			// we want an early out so use the oposite

			if (edge10CrossPixel > 0 || edge21CrossPixel > 0 || edge02CrossPixel > 0) // pixel is NOT in the triangle
				continue;
			

			/*float gradient = px / static_cast<float>(m_Width);
			gradient += py / static_cast<float>(m_Width);
			gradient /= 2.0f;

			ColorRGB finalColor{ gradient, gradient, gradient };*/


			ColorRGB finalColor{ 1, 1, 1 };

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void dae::Renderer::Render_W1_Part2()
{
	// Define Triangle - Vertices in WORLD space
	std::vector<Vertex> vertices_world
	{
		{{0.f, 2.f, 0.f}},
		{{1.f, 0.0f, 0.0f}},
		{{-1.f, 0.f, 0.0f}}
	};
	
	std::vector<Vertex> vertices_screen{};

	VertexTransformationFunction(vertices_world, vertices_screen);



	// safe current vertices -> will later be done in a for loop, looping all triangles
	const Vector2 v0{ vertices_screen[0].position.x, vertices_screen[0].position.y };
	const Vector2 v1{ vertices_screen[1].position.x, vertices_screen[1].position.y };
	const Vector2 v2{ vertices_screen[2].position.x, vertices_screen[2].position.y };

	// edges to check using cross
	const Vector2 edge10{ v1 - v0 };
	const Vector2 edge21{ v2 - v1 };
	const Vector2 edge02{ v0 - v2 };


	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			// get current pixel
			const Vector2 currPixel{ static_cast<float>(px),static_cast<float>(py) };

			// get vector from current vertex to pixel
			const Vector2 v0toPixel{ v0 - currPixel };
			const Vector2 v1toPixel{ v1 - currPixel };
			const Vector2 v2toPixel{ v2 - currPixel };

			// calculate all cross products and store them for later -> used in barycentric coordinates
			const float edge10CrossPixel{ Vector2::Cross(edge10, v0toPixel) };
			const float edge21CrossPixel{ Vector2::Cross(edge21, v1toPixel) };
			const float edge02CrossPixel{ Vector2::Cross(edge02, v2toPixel) };

			// check if everything is clockwise -> <= 0
			// if true, it is in the triangle, if not , it isn't
			// we want an early out so use the oposite

			ColorRGB finalColor{ 1, 1, 1 };

			if (edge10CrossPixel > 0 || edge21CrossPixel > 0 || edge02CrossPixel > 0) // pixel is NOT in the triangle
				finalColor = colors::Black;

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void dae::Renderer::Render_W1_Part3()
{
	// Define Triangle - Vertices in WORLD space
	std::vector<Vertex> vertices_world
	{
		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices_screen{};

	VertexTransformationFunction(vertices_world, vertices_screen);



	// safe current vertices -> will later be done in a for loop, looping all triangles
	const Vector2 v0{ vertices_screen[0].position.x, vertices_screen[0].position.y };
	const Vector2 v1{ vertices_screen[1].position.x, vertices_screen[1].position.y };
	const Vector2 v2{ vertices_screen[2].position.x, vertices_screen[2].position.y };

	// edges to check using cross
	const Vector2 edge10{ v1 - v0 };
	const Vector2 edge21{ v2 - v1 };
	const Vector2 edge02{ v0 - v2 };


	const float triangleArea{ Vector2::Cross({v2 - v0}, edge10) };


	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			// get current pixel
			const Vector2 currPixel{ static_cast<float>(px),static_cast<float>(py) };

			// get vector from current vertex to pixel
			const Vector2 v0toPixel{ v0 - currPixel };
			const Vector2 v1toPixel{ v1 - currPixel };
			const Vector2 v2toPixel{ v2 - currPixel };

			// calculate all cross products and store them for later -> used in barycentric coordinates
			const float edge10CrossPixel{ Vector2::Cross(edge10, v0toPixel) };
			const float edge21CrossPixel{ Vector2::Cross(edge21, v1toPixel) };
			const float edge02CrossPixel{ Vector2::Cross(edge02, v2toPixel) };


			


			// check if everything is clockwise -> <= 0
			// if true, it is in the triangle, if not , it isn't
			// we want an early out so use the oposite

			ColorRGB finalColor{ 1, 1, 1 };

			if (edge10CrossPixel > 0 || edge21CrossPixel > 0 || edge02CrossPixel > 0) // pixel is NOT in the triangle
				finalColor = colors::Black;
			else
			{
				const float weight10{ edge10CrossPixel / triangleArea };
				const float weight21{ edge21CrossPixel / triangleArea };
				const float weight02{ edge02CrossPixel / triangleArea };

				finalColor = { vertices_screen[0].color * weight21 + vertices_screen[1].color * weight02 + vertices_screen[2].color * weight10 };
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
