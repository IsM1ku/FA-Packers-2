# AGENTS.md

## 🛠️ Agent: YUK Extractor and Compressor Format Analyst

### Description
This agent reverse-engineered and analyzed the `.yuk` audio container format used in *Full Auto 2: Battlelines* based on the existing source code for the community-made extractor and compressor. The format interleaves 8 ATRAC3 streams per file. The analysis uncovered flaws in how stream data is divided, particularly near the file's beginning and end, which causes audible glitches or stream bleeding.

### Problems Identified
1. **Incorrect Remainder Division:**  
   The YUK compressor splits leftover audio data after full chunks using arbitrary division logic that does not guarantee ATRAC3 frame alignment. This causes audio corruption at the start and end of the recompressed `.yuk` file.

2. **Header Handling Inconsistency:**  
   The compressor skips the first 464 bytes of each ATRAC stream (assumed header), but this is never preserved or reinserted during extraction, potentially corrupting the beginning of streams on re-import.

3. **Lack of Frame Awareness:**  
   ATRAC3 is a frame-based format (commonly using 0x180-byte frames), and any slicing must respect those boundaries. Current logic does not validate this.

### Proposed Fixes
#### ✅ Frame-Aligned Remainder Handling
- Calculate the total number of full ATRAC frames in the remainder (`remainder / 0x180`).
- Distribute only full frames evenly across the 8 streams.
- Ignore or pad any leftover bytes that do not form a complete frame.

#### ✅ Preserve and Reinsert ATRAC Headers
- During extraction: save the first 464 bytes of each stream as a header or embedded in `.atrac` output.
- During compression: prepend the original header to each stream before interleaving.

#### ✅ Optional Debug Output
- Print file size and per-stream chunk sizes after extraction and compression.
- Log warnings if files are not aligned to expected ATRAC frame sizes.

#### ✅ Validation Tools
- Add a verifier tool that confirms if each stream:
  - Starts with a valid ATRAC header.
  - Contains only full frames.
  - Ends cleanly without mid-frame cuts.

### Outcome
This agent's improvements will make custom music modding for Full Auto 2 more reliable, eliminate audio artifacts, and allow future developers to safely modify `.yuk` files without introducing stream corruption.