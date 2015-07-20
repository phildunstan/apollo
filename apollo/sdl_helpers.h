#pragma once

#include <memory>
#include <tuple>
#include <string>
#include <future>

using TextureInfo = std::tuple<std::unique_ptr<uint8_t>, int, int>;
TextureInfo LoadSDLTexture(const std::string& filename);
