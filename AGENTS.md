# AGENTS.md

## 🛠️ Agent: YUK Stream Validator and Diagnostic Logger

### Description
This agent improves the `.yuk` audio modding pipeline by extending the stream validation tooling. It adds detailed analysis, per-stream byte/frame data, and the ability to log all results to an external file — simplifying debugging and versioned testing.

---

## ✅ Features Added to `validate_yuk.cpp`

### 1. **Log File Output**
- New `--log <filename>` argument.
- Writes all validation output to a log file instead of just stdout.
- Can be used with or without `--raw`.

### 2. **Per-Stream Size and Frame Count**
- Outputs:
  - Total stream data size (after optional header)
  - Number of full 0x180-byte ATRAC3 frames
  - Remainder in bytes
- Helps pinpoint overflows, misalignment, and inconsistencies.

### 3. **RIFF Header Check Enhancements**
- Hex-dumps the unexpected header bytes (e.g. `0x41424344`) for easier comparison.
- Skipped when `--raw` is passed.

### 4. **Validation Summary**
- At the end of the log or console output, prints a table:
  ```
  Summary:
  Stream0: 2323200 bytes, 6050 frames ✓
  Stream1: 2323504 bytes, 6050 frames ✗ (remainder 304)
  ...
  ```

### 5. **Timestamped Logs**
- Automatically inserts a timestamp like:
  ```
  Validation started: 2025-07-29 19:42:17
  ```

---

## 🧪 Usage Example

```bash
validate_yuk burningworldmasaugrace --raw --log validate_log.txt
```

Output:
- Written to `validate_log.txt`
- Includes full per-stream breakdown
- Summary at the end shows counts of aligned streams and header validity

---

## 🧠 Benefits

- Debugging mismatched audio or stream desyncs is faster
- Supports reproducible logs for bug reporting
- Helps catch partial frame injection bugs in `extract_yuk` or `compress_yuk`
- Makes CI-style test scripts possible for mod pack validation

---

## 🔄 Next Steps

- Add option to compare `.hdr` files for per-stream header consistency
- Allow validating `.wav` output against `.atrac` size/frequency
- Possibly integrate `.yuk` file structure scanning directly

This validator and logger agent is now a cornerstone of clean `.yuk` audio modding and regression-free testing.