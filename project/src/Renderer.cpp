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
	// Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// RENDER LOGIC

	for (int index{}; index < m_Width * m_Height; ++index)
	{
		// Clear the DEPTH BUFFER
		m_pDepthBufferPixels[index] = FLT_MAX;

		// Clear Back Buffer
		m_pBackBufferPixels[index] = SDL_MapRGB(m_pBackBuffer->format,
			static_cast<uint8_t>(100),
			static_cast<uint8_t>(100),
			static_cast<uint8_t>(100));

	}

	// Convert from WorldSpace to ScreenSpace
	std::vector<Vertex> vertices_NDC;
	VertexFromWorldToNDCProjected(m_triangleVertices, vertices_NDC);
	std::vector<Vertex> vertices_screenspace;
	VertexFromNDCToScreenSpace(vertices_NDC, vertices_screenspace);

	// For every triangle
	for (int triangleIndex{}; triangleIndex < m_triangleVertices.size() / 3; ++triangleIndex)
	{
		// Define triangle
		std::vector<Vertex> triangle_screenVertices
		{
			vertices_screenspace[3 * triangleIndex	  ],
			vertices_screenspace[3 * triangleIndex + 1],
			vertices_screenspace[3 * triangleIndex + 2]
		};
		std::vector<Vector2> triangle_screenEdges
		{
			triangle_screenVertices[1].position.GetXY() - triangle_screenVertices[0].position.GetXY(),
			triangle_screenVertices[2].position.GetXY() - triangle_screenVertices[1].position.GetXY(),
			triangle_screenVertices[0].position.GetXY() - triangle_screenVertices[2].position.GetXY()
		};


		Vector4 boundingBox =
		{
			(float)std::clamp(int(std::min(triangle_screenVertices[0].position.x, std::min(triangle_screenVertices[1].position.x, triangle_screenVertices[2].position.x))), 0, m_Width  - 1),
			(float)std::clamp(int(std::min(triangle_screenVertices[0].position.y, std::min(triangle_screenVertices[1].position.y, triangle_screenVertices[2].position.y))), 0, m_Height - 1),
			(float)std::clamp(int(std::max(triangle_screenVertices[0].position.x, std::max(triangle_screenVertices[1].position.x, triangle_screenVertices[2].position.x))), 0, m_Width  - 1),
			(float)std::clamp(int(std::max(triangle_screenVertices[0].position.y, std::max(triangle_screenVertices[1].position.y, triangle_screenVertices[2].position.y))), 0, m_Height - 1)
		};


		// Calculate (double) the area of the triangle. Also known as the magnitude of the cross product of 
		// 2 of the triangle edges. (here it doesn't matter we calculate double the are, because we'll need
		// the ratio anyway)
		float parellelogramArea = Vector2::Cross(triangle_screenEdges[0], triangle_screenEdges[1]);

		// For every pixel
		for (int px{ m_UseBoundingBoxes ? (int)boundingBox.x : 0}; px < (m_UseBoundingBoxes ? (int)boundingBox.z : m_Width); ++px)
		{
			for (int py{ m_UseBoundingBoxes ? (int)boundingBox.y : 0 }; py < (m_UseBoundingBoxes ? (int)boundingBox.w : m_Height); ++py)
			{
				// Declare finalColor of the pixel
				ColorRGB finalColor;

				// Declare depth of this pixel
				float pixelDepth{0};

				// Go over every edge of the triangle to see if the pixel lies within the triangle
				for (int index{}; index < 3; ++index)
				{
					// Define the vector from the current vertex to the center of the pixel
					Vector2 p = Vector2(px + 0.5f, py + 0.5f) - triangle_screenVertices[index].position.GetXY();

					// Get the vector of the current index' edge
					Vector2 e = triangle_screenEdges[index];

					// The cross product of two Vector2's will return the magnitude of the area they span.
					// (the magnitude of the vector perpindicular to bot Vector2's)
					float eXpArea = Vector2::Cross(e, p);

					// Calculate the weight of the vertex NOT included in the calculations.
					// e.g. if e = V0-V1 and p = V0-P then this weight would be the weight on V2
					float weight = eXpArea / parellelogramArea;

					// If our calculate weight is not within a 0-1 range, we already know our point is not within the triangle
					if (weight < 0 or weight > 1)
					{
						// Point not in trianlge, we set final color to black and break the loop to continue to the next pixel
						finalColor = colors::Black;
						pixelDepth = FLT_MAX;
						break;
					}
					// if we are in the triangle, add the weighted color to the final color and calculate the depth
					// of the pixel. The weight we calculated is of the vertex not included in the calculations, 
					// since we use index and index + 1, index + 2 will be left over, so that's
					// the vertex index beloning to the calculated weight
					else
					{
						finalColor += triangle_screenVertices[(index + 2) % 3].color * weight;
						pixelDepth += triangle_screenVertices[(index + 2) % 3].position.z * weight;
					}
				}

				// Now we check if our pixelDepth is closer to what was previously in the DepthBuffer, if it is
				// we overwrite the Depth buffer
				if (pixelDepth < m_pDepthBufferPixels[py + m_Height * px] and pixelDepth > 0)
				{
					m_pDepthBufferPixels[py + m_Height * px] = pixelDepth;

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

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexFromWorldToNDCProjected(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.resize(vertices_in.size());

	for (int index{}; index < vertices_in.size(); ++index)
	{
		Vector3 vertexInCameraSpace		= { m_Camera.invViewMatrix.TransformPoint(vertices_in[index].position) };

		Vector2 vertexInProjectedSpace	= { vertexInCameraSpace.x / vertexInCameraSpace.z,
											vertexInCameraSpace.y / vertexInCameraSpace.z};

		Vector2 vertexInNDC				= { vertexInProjectedSpace.x / (m_Camera.fov * m_AspectRatio),
											vertexInProjectedSpace.y / (m_Camera.fov) };

		vertices_out[index].position	= { vertexInNDC.x, vertexInNDC.y, vertexInCameraSpace.z };
		vertices_out[index].color		= vertices_in[index].color;
	}
}
void dae::Renderer::VertexFromNDCToScreenSpace(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.resize(vertices_in.size());

	for (int index{}; index < vertices_in.size(); ++index)
	{
		Vector3 vertexInScreenSpace		= { (vertices_in[index].position.x + 1) / 2 * m_Width,
											(1 - vertices_in[index].position.y) / 2 * m_Height,
											vertices_in[index].position.z };

		vertices_out[index].position	= vertexInScreenSpace;
		vertices_out[index].color		= vertices_in[index].color;
	}
}

void dae::Renderer::ToggleUseBoundingBoxes()
{
	m_UseBoundingBoxes = !m_UseBoundingBoxes;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
