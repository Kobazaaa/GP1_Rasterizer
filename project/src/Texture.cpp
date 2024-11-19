#include "Texture.h"
#include "Vector2.h"
#include <iostream>
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		SDL_Surface* pSurface = IMG_Load(path.c_str());
		if (pSurface == nullptr)
		{
			std::cerr << "Texture::LoadFromFile > Failed to load texture: " << path << " Error: " << IMG_GetError() << std::endl;
			throw std::runtime_error("Failed to load texture");
		}

		return new Texture(pSurface);
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		// Set the default return color to black
		ColorRGB returnColor{ 0, 0, 0 };

		// Wrap the UV coordinates
		float u = uv.x - std::floor(uv.x);
		float v = uv.y - std::floor(uv.y);

		// Since our UV coordinates are of values between [0; 1] and SDL requests pixel indexes,
		// we multiply the UV with width and height of the background
		int x = u * m_pSurface->w;
		int y = v * m_pSurface->h;

		// Bytes per pixel
		const Uint8 bytesPerPixel = m_pSurface->format->BytesPerPixel;

		// Retrieve the address of the pixel we need (startAddress + y * width + x * bpp)
		Uint8* pPixelAddr = (Uint8*)m_pSurfacePixels + y * m_pSurface->pitch + x * bytesPerPixel;
		// Convert the pixelAddress to a Uint32, since that is what the SDL_GetRGB expects
		Uint32 pixelData = *(Uint32*)pPixelAddr;

		SDL_Color Color = { 0x00, 0x00, 0x00 };

		// Retrieve the RGB values of the specific pixel
		SDL_GetRGB(pixelData, m_pSurface->format, &Color.r, &Color.g, &Color.b);

		// Set our returnColor to the color SDL gave us
		returnColor.r = Color.r;
		returnColor.g = Color.g;
		returnColor.b = Color.b;
		// SDL uses colors in ranges 0-255, we use ranges 0-1, therefore we divide our returnColor by 255
		returnColor /= 255.f;

		// return the background color of the pixel
		return returnColor;
	}
}