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

// Auto-detect decode into BGRA (allocates; free with wuffs_img_free)
WUFFS_IMG_API int wuffs_img_decode_bgra_premul(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
// Auto-detect decode into RGBA (allocates; free with wuffs_img_free)
WUFFS_IMG_API int wuffs_img_decode_rgba_premul(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);

// Free memory allocated by the library (malloc-based APIs)
WUFFS_IMG_API void wuffs_img_free(void* p);

// JPEG
WUFFS_IMG_API int wuffs_img_decode_jpeg_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_jpeg_bgra_fast(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_jpeg_bgra_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_jpeg_rgba(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_jpeg_rgba_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height);

// PNG
WUFFS_IMG_API int wuffs_img_decode_png_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_png_bgra_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_png_rgba(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_png_rgba_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height);

// GIF (still: first frame; animation: see frames API)
WUFFS_IMG_API int wuffs_img_decode_gif_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_gif_bgra_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_gif_rgba(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_gif_rgba_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height);

// GIF frames (allocates one buffer per frame; free with wuffs_img_free_gif_frames)
WUFFS_IMG_API int wuffs_img_decode_gif_bgra_frames(
    const uint8_t* data,
    size_t data_len,
    uint8_t*** out_frame_ptrs,
    uint32_t** out_delays_ms, // can be NULL
    int* out_count,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_gif_rgba_frames(
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

// BMP
WUFFS_IMG_API int wuffs_img_decode_bmp_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_bmp_bgra_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_bmp_rgba(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_bmp_rgba_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height);

// WEBP (still frames)
WUFFS_IMG_API int wuffs_img_decode_webp_bgra(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_webp_bgra_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_webp_rgba(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height);
WUFFS_IMG_API int wuffs_img_decode_webp_rgba_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height);

// Probes (width/height only)
WUFFS_IMG_API int wuffs_img_probe(const uint8_t* data, size_t data_len,
                                  int* out_width, int* out_height,
                                  char* out_ext, size_t out_ext_len,
                                  char* out_error, size_t out_error_len);
WUFFS_IMG_API int wuffs_img_probe_jpeg(const uint8_t* data, size_t data_len,
                                       int* out_width, int* out_height);
WUFFS_IMG_API int wuffs_img_probe_png(const uint8_t* data, size_t data_len,
                                      int* out_width, int* out_height);
WUFFS_IMG_API int wuffs_img_probe_gif(const uint8_t* data, size_t data_len,
                                      int* out_width, int* out_height);

// Unified auto-parse and decode to RGBA_PREMUL (allocates; free with wuffs_img_free)
// Returns: 0 on success; 1 if special case (e.g. multi-frame GIF) so not decoded; negative on error
WUFFS_IMG_API int wuffs_img_decode_auto_rgba_alloc(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    size_t* out_size,
    int* out_width,
    int* out_height,
    char* out_ext,
    size_t out_ext_len,
    char* out_error,
    size_t out_error_len);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WUFFS_IMG_H_