#pragma once
#include "Maths.h"
#include "Texture.h"
#include <vector>

namespace dae
{
	struct Vertex
	{
		Vector3 position{};
		ColorRGB color{colors::White};
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};	
	};

	struct Vertex_Out
	{
		Vector4 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};

	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	struct Mesh
	{
		inline void LoadDiffuseTexture(const std::string& path)		{ m_upDiffuseTxt.reset(Texture::LoadFromFile(path)); }
		inline void LoadNormalMap(const std::string& path)			{ m_upNormalTxt.reset(Texture::LoadFromFile(path)); }
		inline void LoadGlossinessMap(const std::string& path)		{ m_upGlossTxt.reset(Texture::LoadFromFile(path)); }
		inline void LoadSpecularMap(const std::string& path)		{ m_upSpecularTxt.reset(Texture::LoadFromFile(path)); }

		inline ColorRGB SampleDiffuse(const Vector2& interpUV)
		{
			return m_upDiffuseTxt->Sample(interpUV);
		}
		inline ColorRGB SamplePhong(const Vector3& dirToLight, const Vector3& viewDir, const Vector3& interpNormal, const Vector2& interpUV, float shininess)
		{
			float ks = m_upSpecularTxt->Sample(interpUV).r;
			float exp = m_upGlossTxt->Sample(interpUV).r * shininess;

			Vector3 reflect{ dirToLight - 2 * Vector3::Dot(interpNormal, dirToLight) * interpNormal };
			float cosAlpha{ std::max(Vector3::Dot(reflect, viewDir), 0.f) };
			return ColorRGB(1, 1, 1) * ks * std::pow(cosAlpha, exp);
		}
		inline Vector3 SampleNormalMap(const Vector3& interpNormal, const Vector3& interpTangent, const Vector2& interpUV)
		{
			// Calculate the tangent space matrix
			Vector3 binormal = Vector3::Cross(interpNormal, interpTangent);
			Matrix tangentSpaceAxis = Matrix(
				interpTangent,
				binormal,
				interpNormal,
				Vector3::Zero
			);

			// Sample the normal map
			ColorRGB nrmlMap = m_upNormalTxt->Sample(interpUV);
			Vector3 normal{ nrmlMap.r, nrmlMap.g, nrmlMap.b };
			normal = 2.f * normal - Vector3(1.f, 1.f, 1.f);
			normal = tangentSpaceAxis.TransformVector(normal);

			return normal.Normalized();
		}

		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleStrip };

		std::vector<Vertex_Out> vertices_out{};
		Matrix worldMatrix{};

		// Textures
		std::unique_ptr<Texture> m_upDiffuseTxt;
		std::unique_ptr<Texture> m_upNormalTxt;
		std::unique_ptr<Texture> m_upGlossTxt;
		std::unique_ptr<Texture> m_upSpecularTxt;


		// Helper Containers
		std::vector<uint32_t> vertexCounter{};
	};
}
