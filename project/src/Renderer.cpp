//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	// Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_AspectRatio = float(m_Width) / m_Height;

	// Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	// Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	// Initialize Triangles
	m_triangleVertices.insert(m_triangleVertices.end(), 
		{	// TRIANGLE 0
			{ { 0.0f,  2.0f, 0.0f}, {1.f, 0.f, 0.f} },
			{ { 1.5f, -1.0f, 0.0f}, {1.f, 0.f, 0.f} },
			{ {-1.5f, -1.0f, 0.0f}, {1.f, 0.f, 0.f} },

			// TRIANGLE 1
			{ { 0.0f,  4.0f, 2.0f}, {1.f, 0.f, 0.f} },
			{ { 3.0f, -2.0f, 2.0f}, {0.f, 1.f, 0.f} },
			{ {-3.0f, -2.0f, 2.0f}, {0.f, 0.f, 1.f} }
		});
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	// @START
	SDL_FillRect(m_pBackBuffer, NULL, 0x646464);
	for (int index{}; index < m_Width * m_Height; ++index)
	{
		// Clear the DEPTH BUFFER
		m_pDepthBufferPixels[index] = FLT_MAX;
	}
	
	// Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);


	// RENDER LOGIC

	// Convert from WorldSpace to ScreenSpace
	std::vector<Vertex> vertices_NDC;
	WorldVertexToProjectedNDC(m_triangleVertices, vertices_NDC);
	std::vector<Vertex> vertices_rasterized;
	NDCVertexToRasterSpace(vertices_NDC, vertices_rasterized);

	// For every triangle
	for (int triangleIndex{}; triangleIndex < m_triangleVertices.size() / 3; ++triangleIndex)
	{
		// Define triangle
		std::vector<Vertex> triangle_rasterVertices
		{
			vertices_rasterized[3 * triangleIndex	 ],
			vertices_rasterized[3 * triangleIndex + 1],
			vertices_rasterized[3 * triangleIndex + 2]
		};
		Vector4 boundingBox =
		{
			(float)std::clamp(int(std::min(triangle_rasterVertices[0].position.x, std::min(triangle_rasterVertices[1].position.x, triangle_rasterVertices[2].position.x))), 0, m_Width  - 1),
			(float)std::clamp(int(std::min(triangle_rasterVertices[0].position.y, std::min(triangle_rasterVertices[1].position.y, triangle_rasterVertices[2].position.y))), 0, m_Height - 1),
			(float)std::clamp(int(std::max(triangle_rasterVertices[0].position.x, std::max(triangle_rasterVertices[1].position.x, triangle_rasterVertices[2].position.x))), 0, m_Width  - 1),
			(float)std::clamp(int(std::max(triangle_rasterVertices[0].position.y, std::max(triangle_rasterVertices[1].position.y, triangle_rasterVertices[2].position.y))), 0, m_Height - 1)
		};


		// For every pixel (withing the bounding box)
		for (int px{ (int)boundingBox.x }; px < (int)boundingBox.z; ++px)
		{
			for (int py{ (int)boundingBox.y }; py < (int)boundingBox.w; ++py)
			{
				// Declare finalColor of the pixel
				ColorRGB finalColor{};
				// Declare depth of this pixel
				float pixelDepth{ FLT_MAX };

				// Calculate the barycentric coordinates of that pixel in relationship to the triangle
				Vector2 pixelCoord = Vector2(px + 0.5f, py + 0.5f);
				Vector3 barycentricCoords = CalculateBarycentricCoordinates(
						triangle_rasterVertices[0].position.GetXY(),
						triangle_rasterVertices[1].position.GetXY(),
						triangle_rasterVertices[2].position.GetXY(),
						pixelCoord);

				// Check if our barycentric coordinates are valid, if so, calculate the color and depth
				if (AreBarycentricValid(barycentricCoords))
				{
					finalColor += barycentricCoords.x * triangle_rasterVertices[0].color
							    + barycentricCoords.y * triangle_rasterVertices[1].color
							    + barycentricCoords.z * triangle_rasterVertices[2].color;
					
					pixelDepth = barycentricCoords.x * triangle_rasterVertices[0].position.z
							   + barycentricCoords.y * triangle_rasterVertices[1].position.z
							   + barycentricCoords.z * triangle_rasterVertices[2].position.z;
				}


				// Now we check if our pixelDepth is closer to what was previously in the DepthBuffer, if it is
				// we overwrite the Depth buffer
				if (pixelDepth < m_pDepthBufferPixels[m_Width * py + px] and pixelDepth > FLT_EPSILON)
				{
					m_pDepthBufferPixels[m_Width * py + px] = pixelDepth;

					//Update Color in Buffer
					finalColor.MaxToOne();

					m_pBackBufferPixels[m_Width * py + px] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::WorldVertexToProjectedNDC(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.resize(vertices_in.size());

	for (int index{}; index < vertices_in.size(); ++index)
	{
		Vector3 vertexInCameraSpace		= { m_Camera.worldToCamera.TransformPoint(vertices_in[index].position) };

		Vector2 vertexInProjectedSpace	= { vertexInCameraSpace.x / abs(vertexInCameraSpace.z),
											vertexInCameraSpace.y / abs(vertexInCameraSpace.z)};

		Vector2 vertexInNDC				= { vertexInProjectedSpace.x / (m_Camera.fov * m_AspectRatio),
											vertexInProjectedSpace.y / (m_Camera.fov) };

		vertices_out[index].position	= { vertexInNDC.x, vertexInNDC.y, vertexInCameraSpace.z };
		vertices_out[index].color		= vertices_in[index].color;
	}
}
void dae::Renderer::NDCVertexToRasterSpace(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.resize(vertices_in.size());

	for (int index{}; index < vertices_in.size(); ++index)
	{
		Vector3 vertexInRasterSpace		= { (vertices_in[index].position.x + 1) / 2 * m_Width,
											(1 - vertices_in[index].position.y) / 2 * m_Height,
											vertices_in[index].position.z };

		vertices_out[index].position	= vertexInRasterSpace;
		vertices_out[index].color		= vertices_in[index].color;
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
