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

// GIF frames (allocates one buffer per frame; free with wuffs_img_free_gif_frames)
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

// Probes (width/height only)
WUFFS_IMG_API int wuffs_img_probe(const uint8_t* data, size_t data_len,
                                  int* out_width, int* out_height);
WUFFS_IMG_API int wuffs_img_probe_jpeg(const uint8_t* data, size_t data_len,
                                       int* out_width, int* out_height);
WUFFS_IMG_API int wuffs_img_probe_png(const uint8_t* data, size_t data_len,
                                      int* out_width, int* out_height);
WUFFS_IMG_API int wuffs_img_probe_gif(const uint8_t* data, size_t data_len,
                                      int* out_width, int* out_height);

// Windows Heap-based decode (library allocates via HeapAlloc, caller frees via HeapFree)
// All return pointer to heap memory and set out_size = width*height*4 on success; negative on failure.
WUFFS_IMG_API intptr_t wuffs_heap_decode_jpeg_bgra(const uint8_t* data, size_t data_len, int* out_width, int* out_height, size_t* out_size);
WUFFS_IMG_API intptr_t wuffs_heap_decode_png_bgra(const uint8_t* data, size_t data_len, int* out_width, int* out_height, size_t* out_size);
WUFFS_IMG_API intptr_t wuffs_heap_decode_gif_bgra(const uint8_t* data, size_t data_len, int* out_width, int* out_height, size_t* out_size);
WUFFS_IMG_API intptr_t wuffs_heap_decode_bmp_bgra(const uint8_t* data, size_t data_len, int* out_width, int* out_height, size_t* out_size);
WUFFS_IMG_API intptr_t wuffs_heap_decode_webp_bgra(const uint8_t* data, size_t data_len, int* out_width, int* out_height, size_t* out_size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // WUFFS_IMG_H_