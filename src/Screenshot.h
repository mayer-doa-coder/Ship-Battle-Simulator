#pragma once

// Environment build: saves the frame on screen as a PNG file (F12, or --shot on the command line), with no image library.
//
// PNG's pixel data is a zlib stream, and a zlib stream is allowed to hold its bytes UNCOMPRESSED in "stored" blocks. So the writer is
// short: read the pixels, put a filter byte of 0 in front of each row, wrap the rows in stored blocks with an Adler-32 checksum, and
// frame it all in the three chunks a PNG needs (header, data, end), each with a CRC-32. The file is larger than a compressed one
// (about 2.7 MB at 1280 x 720) and opens in anything.

#include <glad/glad.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace ScreenshotDetail {

inline std::uint32_t crc32(const std::uint8_t* data, std::size_t n, std::uint32_t crc = 0)
{
    static std::uint32_t table[256];
    static bool ready = false;
    if (!ready) {
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        ready = true;
    }
    crc = ~crc;
    for (std::size_t i = 0; i < n; ++i)
        crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    return ~crc;
}

inline void put32(std::vector<std::uint8_t>& v, std::uint32_t x)
{
    v.push_back(static_cast<std::uint8_t>(x >> 24)); v.push_back(static_cast<std::uint8_t>(x >> 16));
    v.push_back(static_cast<std::uint8_t>(x >> 8));  v.push_back(static_cast<std::uint8_t>(x));
}

// One chunk: length, a 4-letter type, the data, and the CRC of the type and the data together.
inline void chunk(std::vector<std::uint8_t>& out, const char type[4], const std::vector<std::uint8_t>& data)
{
    put32(out, static_cast<std::uint32_t>(data.size()));
    std::vector<std::uint8_t> body(type, type + 4);
    body.insert(body.end(), data.begin(), data.end());
    out.insert(out.end(), body.begin(), body.end());
    put32(out, crc32(body.data(), body.size()));
}

} // namespace ScreenshotDetail

// Reads the back buffer (call it after drawing and BEFORE swapping) and writes it to `path`. Returns false if the file cannot be written.
inline bool saveScreenshotPng(const char* path, int width, int height)
{
    using namespace ScreenshotDetail;
    if (width <= 0 || height <= 0)
        return false;

    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL's first row is the BOTTOM of the picture and a PNG's first row is the top, so the rows are written in reverse order.
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 3 + 1;
    std::vector<std::uint8_t> raw(rowBytes * height);
    for (int y = 0; y < height; ++y) {
        std::uint8_t* row = &raw[static_cast<std::size_t>(y) * rowBytes];
        row[0] = 0;                                              // filter type: none
        const std::uint8_t* src = &pixels[static_cast<std::size_t>(height - 1 - y) * width * 4];
        for (int x = 0; x < width; ++x) {
            row[1 + x * 3 + 0] = src[x * 4 + 0];
            row[1 + x * 3 + 1] = src[x * 4 + 1];
            row[1 + x * 3 + 2] = src[x * 4 + 2];
        }
    }

    // zlib: a 2-byte header, stored blocks of up to 65535 bytes, and the Adler-32 of the raw data, big-endian.
    std::vector<std::uint8_t> z;
    z.push_back(0x78); z.push_back(0x01);
    std::uint32_t a = 1, b = 0;
    for (std::uint8_t byte : raw) {
        a = (a + byte) % 65521u;
        b = (b + a) % 65521u;
    }
    for (std::size_t pos = 0; pos < raw.size();) {
        const std::size_t n = std::min<std::size_t>(65535, raw.size() - pos);
        const bool last = (pos + n == raw.size());
        z.push_back(last ? 1 : 0);
        z.push_back(static_cast<std::uint8_t>(n & 0xFF)); z.push_back(static_cast<std::uint8_t>(n >> 8));
        z.push_back(static_cast<std::uint8_t>(~n & 0xFF)); z.push_back(static_cast<std::uint8_t>((~n >> 8) & 0xFF));
        z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
        pos += n;
    }
    put32(z, (b << 16) | a);

    std::vector<std::uint8_t> file = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    std::vector<std::uint8_t> header;
    put32(header, static_cast<std::uint32_t>(width));
    put32(header, static_cast<std::uint32_t>(height));
    header.push_back(8);      // bit depth
    header.push_back(2);      // colour type 2: truecolour (red, green, blue)
    header.push_back(0); header.push_back(0); header.push_back(0);
    chunk(file, "IHDR", header);
    chunk(file, "IDAT", z);
    chunk(file, "IEND", std::vector<std::uint8_t>());

    std::FILE* f = nullptr;
#ifdef _MSC_VER
    if (fopen_s(&f, path, "wb") != 0)       // the Windows C runtime flags plain fopen as unsafe
        f = nullptr;
#else
    f = std::fopen(path, "wb");
#endif
    if (f == nullptr)
        return false;
    const bool ok = std::fwrite(file.data(), 1, file.size(), f) == file.size();
    std::fclose(f);
    return ok;
}
