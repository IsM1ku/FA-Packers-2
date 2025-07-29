# AGENTS.md

## 🛠️ Agent: Final Review of `extract_yuk` Audio Tool

### Description
This agent audited the final compiled version of the Full Auto 2 `.yuk` extractor (`extract_yuk.exe`). Although stream alignment and byte-level correctness have been achieved, the resulting `.wav` files still contain unwanted audio bleed due to incomplete metadata passed to `vgmstream`.

---

## 🧩 Current Issues

### 1. 🔴 Inaccurate `.txth` Metadata
The tool generates a basic `.atrac.txth` file with:
```text
num_samples = data_size
```
This is **incorrect**, as `data_size` is in bytes. ATRAC3 decoding requires accurate frame-based sample calculation:
```text
num_samples = (data_size / 0x180) * 1024
```
Failing to provide this causes `vgmstream` to decode past the last valid frame, producing audio bleed or noise at the end of each `.wav` file.

---

### 2. ⚠️ All Streams Use Shared `.txth`
The extractor currently creates a single `.atrac.txth` and applies it universally. However, stream sizes and frame counts could differ slightly. Validation logs confirm matching sizes **for now**, but future edge cases (e.g. modded `.yuk`s) may require individual `.txth` files.

---

## ✅ Recommended Fixes

### ✅ Patch `writeHeaderInfo()` to Calculate Sample Count
Update the function to generate the following metadata:

```text
codec = ATRAC3
sample_rate = 48000
channels = 2
start_offset = 0
interleave = 0x180
frame_size = 0x180
samples_per_frame = 1024
num_samples = (data_size / 0x180) * 1024
```

Pass `dataSize` from the extractor per stream and compute `num_samples` from it.

---

### ✅ Optional Enhancements

- Write one `.txth` file per stream with matching filename.
- Allow the user to disable `.wav` generation (for faster extraction).
- Auto-detect and warn if stream headers (`.hdr`) differ in the first 464 bytes.

---

## 🧪 Validation Reference

- Used `validate_yuk --raw --log` to confirm stream frame alignment.
- All 8 extracted `.atrac` streams are:
  - Byte aligned
  - Proper size
  - But WAV output still exhibits decoding artifacts
- Trimming ends cleanly once `num_samples` is correctly defined.

---

## 🔄 Next Steps

- Patch `extract_yuk.cpp` source (or regenerate) to compute proper sample counts.
- Distribute per-stream `.txth` if needed.
- Consider including `.hdr` in WAV conversion if it enhances fidelity.

This audit confirms the extractor is **byte-perfect structurally**, but still needs metadata accuracy improvements for clean decoded audio output.