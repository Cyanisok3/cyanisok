#include "../../include/AIUtil/ImageValidation.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

namespace
{

using image_validation::ImageDimensions;

std::uint16_t readBigEndian16(
    const std::vector<unsigned char>& data,
    std::size_t offset)
{
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

std::uint32_t readBigEndian32(
    const std::vector<unsigned char>& data,
    std::size_t offset)
{
    return
        (static_cast<std::uint32_t>(data[offset]) << 24U) |
        (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
        (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
        static_cast<std::uint32_t>(data[offset + 3U]);
}

std::uint32_t readLittleEndian24(
    const std::vector<unsigned char>& data,
    std::size_t offset)
{
    return
        static_cast<std::uint32_t>(data[offset]) |
        (static_cast<std::uint32_t>(data[offset + 1U]) << 8U) |
        (static_cast<std::uint32_t>(data[offset + 2U]) << 16U);
}

std::uint32_t readLittleEndian32(
    const std::vector<unsigned char>& data,
    std::size_t offset)
{
    return
        static_cast<std::uint32_t>(data[offset]) |
        (static_cast<std::uint32_t>(data[offset + 1U]) << 8U) |
        (static_cast<std::uint32_t>(data[offset + 2U]) << 16U) |
        (static_cast<std::uint32_t>(data[offset + 3U]) << 24U);
}

bool matches(
    const std::vector<unsigned char>& data,
    std::size_t offset,
    const char* value,
    std::size_t length)
{
    return offset <= data.size() &&
        length <= data.size() - offset &&
        std::memcmp(data.data() + offset, value, length) == 0;
}

ImageDimensions validateDimensions(
    std::uint32_t width,
    std::uint32_t height)
{
    if (width == 0 || height == 0)
    {
        throw std::invalid_argument("Image dimensions are invalid");
    }

    const std::uint64_t pixels =
        static_cast<std::uint64_t>(width) *
        static_cast<std::uint64_t>(height);
    if (width > image_validation::kMaxDimension ||
        height > image_validation::kMaxDimension ||
        pixels > image_validation::kMaxPixels)
    {
        throw std::invalid_argument(
            "Image dimensions exceed the 8192px or 16-megapixel limit");
    }
    return {width, height};
}

ImageDimensions parsePng(const std::vector<unsigned char>& data)
{
    if (data.size() < 24U ||
        readBigEndian32(data, 8U) != 13U ||
        !matches(data, 12U, "IHDR", 4U))
    {
        throw std::invalid_argument("Invalid PNG image header");
    }
    return validateDimensions(
        readBigEndian32(data, 16U),
        readBigEndian32(data, 20U));
}

ImageDimensions parseGif(const std::vector<unsigned char>& data)
{
    if (data.size() < 10U)
    {
        throw std::invalid_argument("Invalid GIF image header");
    }
    const std::uint32_t width =
        static_cast<std::uint32_t>(data[6U]) |
        (static_cast<std::uint32_t>(data[7U]) << 8U);
    const std::uint32_t height =
        static_cast<std::uint32_t>(data[8U]) |
        (static_cast<std::uint32_t>(data[9U]) << 8U);
    return validateDimensions(width, height);
}

bool isJpegStartOfFrame(unsigned char marker)
{
    return
        (marker >= 0xC0U && marker <= 0xC3U) ||
        (marker >= 0xC5U && marker <= 0xC7U) ||
        (marker >= 0xC9U && marker <= 0xCBU) ||
        (marker >= 0xCDU && marker <= 0xCFU);
}

ImageDimensions parseJpeg(const std::vector<unsigned char>& data)
{
    std::size_t offset = 2U;
    while (offset < data.size())
    {
        while (offset < data.size() && data[offset] != 0xFFU)
        {
            ++offset;
        }
        while (offset < data.size() && data[offset] == 0xFFU)
        {
            ++offset;
        }
        if (offset >= data.size())
        {
            break;
        }

        const unsigned char marker = data[offset++];
        if (marker == 0xD8U || marker == 0xD9U ||
            marker == 0x01U ||
            (marker >= 0xD0U && marker <= 0xD7U))
        {
            continue;
        }
        if (offset + 2U > data.size())
        {
            break;
        }

        const std::uint16_t segmentLength = readBigEndian16(data, offset);
        if (segmentLength < 2U ||
            static_cast<std::size_t>(segmentLength) > data.size() - offset)
        {
            throw std::invalid_argument("Invalid JPEG image header");
        }
        if (isJpegStartOfFrame(marker))
        {
            if (segmentLength < 7U)
            {
                throw std::invalid_argument("Invalid JPEG dimensions");
            }
            return validateDimensions(
                readBigEndian16(data, offset + 5U),
                readBigEndian16(data, offset + 3U));
        }
        offset += segmentLength;
    }
    throw std::invalid_argument("JPEG dimensions were not found");
}

ImageDimensions parseWebp(const std::vector<unsigned char>& data)
{
    std::size_t chunkOffset = 12U;
    while (chunkOffset + 8U <= data.size())
    {
        const std::uint32_t chunkSize =
            readLittleEndian32(data, chunkOffset + 4U);
        const std::size_t payloadOffset = chunkOffset + 8U;
        if (chunkSize > data.size() - payloadOffset)
        {
            throw std::invalid_argument("Invalid WebP image header");
        }

        if (matches(data, chunkOffset, "VP8X", 4U) && chunkSize >= 10U)
        {
            return validateDimensions(
                1U + readLittleEndian24(data, payloadOffset + 4U),
                1U + readLittleEndian24(data, payloadOffset + 7U));
        }
        if (matches(data, chunkOffset, "VP8 ", 4U) && chunkSize >= 10U)
        {
            if (data[payloadOffset + 3U] != 0x9DU ||
                data[payloadOffset + 4U] != 0x01U ||
                data[payloadOffset + 5U] != 0x2AU)
            {
                throw std::invalid_argument("Invalid WebP VP8 header");
            }
            const std::uint32_t width =
                (static_cast<std::uint32_t>(data[payloadOffset + 6U]) |
                 (static_cast<std::uint32_t>(data[payloadOffset + 7U]) << 8U)) &
                0x3FFFU;
            const std::uint32_t height =
                (static_cast<std::uint32_t>(data[payloadOffset + 8U]) |
                 (static_cast<std::uint32_t>(data[payloadOffset + 9U]) << 8U)) &
                0x3FFFU;
            return validateDimensions(width, height);
        }
        if (matches(data, chunkOffset, "VP8L", 4U) && chunkSize >= 5U)
        {
            if (data[payloadOffset] != 0x2FU)
            {
                throw std::invalid_argument("Invalid WebP VP8L header");
            }
            const std::uint32_t bits =
                readLittleEndian32(data, payloadOffset + 1U);
            return validateDimensions(
                1U + (bits & 0x3FFFU),
                1U + ((bits >> 14U) & 0x3FFFU));
        }

        const std::size_t paddedChunkSize =
            static_cast<std::size_t>(chunkSize) + (chunkSize & 1U);
        if (paddedChunkSize > data.size() - payloadOffset)
        {
            throw std::invalid_argument("Invalid WebP chunk length");
        }
        chunkOffset = payloadOffset + paddedChunkSize;
    }
    throw std::invalid_argument("WebP dimensions were not found");
}

} // namespace

namespace image_validation
{

ImageDimensions validateEncodedImage(
    const std::vector<unsigned char>& imageData)
{
    if (imageData.empty())
    {
        throw std::invalid_argument("Image data is empty");
    }
    if (imageData.size() > kMaxEncodedBytes)
    {
        throw std::invalid_argument("Image exceeds the 6MB encoded size limit");
    }

    static constexpr std::array<unsigned char, 8> pngSignature = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU
    };
    if (imageData.size() >= pngSignature.size() &&
        std::equal(
            pngSignature.begin(),
            pngSignature.end(),
            imageData.begin()))
    {
        return parsePng(imageData);
    }
    if (matches(imageData, 0U, "GIF87a", 6U) ||
        matches(imageData, 0U, "GIF89a", 6U))
    {
        return parseGif(imageData);
    }
    if (imageData.size() >= 2U &&
        imageData[0U] == 0xFFU &&
        imageData[1U] == 0xD8U)
    {
        return parseJpeg(imageData);
    }
    if (matches(imageData, 0U, "RIFF", 4U) &&
        matches(imageData, 8U, "WEBP", 4U))
    {
        return parseWebp(imageData);
    }

    throw std::invalid_argument(
        "Unsupported image format; use PNG, JPEG, GIF, or WebP");
}

} // namespace image_validation
