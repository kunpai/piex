// piex_api.h's implementation over PIEX_NS: piex (piex_api_c.cc) or piex_rust
// (piex_api_rs.cc). The types are piex::'s on both sides, as piex_rust.h takes them, and
// nothing here depends on which namespace it is: the two copies are the same code.

#include <cstring>
#include <string>
#include <vector>

#include "piex_api.h"

#ifndef PIEX_NS
#error "define PIEX_NS (piex or piex_rust) before including piex_api.inc"
#endif

namespace {

using ::piex::Error;
using ::piex::Image;
using ::piex::PreviewImageData;
using ::piex::StreamInterface;
using ::piex::binary_parse::RangeCheckedBytePtr;
using ::piex::image_type_recognition::RawImageTypes;

// The caller's bytes as a StreamInterface.
class BytesStream : public StreamInterface {
 public:
  explicit BytesStream(const piexc_stream* s)
      : data_(s ? s->data : nullptr), len_(s ? s->len : 0) {}

  Error GetData(const size_t offset, const size_t length,
                std::uint8_t* data) override {
    if (offset > len_ || length > len_ - offset) {
      return ::piex::kFail;
    }
    if (length > 0) {
      std::memcpy(data, data_ + offset, length);
    }
    return ::piex::kOk;
  }

 private:
  const std::uint8_t* data_;
  size_t len_;
};

std::uint64_t Fnv(const std::string& s) {
  std::uint64_t h = 14695981039346656037ull;
  for (unsigned char c : s) {
    h ^= c;
    h *= 1099511628211ull;
  }
  return h;
}

void ToText(const std::string& s, piexc_text* t) {
  t->len = s.size();
  t->fnv = Fnv(s);
  const size_t n = s.size() < PIEXC_TEXT_MAX ? s.size() : PIEXC_TEXT_MAX;
  std::memcpy(t->head, s.data(), n);
  t->head[n] = '\0';
}

std::string FromText(const piexc_text& t) {
  return std::string(t.head, strnlen(t.head, PIEXC_TEXT_MAX));
}

void ToU32s(const std::vector<std::uint32_t>& v, piexc_u32s* out) {
  out->len = v.size();
  for (size_t i = 0; i < PIEXC_U32S_MAX; ++i) {
    out->v[i] = i < v.size() ? v[i] : 0;
  }
}

std::vector<std::uint32_t> FromU32s(const piexc_u32s& in) {
  const size_t n = in.len < PIEXC_U32S_MAX ? static_cast<size_t>(in.len) : PIEXC_U32S_MAX;
  return std::vector<std::uint32_t>(in.v, in.v + n);
}

void ToImage(const Image& in, piexc_image* out) {
  out->width = in.width;
  out->height = in.height;
  out->length = in.length;
  out->offset = in.offset;
  out->format = static_cast<std::int32_t>(in.format);
}

Image FromImage(const piexc_image& in) {
  Image out;
  out.width = in.width;
  out.height = in.height;
  out.length = in.length;
  out.offset = in.offset;
  out.format = static_cast<Image::Format>(in.format);
  return out;
}

void ToRational(const PreviewImageData::Rational& in, piexc_rational* out) {
  out->numerator = in.numerator;
  out->denominator = in.denominator;
}

PreviewImageData::Rational FromRational(const piexc_rational& in) {
  PreviewImageData::Rational out;
  out.numerator = in.numerator;
  out.denominator = in.denominator;
  return out;
}

void ToPreview(const PreviewImageData& in, piexc_preview* out) {
  std::memset(out, 0, sizeof(*out));
  ToImage(in.preview, &out->preview);
  ToImage(in.thumbnail, &out->thumbnail);
  out->exif_orientation = in.exif_orientation;
  out->color_space = static_cast<std::int32_t>(in.color_space);
  out->full_width = in.full_width;
  out->full_height = in.full_height;
  ToText(in.maker, &out->maker);
  ToText(in.model, &out->model);
  ToText(in.date_time, &out->date_time);
  out->iso = in.iso;
  ToRational(in.exposure_time, &out->exposure_time);
  ToRational(in.fnumber, &out->fnumber);
  ToRational(in.focal_length, &out->focal_length);
  out->gps.is_valid = in.gps.is_valid;
  out->gps.latitude_ref = in.gps.latitude_ref;
  out->gps.longitude_ref = in.gps.longitude_ref;
  out->gps.altitude_ref = in.gps.altitude_ref;
  ToRational(in.gps.altitude, &out->gps.altitude);
  for (int i = 0; i < 3; ++i) {
    ToRational(in.gps.latitude[i], &out->gps.latitude[i]);
    ToRational(in.gps.longitude[i], &out->gps.longitude[i]);
    ToRational(in.gps.time_stamp[i], &out->gps.time_stamp[i]);
  }
  ToText(in.gps.date_stamp, &out->gps.date_stamp);
  ToU32s(in.cfa_pattern_dim, &out->cfa_pattern_dim);
}

// `{}`: Gps's latitude_ref and longitude_ref have no initializer of their own
PreviewImageData FromPreview(const piexc_preview& in) {
  PreviewImageData out{};
  out.preview = FromImage(in.preview);
  out.thumbnail = FromImage(in.thumbnail);
  out.exif_orientation = in.exif_orientation;
  out.color_space = static_cast<PreviewImageData::ColorSpace>(in.color_space);
  out.full_width = in.full_width;
  out.full_height = in.full_height;
  out.maker = FromText(in.maker);
  out.model = FromText(in.model);
  out.date_time = FromText(in.date_time);
  out.iso = in.iso;
  out.exposure_time = FromRational(in.exposure_time);
  out.fnumber = FromRational(in.fnumber);
  out.focal_length = FromRational(in.focal_length);
  out.gps.is_valid = in.gps.is_valid;
  out.gps.latitude_ref = in.gps.latitude_ref;
  out.gps.longitude_ref = in.gps.longitude_ref;
  out.gps.altitude_ref = in.gps.altitude_ref;
  out.gps.altitude = FromRational(in.gps.altitude);
  for (int i = 0; i < 3; ++i) {
    out.gps.latitude[i] = FromRational(in.gps.latitude[i]);
    out.gps.longitude[i] = FromRational(in.gps.longitude[i]);
    out.gps.time_stamp[i] = FromRational(in.gps.time_stamp[i]);
  }
  out.gps.date_stamp = FromText(in.gps.date_stamp);
  out.cfa_pattern_dim = FromU32s(in.cfa_pattern_dim);
  return out;
}

RangeCheckedBytePtr Source(const uint8_t* data, size_t len) {
  return RangeCheckedBytePtr(data, len);
}

}  // namespace

extern "C" {

size_t piexc_bytes_required_for_is_raw(void) {
  return PIEX_NS::BytesRequiredForIsRaw();
}

bool piexc_is_raw(const piexc_stream* data) {
  BytesStream stream(data);
  return PIEX_NS::IsRaw(data ? &stream : nullptr);
}

int32_t piexc_get_preview_image_data(const piexc_stream* data,
                                     piexc_preview* preview_image_data,
                                     int32_t* output_type) {
  BytesStream stream(data);
  PreviewImageData preview{};
  if (preview_image_data) {
    preview = FromPreview(*preview_image_data);
  }
  RawImageTypes type = output_type ? static_cast<RawImageTypes>(*output_type)
                                   : ::piex::image_type_recognition::kNonRawImage;
  const Error err = PIEX_NS::GetPreviewImageData(
      data ? &stream : nullptr, preview_image_data ? &preview : nullptr,
      output_type ? &type : nullptr);
  if (preview_image_data) {
    ToPreview(preview, preview_image_data);
  }
  if (output_type) {
    *output_type = static_cast<int32_t>(type);
  }
  return static_cast<int32_t>(err);
}

bool piexc_get_dng_information(const piexc_stream* data, uint32_t* width,
                               uint32_t* height, piexc_u32s* cfa_pattern_dim) {
  BytesStream stream(data);
  std::vector<std::uint32_t> cfa;
  if (cfa_pattern_dim) {
    cfa = FromU32s(*cfa_pattern_dim);
  }
  const bool ok = PIEX_NS::GetDngInformation(
      data ? &stream : nullptr, width, height, cfa_pattern_dim ? &cfa : nullptr);
  if (cfa_pattern_dim) {
    ToU32s(cfa, cfa_pattern_dim);
  }
  return ok;
}

bool piexc_get_orientation(const piexc_stream* data, uint32_t* orientation) {
  BytesStream stream(data);
  return PIEX_NS::GetOrientation(data ? &stream : nullptr, orientation);
}

uint64_t piexc_supported_extensions(piexc_text* joined) {
  const std::vector<std::string> exts = PIEX_NS::SupportedExtensions();
  if (joined) {
    std::string all;
    for (size_t i = 0; i < exts.size(); ++i) {
      all += (i ? "," : "") + exts[i];
    }
    ToText(all, joined);
  }
  return exts.size();
}

bool piexc_cr3_get_orientation(const piexc_stream* stream, uint32_t* orientation) {
  BytesStream bytes(stream);
  return PIEX_NS::Cr3GetOrientation(stream ? &bytes : nullptr, orientation);
}

int32_t piexc_cr3_get_preview_data(const piexc_stream* stream,
                                   piexc_preview* preview_image_data) {
  BytesStream bytes(stream);
  PreviewImageData preview{};
  if (preview_image_data) {
    preview = FromPreview(*preview_image_data);
  }
  const Error err = PIEX_NS::Cr3GetPreviewData(
      stream ? &bytes : nullptr, preview_image_data ? &preview : nullptr);
  if (preview_image_data) {
    ToPreview(preview, preview_image_data);
  }
  return static_cast<int32_t>(err);
}

int32_t piexc_recognize_raw_image_type_lite(const uint8_t* data, size_t len) {
  return static_cast<int32_t>(
      PIEX_NS::image_type_recognition::RecognizeRawImageTypeLite(Source(data, len)));
}

bool piexc_is_raw_type(int32_t type) {
  return PIEX_NS::image_type_recognition::IsRaw(static_cast<RawImageTypes>(type));
}

bool piexc_is_of_type(const uint8_t* data, size_t len, int32_t type) {
  return PIEX_NS::image_type_recognition::IsOfType(Source(data, len),
                                                   static_cast<RawImageTypes>(type));
}

size_t piexc_get_number_of_bytes_for_is_raw_lite(void) {
  return PIEX_NS::image_type_recognition::GetNumberOfBytesForIsRawLite();
}

size_t piexc_get_number_of_bytes_for_is_of_type(int32_t type) {
  return PIEX_NS::image_type_recognition::GetNumberOfBytesForIsOfType(
      static_cast<RawImageTypes>(type));
}

bool piexc_is_raw_lite(const uint8_t* data, size_t len) {
  return PIEX_NS::image_type_recognition::IsRawLite(Source(data, len));
}

}  // extern "C"
