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
  wuffs_base__table_u8 plane0 = result.pixbuf.plane(0);

  const size_t expected_stride = static_cast<size_t>(width) * 4;
  const size_t dst_size = expected_stride * static_cast<size_t>(height);

  uint8_t* dst = static_cast<uint8_t*>(malloc(dst_size));
  if (!dst) {
    return -5; // out of memory
  }

  if (plane0.width == plane0.stride) {
    // Tight buffer; copy in one go.
    if (plane0.height != dst_size) {
      // Sanity check; fall back to row copy if mismatch.
      for (uint32_t y = 0; y < height; y++) {
        const uint8_t* src_row = plane0.ptr + y * plane0.stride;
        uint8_t* dst_row = dst + y * expected_stride;
        memcpy(dst_row, src_row, expected_stride);
      }
    } else {
      memcpy(dst, plane0.ptr, dst_size);
    }
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