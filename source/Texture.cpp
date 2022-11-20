#include "Texture.h"
#include "Vector2.h"
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
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		SDL_Surface* pLoadedSurface{ IMG_Load(path.c_str()) };


		return new Texture(pLoadedSurface);
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//Sample the correct texel for the given uv

		// convert from 0,1 range to 0, width/height range
		const int u{ static_cast<int>(uv.x * m_pSurface->w) };
		const int v{ static_cast<int>(uv.y * m_pSurface->h) };

		// convert to single index -> just like the backbufffer
		uint8_t r{}, g{}, b{}; // didn't make these pointers, would get too confusing in the last step
		//const uint32_t pixel{ static_cast<uint32_t>(u + (v * m_pSurface->w)) };
		const uint32_t pixel{ m_pSurfacePixels[u + (v * m_pSurface->w)]};
		SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);
		// fill in the RGB

		// instead of dividing all the time, calculate 1/255 and multiply each value -> sets back into a 0,1 range
		return ColorRGB{ r * m_DivideColor, g * m_DivideColor, b * m_DivideColor };
	}
}