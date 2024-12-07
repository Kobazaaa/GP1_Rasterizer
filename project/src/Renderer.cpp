//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

//todo remove
#include <execution>

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
	m_Camera.Initialize(45.f, { 0.f, 5.f, -64.f }, m_AspectRatio);

	m_MeshesWorld.resize(1);
	//Utils::ParseOBJ("resources/tuktuk2.obj", m_MeshesWorld[0].vertices, m_MeshesWorld[0].indices);
	Utils::ParseOBJ("resources/vehicle.obj", m_MeshesWorld[0].vertices, m_MeshesWorld[0].indices, m_MeshesWorld[0].vertexCounter);
	m_MeshesWorld[0].primitiveTopology = PrimitiveTopology::TriangleList;
	//m_MeshesWorld[0].worldMatrix = Matrix::CreateTranslation({0.f, 0.f, 50.f});

	//// Initialize Mesh with TriangleStrip
	//m_MeshesWorld.push_back(
	//	Mesh
	//	{
	//		// Vertices
	//		{
	//			Vertex{ { -3 - 4,  3, -4}, colors::White, { 0.0f, 0.0f } },
	//			Vertex{ {  0 - 4,  3, -4}, colors::White, { 0.5f, 0.0f } },
	//			Vertex{ {  3 - 4,  3, -4}, colors::White, { 1.0f, 0.0f } },
	//			Vertex{ { -3 - 4,  0, -4}, colors::White, { 0.0f, 0.5f } },
	//			Vertex{ {  0 - 4,  0, -4}, colors::White, { 0.5f, 0.5f } },
	//			Vertex{ {  3 - 4,  0, -4}, colors::White, { 1.0f, 0.5f } },
	//			Vertex{ { -3 - 4, -3, -4}, colors::White, { 0.0f, 1.0f } },
	//			Vertex{ {  0 - 4, -3, -4}, colors::White, { 0.5f, 1.0f } },
	//			Vertex{ {  3 - 4, -3, -4}, colors::White, { 1.0f, 1.0f } }
	//		},
	//		// Indices
	//		{
	//			3, 0, 4, 1, 5, 2,
	//			2, 6,
	//			6, 3, 7, 4, 8, 5
	//		},
	//		// Topology
	//		PrimitiveTopology::TriangleStrip
	//	}
	//);
	//
	//// Initialize Mesh with TriangleList
	//m_MeshesWorld.push_back(
	//	Mesh
	//	{
	//		// Vertices
	//		{
	//			Vertex{ { -3 + 4,  3, -2}, colors::Green, { 0.0f, 0.0f } },
	//			Vertex{ {  0 + 4,  3, -2}, colors::Blue, { 0.5f, 0.0f } },
	//			Vertex{ {  3 + 4,  3, -2}, colors::White, { 1.0f, 0.0f } },
	//			Vertex{ { -3 + 4,  0, -2}, colors::Red, { 0.0f, 0.5f } },
	//			Vertex{ {  0 + 4,  0, -2}, colors::White, { 0.5f, 0.5f } },
	//			Vertex{ {  3 + 4,  0, -2}, colors::White, { 1.0f, 0.5f } },
	//			Vertex{ { -3 + 4, -3, -2}, colors::White, { 0.0f, 1.0f } },
	//			Vertex{ {  0 + 4, -3, -2}, colors::White, { 0.5f, 1.0f } },
	//			Vertex{ {  3 + 4, -3, -2}, colors::White, { 1.0f, 1.0f } }
	//		},
	//		// Indices
	//		{
	//			3, 0, 1,	1, 4, 3,	4, 1, 2, 
	//			2, 5, 4,	6, 3, 4,	4, 7, 6, 
	//			7, 4, 5,	5, 8, 7
	//		},
	//		// Topology
	//		PrimitiveTopology::TriangleList
	//	}
	//);

	// Load a texture
	//m_upDiffuseTxt.reset(Texture::LoadFromFile("resources/uv_grid_2.png"));
	//m_upDiffuseTxt.reset(Texture::LoadFromFile("resources/tuktuk.png"));
	m_upDiffuseTxt.reset(Texture::LoadFromFile("resources/vehicle_diffuse.png"));
	m_upNormalTxt.reset(Texture::LoadFromFile("resources/vehicle_normal.png"));
	m_upGlossTxt.reset(Texture::LoadFromFile("resources/vehicle_gloss.png"));
	m_upSpecularTxt.reset(Texture::LoadFromFile("resources/vehicle_specular.png"));
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	const float rotationSpeedRadians = 1;
	if(m_RotateMesh) m_MeshesWorld[0].worldMatrix = Matrix::CreateRotationY(rotationSpeedRadians * pTimer->GetElapsed())
													* m_MeshesWorld[0].worldMatrix;
}

void Renderer::Render()
{
	// @START
	SDL_FillRect(m_pBackBuffer, NULL, 0x646464);
	for (int index{}; index < m_Width * m_Height; ++index)
	{
		// Clear the DEPTH BUFFER
		m_pDepthBufferPixels[index] = 1;
	}
	
	// Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// RENDER LOGIC

	// For every triangle mesh
	for (int triangleMeshIndex{}; triangleMeshIndex < m_MeshesWorld.size(); ++triangleMeshIndex)
	{
		Mesh& currentMesh = m_MeshesWorld[triangleMeshIndex];
		// predefine a triangle we can reuse
		std::array<Vertex_Out, 3> triangleNDC{};

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

		// Project the entire mesh to NDC coordinates
		ProjectMeshToNDC(currentMesh);

		// Loop over all the triangles
		for (int triangleIndex{}; triangleIndex < triangleCount; ++triangleIndex)
		{
			uint32_t indexPos0 = currentMesh.indices[indexJump * triangleIndex + 0];
			uint32_t indexPos1 = currentMesh.indices[indexJump * triangleIndex + 1];
			uint32_t indexPos2 = currentMesh.indices[indexJump * triangleIndex + 2];
			// Skip if duplicate indices
			if (indexPos0 == indexPos1 or indexPos0 == indexPos2 or indexPos1 == indexPos2) continue;
			// If the triangle strip method is in use, swap the indices of odd indexed triangles
			if (triangleStripMethod and (triangleIndex & 1)) std::swap(indexPos1, indexPos2);
			
			// Define triangle in NDC
			triangleNDC[0] = currentMesh.vertices_out[indexPos0];
			triangleNDC[1] = currentMesh.vertices_out[indexPos1];
			triangleNDC[2] = currentMesh.vertices_out[indexPos2];

			// Cull the triangle if one or more of the NDC vertices are outside the frustum
			if (!IsNDCTriangleInFrustum(triangleNDC[0])) continue;
			if (!IsNDCTriangleInFrustum(triangleNDC[1])) continue;
			if (!IsNDCTriangleInFrustum(triangleNDC[2])) continue;

			// Rasterize the vertices
			RasterizeVertex(triangleNDC[0]);
			RasterizeVertex(triangleNDC[1]);
			RasterizeVertex(triangleNDC[2]);

			// Create an alias for triangleNDC, since they are now in raster space
			std::array<Vertex_Out, 3>& triangle_rasterVertices = triangleNDC;

			// Define the triangle's bounding box
			Vector2 min = {  FLT_MAX,  FLT_MAX };
			Vector2 max = { -FLT_MAX, -FLT_MAX };
			{
				// Minimums
				min = Vector2::Min(min, triangle_rasterVertices[0].position.GetXY());
				min = Vector2::Min(min, triangle_rasterVertices[1].position.GetXY());
				min = Vector2::Min(min, triangle_rasterVertices[2].position.GetXY());
				// Clamp between screen min and max, but also make sure that, due to floating point -> int rounding happens correct
				min.x = std::clamp(std::floor(min.x), 0.f, m_Width  - 1.f);
				min.y = std::clamp(std::floor(min.y), 0.f, m_Height - 1.f);
				
				// Maximums
				max = Vector2::Max(max, triangle_rasterVertices[0].position.GetXY());
				max = Vector2::Max(max, triangle_rasterVertices[1].position.GetXY());
				max = Vector2::Max(max, triangle_rasterVertices[2].position.GetXY());
				// Clamp between screen min and max, but also make sure that, due to floating point -> int rounding happens correct
				max.x = std::clamp(std::ceil(max.x), 0.f, m_Width  - 1.f);
				max.y = std::clamp(std::ceil(max.y), 0.f, m_Height - 1.f);
			}


			// For every pixel (within the bounding box)
			for (int py{ int(min.y) }; py < int(max.y); ++py)
			{
				for (int px{ int(min.x) }; px < int(max.x); ++px)
				{
					// Declare finalColor of the pixel
					ColorRGB finalColor{};

					// Declare wInterpolated and zBufferValue of this pixel
					float wInterpolated{ FLT_MAX };
					float zBufferValue { FLT_MAX };

					// Calculate the barycentric coordinates of that pixel in relationship to the triangle,
					// these barycentric coordinates CAN be invalid (point outside triangle)
					Vector2 pixelCoord = Vector2(px + 0.5f, py + 0.5f);
					Vector3 barycentricCoords = CalculateBarycentricCoordinates(
						triangle_rasterVertices[0].position.GetXY(),
						triangle_rasterVertices[1].position.GetXY(),
						triangle_rasterVertices[2].position.GetXY(),
						pixelCoord);

					if (barycentricCoords.x == 0)
					{
						int i = 10;
					}

					// Check if our barycentric coordinates are valid, if not, skip to the next pixel
					if (!AreBarycentricValid(barycentricCoords, true, false)) continue;

					// Now we interpolated both our Z and W depths
					InterpolateDepths(zBufferValue, wInterpolated, triangle_rasterVertices, barycentricCoords);
					if (zBufferValue < 0 or zBufferValue > 1) continue; // if z-depth is outside of frustum, skip to next pixel
					if (wInterpolated < 0) continue; // if w-depth is negative (behind camera), skip to next pixel

					// If out current value in the zBuffer is smaller then our new one, skip to the next pixel
					if (zBufferValue > m_pDepthBufferPixels[m_Width * py + px]) continue;

					// Now that we are sure our z-depth is smaller then the one in the zBuffer, we can update the zBuffer and interpolate the attributes
					m_pDepthBufferPixels[m_Width * py + px] = zBufferValue;

					// Correctly interpolated attributes
					Vertex_Out interpolatedAttributes{};
					InterpolateAllAttributes(triangle_rasterVertices, barycentricCoords, wInterpolated, interpolatedAttributes);
					interpolatedAttributes.position.z = zBufferValue;
					interpolatedAttributes.position.w = wInterpolated;

					PixelShading(interpolatedAttributes, finalColor);

					if (m_DepthBufferVisualization)
					{
						float remappedZ = Remap01(zBufferValue, 0.995f, 1.f);
						finalColor = { remappedZ , remappedZ , remappedZ };
					}
					
					// Make sure our colors are within the correct 0-1 range (while keeping relative differences)
					finalColor.MaxToOne();

					//Update Color in Buffer
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

void dae::Renderer::ProjectMeshToNDC(Mesh& mesh) const
{
	mesh.vertices_out.resize(mesh.vertices.size());

	// Calculate the transformation matrix
	Matrix worldViewProjectionMatrix = mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;
	
	std::for_each(std::execution::par, mesh.vertexCounter.begin(), mesh.vertexCounter.end(), [&](int index)
		{
			// Transform every vertex
			Vector4 transformedPosition = worldViewProjectionMatrix.TransformPoint(mesh.vertices[index].position.ToPoint4());
			mesh.vertices_out[index].position = transformedPosition;

			if (mesh.vertices_out[index].position.w <= 0) return;

			// Perform the perspective divide
			float invW = 1.f / transformedPosition.w;
			mesh.vertices_out[index].position.x *= invW;
			mesh.vertices_out[index].position.y *= invW;
			mesh.vertices_out[index].position.z *= invW;


			// Update the other attributes
			mesh.vertices_out[index].color = mesh.vertices[index].color;
			mesh.vertices_out[index].uv = mesh.vertices[index].uv;

			mesh.vertices_out[index].normal = mesh.worldMatrix.TransformVector(mesh.vertices[index].normal).Normalized();
			mesh.vertices_out[index].tangent = mesh.worldMatrix.TransformVector(mesh.vertices[index].tangent).Normalized();
			mesh.vertices_out[index].viewDirection = (mesh.worldMatrix.TransformPoint(mesh.vertices_out[index].position) - m_Camera.origin.ToPoint4()).Normalized();

		});
}
void dae::Renderer::RasterizeVertex(Vertex_Out& vertex) const
{
	vertex.position.x = (1.f + vertex.position.x) * 0.5f * m_Width;
	vertex.position.y = (1.f - vertex.position.y) * 0.5f * m_Height;
}

void dae::Renderer::InterpolateDepths(float& zDepth, float& wDepth, const std::array<Vertex_Out, 3>& triangle, const Vector3& weights)
{
	// Now we interpolated both our Z and W depths
	const float& Z0 = triangle[0].position.z;
	const float& Z1 = triangle[1].position.z;
	const float& Z2 = triangle[2].position.z;
	zDepth = InterpolateDepth(Z0, Z1, Z2, weights);

	const float& W0 = triangle[0].position.w;
	const float& W1 = triangle[1].position.w;
	const float& W2 = triangle[2].position.w;
	wDepth = InterpolateDepth(W0, W1, W2, weights);
}
void dae::Renderer::InterpolateAllAttributes(const std::array<Vertex_Out, 3>& triangle, const Vector3& weights, const float wInterpolated, Vertex_Out& output)
{
	// Get W components
	const float& W0 = triangle[0].position.w;
	const float& W1 = triangle[1].position.w;
	const float& W2 = triangle[2].position.w;

	// Correctly interpolated position
	const Vector4& P0 = triangle[0].position;
	const Vector4& P1 = triangle[1].position;
	const Vector4& P2 = triangle[2].position;
	output.position = InterpolateAttribute(P0, P1, P2, W0, W1, W2, wInterpolated, weights);

	// Correctly interpolated color
	const ColorRGB& C0 = triangle[0].color;
	const ColorRGB& C1 = triangle[1].color;
	const ColorRGB& C2 = triangle[2].color;
	output.color = InterpolateAttribute(C0, C1, C2, W0, W1, W2, wInterpolated, weights);

	// Correctly interpolated uv
	const Vector2& UV0 = triangle[0].uv;
	const Vector2& UV1 = triangle[1].uv;
	const Vector2& UV2 = triangle[2].uv;
	output.uv = InterpolateAttribute(UV0, UV1, UV2, W0, W1, W2, wInterpolated, weights);

	// Correctly interpolated normal
	const Vector3& N0 = triangle[0].normal;
	const Vector3& N1 = triangle[1].normal;
	const Vector3& N2 = triangle[2].normal;
	output.normal = InterpolateAttribute(N0, N1, N2, W0, W1, W2, wInterpolated, weights).Normalized();

	// Correctly interpolated tangent
	const Vector3& T0 = triangle[0].tangent;
	const Vector3& T1 = triangle[1].tangent;
	const Vector3& T2 = triangle[2].tangent;
	output.tangent = InterpolateAttribute(T0, T1, T2, W0, W1, W2, wInterpolated, weights).Normalized();

	// Correctly interpolated viewDirection
	const Vector3& VD0 = triangle[0].viewDirection;
	const Vector3& VD1 = triangle[1].viewDirection;
	const Vector3& VD2 = triangle[2].viewDirection;
	output.viewDirection = InterpolateAttribute(VD0, VD1, VD2, W0, W1, W2, wInterpolated, weights).Normalized();
}

void dae::Renderer::PixelShading(const Vertex_Out& v, ColorRGB& color)
{
	// Ambient Color
	const ColorRGB ambient = { 0.03f, 0.03f, 0.03f };

	// Set up the light
	Vector3 lightDirection = { 0.577f , -0.577f , 0.577f };
	Vector3 directionToLight = -lightDirection.Normalized();

	// Sample the normal
	Vector3 sampledNormal{};
	if (m_UseNormalMap)		sampledNormal = SampleFromNormalMap(v.normal, v.tangent, v.uv, m_upNormalTxt);
	else					sampledNormal = v.normal;

	// Calculate the observed area
	const float observedArea = Vector3::Dot(sampledNormal, directionToLight);
	if (observedArea <= 0.f and
		(m_CurrentShadingMode == ShadingMode::ObservedArea or
		 m_CurrentShadingMode == ShadingMode::Combined)) return;

	// Calculate the lambert diffuse color
	const ColorRGB cd = m_upDiffuseTxt->Sample(v.uv);
	const float kd = 7.f;
	const ColorRGB lambertDiffuse = (cd * kd) * ONE_DIV_PI;

	// Calculate the specularity
	const float shininess = 25.f;
	const ColorRGB specular = SamplePhong(directionToLight, v.viewDirection, sampledNormal, v.uv, shininess, m_upGlossTxt, m_upSpecularTxt);

	switch (m_CurrentShadingMode)
	{
	case dae::Renderer::ShadingMode::ObservedArea:
		color = { observedArea, observedArea, observedArea };
		break;
	case dae::Renderer::ShadingMode::Diffuse:
		color = lambertDiffuse;
		break;
	case dae::Renderer::ShadingMode::Specular:
		color = specular;
		break;
	case dae::Renderer::ShadingMode::Combined:
		color = (lambertDiffuse + specular + ambient) * observedArea;
		break;
	default:
		break;
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::CycleShadingMode()
{
	switch (m_CurrentShadingMode)
	{
	case dae::Renderer::ShadingMode::ObservedArea:
		m_CurrentShadingMode = ShadingMode::Diffuse;
		break;
	case dae::Renderer::ShadingMode::Diffuse:
		m_CurrentShadingMode = ShadingMode::Specular;
		break;
	case dae::Renderer::ShadingMode::Specular:
		m_CurrentShadingMode = ShadingMode::Combined;
		break;
	case dae::Renderer::ShadingMode::Combined:
		m_CurrentShadingMode = ShadingMode::ObservedArea;
		break;
	default:
		break;
	}
}
