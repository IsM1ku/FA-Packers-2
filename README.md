# Full Auto 2 YUK Tools

This repository contains simple utilities for working with the `*.yuk` music container used in **Full Auto 2: Battlelines**.

- `extract_yuk.cpp` – extracts the interleaved ATRAC streams from a YUK file.
- `compress_yuk.cpp` – rebuilds a YUK file from the extracted streams.
- `validate_yuk.cpp` – checks extracted streams for valid headers and frame alignment.

## Building

All programs are single source files that can be built with a C++17 compiler:

```bash
g++ -std=c++17 -o extract_yuk extract_yuk.cpp
g++ -std=c++17 -o compress_yuk compress_yuk.cpp
g++ -std=c++17 -o validate_yuk validate_yuk.cpp
```

`at3tool/ps3_at3tool.exe` and `vgmstream` must be present if you plan to convert audio automatically. See the notes below.

## Usage

### Extract

```
./extract_yuk <input.yuk> <output_dir>
```

The extractor creates two folders under `<output_dir>`:

- `atrac/` – contains the raw ATRAC streams (`*_StreamX.atrac`) and
  corresponding headers (`*_StreamX.hdr`).
- `wav/` – if `vgmstream-cli.exe` is present in the `vgmstream/` subfolder,
  WAV conversions of the streams will be written here.

A small `atrac.txth` file describing the streams is also generated for use with
`vgmstream`.

### Compress

```
./compress_yuk <input_dir> <output.yuk>
```

`<input_dir>` must contain eight WAV files named
`<input_dir>_Stream0.wav` through `Stream7.wav` along with matching header files
`*.hdr` produced by the extractor. The compressor converts the WAVs back to ATRAC
using `ps3_at3tool.exe`, reinserts the saved headers and interleaves the streams
while ensuring frame alignment.

### Validate

```
./validate_yuk <stream_prefix>
```

`<stream_prefix>` should be the path and base name of extracted ATRAC files
(e.g. `out/atrac/music_Stream`). The validator checks each stream for a valid
RIFF header and that its length is a multiple of the 0x180 ATRAC frame size.

## Notes

- `ps3_at3tool.exe` must be placed under `at3tool/` for compression.
- `vgmstream-cli.exe` can be placed under `vgmstream/` to enable automatic
  WAV conversion when extracting.
