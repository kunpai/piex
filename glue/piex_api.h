/* piex_api.h -- a C API over piex's C++ API, the one both sides of crbound's piex run export.
 *
 * piex (src/) has only a C++ API. Its Rust port is a drop-in for that API: rust/include/
 * piex_rust.h declares the same functions under namespace piex_rust, taking the same piex::
 * types, through the port's cxx bridge. piex_api.inc implements this header once, over a
 * namespace PIEX_NS; piex_api_c.cc compiles it over piex (into the library's archive) and
 * piex_api_rs.cc over piex_rust (into the port's), so the two copies are the same code and
 * differ only in which implementation each call reaches.
 *
 * One C function per C++ API function, in piex_rust.h's order. A pointer stays nullable: NULL
 * reaches the C++ function as nullptr. A stream is the caller's bytes; its GetData copies
 * `length` bytes from `offset`, or answers kFail with the buffer untouched when offset +
 * length passes their end (piex_types.h's contract). A piexc_preview is read in as the
 * PreviewImageData the call is given and written back after it, so a field the call leaves
 * alone comes back as it went in.
 */
#ifndef PIEX_API_H
#define PIEX_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIEXC_TEXT_MAX 255
#define PIEXC_U32S_MAX 4

typedef struct piexc_stream {
	const uint8_t *data;
	size_t len;
} piexc_stream;

/* A std::string: its length, the FNV-1a 64 hash of all of it, and its first PIEXC_TEXT_MAX
 * bytes then a NUL. Read in, it is `head` up to its first NUL. */
typedef struct piexc_text {
	uint64_t len;
	uint64_t fnv;
	char head[PIEXC_TEXT_MAX + 1];
} piexc_text;

/* A std::vector<std::uint32_t>: its size and its first PIEXC_U32S_MAX elements. Read in, it
 * is the first min(len, PIEXC_U32S_MAX) of `v`. */
typedef struct piexc_u32s {
	uint64_t len;
	uint32_t v[PIEXC_U32S_MAX];
} piexc_u32s;

typedef struct piexc_image {          /* piex::Image */
	uint16_t width;
	uint16_t height;
	uint32_t length;
	uint32_t offset;
	int32_t format;                   /* Image::Format */
} piexc_image;

typedef struct piexc_rational {       /* PreviewImageData::Rational */
	uint32_t numerator;
	uint32_t denominator;
} piexc_rational;

typedef struct piexc_gps {            /* PreviewImageData::Gps */
	bool is_valid;
	char latitude_ref;
	piexc_rational latitude[3];
	char longitude_ref;
	piexc_rational longitude[3];
	bool altitude_ref;
	piexc_rational altitude;
	piexc_rational time_stamp[3];
	piexc_text date_stamp;
} piexc_gps;

typedef struct piexc_preview {        /* piex::PreviewImageData */
	piexc_image preview;
	piexc_image thumbnail;
	uint32_t exif_orientation;
	int32_t color_space;              /* PreviewImageData::ColorSpace */
	uint32_t full_width;
	uint32_t full_height;
	piexc_text maker;
	piexc_text model;
	piexc_text date_time;
	uint32_t iso;
	piexc_rational exposure_time;
	piexc_rational fnumber;
	piexc_rational focal_length;
	piexc_gps gps;
	piexc_u32s cfa_pattern_dim;
} piexc_preview;

/* piex.h / piex_rust.h. An Error is returned as its value: 0 kOk, 1 kFail, 2 kUnsupported.
 * A RawImageTypes is its enumerator's value (image_type_recognition_lite.h). */
size_t piexc_bytes_required_for_is_raw(void);
bool piexc_is_raw(const piexc_stream *data);
/* output_type, when not NULL, is read in as the RawImageTypes passed and written back */
int32_t piexc_get_preview_image_data(const piexc_stream *data, piexc_preview *preview_image_data,
                                     int32_t *output_type);
bool piexc_get_dng_information(const piexc_stream *data, uint32_t *width, uint32_t *height,
                               piexc_u32s *cfa_pattern_dim);
bool piexc_get_orientation(const piexc_stream *data, uint32_t *orientation);
/* the number of extensions; `joined`, when not NULL, gets them joined by ',' */
uint64_t piexc_supported_extensions(piexc_text *joined);

/* piex_cr3.h / piex_rust.h's CR3 APIs */
bool piexc_cr3_get_orientation(const piexc_stream *stream, uint32_t *orientation);
int32_t piexc_cr3_get_preview_data(const piexc_stream *stream, piexc_preview *preview_image_data);

/* image_type_recognition_lite.h: the source is a RangeCheckedBytePtr over (data, len) */
int32_t piexc_recognize_raw_image_type_lite(const uint8_t *data, size_t len);
bool piexc_is_raw_type(int32_t type);
bool piexc_is_of_type(const uint8_t *data, size_t len, int32_t type);
size_t piexc_get_number_of_bytes_for_is_raw_lite(void);
size_t piexc_get_number_of_bytes_for_is_of_type(int32_t type);
bool piexc_is_raw_lite(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
