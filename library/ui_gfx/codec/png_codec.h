
#ifndef __ui_gfx_png_codec_h__
#define __ui_gfx_png_codec_h__

#pragma once

#include <vector>

#include "base/basic_types.h"

class SkBitmap;

namespace gfx
{

class Size;

// Interface for encoding and decoding PNG data. This is a wrapper around
// libpng, which has an inconvenient interface for callers. This is currently
// designed for use in tests only (where we control the files), so the handling
// isn't as robust as would be required for a browser (see Decode() for more).
// WebKit has its own more complicated PNG decoder which handles, among other
// things, partially downloaded data.
class PNGCodec
{
public:
    enum ColorFormat
    {
        // 3 bytes per pixel (packed), in RGB order regardless of endianness.
        // This is the native JPEG format.
        FORMAT_RGB,

        // 4 bytes per pixel, in RGBA order in memory regardless of endianness.
        FORMAT_RGBA,

        // 4 bytes per pixel, in BGRA order in memory regardless of endianness.
        // This is the default Windows DIB order.
        FORMAT_BGRA,

        // 4 bytes per pixel, in pre-multiplied kARGB_8888_Config format. For use
        // with directly writing to a skia bitmap.
        FORMAT_SkBitmap
    };

    // Represents a comment in the tEXt ancillary chunk of the png.
    struct Comment
    {
        Comment(const std::string& k, const std::string& t);
        ~Comment();

        std::string key;
        std::string text;
    };

    // Calls PNGCodec::EncodeWithCompressionLevel with the default compression
    // level.
    static bool Encode(const unsigned char* input,
                       ColorFormat format,
                       const Size& size,
                       int row_byte_width,
                       bool discard_transparency,
                       const std::vector<Comment>& comments,
                       std::vector<unsigned char>* output);

    // Encodes the given raw 'input' data, with each pixel being represented as
    // given in 'format'. The encoded PNG data will be written into the supplied
    // vector and true will be returned on success. On failure (false), the
    // contents of the output buffer are undefined.
    //
    // When writing alpha values, the input colors are assumed to be post
    // multiplied.
    //
    // size: dimensions of the image
    // row_byte_width: the width in bytes of each row. This may be greater than
    //   w * bytes_per_pixel if there is extra padding at the end of each row
    //   (often, each row is padded to the next machine word).
    // discard_transparency: when true, and when the input data format includes
    //   alpha values, these alpha values will be discarded and only RGB will be
    //   written to the resulting file. Otherwise, alpha values in the input
    //   will be preserved.
    // comments: comments to be written in the png's metadata.
    // compression_level: An integer between -1 and 9, corresponding to zlib's
    //   compression levels. -1 is the default.
    static bool EncodeWithCompressionLevel(const unsigned char* input,
                                           ColorFormat format,
                                           const Size& size,
                                           int row_byte_width,
                                           bool discard_transparency,
                                           const std::vector<Comment>& comments,
                                           int compression_level,
                                           std::vector<unsigned char>* output);

    // Call PNGCodec::Encode on the supplied SkBitmap |input|, which is assumed
    // to be BGRA, 32 bits per pixel. The params |discard_transparency| and
    // |output| are passed directly to Encode; refer to Encode for more
    // information. During the call, an SkAutoLockPixels lock is held on |input|.
    static bool EncodeBGRASkBitmap(const SkBitmap& input,
                                   bool discard_transparency, std::vector<unsigned char>* output);

    // Decodes the PNG data contained in input of length input_size. The
    // decoded data will be placed in *output with the dimensions in *w and *h
    // on success (returns true). This data will be written in the 'format'
    // format. On failure, the values of these output variables are undefined.
    //
    // This function may not support all PNG types, and it hasn't been tested
    // with a large number of images, so assume a new format may not work. It's
    // really designed to be able to read in something written by Encode() above.
    static bool Decode(const unsigned char* input, size_t input_size,
                       ColorFormat format, std::vector<unsigned char>* output, int* w, int* h);

    // Decodes the PNG data directly into the passed in SkBitmap. This is
    // significantly faster than the vector<unsigned char> version of Decode()
    // above when dealing with PNG files that are >500K, which a lot of theme
    // images are. (There are a lot of themes that have a NTP image of about ~1
    // megabyte, and those require a 7-10 megabyte side buffer.)
    //
    // Returns true if data is non-null and can be decoded as a png, false
    // otherwise.
    static bool Decode(const unsigned char* input, size_t input_size,
                       SkBitmap* bitmap);

    // Create a SkBitmap from a decoded BGRA DIB. The caller owns the returned
    // SkBitmap.
    static SkBitmap* CreateSkBitmapFromBGRAFormat(
        std::vector<unsigned char>& bgra, int width, int height);

private:
    DISALLOW_COPY_AND_ASSIGN(PNGCodec);
};

} //namespace gfx

#endif //__ui_gfx_png_codec_h__