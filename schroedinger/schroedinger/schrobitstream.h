
#ifndef _SCHRO_BITSTREAM_H_
#define _SCHRO_BITSTREAM_H_

#define SCHRO_MAX_TRANSFORM_DEPTH 8
#define SCHRO_MAX_REFERENCE_FRAMES 10
#define SCHRO_FRAME_QUEUE_LENGTH 40
#define SCHRO_LIMIT_WIDTH 2048

typedef enum _SchroParseCode SchroParseCode;
enum _SchroParseCode {
  SCHRO_PARSE_CODE_ACCESS_UNIT = 0x00,
  SCHRO_PARSE_CODE_INTRA_REF = 0x0c,
  SCHRO_PARSE_CODE_INTRA_NON_REF = 0x08,
  SCHRO_PARSE_CODE_INTER_REF_1 = 0x0d,
  SCHRO_PARSE_CODE_INTER_REF_2 = 0x0e,
  SCHRO_PARSE_CODE_INTER_NON_REF_1 = 0x09,
  SCHRO_PARSE_CODE_INTER_NON_REF_2 = 0x0a,
  SCHRO_PARSE_CODE_END_SEQUENCE = 0x10
};

#define SCHRO_PARSE_CODE_PICTURE(is_ref,n_refs) (8 | ((is_ref)<<2) | (n_refs))

#define SCHRO_PARSE_CODE_IS_PICTURE(x) ((x) & 0x8)
#define SCHRO_PARSE_CODE_NUM_REFS(x) ((x) & 0x3)
#define SCHRO_PARSE_CODE_IS_REF(x) ((x) & 0x4)

enum _SchroVideoFormatEnum {
  SCHRO_VIDEO_FORMAT_CUSTOM = 0,
  SCHRO_VIDEO_FORMAT_QSIF,
  SCHRO_VIDEO_FORMAT_QCIF,
  SCHRO_VIDEO_FORMAT_SIF,
  SCHRO_VIDEO_FORMAT_CIF,
  SCHRO_VIDEO_FORMAT_4SIF,
  SCHRO_VIDEO_FORMAT_4CIF,
  SCHRO_VIDEO_FORMAT_SD480,
  SCHRO_VIDEO_FORMAT_SD576,
  SCHRO_VIDEO_FORMAT_HD720,
  SCHRO_VIDEO_FORMAT_HD1080,
  SCHRO_VIDEO_FORMAT_2KCINEMA,
  SCHRO_VIDEO_FORMAT_4KCINEMA
};

typedef enum _SchroChromaFormat SchroChromaFormat;
enum _SchroChromaFormat {
  SCHRO_CHROMA_444 = 0,
  SCHRO_CHROMA_422,
  SCHRO_CHROMA_420
};

enum _SchroColourMatrix {
  SCHRO_COLOUR_MATRIX_CUSTOM = 0,
  SCHRO_COLOUR_MATRIX_SDTV = 1,
  SCHRO_COLOUR_MATRIX_HDTV = 2,
  SCHRO_COLOUR_MATRIX_YCgCo = 3
};

enum _SchroColourPrimaries {
  SCHRO_COLOUR_PRIMARY_CUSTOM = 0,
  SCHRO_COLOUR_PRIMARY_NTSC = 1,
  SCHRO_COLOUR_PRIMARY_PAL = 2,
  SCHRO_COLOUR_PRIMARY_HDTV = 3
};

enum _SchroTransferChar {
  SCHRO_TRANSFER_CHAR_TV = 0,
  SCHRO_TRANSFER_CHAR_EXTENDED = 1,
  SCHRO_TRANSFER_CHAR_LINEAR = 2
};

enum _SchroWaveletIndex {
  SCHRO_WAVELET_DESL_9_3,
  SCHRO_WAVELET_5_3,
  SCHRO_WAVELET_13_5,
  SCHRO_WAVELET_HAAR_0,
  SCHRO_WAVELET_HAAR_1,
  SCHRO_WAVELET_HAAR_2,
  SCHRO_WAVELET_FIDELITY,
  SCHRO_WAVELET_DAUB_9_7
};

#endif

