#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Ensure C linkage for exported functions when compiled as C++
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define WUFFS_IMG_API __declspec(dllexport)
#else
#define WUFFS_IMG_API
#endif

// Return 0 on success; negative values indicate failure.
WUFFS_IMG_API int wuffs_img_decode_bgra_premul(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels, // malloc'ed buffer the caller must free via wuffs_img_free
    int* out_width,
    int* out_height);

WUFFS_IMG_API void wuffs_img_free(void* p);

#ifdef __cplusplus
}  // extern "C"
#endif

// ---------------- Implementation ----------------

#define WUFFS_IMPLEMENTATION
#define WUFFS_CONFIG__STATIC_FUNCTIONS
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__AUX__BASE
#define WUFFS_CONFIG__MODULE__AUX__IMAGE
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__ADLER32
#define WUFFS_CONFIG__MODULE__CRC32
#define WUFFS_CONFIG__MODULE__DEFLATE
#define WUFFS_CONFIG__MODULE__GIF
#define WUFFS_CONFIG__MODULE__JPEG
#define WUFFS_CONFIG__MODULE__PNG
#define WUFFS_CONFIG__MODULE__ZLIB

#define WUFFS_CONFIG__DST_PIXEL_FORMAT__ENABLE_ALLOWLIST
#define WUFFS_CONFIG__DST_PIXEL_FORMAT__ALLOW_BGRA_PREMUL

// Include the amalgamated Wuffs release. This file contains both C and C++
// (wuffs_aux) code; we therefore compile this translation unit as C++.
#include "../release/c/wuffs-unsupported-snapshot.c"

extern "C" WUFFS_IMG_API int wuffs_img_decode_bgra_premul(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height) {
  if (!data || data_len == 0 || !out_pixels || !out_width || !out_height) {
    return -1;
  }

  *out_pixels = nullptr;
  *out_width = 0;
  *out_height = 0;

  wuffs_aux::sync_io::MemoryInput input(data, data_len);

  uint64_t dia_flags = 0; // no metadata handling here

  wuffs_aux::DecodeImageCallbacks callbacks;
  wuffs_aux::DecodeImageResult result = wuffs_aux::DecodeImage(
      callbacks,
      input,
      wuffs_aux::DecodeImageArgQuirks(nullptr, 0),
      wuffs_aux::DecodeImageArgFlags(dia_flags),
      wuffs_aux::DecodeImageArgPixelBlend(WUFFS_BASE__PIXEL_BLEND__SRC),
      wuffs_aux::DecodeImageArgBackgroundColor(0),
      wuffs_aux::DecodeImageArgMaxInclDimension(16384));

  if (!result.error_message.empty()) {
    return -2; // decode error
  }
  
  // Acquire the pixel buffer before local objects go out of scope.
  wuffs_base__pixel_buffer pixbuf = result.pixbuf;
  (void)pixbuf;

  if (!result.pixbuf.pixcfg.is_valid()) {
    return -3; // invalid pixel buffer
  }

  if (!result.pixbuf.pixcfg.pixel_format().is_interleaved() ||
      (result.pixbuf.pixcfg.pixel_format().repr !=
       WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL)) {
    return -4; // unexpected pixel format
  }

  const uint32_t width = result.pixbuf.pixcfg.width();
  const uint32_t height = result.pixbuf.pixcfg.height();
  wuffs_base__table_u8 plane0 = pixbuf.plane(0);

  const size_t expected_stride = static_cast<size_t>(width) * 4;
  const size_t dst_size = expected_stride * static_cast<size_t>(height);

  uint8_t* dst = static_cast<uint8_t*>(malloc(dst_size));
  if (!dst) {
    return -5; // out of memory
  }

  if (plane0.width == expected_stride) {
    // Tight buffer; copy in one go.
    memcpy(dst, plane0.ptr, dst_size);
  } else {
    // Not tight; copy row by row.
    for (uint32_t y = 0; y < height; y++) {
      const uint8_t* src_row = plane0.ptr + y * plane0.stride;
      uint8_t* dst_row = dst + y * expected_stride;
      memcpy(dst_row, src_row, expected_stride);
    }
  }

  // Transfer ownership to caller.
  *out_pixels = dst;
  *out_width = static_cast<int>(width);
  *out_height = static_cast<int>(height);

  return 0;
}

extern "C" WUFFS_IMG_API void wuffs_img_free(void* p) {
  if (p) {
    free(p);
  }
}

// ---------------- Per-format decoders (JPEG/PNG/GIF) ----------------

static int decode_with_image_decoder(
    wuffs_base__image_decoder* image_decoder,
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height) {
  if (!data || data_len == 0 || !out_pixels || !out_width || !out_height) {
    return -1;
  }

  *out_pixels = nullptr;
  *out_width = 0;
  *out_height = 0;

  wuffs_base__io_buffer src = wuffs_base__ptr_u8__reader(
      (uint8_t*)data, (size_t)data_len, true /* closed */);

  wuffs_base__image_config ic{};
  wuffs_base__status status =
      wuffs_base__image_decoder__decode_image_config(image_decoder, &ic, &src);
  if (status.repr) {
    return -2;
  }

  uint32_t width = wuffs_base__pixel_config__width(&ic.pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&ic.pixcfg);

  wuffs_base__pixel_config__set(&ic.pixcfg,
                                WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
                                WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width,
                                height);

  size_t stride = (size_t)width * 4u;
  size_t dst_size = stride * (size_t)height;
  uint8_t* dst = (uint8_t*)malloc(dst_size);
  if (!dst) {
    return -5;
  }

  wuffs_base__pixel_buffer pb{};
  status = wuffs_base__pixel_buffer__set_from_slice(
      &pb, &ic.pixcfg, wuffs_base__make_slice_u8(dst, dst_size));
  if (status.repr) {
    free(dst);
    return -6;
  }

  // Decode only the first frame.
  wuffs_base__status status_fc;
  wuffs_base__frame_config fc{};
  status_fc =
      wuffs_base__image_decoder__decode_frame_config(image_decoder, &fc, &src);
  if (status_fc.repr && (status_fc.repr != wuffs_base__note__end_of_data)) {
    free(dst);
    return -7;
  }

  // Choose blend: for first frame, SRC is sufficient.
  wuffs_base__pixel_blend blend = WUFFS_BASE__PIXEL_BLEND__SRC;

  // Allocate work buffer if needed.
  wuffs_base__range_ii_u64 wr =
      wuffs_base__image_decoder__workbuf_len(image_decoder);
  size_t work_len = (size_t)wr.min_incl;
  uint8_t* work_mem = NULL;
  if (work_len) {
    work_mem = (uint8_t*)malloc(work_len);
    if (!work_mem) {
      free(dst);
      return -8;
    }
  }

  status = wuffs_base__image_decoder__decode_frame(
      image_decoder, &pb, &src, blend, wuffs_base__make_slice_u8(work_mem, work_len),
      NULL);

  if (work_mem) {
    free(work_mem);
  }

  if (status.repr) {
    free(dst);
    return -9;
  }

  *out_pixels = dst;
  *out_width = (int)width;
  *out_height = (int)height;
  return 0;
}

extern "C" WUFFS_IMG_API int wuffs_img_decode_jpeg_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height) {
  wuffs_jpeg__decoder jpeg = {};
  wuffs_base__status s = wuffs_jpeg__decoder__initialize(
      &jpeg, sizeof jpeg, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
  if (s.repr) {
    return -10;
  }
  return decode_with_image_decoder(
      wuffs_jpeg__decoder__upcast_as__wuffs_base__image_decoder(&jpeg), data,
      data_len, out_pixels, out_width, out_height);
}

extern "C" WUFFS_IMG_API int wuffs_img_decode_png_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height) {
  wuffs_png__decoder png = {};
  wuffs_base__status s = wuffs_png__decoder__initialize(
      &png, sizeof png, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
  if (s.repr) {
    return -10;
  }
  return decode_with_image_decoder(
      wuffs_png__decoder__upcast_as__wuffs_base__image_decoder(&png), data,
      data_len, out_pixels, out_width, out_height);
}

extern "C" WUFFS_IMG_API int wuffs_img_decode_gif_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height) {
  wuffs_gif__decoder gif = {};
  wuffs_base__status s = wuffs_gif__decoder__initialize(
      &gif, sizeof gif, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
  if (s.repr) {
    return -10;
  }
  return decode_with_image_decoder(
      wuffs_gif__decoder__upcast_as__wuffs_base__image_decoder(&gif), data,
      data_len, out_pixels, out_width, out_height);
}