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

	// Initialize Mesh with TriangleStrip
	m_MeshesWorld.push_back(
		Mesh
		{
			// Vertices
			{
				Vertex{ { -3 - 4,  3, -2}, colors::White, { 0.0f, 0.0f } },
				Vertex{ {  0 - 4,  3, -2}, colors::White, { 0.5f, 0.0f } },
				Vertex{ {  3 - 4,  3, -2}, colors::White, { 1.0f, 0.0f } },
				Vertex{ { -3 - 4,  0, -2}, colors::White, { 0.0f, 0.5f } },
				Vertex{ {  0 - 4,  0, -2}, colors::White, { 0.5f, 0.5f } },
				Vertex{ {  3 - 4,  0, -2}, colors::White, { 1.0f, 0.5f } },
				Vertex{ { -3 - 4, -3, -2}, colors::White, { 0.0f, 1.0f } },
				Vertex{ {  0 - 4, -3, -2}, colors::White, { 0.5f, 1.0f } },
				Vertex{ {  3 - 4, -3, -2}, colors::White, { 1.0f, 1.0f } }
			},
			// Indices
			{
				3, 0, 4, 1, 5, 2,
				2, 6,
				6, 3, 7, 4, 8, 5
			},
			// Topology
			PrimitiveTopology::TriangleStrip
		}
	);

	// Initialize Mesh with TriangleList
	m_MeshesWorld.push_back(
		Mesh
		{
			// Vertices
			{
				Vertex{ { -3 + 4,  3, -2}, colors::White, { 0.0f, 0.0f } },
				Vertex{ {  0 + 4,  3, -2}, colors::White, { 0.5f, 0.0f } },
				Vertex{ {  3 + 4,  3, -2}, colors::White, { 1.0f, 0.0f } },
				Vertex{ { -3 + 4,  0, -2}, colors::White, { 0.0f, 0.5f } },
				Vertex{ {  0 + 4,  0, -2}, colors::White, { 0.5f, 0.5f } },
				Vertex{ {  3 + 4,  0, -2}, colors::White, { 1.0f, 0.5f } },
				Vertex{ { -3 + 4, -3, -2}, colors::White, { 0.0f, 1.0f } },
				Vertex{ {  0 + 4, -3, -2}, colors::White, { 0.5f, 1.0f } },
				Vertex{ {  3 + 4, -3, -2}, colors::White, { 1.0f, 1.0f } }
			},
			// Indices
			{
				3, 0, 1,	1, 4, 3,	4, 1, 2, 
				2, 5, 4,	6, 3, 4,	4, 7, 6, 
				7, 4, 5,	5, 8, 7
			},
			// Topology
			PrimitiveTopology::TriangleList
		}
	);

	// Load a texture
	m_upTexture.reset(Texture::LoadFromFile("resources/uv_grid_2.png"));
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

	// For every triangle mesh
	for (int triangleMeshIndex{}; triangleMeshIndex < m_MeshesWorld.size(); ++triangleMeshIndex)
	{
		int indexJump = 0;
		int triangleCount = 0;
		bool triangleStripMethod = false;

		// Determine the triangle count and index jump depending on the PrimitiveTopology
		if (m_MeshesWorld[triangleMeshIndex].primitiveTopology == PrimitiveTopology::TriangleList)
		{
			indexJump = 3;
			triangleCount = m_MeshesWorld[triangleMeshIndex].indices.size() / 3;
			triangleStripMethod = false;
		}
		else if (m_MeshesWorld[triangleMeshIndex].primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			indexJump = 1;
			triangleCount = m_MeshesWorld[triangleMeshIndex].indices.size() - 2;
			triangleStripMethod = true;
		}


		// Convert positions from WorldSpace to ScreenSpace
		std::vector<Vertex> vertices_NDC;
		WorldVertexToProjectedNDC(m_MeshesWorld[triangleMeshIndex].vertices, vertices_NDC);
		std::vector<Vertex> vertices_rasterized;
		NDCVertexToRasterSpace(vertices_NDC, vertices_rasterized);


		// Loop over all the triangles
		for (int triangleIndex{}; triangleIndex < triangleCount; ++triangleIndex)
		{
			int indexPos0 = m_MeshesWorld[triangleMeshIndex].indices[indexJump * triangleIndex + 0];
			int indexPos1 = m_MeshesWorld[triangleMeshIndex].indices[indexJump * triangleIndex + 1];
			int indexPos2 = m_MeshesWorld[triangleMeshIndex].indices[indexJump * triangleIndex + 2];
			// If the triangle strip method is in use, swap the indices of odd indexed triangles
			if (triangleStripMethod and (triangleIndex & 1)) std::swap(indexPos1, indexPos2);

			// Define triangle
			std::vector<Vertex> triangle_rasterVertices
			{
				vertices_rasterized[indexPos0],
				vertices_rasterized[indexPos1],
				vertices_rasterized[indexPos2]
			};

			// Define the triangle's bounding box
			Vector2 min = { FLT_MAX, FLT_MAX };
			Vector2 max = { -FLT_MAX, -FLT_MAX };
			{
				// Mins
				min = Vector2::Min(min, triangle_rasterVertices[0].position.GetXY());
				min = Vector2::Min(min, triangle_rasterVertices[1].position.GetXY());
				min = Vector2::Min(min, triangle_rasterVertices[2].position.GetXY());
				min.x = std::clamp(std::round(min.x), 0.f, m_Width  - 1.f);
				min.y = std::clamp(std::round(min.y), 0.f, m_Height - 1.f);
				
				// Maxs
				max = Vector2::Max(max, triangle_rasterVertices[0].position.GetXY());
				max = Vector2::Max(max, triangle_rasterVertices[1].position.GetXY());
				max = Vector2::Max(max, triangle_rasterVertices[2].position.GetXY());
				max.x = std::clamp(std::round(max.x), 0.f, m_Width  - 1.f);
				max.y = std::clamp(std::round(max.y), 0.f, m_Height - 1.f);
			}


			// For every pixel (within the bounding box)
			for (int py{ int(min.y) }; py < int(max.y); ++py)
			{
				for (int px{ int(min.x) }; px < int(max.x); ++px)
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
						// Correctly interpolated depth
						float Z0 = triangle_rasterVertices[0].position.z;
						float Z1 = triangle_rasterVertices[1].position.z;
						float Z2 = triangle_rasterVertices[2].position.z;

						pixelDepth = InterpolateDepth(Z0, Z1, Z2, barycentricCoords);
						// If we have a negative depth, this point is behind the camera and we can go to the next point
						if (pixelDepth < 0) continue;

						// Correctly interpolated color
						ColorRGB C0 = triangle_rasterVertices[0].color;
						ColorRGB C1 = triangle_rasterVertices[1].color;
						ColorRGB C2 = triangle_rasterVertices[2].color;

						finalColor += InterpolateAttribute(C0, C1, C2, Z0, Z1, Z2, pixelDepth, barycentricCoords);
						
						// Correctly interpolated uv
						Vector2 UV0 = triangle_rasterVertices[0].uv;
						Vector2 UV1 = triangle_rasterVertices[1].uv;
						Vector2 UV2 = triangle_rasterVertices[2].uv;

						Vector2 finalUV = InterpolateAttribute(UV0, UV1, UV2, Z0, Z1, Z2, pixelDepth, barycentricCoords);
						finalColor = m_upTexture->Sample(finalUV);
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
		 
		Vector2 vertexInProjectedSpace	= { vertexInCameraSpace.x / vertexInCameraSpace.z,
											vertexInCameraSpace.y / vertexInCameraSpace.z};

		Vector2 vertexInNDC				= { vertexInProjectedSpace.x / (m_Camera.fov * m_AspectRatio),
											vertexInProjectedSpace.y / (m_Camera.fov) };

		vertices_out[index]				= vertices_in[index];
		vertices_out[index].position	= { vertexInNDC.x, vertexInNDC.y, vertexInCameraSpace.z };
	}
}
void dae::Renderer::NDCVertexToRasterSpace(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.resize(vertices_in.size());

	for (int index{}; index < vertices_in.size(); ++index)
	{
		Vector3 vertexInRasterSpace		= { (vertices_in[index].position.x + 1.f) / 2.f * m_Width,
											(1.f - vertices_in[index].position.y) / 2.f * m_Height,
											vertices_in[index].position.z };

		vertices_out[index]				= vertices_in[index];
		vertices_out[index].position	= vertexInRasterSpace;
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
