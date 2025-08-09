#ifndef WUFFS_IMG_H_
#define WUFFS_IMG_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef WUFFS_IMG_BUILD
    #define WUFFS_IMG_API __declspec(dllexport)
  #else
    #define WUFFS_IMG_API __declspec(dllimport)
  #endif
#else
  #define WUFFS_IMG_API
#endif

// Decodes an image (JPEG/PNG/GIF/WebP still) from memory into BGRA premultiplied
// pixels. The output buffer is allocated with malloc and must be freed with
// wuffs_img_free. Returns 0 on success; negative values on failure.
WUFFS_IMG_API int wuffs_img_decode_bgra_premul(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);

WUFFS_IMG_API void wuffs_img_free(void* p);

// Per-format decoders: force decoding via the specified codec.
WUFFS_IMG_API int wuffs_img_decode_jpeg_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);

WUFFS_IMG_API int wuffs_img_decode_png_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);

WUFFS_IMG_API int wuffs_img_decode_gif_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);

// Decode all frames of a GIF. Returns an array of frame buffers (each width*height*4 bytes, BGRA premul).
// Optionally returns per-frame delays in milliseconds. Caller must free with wuffs_img_free_gif_frames.
WUFFS_IMG_API int wuffs_img_decode_gif_bgra_frames(
    const uint8_t* data,
    size_t data_len,
    uint8_t*** out_frame_ptrs,
    uint32_t** out_delays_ms, // can be NULL
    int* out_count,
    int* out_width,
    int* out_height);

WUFFS_IMG_API void wuffs_img_free_gif_frames(
    uint8_t** frame_ptrs,
    uint32_t* delays_ms,
    int count);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WUFFS_IMG_H_