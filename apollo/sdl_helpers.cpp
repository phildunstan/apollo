#include "sdl_helpers.h"

#include <cassert>
#include <experimental/resumable>

#include "SDL.h"
#include "SDL_image.h"

using namespace std;

template <typename SwizzleFunc>
void CopyAndSwizzlePixels(const uint8_t* src, int w, int h, uint8_t* dst, SwizzleFunc func)
{
	for (auto y = 0; y < h; ++y)
	{
		for (auto x = 0; x < w; ++x)
		{
			func(&src[4 * (y * w + x)], &dst[4 * (y * w + x)]);
		}
	}
}

TextureInfo LoadSDLTexture(const string& filename)
{
	auto surface = unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>(IMG_Load(filename.c_str()), SDL_FreeSurface);
	if (!surface)
	{
		printf("Unable to load the SDL Surface from %s: %s\n", filename.c_str(), IMG_GetError());
		return make_tuple(nullptr, 0, 0);
	}

	if (SDL_LockSurface(surface.get()) != 0)
	{
		printf("Unable to lock the SDL Surface: %s\n", SDL_GetError());
	}

	// copy the pixels into our own buffer and swizzle the pixels into RGBA8888
	auto swizzledPixels = unique_ptr<uint8_t[]>(new uint8_t[4 * surface->w * surface->h]);
	switch (surface->format->format)
	{
	case SDL_PIXELFORMAT_ABGR8888:
		CopyAndSwizzlePixels(static_cast<uint8_t*>(surface->pixels), surface->w, surface->h, swizzledPixels.get(), [] (const uint8_t* srcPixel, uint8_t* dstPixel)
		{
			// Not sure what is happening here, but the pixel data is not coming in as ABGR8888, It appears to be RBGA8888
			dstPixel[0] = srcPixel[0];
			dstPixel[1] = srcPixel[1];
			dstPixel[2] = srcPixel[2];
			dstPixel[3] = srcPixel[3];
		});
		break;
	default:
		printf("Unsupported SDL Surface pixel format 0x%x\n", surface->format->format);
	}

	SDL_UnlockSurface(surface.get());

	return make_tuple(move(swizzledPixels), surface->w, surface->h);
}