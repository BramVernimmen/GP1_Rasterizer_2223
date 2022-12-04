//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"
#include "BRDFs.h"

#include <iostream>

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow),
	m_pTexture{Texture::LoadFromFile("Resources/uv_grid_2.png")},
	m_pTextureTukTuk{Texture::LoadFromFile("Resources/tuktuk.png")},
	m_pTextureVehicleDiffuse{Texture::LoadFromFile("Resources/vehicle_diffuse.png")},
	m_pTextureVehicleNormal{Texture::LoadFromFile("Resources/vehicle_normal.png")},
	m_pTextureVehicleGloss{Texture::LoadFromFile("Resources/vehicle_gloss.png")},
	m_pTextureVehicleSpecular{Texture::LoadFromFile("Resources/vehicle_specular.png")}
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	m_AspectRatio = m_Width / static_cast<float>(m_Height);

	//Initialize Camera
	//m_Camera.Initialize(60.f, { .0f,.0f,-10.f }, m_AspectRatio);
	//m_Camera.Initialize(60.f, { .0f,5.f,-30.f }, m_AspectRatio);
	m_Camera.Initialize(45.f, { 0.0f, 0.0f, 0.0f }, m_AspectRatio);

	TestPlane = Mesh{
			{
				Vertex{{-3, 3, -2},		{1,1,1},	{0,0}},
				Vertex{{0,3,-2},		{1,1,1},	{0.5f, 0}},
				Vertex{{3,3,-2},		{1,1,1},	{1,0}},
				Vertex{{-3, 0, -2},		{1,1,1},	{0, 0.5f}},
				Vertex{{0,0,-2},		{1,1,1},	{0.5f, 0.5f}},
				Vertex{{3,0,-2},		{1,1,1},	{1, 0.5f}},
				Vertex{{-3,-3,-2},		{1,1,1},	{0,1}},
				Vertex{{0,-3,-2},		{1,1,1},	{0.5f, 1}},
				Vertex{{3,-3,-2},		{1,1,1},	{1,1}}
			},
			{
				3,0,1,		1,4,3,		4,1,2,
				2,5,4,		6,3,4,		4,7,6,
				7,4,5,		5,8,7
		},
		PrimitiveTopology::TriangleList
	};

	Utils::ParseOBJ("Resources/tuktuk.obj", TukTuk.vertices, TukTuk.indices);
	Utils::ParseOBJ("Resources/vehicle.obj", Vehicle.vertices, Vehicle.indices);
	m_TranslateObjectPosition = Matrix::CreateTranslation(0.f, 0.f, 50.f);
	Vehicle.worldMatrix *= m_TranslateObjectPosition;
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
	delete m_pTextureTukTuk;
	delete m_pTextureVehicleDiffuse;
	delete m_pTextureVehicleGloss;
	delete m_pTextureVehicleNormal;
	delete m_pTextureVehicleSpecular;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
	//TukTuk.worldMatrix *= Matrix::CreateRotationY(0.003f); // -> Week 03
	if (m_CanRotate)
	{
		m_CurrentRotation += m_RotationSpeed * pTimer->GetElapsed();
		Vehicle.worldMatrix = Matrix::CreateRotationY(m_CurrentRotation) * m_TranslateObjectPosition; // -> Week 04
	}

}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// WEEK 1
	//Render_W1_Part1(); // Rasterizer Stage Only
	//Render_W1_Part2(); // Projection Stage (Camera)
	//Render_W1_Part3(); // Barycentric Coordinates
	//Render_W1_Part4(); // Depth Buffer
	//Render_W1_Part5(); // BoundingBox Optimization
	
	// WEEK 2
	//Render_W2_Part1(); // TriangleList
	//Render_W2_Part2(); // TriangleStrip
	//Render_W2_Part3(); // Textures and UV
	//Render_W2_Part4(); // Depth Interpolation

	// WEEK 3
	//Render_W3_Part1(); // Added Frustum Culling

	// WEEK 4
	Render_W4_Part1(); // Pixel Shading Stage


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

void Renderer::VertexTransformationFunction(Mesh& currentMesh) const
{
	// reserve the vertices_out so it's big enough
	currentMesh.vertices_out.reserve(currentMesh.vertices.size());
	const Matrix worldViewProjectionMatrix{ currentMesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };
	for (const Vertex& currVertex: currentMesh.vertices) // we make copies, can't edit the original ones
	{
		Vertex_Out newVertexOut{
			Vector4{currVertex.position, 1.f},
			currVertex.color,
			currVertex.uv,
			currentMesh.worldMatrix.TransformVector(currVertex.normal),
			currentMesh.worldMatrix.TransformVector(currVertex.tangent),
			currentMesh.worldMatrix.TransformPoint(currVertex.position )- m_Camera.origin };

		newVertexOut.position = worldViewProjectionMatrix.TransformPoint(newVertexOut.position);

		// perspective divide
		const float perspectiveDivideInverse{ 1.f / newVertexOut.position.w };
		newVertexOut.position.x *= perspectiveDivideInverse;
		newVertexOut.position.y *= perspectiveDivideInverse;
		newVertexOut.position.z *= perspectiveDivideInverse;


		currentMesh.vertices_out.emplace_back(newVertexOut);
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

void dae::Renderer::Render_W1_Part4()
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);

	// Define Triangle - Vertices in WORLD space
	std::vector<Vertex> vertices_world
	{
		// Triangle 0
		{{0.f, 2.f, 0.f}, {1,0,0}},
		{{1.5f, -1.f, 0.f}, {1,0,0}},
		{{-1.5f, -1.f, 0.f}, {1,0,0}},

		// Triangle 1
		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices_screen{};

	VertexTransformationFunction(vertices_world, vertices_screen);

	for (int i{0}; i < static_cast<int>(vertices_screen.size()); i += 3)
	{
		// safe current vertices
		const Vector2 v0{ vertices_screen[i].position.x, vertices_screen[i].position.y };
		const Vector2 v1{ vertices_screen[i + 1].position.x, vertices_screen[i + 1].position.y };
		const Vector2 v2{ vertices_screen[i + 2].position.x, vertices_screen[i + 2].position.y };

		// edges to check using cross
		const Vector2 edge10{ v1 - v0 };
		const Vector2 edge21{ v2 - v1 };
		const Vector2 edge02{ v0 - v2 };


		const float triangleArea{ Vector2::Cross({v2 - v0}, edge10) };
		const float invTriangleArea{ 1.f / triangleArea };

		//RENDER LOGIC
		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				const int pixelIndex{ px + py * m_Width };
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


				ColorRGB finalColor{ 1, 1, 1 };

				// barycentric weights
				const float weight10{ edge10CrossPixel * invTriangleArea };
				const float weight21{ edge21CrossPixel * invTriangleArea };
				const float weight02{ edge02CrossPixel * invTriangleArea };

				// depths
				const float depthV0{ vertices_screen[i].position.z };
				const float depthV1{ vertices_screen[i+ 1].position.z };
				const float depthV2{ vertices_screen[i + 2].position.z };
				
				// interpolate to get the value
				// didn't know how to do this for this step, so looked a week ahead :)
				const float interpolatedDepthValue
				{
					1.f /
					(
						weight21 * (1.f / depthV0) + //weight10 * (1.f / depthV0) +
						weight02 * (1.f / depthV1) + //weight21 * (1.f / depthV1) +
						weight10 * (1.f / depthV2)	 //weight02 * (1.f / depthV2)
					)
				};

				// check if calculated depth value is smaller then current
				// do the opposite for early out
				if (interpolatedDepthValue >= m_pDepthBufferPixels[pixelIndex])
					continue;
				// set the depthbufferpixel
				m_pDepthBufferPixels[pixelIndex] = interpolatedDepthValue;



				finalColor = { vertices_screen[i].color * weight21 + vertices_screen[i + 1].color * weight02 + vertices_screen[i + 2].color * weight10 };

				//Update Color in Buffer
				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}

}

void dae::Renderer::Render_W1_Part5()
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);

	// Define Triangle - Vertices in WORLD space
	std::vector<Vertex> vertices_world
	{
		// Triangle 0
		{{0.f, 2.f, 0.f}, {1,0,0}},
		{{1.5f, -1.f, 0.f}, {1,0,0}},
		{{-1.5f, -1.f, 0.f}, {1,0,0}},

		// Triangle 1
		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices_screen{};

	VertexTransformationFunction(vertices_world, vertices_screen);

	for (int i{ 0 }; i < static_cast<int>(vertices_screen.size()); i += 3)
	{
		// safe current vertices
		const Vector2 v0{ vertices_screen[i].position.x, vertices_screen[i].position.y };
		const Vector2 v1{ vertices_screen[i + 1].position.x, vertices_screen[i + 1].position.y };
		const Vector2 v2{ vertices_screen[i + 2].position.x, vertices_screen[i + 2].position.y };

		

		


		// edges to check using cross
		const Vector2 edge10{ v1 - v0 };
		const Vector2 edge21{ v2 - v1 };
		const Vector2 edge02{ v0 - v2 };


		const float triangleArea{ Vector2::Cross({v2 - v0}, edge10) };
		const float invTriangleArea{ 1.f / triangleArea };


		// setup bounding box
		Vector2 boundingBoxMin{ Vector2::Min(v0, Vector2::Min(v1, v2)) };
		Vector2 boundingBoxMax{ Vector2::Max(v0, Vector2::Max(v1, v2)) };
		// clamp to screensize
		// this could give a lot of if statements, easier way is to also check using Min and Max with a minVector of 0 and a screenvector containing the size
		Vector2 screenSize{ static_cast<float>(m_Width), static_cast<float>(m_Height) }; // max values of the screen
		boundingBoxMin = Vector2::Min(screenSize, Vector2::Max(boundingBoxMin, Vector2::Zero)); // this way, we will always be >= zero and <= screensize
		boundingBoxMax = Vector2::Min(screenSize, Vector2::Max(boundingBoxMax, Vector2::Zero));

		//RENDER LOGIC
		// adapt to use the boundingboxMin and max instead
		// only the pixels inside this box will be checked
		// boundingbox is modified to the min and max size of the current triangle -> much less pixels to check
		// boundingbox should always be square, so think of it as reducing the screensize for the current triangle
		for (int px{static_cast<int>(boundingBoxMin.x)}; px < static_cast<int>(boundingBoxMax.x); ++px)
		{
			for (int py{ static_cast<int>(boundingBoxMin.y)}; py < static_cast<int>(boundingBoxMax.y); ++py)
			{
				const int pixelIndex{ px + py * m_Width };
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


				ColorRGB finalColor{ 1, 1, 1 };

				// barycentric weights
				const float weight10{ edge10CrossPixel * invTriangleArea };
				const float weight21{ edge21CrossPixel * invTriangleArea };
				const float weight02{ edge02CrossPixel * invTriangleArea };

				// depths
				const float depthV0{ vertices_screen[i].position.z };
				const float depthV1{ vertices_screen[i + 1].position.z };
				const float depthV2{ vertices_screen[i + 2].position.z };

				// interpolate to get the value
				// didn't know how to do this for this step, so looked a week ahead :)
				const float interpolatedDepthValue
				{
					1.f /
					(
						weight21 * (1.f / depthV0) + //weight10 * (1.f / depthV0) +
						weight02 * (1.f / depthV1) + //weight21 * (1.f / depthV1) +
						weight10 * (1.f / depthV2)	 //weight02 * (1.f / depthV2)
					)
				};

				// check if calculated depth value is smaller then current
				// do the opposite for early out
				if (interpolatedDepthValue >= m_pDepthBufferPixels[pixelIndex])
					continue;
				// set the depthbufferpixel
				m_pDepthBufferPixels[pixelIndex] = interpolatedDepthValue;



				finalColor = { vertices_screen[i].color * weight21 + vertices_screen[i + 1].color * weight02 + vertices_screen[i + 2].color * weight10 };

				//Update Color in Buffer
				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}

}



void dae::Renderer::Render_W2_Part1()
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);

	// Define Mesh (in world space)
	std::vector<Mesh> meshes_world
	{
		Mesh{
			{
				Vertex{{-3, 3, -2}},
				Vertex{{0,3,-2}},
				Vertex{{3,3,-2}},
				Vertex{{-3, 0, -2}},
				Vertex{{0,0,-2}},
				Vertex{{3,0,-2}},
				Vertex{{-3,-3,-2}},
				Vertex{{0,-3,-2}},
				Vertex{{3,-3,-2}}
			},
			{
				3,0,1,		1,4,3,		4,1,2,
				2,5,4,		6,3,4,		4,7,6,
				7,4,5,		5,8,7
		},
		PrimitiveTopology::TriangleList
		}
	};


	for (const Mesh& currMesh : meshes_world) // we loop over all meshes, transform the vertices and use those
	{
		std::vector<Vertex> vertices_screen{}; // of the current mesh
		VertexTransformationFunction(currMesh.vertices, vertices_screen);

		for (int i{ 0 }; i < static_cast<int>(currMesh.indices.size()); i += 3)
		{
			// to make it easier, get the indexes for the vertices first
			const uint32_t indexV0{ currMesh.indices[i] };
			const uint32_t indexV1{ currMesh.indices[i + 1] };
			const uint32_t indexV2{ currMesh.indices[i + 2] };
			// safe current vertices
			const Vector2 v0{ vertices_screen[indexV0].position.x, vertices_screen[indexV0].position.y };
			const Vector2 v1{ vertices_screen[indexV1].position.x, vertices_screen[indexV1].position.y };
			const Vector2 v2{ vertices_screen[indexV2].position.x, vertices_screen[indexV2].position.y };






			// edges to check using cross
			const Vector2 edge10{ v1 - v0 };
			const Vector2 edge21{ v2 - v1 };
			const Vector2 edge02{ v0 - v2 };


			const float triangleArea{ Vector2::Cross({v2 - v0}, edge10) };
			const float invTriangleArea{ 1.f / triangleArea };


			// setup bounding box
			Vector2 boundingBoxMin{ Vector2::Min(v0, Vector2::Min(v1, v2)) };
			Vector2 boundingBoxMax{ Vector2::Max(v0, Vector2::Max(v1, v2)) };
			// clamp to screensize
			// this could give a lot of if statements, easier way is to also check using Min and Max with a minVector of 0 and a screenvector containing the size
			Vector2 screenSize{ static_cast<float>(m_Width), static_cast<float>(m_Height) }; // max values of the screen
			boundingBoxMin = Vector2::Min(screenSize, Vector2::Max(boundingBoxMin, Vector2::Zero)); // this way, we will always be >= zero and <= screensize
			boundingBoxMax = Vector2::Min(screenSize, Vector2::Max(boundingBoxMax, Vector2::Zero));

			//RENDER LOGIC
			// adapt to use the boundingboxMin and max instead
			// only the pixels inside this box will be checked
			// boundingbox is modified to the min and max size of the current triangle -> much less pixels to check
			// boundingbox should always be square, so think of it as reducing the screensize for the current triangle
			for (int px{ static_cast<int>(boundingBoxMin.x) }; px < boundingBoxMax.x; ++px)
			{
				for (int py{ static_cast<int>(boundingBoxMin.y) }; py < boundingBoxMax.y; ++py)
				{
					const int pixelIndex{ px + py * m_Width };
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


					ColorRGB finalColor{ 1, 1, 1 };

					// barycentric weights
					const float weight10{ edge10CrossPixel * invTriangleArea };
					const float weight21{ edge21CrossPixel * invTriangleArea };
					const float weight02{ edge02CrossPixel * invTriangleArea };

					// depths
					const float depthV0{ vertices_screen[indexV0].position.z };
					const float depthV1{ vertices_screen[indexV1].position.z };
					const float depthV2{ vertices_screen[indexV2].position.z };

					// interpolate to get the value
					// didn't know how to do this for this step, so looked a week ahead :)
					const float interpolatedDepthValue
					{
						1.f /
						(
							weight21 * (1.f / depthV0) + //weight10 * (1.f / depthV0) +
							weight02 * (1.f / depthV1) + //weight21 * (1.f / depthV1) +
							weight10 * (1.f / depthV2)	 //weight02 * (1.f / depthV2)
						)
					};

					// check if calculated depth value is smaller then current
					// do the opposite for early out
					if (interpolatedDepthValue >= m_pDepthBufferPixels[pixelIndex])
						continue;
					// set the depthbufferpixel
					m_pDepthBufferPixels[pixelIndex] = interpolatedDepthValue;



					finalColor = { vertices_screen[indexV0].color * weight21 + vertices_screen[indexV1].color * weight02 + vertices_screen[indexV2].color * weight10 };

					//Update Color in Buffer
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}

}

void dae::Renderer::Render_W2_Part2()
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);

	// Define Mesh (in world space)
	std::vector<Mesh> meshes_world
	{
		Mesh{
			{
				Vertex{{-3, 3, -2}},
				Vertex{{0,3,-2}},
				Vertex{{3,3,-2}},
				Vertex{{-3, 0, -2}},
				Vertex{{0,0,-2}},
				Vertex{{3,0,-2}},
				Vertex{{-3,-3,-2}},
				Vertex{{0,-3,-2}},
				Vertex{{3,-3,-2}}
			},
			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
		},
		PrimitiveTopology::TriangleStrip
		}
	};


	for (const Mesh& currMesh : meshes_world) // we loop over all meshes, transform the vertices and use those
	{
		std::vector<Vertex> vertices_screen{}; // of the current mesh
		VertexTransformationFunction(currMesh.vertices, vertices_screen);

		bool useModulo{ false };
		int incrementor{ 3 };

		if (currMesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			useModulo = true;
			incrementor = 1;
			// when using triangleStrip we go down the list of indices one by one see slides W7, slide 8 - 11
		}


		for (int i{ 0 }; i < static_cast<int>(currMesh.indices.size()- 2); i += incrementor)
		{
			// to make it easier, get the indexes for the vertices first
			const uint32_t indexV0{ currMesh.indices[i] };
			// when using triangleStrip, we want to swap these if current triangle is odd (% 2 == 1)
			const int moduloResult{ useModulo * (i % 2) }; // modulo can be heavy, calculate it once instead of twice
			const uint32_t indexV1{ currMesh.indices[i + 1 + moduloResult]}; // if triangle is odd, we do index = i + 1 + (1* 1)
			const uint32_t indexV2{ currMesh.indices[i + 2 - moduloResult]}; // if triangle is odd, we do index = i + 2 - (1* 1)

			// check if there are multiple of the same indexes, use early out, these are buffers
			if (indexV0 == indexV1 || indexV0 == indexV2 || indexV1 == indexV2)
				continue;

			// safe current vertices
			const Vector2 v0{ vertices_screen[indexV0].position.x, vertices_screen[indexV0].position.y };
			const Vector2 v1{ vertices_screen[indexV1].position.x, vertices_screen[indexV1].position.y };
			const Vector2 v2{ vertices_screen[indexV2].position.x, vertices_screen[indexV2].position.y };






			// edges to check using cross
			const Vector2 edge10{ v1 - v0 };
			const Vector2 edge21{ v2 - v1 };
			const Vector2 edge02{ v0 - v2 };


			const float triangleArea{ Vector2::Cross({v2 - v0}, edge10) };
			const float invTriangleArea{ 1.f / triangleArea };


			// setup bounding box
			Vector2 boundingBoxMin{ Vector2::Min(v0, Vector2::Min(v1, v2)) };
			Vector2 boundingBoxMax{ Vector2::Max(v0, Vector2::Max(v1, v2)) };
			// clamp to screensize
			// this could give a lot of if statements, easier way is to also check using Min and Max with a minVector of 0 and a screenvector containing the size
			Vector2 screenSize{ static_cast<float>(m_Width), static_cast<float>(m_Height) }; // max values of the screen
			boundingBoxMin = Vector2::Min(screenSize, Vector2::Max(boundingBoxMin, Vector2::Zero)); // this way, we will always be >= zero and <= screensize
			boundingBoxMax = Vector2::Min(screenSize, Vector2::Max(boundingBoxMax, Vector2::Zero));

			//RENDER LOGIC
			// adapt to use the boundingboxMin and max instead
			// only the pixels inside this box will be checked
			// boundingbox is modified to the min and max size of the current triangle -> much less pixels to check
			// boundingbox should always be square, so think of it as reducing the screensize for the current triangle
			for (int px{ static_cast<int>(boundingBoxMin.x) }; px < boundingBoxMax.x; ++px)
			{
				for (int py{ static_cast<int>(boundingBoxMin.y) }; py < boundingBoxMax.y; ++py)
				{
					const int pixelIndex{ px + py * m_Width };
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


					ColorRGB finalColor{ 1, 1, 1 };

					// barycentric weights
					const float weight10{ edge10CrossPixel * invTriangleArea };
					const float weight21{ edge21CrossPixel * invTriangleArea };
					const float weight02{ edge02CrossPixel * invTriangleArea };

					// depths
					const float depthV0{ vertices_screen[indexV0].position.z };
					const float depthV1{ vertices_screen[indexV1].position.z };
					const float depthV2{ vertices_screen[indexV2].position.z };

					// interpolate to get the value
					// didn't know how to do this for this step, so looked a week ahead :)
					const float interpolatedDepthValue
					{
						1.f /
						(
							weight21 * (1.f / depthV0) + //weight10 * (1.f / depthV0) +
							weight02 * (1.f / depthV1) + //weight21 * (1.f / depthV1) +
							weight10 * (1.f / depthV2)	 //weight02 * (1.f / depthV2)
						)
					};

					// check if calculated depth value is smaller then current
					// do the opposite for early out
					if (interpolatedDepthValue >= m_pDepthBufferPixels[pixelIndex])
						continue;
					// set the depthbufferpixel
					m_pDepthBufferPixels[pixelIndex] = interpolatedDepthValue;



					finalColor = { vertices_screen[indexV0].color * weight21 + vertices_screen[indexV1].color * weight02 + vertices_screen[indexV2].color * weight10 };

					//Update Color in Buffer
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}

}

void dae::Renderer::Render_W2_Part3()
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);

	// Define Mesh (in world space)
	std::vector<Mesh> meshes_world
	{
		Mesh{
			{
				Vertex{{-3, 3, -2},		{1,1,1},	{0,0}},
				Vertex{{0,3,-2},		{1,1,1},	{0.5f, 0}},
				Vertex{{3,3,-2},		{1,1,1},	{1,0}},
				Vertex{{-3, 0, -2},		{1,1,1},	{0, 0.5f}},
				Vertex{{0,0,-2},		{1,1,1},	{0.5f, 0.5f}},
				Vertex{{3,0,-2},		{1,1,1},	{1, 0.5f}},
				Vertex{{-3,-3,-2},		{1,1,1},	{0,1}},
				Vertex{{0,-3,-2},		{1,1,1},	{0.5f, 1}},
				Vertex{{3,-3,-2},		{1,1,1},	{1,1}}
			},
			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5

				// for TriangleList
				/*3,0,1,		1,4,3,		4,1,2,
				2,5,4,		6,3,4,		4,7,6,
				7,4,5,		5,8,7*/
		},
		PrimitiveTopology::TriangleStrip
		//PrimitiveTopology::TriangleList
		}
	};


	for (const Mesh& currMesh : meshes_world) // we loop over all meshes, transform the vertices and use those
	{
		std::vector<Vertex> vertices_screen{}; // of the current mesh
		VertexTransformationFunction(currMesh.vertices, vertices_screen);

		bool useModulo{ false };
		int incrementor{ 3 };

		if (currMesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			useModulo = true;
			incrementor = 1;
			// when using triangleStrip we go down the list of indices one by one see slides W7, slide 8 - 11
		}


		for (int i{ 0 }; i < static_cast<int>(currMesh.indices.size()- 2); i += incrementor)
		{
			// to make it easier, get the indexes for the vertices first
			const uint32_t indexV0{ currMesh.indices[i] };
			// when using triangleStrip, we want to swap these if current triangle is odd (% 2 == 1)
			const int moduloResult{ useModulo * (i % 2) }; // modulo can be heavy, calculate it once instead of twice
			const uint32_t indexV1{ currMesh.indices[i + 1 + moduloResult]}; // if triangle is odd, we do index = i + 1 + (1* 1)
			const uint32_t indexV2{ currMesh.indices[i + 2 - moduloResult]}; // if triangle is odd, we do index = i + 2 - (1* 1)

			// check if there are multiple of the same indexes, use early out, these are buffers
			if (indexV0 == indexV1 || indexV0 == indexV2 || indexV1 == indexV2)
				continue;

			// safe current vertices
			const Vector2 v0{ vertices_screen[indexV0].position.x, vertices_screen[indexV0].position.y };
			const Vector2 v1{ vertices_screen[indexV1].position.x, vertices_screen[indexV1].position.y };
			const Vector2 v2{ vertices_screen[indexV2].position.x, vertices_screen[indexV2].position.y };






			// edges to check using cross
			const Vector2 edge10{ v1 - v0 };
			const Vector2 edge21{ v2 - v1 };
			const Vector2 edge02{ v0 - v2 };


			const float triangleArea{ Vector2::Cross({v2 - v0}, edge10) };
			const float invTriangleArea{ 1.f / triangleArea };


			// setup bounding box
			Vector2 boundingBoxMin{ Vector2::Min(v0, Vector2::Min(v1, v2)) };
			Vector2 boundingBoxMax{ Vector2::Max(v0, Vector2::Max(v1, v2)) };
			// clamp to screensize
			// this could give a lot of if statements, easier way is to also check using Min and Max with a minVector of 0 and a screenvector containing the size
			Vector2 screenSize{ static_cast<float>(m_Width), static_cast<float>(m_Height) }; // max values of the screen
			boundingBoxMin = Vector2::Min(screenSize, Vector2::Max(boundingBoxMin, Vector2::Zero)); // this way, we will always be >= zero and <= screensize
			boundingBoxMax = Vector2::Min(screenSize, Vector2::Max(boundingBoxMax, Vector2::Zero));

			//RENDER LOGIC
			// adapt to use the boundingboxMin and max instead
			// only the pixels inside this box will be checked
			// boundingbox is modified to the min and max size of the current triangle -> much less pixels to check
			// boundingbox should always be square, so think of it as reducing the screensize for the current triangle
			for (int px{ static_cast<int>(boundingBoxMin.x) }; px < boundingBoxMax.x; ++px)
			{
				for (int py{ static_cast<int>(boundingBoxMin.y) }; py < boundingBoxMax.y; ++py)
				{
					const int pixelIndex{ px + py * m_Width };
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



					// barycentric weights
					const float weight10{ edge10CrossPixel * invTriangleArea };
					const float weight21{ edge21CrossPixel * invTriangleArea };
					const float weight02{ edge02CrossPixel * invTriangleArea };

					// depths
					const float depthV0{ vertices_screen[indexV0].position.z };
					const float depthV1{ vertices_screen[indexV1].position.z };
					const float depthV2{ vertices_screen[indexV2].position.z };

					// interpolate to get the value
					// didn't know how to do this for this step, so looked a week ahead :)
					const float interpolatedDepthValue
					{
						1.f /
						(
							weight21 * (1.f / depthV0) + 
							weight02 * (1.f / depthV1) + 
							weight10 * (1.f / depthV2)	 
						)
					};

					// check if calculated depth value is smaller then current
					// do the opposite for early out
					if (interpolatedDepthValue >= m_pDepthBufferPixels[pixelIndex])
						continue;
					// set the depthbufferpixel
					m_pDepthBufferPixels[pixelIndex] = interpolatedDepthValue;

					const Vector2 interpolatedUV
					{
						vertices_screen[indexV0].uv * weight21 +
						vertices_screen[indexV1].uv * weight02 +
						vertices_screen[indexV2].uv * weight10 
					};

					ColorRGB finalColor{ m_pTexture->Sample(interpolatedUV)};

					//Update Color in Buffer
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}

}

void dae::Renderer::Render_W2_Part4()
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);

	// Define Mesh (in world space)
	std::vector<Mesh> meshes_world
	{
		Mesh{
			{
				Vertex{{-3, 3, -2},		{1,1,1},	{0,0}},
				Vertex{{0,3,-2},		{1,1,1},	{0.5f, 0}},
				Vertex{{3,3,-2},		{1,1,1},	{1,0}},
				Vertex{{-3, 0, -2},		{1,1,1},	{0, 0.5f}},
				Vertex{{0,0,-2},		{1,1,1},	{0.5f, 0.5f}},
				Vertex{{3,0,-2},		{1,1,1},	{1, 0.5f}},
				Vertex{{-3,-3,-2},		{1,1,1},	{0,1}},
				Vertex{{0,-3,-2},		{1,1,1},	{0.5f, 1}},
				Vertex{{3,-3,-2},		{1,1,1},	{1,1}}
			},
			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5

				// for TriangleList
				/*3,0,1,		1,4,3,		4,1,2,
				2,5,4,		6,3,4,		4,7,6,
				7,4,5,		5,8,7*/
		},
		PrimitiveTopology::TriangleStrip
		//PrimitiveTopology::TriangleList
		}
	};


	for (const Mesh& currMesh : meshes_world) // we loop over all meshes, transform the vertices and use those
	{
		std::vector<Vertex> vertices_screen{}; // of the current mesh
		VertexTransformationFunction(currMesh.vertices, vertices_screen);

		bool useModulo{ false };
		int incrementor{ 3 };

		if (currMesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			useModulo = true;
			incrementor = 1;
			// when using triangleStrip we go down the list of indices one by one see slides W7, slide 8 - 11
		}


		for (int i{ 0 }; i < static_cast<int>(currMesh.indices.size()- 2); i += incrementor)
		{
			// to make it easier, get the indexes for the vertices first
			const uint32_t indexV0{ currMesh.indices[i] };
			// when using triangleStrip, we want to swap these if current triangle is odd (% 2 == 1)
			const int moduloResult{ useModulo * (i % 2) }; // modulo can be heavy, calculate it once instead of twice
			const uint32_t indexV1{ currMesh.indices[i + 1 + moduloResult]}; // if triangle is odd, we do index = i + 1 + (1* 1)
			const uint32_t indexV2{ currMesh.indices[i + 2 - moduloResult]}; // if triangle is odd, we do index = i + 2 - (1* 1)

			// check if there are multiple of the same indexes, use early out, these are buffers
			if (indexV0 == indexV1 || indexV0 == indexV2 || indexV1 == indexV2)
				continue;

			// safe current vertices
			const Vector2 v0{ vertices_screen[indexV0].position.x, vertices_screen[indexV0].position.y };
			const Vector2 v1{ vertices_screen[indexV1].position.x, vertices_screen[indexV1].position.y };
			const Vector2 v2{ vertices_screen[indexV2].position.x, vertices_screen[indexV2].position.y };


			// edges to check using cross
			const Vector2 edge10{ v1 - v0 };
			const Vector2 edge21{ v2 - v1 };
			const Vector2 edge02{ v0 - v2 };


			const float triangleArea{ Vector2::Cross({v2 - v0}, edge10) };
			const float invTriangleArea{ 1.f / triangleArea };


			// setup bounding box
			Vector2 boundingBoxMin{ Vector2::Min(v0, Vector2::Min(v1, v2)) };
			Vector2 boundingBoxMax{ Vector2::Max(v0, Vector2::Max(v1, v2)) };
			// clamp to screensize
			// this could give a lot of if statements, easier way is to also check using Min and Max with a minVector of 0 and a screenvector containing the size
			Vector2 screenSize{ static_cast<float>(m_Width), static_cast<float>(m_Height) }; // max values of the screen
			boundingBoxMin = Vector2::Min(screenSize, Vector2::Max(boundingBoxMin, Vector2::Zero)); // this way, we will always be >= zero and <= screensize
			boundingBoxMax = Vector2::Min(screenSize, Vector2::Max(boundingBoxMax, Vector2::Zero));

			//RENDER LOGIC
			// adapt to use the boundingboxMin and max instead
			// only the pixels inside this box will be checked
			// boundingbox is modified to the min and max size of the current triangle -> much less pixels to check
			// boundingbox should always be square, so think of it as reducing the screensize for the current triangle
			for (int px{ static_cast<int>(boundingBoxMin.x) }; px < boundingBoxMax.x; ++px)
			{
				for (int py{ static_cast<int>(boundingBoxMin.y) }; py < boundingBoxMax.y; ++py)
				{
					const int pixelIndex{ px + py * m_Width };
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



					// barycentric weights
					const float weight10{ edge10CrossPixel * invTriangleArea };
					const float weight21{ edge21CrossPixel * invTriangleArea };
					const float weight02{ edge02CrossPixel * invTriangleArea };

					// depths
					const float depthV0{ vertices_screen[indexV0].position.z };
					const float depthV1{ vertices_screen[indexV1].position.z };
					const float depthV2{ vertices_screen[indexV2].position.z };

					// interpolate to get the value
					// didn't know how to do this for this step, so looked a week ahead :)
					const float interpolatedDepthValue
					{
						1.f /
						(
							weight21 * (1.f / depthV0) + 
							weight02 * (1.f / depthV1) + 
							weight10 * (1.f / depthV2)	 
						)
					};

					// check if calculated depth value is smaller then current
					// do the opposite for early out
					if (interpolatedDepthValue >= m_pDepthBufferPixels[pixelIndex])
						continue;
					// set the depthbufferpixel
					m_pDepthBufferPixels[pixelIndex] = interpolatedDepthValue;

					const Vector2 interpolatedUV
					{
						(
						(vertices_screen[indexV0].uv / vertices_screen[indexV0].position.z) * weight21 +
						(vertices_screen[indexV1].uv / vertices_screen[indexV1].position.z) * weight02 +
						(vertices_screen[indexV2].uv / vertices_screen[indexV2].position.z) * weight10
						) * interpolatedDepthValue
					};

					ColorRGB finalColor{ m_pTexture->Sample(interpolatedUV)};

					//Update Color in Buffer
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}

}


void dae::Renderer::Render_W3_Part1()
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 0, 0, 0));
	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), 1.f);

	// Define Mesh (in world space)
	std::vector<Mesh> meshes_world
	{
		TukTuk
	};

	for (Mesh& currMesh : meshes_world) // we loop over all meshes, transform the vertices and use those
	{
		VertexTransformationFunction(currMesh);
		// get all vertices into screen space
		std::vector<Vector2> vertices_screen{};
		vertices_screen.reserve(currMesh.vertices_out.size());
		for (const auto& currVertex : currMesh.vertices_out)
		{
			vertices_screen.emplace_back(Vector2{ (currVertex.position.x + 1) * 0.5f * m_Width, (1 - currVertex.position.y) * 0.5f * m_Height });
		}


		bool useModulo{ false };
		int incrementor{ 3 };

		if (currMesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			useModulo = true;
			incrementor = 1;
			// when using triangleStrip we go down the list of indices one by one see slides W7, slide 8 - 11
		}


		for (int i{ 0 }; i < static_cast<int>(currMesh.indices.size()- 2); i += incrementor)
		{
			// to make it easier, get the indexes for the vertices first
			const uint32_t indexV0{ currMesh.indices[i] };
			// when using triangleStrip, we want to swap these if current triangle is odd (% 2 == 1)
			const int moduloResult{ useModulo * (i % 2) }; // modulo can be heavy, calculate it once instead of twice
			const uint32_t indexV1{ currMesh.indices[i + 1 + moduloResult]}; // if triangle is odd, we do index = i + 1 + (1* 1)
			const uint32_t indexV2{ currMesh.indices[i + 2 - moduloResult]}; // if triangle is odd, we do index = i + 2 - (1* 1)

			// check if there are multiple of the same indexes, use early out, these are buffers
			if (indexV0 == indexV1 || indexV0 == indexV2 || indexV1 == indexV2)
				continue;
			

			// will keep my old and longer check in here for reference
			// doing everything in a function really simplifies the process
			// 
			// check for x pos
			//const bool xIndex0InFrustrum{ currMesh.vertices_out[indexV0].position.x >= -1 && currMesh.vertices_out[indexV0].position.x <= 1 };
			//const bool xIndex1InFrustrum{ currMesh.vertices_out[indexV1].position.x >= -1 && currMesh.vertices_out[indexV1].position.x <= 1 };
			//const bool xIndex2InFrustrum{ currMesh.vertices_out[indexV2].position.x >= -1 && currMesh.vertices_out[indexV2].position.x <= 1 };
			//const bool xInFrustrum{ xIndex0InFrustrum && xIndex1InFrustrum && xIndex2InFrustrum };
			//
			//// check for y pos
			//const bool yIndex0InFrustrum{ currMesh.vertices_out[indexV0].position.y >= -1 && currMesh.vertices_out[indexV0].position.y <= 1 };
			//const bool yIndex1InFrustrum{ currMesh.vertices_out[indexV1].position.y >= -1 && currMesh.vertices_out[indexV1].position.y <= 1 };
			//const bool yIndex2InFrustrum{ currMesh.vertices_out[indexV2].position.y >= -1 && currMesh.vertices_out[indexV2].position.y <= 1 };
			//const bool yInFrustrum{ yIndex0InFrustrum && yIndex1InFrustrum && yIndex2InFrustrum };
			//
			//// check for z pos
			//const bool zIndex0InFrustrum{ currMesh.vertices_out[indexV0].position.z >= 0 && currMesh.vertices_out[indexV0].position.z <= 1 };
			//const bool zIndex1InFrustrum{ currMesh.vertices_out[indexV1].position.z >= 0 && currMesh.vertices_out[indexV1].position.z <= 1 };
			//const bool zIndex2InFrustrum{ currMesh.vertices_out[indexV2].position.z >= 0 && currMesh.vertices_out[indexV2].position.z <= 1 };
			//const bool zInFrustrum{ zIndex0InFrustrum && zIndex1InFrustrum && zIndex2InFrustrum };


			// check to see if all positions are within the frustrum
			// store the results in seperate bools for readability
			const bool isV0InFrustrum{ CheckPositionInFrustrum(currMesh.vertices_out[indexV0].position.GetXYZ()) };
			const bool isV1InFrustrum{ CheckPositionInFrustrum(currMesh.vertices_out[indexV1].position.GetXYZ()) };
			const bool isV2InFrustrum{ CheckPositionInFrustrum(currMesh.vertices_out[indexV2].position.GetXYZ()) };
			// if it is in frustrum, code below will return false, we go through the rest of the code
			// if it isn't inside, it returns true and we continue to the next loop
			if (!(isV0InFrustrum && isV1InFrustrum && isV2InFrustrum))
				continue;

			// safe current vertices
			const Vector2 v0{ vertices_screen[indexV0].x, vertices_screen[indexV0].y };
			const Vector2 v1{ vertices_screen[indexV1].x, vertices_screen[indexV1].y };
			const Vector2 v2{ vertices_screen[indexV2].x, vertices_screen[indexV2].y };


			// edges to check using cross
			const Vector2 edge10{ v1 - v0 };
			const Vector2 edge21{ v2 - v1 };
			const Vector2 edge02{ v0 - v2 };


			const float triangleArea{ Vector2::Cross({v2 - v0}, edge10) };
			const float invTriangleArea{ 1.f / triangleArea };


			// setup bounding box
			Vector2 boundingBoxMin{ Vector2::Min(v0, Vector2::Min(v1, v2)) };
			Vector2 boundingBoxMax{ Vector2::Max(v0, Vector2::Max(v1, v2)) };
			// clamp to screensize
			// this could give a lot of if statements, easier way is to also check using Min and Max with a minVector of 0 and a screenvector containing the size
			Vector2 screenSize{ static_cast<float>(m_Width), static_cast<float>(m_Height) }; // max values of the screen
			boundingBoxMin = Vector2::Min(screenSize, Vector2::Max(boundingBoxMin, Vector2::Zero)); // this way, we will always be >= zero and <= screensize
			boundingBoxMax = Vector2::Min(screenSize, Vector2::Max(boundingBoxMax, Vector2::Zero));

			//RENDER LOGIC
			// adapt to use the boundingboxMin and max instead
			// only the pixels inside this box will be checked
			// boundingbox is modified to the min and max size of the current triangle -> much less pixels to check
			// boundingbox should always be square, so think of it as reducing the screensize for the current triangle
			for (int px{ static_cast<int>(boundingBoxMin.x) }; px < boundingBoxMax.x; ++px)
			{
				for (int py{ static_cast<int>(boundingBoxMin.y) }; py < boundingBoxMax.y; ++py)
				{
					const int pixelIndex{ px + py * m_Width };
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



					// barycentric weights
					const float weight10{ edge10CrossPixel * invTriangleArea };
					const float weight21{ edge21CrossPixel * invTriangleArea };
					const float weight02{ edge02CrossPixel * invTriangleArea };

					// depths
					const float depthV0{ currMesh.vertices_out[indexV0].position.z };
					const float depthV1{ currMesh.vertices_out[indexV1].position.z };
					const float depthV2{ currMesh.vertices_out[indexV2].position.z };

					// interpolate to get the value
					// didn't know how to do this for this step, so looked a week ahead :)
					const float interpolatedDepthValue
					{
						1.f /
						(
							weight21 * (1.f / depthV0) +
							weight02 * (1.f / depthV1) +
							weight10 * (1.f / depthV2)
						)
					};

					// final check to see if it is in frustrum
					const bool isInFrustrum{ (interpolatedDepthValue >= 0 && interpolatedDepthValue <= 1)};

					if (interpolatedDepthValue >= m_pDepthBufferPixels[pixelIndex] || !isInFrustrum )
						continue;
					// set the depthbufferpixel
					m_pDepthBufferPixels[pixelIndex] = interpolatedDepthValue;


					// view space depths
					const float viewSpaceDepthV0{ currMesh.vertices_out[indexV0].position.w };
					const float viewSpaceDepthV1{ currMesh.vertices_out[indexV1].position.w };
					const float viewSpaceDepthV2{ currMesh.vertices_out[indexV2].position.w };

					const float interpolatedViewSpaceDepthValue
					{
						1.f /
						(
							weight21 * (1.f / viewSpaceDepthV0) +
							weight02 * (1.f / viewSpaceDepthV1) +
							weight10 * (1.f / viewSpaceDepthV2)
						)
					};

					const Vector2 interpolatedUV
					{
						(
						((currMesh.vertices_out[indexV0].uv / currMesh.vertices_out[indexV0].position.w) * weight21) +
						((currMesh.vertices_out[indexV1].uv / currMesh.vertices_out[indexV1].position.w) * weight02) +
						((currMesh.vertices_out[indexV2].uv / currMesh.vertices_out[indexV2].position.w) * weight10)
						) * interpolatedViewSpaceDepthValue
					};


					ColorRGB finalColor{};
					if (m_ShowDepth == false)
					{
						finalColor = m_pTextureTukTuk->Sample(interpolatedUV);
					}
					else
					{
						finalColor = ColorRGB::Remap(interpolatedDepthValue, 0.995f, 1.f);
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
	}

}


void dae::Renderer::Render_W4_Part1()
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
	std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), 1.f);

	// Define Mesh (in world space)
	std::vector<Mesh> meshes_world
	{
		Vehicle
	};

	for (Mesh& currMesh : meshes_world) // we loop over all meshes, transform the vertices and use those
	{
		VertexTransformationFunction(currMesh);
		// get all vertices into screen space
		std::vector<Vector2> vertices_screen{};
		vertices_screen.reserve(currMesh.vertices_out.size());
		for (const auto& currVertex : currMesh.vertices_out)
		{
			vertices_screen.emplace_back(Vector2{ (currVertex.position.x + 1) * 0.5f * m_Width, (1 - currVertex.position.y) * 0.5f * m_Height });
		}


		bool useModulo{ false };
		int incrementor{ 3 };

		if (currMesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			useModulo = true;
			incrementor = 1;
			// when using triangleStrip we go down the list of indices one by one see slides W7, slide 8 - 11
		}


		for (int i{ 0 }; i < static_cast<int>(currMesh.indices.size()- 2); i += incrementor)
		{
			// to make it easier, get the indexes for the vertices first
			const uint32_t indexV0{ currMesh.indices[i] };
			// when using triangleStrip, we want to swap these if current triangle is odd (% 2 == 1)
			const int moduloResult{ useModulo * (i % 2) }; // modulo can be heavy, calculate it once instead of twice
			const uint32_t indexV1{ currMesh.indices[i + 1 + moduloResult]}; // if triangle is odd, we do index = i + 1 + (1* 1)
			const uint32_t indexV2{ currMesh.indices[i + 2 - moduloResult]}; // if triangle is odd, we do index = i + 2 - (1* 1)

			// check if there are multiple of the same indexes, use early out, these are buffers
			if (indexV0 == indexV1 || indexV0 == indexV2 || indexV1 == indexV2)
				continue;
			

			// check to see if all positions are within the frustrum
			// store the results in seperate bools for readability
			const bool isV0InFrustrum{ CheckPositionInFrustrum(currMesh.vertices_out[indexV0].position.GetXYZ()) };
			const bool isV1InFrustrum{ CheckPositionInFrustrum(currMesh.vertices_out[indexV1].position.GetXYZ()) };
			const bool isV2InFrustrum{ CheckPositionInFrustrum(currMesh.vertices_out[indexV2].position.GetXYZ()) };
			// if it is in frustrum, code below will return false, we go through the rest of the code
			// if it isn't inside, it returns true and we continue to the next loop
			if (!(isV0InFrustrum && isV1InFrustrum && isV2InFrustrum))
				continue;

			// safe current vertices
			const Vector2 v0{ vertices_screen[indexV0].x, vertices_screen[indexV0].y };
			const Vector2 v1{ vertices_screen[indexV1].x, vertices_screen[indexV1].y };
			const Vector2 v2{ vertices_screen[indexV2].x, vertices_screen[indexV2].y };


			// edges to check using cross
			const Vector2 edge10{ v1 - v0 };
			const Vector2 edge21{ v2 - v1 };
			const Vector2 edge02{ v0 - v2 };


			const float triangleArea{ Vector2::Cross({v2 - v0}, edge10) };
			const float invTriangleArea{ 1.f / triangleArea };


			// setup bounding box
			Vector2 boundingBoxMin{ Vector2::Min(v0, Vector2::Min(v1, v2)) };
			Vector2 boundingBoxMax{ Vector2::Max(v0, Vector2::Max(v1, v2)) };
			// clamp to screensize
			// this could give a lot of if statements, easier way is to also check using Min and Max with a minVector of 0 and a screenvector containing the size
			Vector2 screenSize{ static_cast<float>(m_Width), static_cast<float>(m_Height) }; // max values of the screen
			boundingBoxMin = Vector2::Min(screenSize, Vector2::Max(boundingBoxMin, Vector2::Zero)); // this way, we will always be >= zero and <= screensize
			boundingBoxMax = Vector2::Min(screenSize, Vector2::Max(boundingBoxMax, Vector2::Zero));

			//RENDER LOGIC
			// adapt to use the boundingboxMin and max instead
			// only the pixels inside this box will be checked
			// boundingbox is modified to the min and max size of the current triangle -> much less pixels to check
			// boundingbox should always be square, so think of it as reducing the screensize for the current triangle
			for (int px{ static_cast<int>(boundingBoxMin.x) }; px < boundingBoxMax.x; ++px)
			{
				for (int py{ static_cast<int>(boundingBoxMin.y) }; py < boundingBoxMax.y; ++py)
				{
					const int pixelIndex{ px + py * m_Width };
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



					// barycentric weights
					const float weight10{ edge10CrossPixel * invTriangleArea };
					const float weight21{ edge21CrossPixel * invTriangleArea };
					const float weight02{ edge02CrossPixel * invTriangleArea };

					// depths
					const float depthV0{ currMesh.vertices_out[indexV0].position.z };
					const float depthV1{ currMesh.vertices_out[indexV1].position.z };
					const float depthV2{ currMesh.vertices_out[indexV2].position.z };

					// interpolate to get the value
					// didn't know how to do this for this step, so looked a week ahead :)
					const float interpolatedDepthValue
					{
						1.f /
						(
							weight21 * (1.f / depthV0) +
							weight02 * (1.f / depthV1) +
							weight10 * (1.f / depthV2)
						)
					};

					// final check to see if it is in frustrum
					const bool isInFrustrum{ (interpolatedDepthValue >= 0 && interpolatedDepthValue <= 1)};

					if (interpolatedDepthValue >= m_pDepthBufferPixels[pixelIndex] || !isInFrustrum )
						continue;
					// set the depthbufferpixel
					m_pDepthBufferPixels[pixelIndex] = interpolatedDepthValue;


					// view space depths
					const float viewSpaceDepthV0Inv{ 1.f / currMesh.vertices_out[indexV0].position.w };
					const float viewSpaceDepthV1Inv{ 1.f / currMesh.vertices_out[indexV1].position.w };
					const float viewSpaceDepthV2Inv{ 1.f / currMesh.vertices_out[indexV2].position.w };

					const float interpolatedViewSpaceDepthValue
					{
						1.f /
						(
							weight21 * viewSpaceDepthV0Inv +
							weight02 * viewSpaceDepthV1Inv +
							weight10 * viewSpaceDepthV2Inv
						)
					};

					//const Vector2 interpolatedUV
					//{
					//	(
					//	((currMesh.vertices_out[indexV0].uv / currMesh.vertices_out[indexV0].position.w) * weight21) +
					//	((currMesh.vertices_out[indexV1].uv / currMesh.vertices_out[indexV1].position.w) * weight02) +
					//	((currMesh.vertices_out[indexV2].uv / currMesh.vertices_out[indexV2].position.w) * weight10)
					//	) * interpolatedViewSpaceDepthValue
					//};

					const Vector2 interpolatedUV{ InterpolateAttribute(currMesh.vertices_out[indexV0].uv, currMesh.vertices_out[indexV1].uv, currMesh.vertices_out[indexV2].uv, viewSpaceDepthV0Inv, viewSpaceDepthV1Inv, viewSpaceDepthV2Inv, weight21, weight02, weight10, interpolatedViewSpaceDepthValue) };

					ColorRGB finalColor{};
					if (m_ShowDepth == false)
					{
						//finalColor = m_pTextureVehicleDiffuse->Sample(interpolatedUV);
						const Vector2 interpolatedXYPos
						{
							weight21 * currMesh.vertices_out[indexV0].position.GetXY() +
							weight02 * currMesh.vertices_out[indexV1].position.GetXY() +
							weight10 * currMesh.vertices_out[indexV2].position.GetXY()
						};

						//const ColorRGB interpolatedColor
						//{
						//	(
						//	((currMesh.vertices_out[indexV0].color / currMesh.vertices_out[indexV0].position.w) * weight21) +
						//	((currMesh.vertices_out[indexV1].color / currMesh.vertices_out[indexV1].position.w) * weight02) +
						//	((currMesh.vertices_out[indexV2].color / currMesh.vertices_out[indexV2].position.w) * weight10)
						//	)* interpolatedViewSpaceDepthValue
						//};

						const ColorRGB interpolatedColor{ 
							InterpolateAttribute(currMesh.vertices_out[indexV0].color, currMesh.vertices_out[indexV1].color, currMesh.vertices_out[indexV2].color, viewSpaceDepthV0Inv, viewSpaceDepthV1Inv, viewSpaceDepthV2Inv, weight21, weight02, weight10, interpolatedViewSpaceDepthValue) };


						//const Vector3 interpolatedNormal
						//{
						//	((
						//	((currMesh.vertices_out[indexV0].normal / currMesh.vertices_out[indexV0].position.w) * weight21) +
						//	((currMesh.vertices_out[indexV1].normal / currMesh.vertices_out[indexV1].position.w) * weight02) +
						//	((currMesh.vertices_out[indexV2].normal / currMesh.vertices_out[indexV2].position.w) * weight10)
						//	)* interpolatedViewSpaceDepthValue).Normalized()
						//
						//};

						const Vector3 interpolatedNormal{ InterpolateAttribute(currMesh.vertices_out[indexV0].normal, currMesh.vertices_out[indexV1].normal, currMesh.vertices_out[indexV2].normal, viewSpaceDepthV0Inv, viewSpaceDepthV1Inv, viewSpaceDepthV2Inv, weight21, weight02, weight10, interpolatedViewSpaceDepthValue).Normalized()};

						//const Vector3 interpolatedTangent
						//{
						//	((
						//	((currMesh.vertices_out[indexV0].tangent / currMesh.vertices_out[indexV0].position.w) * weight21) +
						//	((currMesh.vertices_out[indexV1].tangent / currMesh.vertices_out[indexV1].position.w) * weight02) +
						//	((currMesh.vertices_out[indexV2].tangent / currMesh.vertices_out[indexV2].position.w) * weight10)
						//	)* interpolatedViewSpaceDepthValue).Normalized()
						//};

						const Vector3 interpolatedTangent{ InterpolateAttribute(currMesh.vertices_out[indexV0].tangent, currMesh.vertices_out[indexV1].tangent, currMesh.vertices_out[indexV2].tangent, viewSpaceDepthV0Inv, viewSpaceDepthV1Inv, viewSpaceDepthV2Inv, weight21, weight02, weight10, interpolatedViewSpaceDepthValue).Normalized()};

						//const Vector3 interpolatedViewDirection
						//{
						//	((
						//	((currMesh.vertices_out[indexV0].viewDirection / currMesh.vertices_out[indexV0].position.w) * weight21) +
						//	((currMesh.vertices_out[indexV1].viewDirection / currMesh.vertices_out[indexV1].position.w) * weight02) +
						//	((currMesh.vertices_out[indexV2].viewDirection / currMesh.vertices_out[indexV2].position.w) * weight10)
						//	)* interpolatedViewSpaceDepthValue).Normalized()
						//};

						const Vector3 interpolatedViewDirection{ InterpolateAttribute(currMesh.vertices_out[indexV0].viewDirection, currMesh.vertices_out[indexV1].viewDirection, currMesh.vertices_out[indexV2].viewDirection, viewSpaceDepthV0Inv, viewSpaceDepthV1Inv, viewSpaceDepthV2Inv, weight21, weight02, weight10, interpolatedViewSpaceDepthValue).Normalized()};


						Vertex_Out shadingInfo{
							Vector4{interpolatedXYPos.x, interpolatedXYPos.y, interpolatedDepthValue, interpolatedViewSpaceDepthValue},
							interpolatedColor,
							interpolatedUV,
							interpolatedNormal,
							interpolatedTangent,
							interpolatedViewDirection};

						finalColor = PixelShading(shadingInfo);
					}
					else
					{
						finalColor = ColorRGB::Remap(interpolatedDepthValue, 0.997f, 1.f);
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
	}

}

ColorRGB dae::Renderer::PixelShading(const Vertex_Out& v)
{
	// LAMBERT info
	const float kd{ 1.f };
	const float ks{ 1.f };


	// Calculate Normal
	Vector3 sampledNormal{ v.normal }; // when not showing normals, keep this -> don't change anything that uses this

	if (m_DisplayNormalMapping)
	{
		const Vector3 binormal{ Vector3::Cross(v.normal, v.tangent).Normalized()};
		const Matrix tangentSpaceAxis{ v.tangent, binormal, v.normal, Vector4{0,0,0,0} };
		const ColorRGB normalSampleColor{ m_pTextureVehicleNormal->Sample(v.uv) };
		sampledNormal = Vector3{ normalSampleColor.r, normalSampleColor.g, normalSampleColor.b };
		sampledNormal = 2.f * sampledNormal - Vector3{ 1,1,1 };
		sampledNormal = tangentSpaceAxis.TransformVector(sampledNormal).Normalized();
	}
	
	// Calculate OBSERVED AREA
	const float observedAreaValue{ std::max(Vector3::Dot(sampledNormal, -m_LightDirection), 0.0f) };
	const ColorRGB observedArea{ observedAreaValue, observedAreaValue, observedAreaValue };

	

	



	
	


	switch (m_CurrentRenderMode)
	{
	case dae::Renderer::RenderMode::ObservedArea:
		return { observedArea }; // OA only
		break;
	case dae::Renderer::RenderMode::Diffuse:
	{
		const ColorRGB diffuse{ BRDF::Lambert(kd, m_pTextureVehicleDiffuse->Sample(v.uv))};
		return { diffuse * m_LightIntensity * observedArea }; // Diffuse 
	}
		break;
	case dae::Renderer::RenderMode::Specular:	// sample Specular and		 Exponent -> greyscale map, pick whatever value...
	{
		const ColorRGB specularColor{ m_pTextureVehicleSpecular->Sample(v.uv) };
		const float exponent{ m_pTextureVehicleGloss->Sample(v.uv).r * m_Shininess };
		
		const ColorRGB specular
		{ 
			BRDF::Phong(specularColor, ks, exponent, m_LightDirection, -v.viewDirection, sampledNormal)
		};

		return { specular * observedArea }; 
	}
		break;
	case dae::Renderer::RenderMode::Combined:
	{
		const ColorRGB diffuse{ BRDF::Lambert(kd, m_pTextureVehicleDiffuse->Sample(v.uv)) };
		
		const ColorRGB specularColor{ m_pTextureVehicleSpecular->Sample(v.uv) };
		const float exponent{ m_pTextureVehicleGloss->Sample(v.uv).r * m_Shininess };

		const ColorRGB specular
		{
			BRDF::Phong(specularColor, ks, exponent, m_LightDirection, -v.viewDirection, sampledNormal)
		};

		return { (diffuse * m_LightIntensity + specular + m_Ambient) * observedArea };
	}
		break;
	}

	return colors::Black;

}



bool dae::Renderer::CheckPositionInFrustrum(const Vector3& position)
{
	const float maxXYZ{ 1.f };
	const float minXY{ -1.f };
	const float minZ{ 0.f };

	return (position.x >= minXY && position.x <= maxXYZ) && (position.y >= minXY && position.y <= maxXYZ) && (position.z >= minZ && position.z <= maxXYZ);
}

//auto dae::Renderer::InterPolateAttribute(const auto& value1, const auto& value2, const auto& value3, float divisionValueInv1, float divisionValueInv2, float divisionValueInv3, float weight1, float weight2, float weight3)
//{
//
//
//}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::CycleRenderMode()
{
	m_CurrentRenderMode = RenderMode((static_cast<int>(m_CurrentRenderMode) + 1) % 4);
}
