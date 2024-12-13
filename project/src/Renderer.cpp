//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

#include <execution>
#include <future>
#include <thread>

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
	m_Camera.Initialize(45.f, { 0.f, 5.f, -64.f }, m_AspectRatio, 0.1f, 100.f);

	// Initialize Meshes
	m_vMeshes.resize(1);

	// MESH 01
	Utils::ParseOBJ("resources/vehicle.obj", m_vMeshes[0].vertices, m_vMeshes[0].indices, m_vMeshes[0].vertexCounter);
	m_vMeshes[0].primitiveTopology = PrimitiveTopology::TriangleList;

	m_vMeshes[0].LoadDiffuseTexture("resources/vehicle_diffuse.png");
	m_vMeshes[0].LoadNormalMap("resources/vehicle_normal.png");
	m_vMeshes[0].LoadGlossinessMap("resources/vehicle_gloss.png");
	m_vMeshes[0].LoadSpecularMap("resources/vehicle_specular.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	const float rotationSpeedRadians = 1;
	if(m_RotateMesh) m_vMeshes[0].worldMatrix = Matrix::CreateRotationY(rotationSpeedRadians * pTimer->GetElapsed())
													* m_vMeshes[0].worldMatrix;
}

void Renderer::Render()
{
	// @START
	SDL_FillRect(m_pBackBuffer, NULL, 0x646464);
	std::fill(&m_pDepthBufferPixels[0], &m_pDepthBufferPixels[m_Width * m_Height], 1);

	// Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// predefine a triangle we can reuse
	std::array<Vertex_Out, 3> triangleNDC{};
	std::array<Vertex_Out, 3> triangleRasterVertices{};
	for (int triangleMeshIndex{}; triangleMeshIndex < m_vMeshes.size(); ++triangleMeshIndex)
	{
		Mesh& currentMesh = m_vMeshes[triangleMeshIndex];

		int indexJump = 0;
		int triangleCount = 0;
		bool triangleStripMethod = false;

		// Determine the triangle count and index jump depending on the PrimitiveTopology
		if (m_vMeshes[triangleMeshIndex].primitiveTopology == PrimitiveTopology::TriangleList)
		{
			indexJump = 3;
			triangleCount = m_vMeshes[triangleMeshIndex].indices.size() / 3;
			triangleStripMethod = false;
		}
		else if (m_vMeshes[triangleMeshIndex].primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			indexJump = 1;
			triangleCount = m_vMeshes[triangleMeshIndex].indices.size() - 2;
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
			// Calculate the minimum depth if the current triangle, which we will use later for early-depth test
			float minDepth = std::min(triangleNDC[0].position.z, std::min(triangleNDC[1].position.z, triangleNDC[2].position.z));

			// Cull the triangle if one or more of the NDC vertices are outside the frustum
			if (!IsNDCTriangleInFrustum(triangleNDC[0])) continue;
			if (!IsNDCTriangleInFrustum(triangleNDC[1])) continue;
			if (!IsNDCTriangleInFrustum(triangleNDC[2])) continue;

			// Rasterize the vertices
			RasterizeVertex(currentMesh.vertices_out[indexPos0]);
			RasterizeVertex(currentMesh.vertices_out[indexPos1]);
			RasterizeVertex(currentMesh.vertices_out[indexPos2]);

			// Define triangle in RasterSpace
			triangleRasterVertices[0] = currentMesh.vertices_out[indexPos0];
			triangleRasterVertices[1] = currentMesh.vertices_out[indexPos1];
			triangleRasterVertices[2] = currentMesh.vertices_out[indexPos2];
			const Vector2& v0 = triangleRasterVertices[0].position.GetXY();
			const Vector2& v1 = triangleRasterVertices[1].position.GetXY();
			const Vector2& v2 = triangleRasterVertices[2].position.GetXY();

			if (m_DrawWireFrames)
			{
				ColorRGB wireFrameColor = colors::White * Remap01(minDepth, 0.998f, 1.f);

				DrawLine(v0.x, v0.y, v1.x, v1.y, wireFrameColor);
				DrawLine(v1.x, v1.y, v2.x, v2.y, wireFrameColor);
				DrawLine(v2.x, v2.y, v0.x, v0.y, wireFrameColor);

				continue;
			}

			// Define the triangle's bounding box
			Vector2 min = { FLT_MAX,  FLT_MAX };
			Vector2 max = { -FLT_MAX, -FLT_MAX };
			{
				// Minimums
				min = Vector2::Min(min, v0);
				min = Vector2::Min(min, v1);
				min = Vector2::Min(min, v2);
				// Clamp between screen min and max, but also make sure that, due to floating point -> int rounding happens correct
				min.x = std::clamp(std::floor(min.x), 0.f, m_Width - 1.f);
				min.y = std::clamp(std::floor(min.y), 0.f, m_Height - 1.f);

				// Maximums
				max = Vector2::Max(max, v0);
				max = Vector2::Max(max, v1);
				max = Vector2::Max(max, v2);
				// Clamp between screen min and max, but also make sure that, due to floating point -> int rounding happens correct
				max.x = std::clamp(std::ceil(max.x), 0.f, m_Width - 1.f);
				max.y = std::clamp(std::ceil(max.y), 0.f, m_Height - 1.f);
			}

			// Pre-calculate the inverse area of the triangle so this doesn't need to happen for
			// every pixels once we calculate the barycentric coordinates (as the triangle area won't change)
			float area = Vector2::Cross(v1 - v0, v2 - v0);
			area = abs(area);
			float invArea = 1.f / area;

			// For every pixel (within the bounding box)
			for (int py{ int(min.y) }; py < int(max.y); ++py)
			{
				for (int px{ int(min.x) }; px < int(max.x); ++px)
				{
					// Do an early depth test!!
					// If the minimum depth of our triangle is already bigger than what is stored in the depth buffer (at a current pixel),
					// there is no chance that that pixel inside the triangle will be closer, so we just skip to the next pixel
					if (minDepth > m_pDepthBufferPixels[m_Width * py + px]) continue;


					// Declare finalColor of the pixel
					ColorRGB finalColor{};

					// Declare wInterpolated and zBufferValue of this pixel
					float wInterpolated{ FLT_MAX };
					float zBufferValue{ FLT_MAX };

					// Calculate the barycentric coordinates of that pixel in relationship to the triangle,
					// these barycentric coordinates CAN be invalid (point outside triangle)
					Vector2 pixelCoord = Vector2(px + 0.5f, py + 0.5f);
					Vector3 barycentricCoords = CalculateBarycentricCoordinates(
						v0, v1, v2, pixelCoord, invArea);

					// Check if our barycentric coordinates are valid, if not, skip to the next pixel
					if (!AreBarycentricValid(barycentricCoords, true, false)) continue;

					// Now we interpolated both our Z and W depths
					InterpolateDepths(zBufferValue, wInterpolated, triangleRasterVertices, barycentricCoords);
					if (zBufferValue < 0 or zBufferValue > 1) continue; // if z-depth is outside of frustum, skip to next pixel
					if (wInterpolated < 0) continue; // if w-depth is negative (behind camera), skip to next pixel

					// If out current value in the zBuffer is smaller then our new one, skip to the next pixel
					if (zBufferValue > m_pDepthBufferPixels[m_Width * py + px]) continue;

					// Now that we are sure our z-depth is smaller then the one in the zBuffer, we can update the zBuffer and interpolate the attributes
					m_pDepthBufferPixels[m_Width * py + px] = zBufferValue;

					// Correctly interpolated attributes
					Vertex_Out interpolatedAttributes{};
					InterpolateAllAttributes(triangleRasterVertices, barycentricCoords, wInterpolated, interpolatedAttributes);
					interpolatedAttributes.position.z = zBufferValue;
					interpolatedAttributes.position.w = wInterpolated;

					finalColor = PixelShading(interpolatedAttributes, currentMesh);

					if (m_DepthBufferVisualization)
					{
						float remappedZ = Remap01(m_pDepthBufferPixels[m_Width * py + px], 0.998f, 1);
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


	// @END
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

			mesh.vertices_out[index].normal			= mesh.worldMatrix.TransformVector(mesh.vertices[index].normal).Normalized();
			mesh.vertices_out[index].tangent		= mesh.worldMatrix.TransformVector(mesh.vertices[index].tangent).Normalized();
			mesh.vertices_out[index].viewDirection  = (mesh.worldMatrix.TransformPoint(mesh.vertices_out[index].position)
													- m_Camera.origin.ToPoint4()).Normalized();


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
	output.normal = InterpolateAttribute(N0, N1, N2, W0, W1, W2, wInterpolated, weights);
	output.normal.Normalize();

	// Correctly interpolated tangent
	const Vector3& T0 = triangle[0].tangent;
	const Vector3& T1 = triangle[1].tangent;
	const Vector3& T2 = triangle[2].tangent;
	output.tangent = InterpolateAttribute(T0, T1, T2, W0, W1, W2, wInterpolated, weights);
	output.tangent.Normalize();

	// Correctly interpolated viewDirection
	const Vector3& VD0 = triangle[0].viewDirection;
	const Vector3& VD1 = triangle[1].viewDirection;
	const Vector3& VD2 = triangle[2].viewDirection;
	output.viewDirection = InterpolateAttribute(VD0, VD1, VD2, W0, W1, W2, wInterpolated, weights);
	output.viewDirection.Normalize();
}

ColorRGB dae::Renderer::PixelShading(const Vertex_Out& v, Mesh& m)
{
	// Ambient Color
	const ColorRGB ambient = { 0.03f, 0.03f, 0.03f };

	// Set up the light
	Vector3 lightDirection = { 0.577f , -0.577f , 0.577f };
	Vector3 directionToLight = -lightDirection.Normalized();

	// Sample the normal
	Vector3 sampledNormal{};
	if (m_UseNormalMap)		sampledNormal = m.SampleNormalMap(v.normal, v.tangent, v.uv);
	else					sampledNormal = v.normal;

	// Calculate the observed area
	const float observedArea = Vector3::Dot(sampledNormal, directionToLight);
	// Skipping if observedArea < 0 happens when we check for the Current Shading Mode, as we really only want to skip (return black) if
	// We are actualy in a mode that uses observedArea (ObservedArea and Combined)

	// Calculate the lambert diffuse color
	const ColorRGB cd = m.SampleDiffuse(v.uv);
	const float kd = 7.f;
	const ColorRGB lambertDiffuse = (cd * kd) * ONE_DIV_PI;

	// Calculate the specularity
	const float shininess = 25.f;
	const ColorRGB specular = m.SamplePhong(directionToLight, v.viewDirection, sampledNormal, v.uv, shininess);

	switch (m_CurrentShadingMode)
	{
	case dae::Renderer::ShadingMode::ObservedArea:
		if (observedArea <= 0.f) return{};
		return ColorRGB{ observedArea, observedArea, observedArea };
		break;
	case dae::Renderer::ShadingMode::Diffuse:
		return lambertDiffuse;
		break;
	case dae::Renderer::ShadingMode::Specular:
		return specular;
		break;
	case dae::Renderer::ShadingMode::Combined:
		if (observedArea <= 0.f) return{};
		return (lambertDiffuse + specular + ambient) * observedArea;
		break;
	default:
		return (lambertDiffuse + specular + ambient) * observedArea;
		break;
	}
}

void dae::Renderer::DrawLine(int x0, int y0, int x1, int y1, const ColorRGB& color)
{
	// Bresenham's Line Algorithm
	// https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm

	int dx = abs(x1 - x0);
	int sx = (x0 < x1) ? 1 : -1;

	int dy = -abs(y1 - y0);
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx + dy;

	while (true)
	{
		if (y0 < m_Height and y0 >= 0
			and x0 < m_Width and x0 >= 0)
		{
			m_pBackBufferPixels[m_Width * y0 + x0] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(color.r * 255),
				static_cast<uint8_t>(color.g * 255),
				static_cast<uint8_t>(color.b * 255));
		}

		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
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
