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

// Fast JPEG decode: lower quality (e.g. faster IDCT / upsampling) for speed.
extern "C" WUFFS_IMG_API int wuffs_img_decode_jpeg_bgra_fast(
    const uint8_t* data,
    size_t data_len,
    uint8_t** out_pixels,
    int* out_width,
    int* out_height) {
  if (!data || data_len == 0 || !out_pixels || !out_width || !out_height) {
    return -1;
  }
  wuffs_jpeg__decoder jpeg = {};
  wuffs_base__status s = wuffs_jpeg__decoder__initialize(
      &jpeg, sizeof jpeg, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
  if (s.repr) {
    return -10;
  }
  // Apply lower quality quirk for speed.
  wuffs_jpeg__decoder__set_quirk(&jpeg, WUFFS_BASE__QUIRK_QUALITY,
                                 WUFFS_BASE__QUIRK_QUALITY__VALUE__LOWER_QUALITY);
  return decode_with_image_decoder(
      wuffs_jpeg__decoder__upcast_as__wuffs_base__image_decoder(&jpeg), data,
      data_len, out_pixels, out_width, out_height);
}

// Decode JPEG into a caller-provided BGRA_PREMUL buffer (stride in bytes).
extern "C" WUFFS_IMG_API int wuffs_img_decode_jpeg_bgra_into(
    const uint8_t* data,
    size_t data_len,
    uint8_t* dst_pixels,
    size_t dst_stride,
    int* out_width,
    int* out_height) {
  if (!data || data_len == 0 || !dst_pixels || dst_stride == 0 || !out_width ||
      !out_height) {
    return -1;
  }
  wuffs_jpeg__decoder dec = {};
  wuffs_base__status s = wuffs_jpeg__decoder__initialize(
      &dec, sizeof dec, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
  if (s.repr) {
    return -10;
  }
  wuffs_base__io_buffer src = wuffs_base__ptr_u8__reader(
      (uint8_t*)data, (size_t)data_len, true);

  wuffs_base__image_config ic{};
  s = wuffs_jpeg__decoder__decode_image_config(&dec, &ic, &src);
  if (s.repr) {
    return -2;
  }
  uint32_t width = wuffs_base__pixel_config__width(&ic.pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&ic.pixcfg);
  wuffs_base__pixel_config__set(&ic.pixcfg, WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
                                WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width, height);
  // Bind pixel buffer to caller memory (allows non-tight stride).
  wuffs_base__pixel_buffer pb{};
  wuffs_base__status spb = wuffs_base__pixel_buffer__set_interleaved(
      &pb, &ic.pixcfg,
      wuffs_base__make_table_u8(dst_pixels, (size_t)width * 4u, (size_t)height,
                                dst_stride),
      wuffs_base__empty_slice_u8());
  if (spb.repr) {
    return -3;
  }
  // Need frame_config first to pick blend and workbuf.
  wuffs_base__frame_config fc{};
  s = wuffs_jpeg__decoder__decode_frame_config(&dec, &fc, &src);
  if (s.repr && (s.repr != wuffs_base__note__end_of_data)) {
    return -4;
  }
  wuffs_base__pixel_blend blend = WUFFS_BASE__PIXEL_BLEND__SRC;
  wuffs_base__range_ii_u64 wr = wuffs_jpeg__decoder__workbuf_len(&dec);
  size_t work_len = (size_t)wr.min_incl;
  uint8_t* work_mem = work_len ? (uint8_t*)malloc(work_len) : nullptr;
  wuffs_base__status df = wuffs_jpeg__decoder__decode_frame(
      &dec, &pb, &src, blend, wuffs_base__make_slice_u8(work_mem, work_len),
      NULL);
  if (work_mem) free(work_mem);
  if (df.repr) {
    return -5;
  }
  *out_width = (int)width;
  *out_height = (int)height;
  return 0;
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

// Decode a GIF into an array of BGRA_PREMUL frames and optional per-frame delays (milliseconds).
extern "C" WUFFS_IMG_API int wuffs_img_decode_gif_bgra_frames(
    const uint8_t* data,
    size_t data_len,
    uint8_t*** out_frame_ptrs,
    uint32_t** out_delays_ms, // can be NULL if caller doesn't need delays
    int* out_count,
    int* out_width,
    int* out_height) {
  if (!data || (data_len == 0) || !out_frame_ptrs || !out_count || !out_width ||
      !out_height) {
    return -1;
  }

  *out_frame_ptrs = nullptr;
  if (out_delays_ms) {
    *out_delays_ms = nullptr;
  }
  *out_count = 0;
  *out_width = 0;
  *out_height = 0;

  // Helper to reset an io buffer over the same data.
  auto make_src = [&](wuffs_base__io_buffer* src) {
    src->data.ptr = const_cast<uint8_t*>(data);
    src->data.len = data_len;
    src->meta.wi = data_len;
    src->meta.ri = 0;
    src->meta.pos = 0;
    src->meta.closed = true;
  };

  // First pass: count frames.
  int frame_count = 0;
  {
    wuffs_gif__decoder dec = {};
    wuffs_base__status st = wuffs_gif__decoder__initialize(
        &dec, sizeof dec, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
    if (st.repr) {
      return -2;
    }
    wuffs_base__io_buffer src{};
    make_src(&src);

    wuffs_base__image_config ic{};
    st = wuffs_gif__decoder__decode_image_config(&dec, &ic, &src);
    if (st.repr) {
      return -3;
    }

    while (true) {
      wuffs_base__frame_config fc{};
      wuffs_base__status sfc = wuffs_gif__decoder__decode_frame_config(&dec, &fc, &src);
      if (sfc.repr == wuffs_base__note__end_of_data) {
        break;
      }
      if (sfc.repr) {
        return -4;
      }
      frame_count++;
    }
  }

  if (frame_count <= 0) {
    return -5;
  }

  // Second pass: decode frames.
  int ret_err = 0;
  uint8_t** frames = (uint8_t**)calloc((size_t)frame_count, sizeof(uint8_t*));
  uint32_t* delays = out_delays_ms ? (uint32_t*)calloc((size_t)frame_count, sizeof(uint32_t)) : nullptr;
  if ((!frames) || (out_delays_ms && !delays)) {
    free(frames);
    free(delays);
    return -6; // OOM
  }

  wuffs_gif__decoder dec = {};
  wuffs_base__status st0 = wuffs_gif__decoder__initialize(
      &dec, sizeof dec, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
  if (st0.repr) {
    free(frames);
    free(delays);
    return -7;
  }

  wuffs_base__io_buffer src2{};
  make_src(&src2);

  wuffs_base__image_config ic{};
  st0 = wuffs_gif__decoder__decode_image_config(&dec, &ic, &src2);
  if (st0.repr || !wuffs_base__image_config__is_valid(&ic)) {
    free(frames);
    free(delays);
    return -8;
  }
  uint32_t width = wuffs_base__pixel_config__width(&ic.pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&ic.pixcfg);

  // Force BGRA_PREMUL destination.
  wuffs_base__pixel_config__set(&ic.pixcfg, WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
                                WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width, height);

  size_t stride = (size_t)width * 4u;
  size_t dst_len = stride * (size_t)height;

  uint8_t* curr = (uint8_t*)calloc(dst_len, 1);
  uint8_t* prev = (uint8_t*)malloc(dst_len);
  if (!curr || !prev) {
    free(curr);
    free(prev);
    free(frames);
    free(delays);
    return -9;
  }

  // Work buffer
  wuffs_base__range_ii_u64 wr = wuffs_gif__decoder__workbuf_len(&dec);
  size_t work_len = (size_t)wr.min_incl;
  uint8_t* work_mem = work_len ? (uint8_t*)malloc(work_len) : nullptr;
  if (work_len && !work_mem) {
    free(curr);
    free(prev);
    free(frames);
    free(delays);
    return -10;
  }

  // Pixel buffer bound to curr surface.
  wuffs_base__pixel_buffer pb{};
  wuffs_base__status sfs = wuffs_base__pixel_buffer__set_from_slice(
      &pb, &ic.pixcfg, wuffs_base__make_slice_u8(curr, dst_len));
  if (sfs.repr) {
    free(work_mem);
    free(curr);
    free(prev);
    free(frames);
    free(delays);
    return -11;
  }

  int out_index = 0;
  const uint64_t FLICKS_PER_MILLISECOND = 705600ULL;

  while (true) {
    wuffs_base__frame_config fc{};
    wuffs_base__status sfc = wuffs_gif__decoder__decode_frame_config(&dec, &fc, &src2);
    if (sfc.repr == wuffs_base__note__end_of_data) {
      break;
    }
    if (sfc.repr) {
      ret_err = -12;
      break;
    }

    if (wuffs_base__frame_config__index(&fc) == 0) {
      // Initialize background for first frame.
      wuffs_base__color_u32_argb_premul bg =
          wuffs_base__frame_config__background_color(&fc);
      uint8_t* p = curr;
      size_t n = dst_len / sizeof(wuffs_base__color_u32_argb_premul);
      for (size_t i = 0; i < n; i++) {
        wuffs_base__poke_u32le__no_bounds_check(p, bg);
        p += sizeof(wuffs_base__color_u32_argb_premul);
      }
    }

    // Save previous if disposal=RESTORE_PREVIOUS
    if (wuffs_base__frame_config__disposal(&fc) ==
        WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS) {
      memcpy(prev, curr, dst_len);
    }

    // Blend mode
    wuffs_base__pixel_blend blend =
        wuffs_base__frame_config__overwrite_instead_of_blend(&fc)
            ? WUFFS_BASE__PIXEL_BLEND__SRC
            : WUFFS_BASE__PIXEL_BLEND__SRC_OVER;

    // Decode frame pixels.
    wuffs_base__status df = wuffs_gif__decoder__decode_frame(
        &dec, &pb, &src2, blend, wuffs_base__make_slice_u8(work_mem, work_len),
        NULL);
    if (df.repr) {
      ret_err = -13;
      break;
    }

    // Snapshot current composited frame.
    uint8_t* frame_copy = (uint8_t*)malloc(dst_len);
    if (!frame_copy) {
      ret_err = -14;
      break;
    }
    memcpy(frame_copy, curr, dst_len);
    frames[out_index] = frame_copy;

    if (delays) {
      wuffs_base__flicks d = wuffs_base__frame_config__duration(&fc);
      uint64_t ms = (d <= 0) ? 0u : (uint64_t)(d / FLICKS_PER_MILLISECOND);
      if (ms > 0xFFFFFFFFULL) {
        ms = 0xFFFFFFFFULL;
      }
      delays[out_index] = (uint32_t)ms;
    }

    out_index++;

    // Apply disposal after presenting frame.
    switch (wuffs_base__frame_config__disposal(&fc)) {
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_BACKGROUND: {
        wuffs_base__rect_ie_u32 bounds = wuffs_base__frame_config__bounds(&fc);
        wuffs_base__color_u32_argb_premul bg =
            wuffs_base__frame_config__background_color(&fc);
        // Fill bounds region with background color.
        for (uint32_t y = bounds.min_incl_y; y < bounds.max_excl_y; y++) {
          uint8_t* row = curr + (size_t)y * stride + (size_t)bounds.min_incl_x * 4u;
          for (uint32_t x = bounds.min_incl_x; x < bounds.max_excl_x; x++) {
            wuffs_base__poke_u32le__no_bounds_check(row, bg);
            row += 4;
          }
        }
        break;
      }
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS: {
        // Swap curr and prev, and rebind pixel buffer to new curr.
        uint8_t* swap = curr;
        curr = prev;
        prev = swap;
        wuffs_base__status sfs2 = wuffs_base__pixel_buffer__set_from_slice(
            &pb, &ic.pixcfg, wuffs_base__make_slice_u8(curr, dst_len));
        if (sfs2.repr) {
          ret_err = -15;
        }
        break;
      }
      default:
        break;
    }
    if (ret_err) {
      break;
    }
  }

  free(work_mem);
  free(prev);
  free(curr);

  if (ret_err) {
    for (int i = 0; i < frame_count; i++) {
      free(frames[i]);
    }
    free(frames);
    free(delays);
    return ret_err;
  }

  *out_frame_ptrs = frames;
  if (out_delays_ms) {
    *out_delays_ms = delays;
  } else {
    free(delays);
  }
  *out_count = frame_count;
  *out_width = (int)width;
  *out_height = (int)height;
  return 0;
}

extern "C" WUFFS_IMG_API void wuffs_img_free_gif_frames(
    uint8_t** frame_ptrs,
    uint32_t* delays_ms,
    int count) {
  if (frame_ptrs) {
    for (int i = 0; i < count; i++) {
      free(frame_ptrs[i]);
    }
    free(frame_ptrs);
  }
  if (delays_ms) {
    free(delays_ms);
  }
}

// ---------------- Probe (get width/height without decoding pixels) ----------------

static int probe_with_decoder(wuffs_base__image_decoder* image_decoder,
                              const uint8_t* data,
                              size_t data_len,
                              int* out_width,
                              int* out_height) {
  if (!data || (data_len == 0) || !out_width || !out_height) {
    return -1;
  }
  wuffs_base__io_buffer src = wuffs_base__ptr_u8__reader(
      (uint8_t*)data, (size_t)data_len, true /* closed */);
  wuffs_base__image_config ic{};
  wuffs_base__status st =
      wuffs_base__image_decoder__decode_image_config(image_decoder, &ic, &src);
  if (st.repr || !wuffs_base__image_config__is_valid(&ic)) {
    return -2;
  }
  *out_width = (int)wuffs_base__pixel_config__width(&ic.pixcfg);
  *out_height = (int)wuffs_base__pixel_config__height(&ic.pixcfg);
  return 0;
}

extern "C" WUFFS_IMG_API int wuffs_img_probe_jpeg(const uint8_t* data,
                                                   size_t data_len,
                                                   int* out_width,
                                                   int* out_height) {
  wuffs_jpeg__decoder dec = {};
  wuffs_base__status s = wuffs_jpeg__decoder__initialize(
      &dec, sizeof dec, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
  if (s.repr) {
    return -10;
  }
  return probe_with_decoder(
      wuffs_jpeg__decoder__upcast_as__wuffs_base__image_decoder(&dec), data,
      data_len, out_width, out_height);
}

extern "C" WUFFS_IMG_API int wuffs_img_probe_png(const uint8_t* data,
                                                  size_t data_len,
                                                  int* out_width,
                                                  int* out_height) {
  wuffs_png__decoder dec = {};
  wuffs_base__status s = wuffs_png__decoder__initialize(
      &dec, sizeof dec, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
  if (s.repr) {
    return -10;
  }
  return probe_with_decoder(
      wuffs_png__decoder__upcast_as__wuffs_base__image_decoder(&dec), data,
      data_len, out_width, out_height);
}

extern "C" WUFFS_IMG_API int wuffs_img_probe_gif(const uint8_t* data,
                                                  size_t data_len,
                                                  int* out_width,
                                                  int* out_height) {
  wuffs_gif__decoder dec = {};
  wuffs_base__status s = wuffs_gif__decoder__initialize(
      &dec, sizeof dec, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
  if (s.repr) {
    return -10;
  }
  return probe_with_decoder(
      wuffs_gif__decoder__upcast_as__wuffs_base__image_decoder(&dec), data,
      data_len, out_width, out_height);
}

static inline bool has_jpeg_magic(const uint8_t* p, size_t n) {
  return (n >= 3) && (p[0] == 0xFF) && (p[1] == 0xD8) && (p[2] == 0xFF);
}
static inline bool has_png_magic(const uint8_t* p, size_t n) {
  static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  return (n >= 8) && (memcmp(p, sig, 8) == 0);
}
static inline bool has_gif_magic(const uint8_t* p, size_t n) {
  return (n >= 6) && (!memcmp(p, "GIF87a", 6) || !memcmp(p, "GIF89a", 6));
}

extern "C" WUFFS_IMG_API int wuffs_img_probe(const uint8_t* data,
                                              size_t data_len,
                                              int* out_width,
                                              int* out_height) {
  if (!data || (data_len == 0) || !out_width || !out_height) {
    return -1;
  }
  const uint8_t* p = data;
  size_t n = data_len;
  if (has_png_magic(p, n)) {
    return wuffs_img_probe_png(data, data_len, out_width, out_height);
  } else if (has_jpeg_magic(p, n)) {
    return wuffs_img_probe_jpeg(data, data_len, out_width, out_height);
  } else if (has_gif_magic(p, n)) {
    return wuffs_img_probe_gif(data, data_len, out_width, out_height);
  }
  // Fallback: try in order.
  if (wuffs_img_probe_png(data, data_len, out_width, out_height) == 0) return 0;
  if (wuffs_img_probe_jpeg(data, data_len, out_width, out_height) == 0) return 0;
  if (wuffs_img_probe_gif(data, data_len, out_width, out_height) == 0) return 0;
  return -2;
}