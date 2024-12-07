#pragma once

#include <memory>
#include <cstdint>
#include <vector>
#include <array>

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

		void CycleShadingMode();
		void ToggleDepthBufferVisualization()	{ m_DepthBufferVisualization = !m_DepthBufferVisualization; }
		void ToggleMeshRotation()				{ m_RotateMesh = !m_RotateMesh; }
		void ToggleNormalMap()					{ m_UseNormalMap = !m_UseNormalMap; }
		void ToggleWireFrames()					{ m_DrawWireFrames = !m_DrawWireFrames; }

		void ProjectMeshToNDC(Mesh& mesh) const;
		void RasterizeVertex(Vertex_Out& vertex) const;
		void InterpolateDepths(float& zDepth, float& wDepth, const std::array<Vertex_Out, 3>& triangle, const Vector3& weights);
		void InterpolateAllAttributes(const std::array<Vertex_Out, 3>& triangle, const Vector3& weights, const float wInterpolated, Vertex_Out& output);
		
		void PixelShading(const Vertex_Out& v, ColorRGB& color);

		void DrawLine(int x0, int y0, int x1, int y1, const ColorRGB& color);
	private:
		enum class ShadingMode
		{
			ObservedArea,	// Lambert Cosine Law
			Diffuse,		// Diffuse Color
			Specular,		// Specular Color
			Combined		// Diffuse + Specular + Ambient
		};
		ShadingMode m_CurrentShadingMode	{ ShadingMode::Combined };
		bool m_DepthBufferVisualization		{ false };
		bool m_RotateMesh					{ true };
		bool m_UseNormalMap					{ true };
		bool m_DrawWireFrames				{ false };

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};
		float m_AspectRatio{};

		int m_Width{};
		int m_Height{};


		std::vector<Mesh> m_MeshesWorld;

		std::unique_ptr<Texture> m_upDiffuseTxt;
		std::unique_ptr<Texture> m_upNormalTxt;
		std::unique_ptr<Texture> m_upGlossTxt;
		std::unique_ptr<Texture> m_upSpecularTxt;
	};
}
