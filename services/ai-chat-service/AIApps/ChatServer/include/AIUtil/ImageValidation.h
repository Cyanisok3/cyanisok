#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace image_validation
{

inline constexpr std::size_t kMaxEncodedBytes = 6U * 1024U * 1024U;
inline constexpr std::size_t kMaxBase64Characters =
    ((kMaxEncodedBytes + 2U) / 3U) * 4U;
inline constexpr std::uint32_t kMaxDimension = 8192U;
inline constexpr std::uint64_t kMaxPixels = 16U * 1024U * 1024U;

struct ImageDimensions
{
    std::uint32_t width;
    std::uint32_t height;
};

ImageDimensions validateEncodedImage(
    const std::vector<unsigned char>& imageData);

} // namespace image_validation
