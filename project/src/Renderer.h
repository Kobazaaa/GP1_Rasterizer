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

		void ToggleDepthBufferVisualization() { m_DepthBufferVisualization = !m_DepthBufferVisualization; }

		void ProjectMeshToNDC(Mesh& mesh) const;
		void RasterizeVertex(Vertex_Out& vertex) const;
		void InterpolateDepths(float& zDepth, float& wDepth, const std::array<Vertex_Out, 3>& triangle, const Vector3& weights);
		void InterpolateAllAttributes(const std::array<Vertex_Out, 3>& triangle, const Vector3& weights, const float wInterpolated, Vertex_Out& output);

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};
		float m_AspectRatio{};

		int m_Width{};
		int m_Height{};

		bool m_DepthBufferVisualization { false };

		std::vector<Mesh> m_MeshesWorld;
		std::unique_ptr<Texture> m_upTexture;
	};
}
