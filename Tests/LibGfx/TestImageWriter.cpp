/*
 * Copyright (c) 2024, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/MemoryStream.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/ImageFormats/BMPLoader.h>
#include <LibGfx/ImageFormats/BMPWriter.h>
#include <LibGfx/ImageFormats/JPEGLoader.h>
#include <LibGfx/ImageFormats/JPEGWriter.h>
#include <LibGfx/ImageFormats/PNGLoader.h>
#include <LibGfx/ImageFormats/PNGWriter.h>
#include <LibGfx/ImageFormats/QOILoader.h>
#include <LibGfx/ImageFormats/QOIWriter.h>
#include <LibGfx/ImageFormats/WebPLoader.h>
#include <LibGfx/ImageFormats/WebPWriter.h>
#include <LibTest/TestCase.h>

static ErrorOr<NonnullRefPtr<Gfx::Bitmap>> expect_single_frame(Gfx::ImageDecoderPlugin& plugin_decoder)
{
    EXPECT_EQ(plugin_decoder.frame_count(), 1u);
    EXPECT(!plugin_decoder.is_animated());
    EXPECT(!plugin_decoder.loop_count());

    auto frame_descriptor = TRY(plugin_decoder.frame(0));
    EXPECT_EQ(frame_descriptor.duration, 0);
    return *frame_descriptor.image;
}

static ErrorOr<NonnullRefPtr<Gfx::Bitmap>> expect_single_frame_of_size(Gfx::ImageDecoderPlugin& plugin_decoder, Gfx::IntSize size)
{
    EXPECT_EQ(plugin_decoder.size(), size);
    auto frame = TRY(expect_single_frame(plugin_decoder));
    EXPECT_EQ(frame->size(), size);
    return frame;
}

template<class Writer, class Loader>
static ErrorOr<NonnullRefPtr<Gfx::Bitmap>> get_roundtrip_bitmap(Gfx::Bitmap const& bitmap)
{
    ByteBuffer encoded_data;
    if constexpr (requires(AllocatingMemoryStream stream) { Writer::encode(stream, bitmap); }) {
        AllocatingMemoryStream stream;
        TRY(Writer::encode(stream, bitmap));
        encoded_data = TRY(stream.read_until_eof());
    } else {
        encoded_data = TRY(Writer::encode(bitmap));
    }

    auto plugin = TRY(Loader::create(encoded_data));
    return expect_single_frame_of_size(*plugin, bitmap.size());
}

template<class Writer, class Loader>
static ErrorOr<void> test_roundtrip(Gfx::Bitmap const& bitmap)
{
    auto decoded = TRY((get_roundtrip_bitmap<Writer, Loader>(bitmap)));

    for (int y = 0; y < bitmap.height(); ++y)
        for (int x = 0; x < bitmap.width(); ++x)
            EXPECT_EQ(decoded->get_pixel(x, y), bitmap.get_pixel(x, y));

    return {};
}

static ErrorOr<AK::NonnullRefPtr<Gfx::Bitmap>> create_test_rgb_bitmap()
{
    auto bitmap = TRY(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRA8888, { 47, 33 }));

    for (int y = 0; y < bitmap->height(); ++y)
        for (int x = 0; x < bitmap->width(); ++x)
            bitmap->set_pixel(x, y, Gfx::Color((x * 255) / bitmap->width(), (y * 255) / bitmap->height(), x + y));

    return bitmap;
}

static ErrorOr<AK::NonnullRefPtr<Gfx::Bitmap>> create_test_rgba_bitmap()
{
    auto bitmap = TRY(create_test_rgb_bitmap());

    for (int y = 0; y < bitmap->height(); ++y) {
        for (int x = 0; x < bitmap->width(); ++x) {
            Color pixel = bitmap->get_pixel(x, y);
            pixel.set_alpha(255 - x);
            bitmap->set_pixel(x, y, pixel);
        }
    }

    return bitmap;
}

TEST_CASE(test_bmp)
{
    TRY_OR_FAIL((test_roundtrip<Gfx::BMPWriter, Gfx::BMPImageDecoderPlugin>(TRY_OR_FAIL(create_test_rgb_bitmap()))));
    TRY_OR_FAIL((test_roundtrip<Gfx::BMPWriter, Gfx::BMPImageDecoderPlugin>(TRY_OR_FAIL(create_test_rgba_bitmap()))));
}

TEST_CASE(test_jpeg)
{
    // JPEG is lossy, so the roundtripped bitmap won't match the original bitmap. But it should still have the same size.
    (void)TRY_OR_FAIL((get_roundtrip_bitmap<Gfx::JPEGWriter, Gfx::JPEGImageDecoderPlugin>(TRY_OR_FAIL(create_test_rgb_bitmap()))));
}

TEST_CASE(test_png)
{
    TRY_OR_FAIL((test_roundtrip<Gfx::PNGWriter, Gfx::PNGImageDecoderPlugin>(TRY_OR_FAIL(create_test_rgb_bitmap()))));
    TRY_OR_FAIL((test_roundtrip<Gfx::PNGWriter, Gfx::PNGImageDecoderPlugin>(TRY_OR_FAIL(create_test_rgba_bitmap()))));
}

TEST_CASE(test_qoi)
{
    TRY_OR_FAIL((test_roundtrip<Gfx::QOIWriter, Gfx::QOIImageDecoderPlugin>(TRY_OR_FAIL(create_test_rgb_bitmap()))));
    TRY_OR_FAIL((test_roundtrip<Gfx::QOIWriter, Gfx::QOIImageDecoderPlugin>(TRY_OR_FAIL(create_test_rgba_bitmap()))));
}

TEST_CASE(test_webp)
{
    TRY_OR_FAIL((test_roundtrip<Gfx::WebPWriter, Gfx::WebPImageDecoderPlugin>(TRY_OR_FAIL(create_test_rgb_bitmap()))));
    TRY_OR_FAIL((test_roundtrip<Gfx::WebPWriter, Gfx::WebPImageDecoderPlugin>(TRY_OR_FAIL(create_test_rgba_bitmap()))));
}
