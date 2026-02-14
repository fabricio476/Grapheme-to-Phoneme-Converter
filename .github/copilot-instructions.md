# AI Coding Agent Instructions for eSpeak NG G2P

## Project Overview

**eSpeak NG - Grapheme-to-Phoneme (G2P) Converter**: A minimal fork of eSpeak NG that strips audio synthesis functionality, retaining only text→phoneme conversion. Written in C, CMake-based build system, supports 100+ languages.

**Core purpose**: Convert written text to phoneme representations (IPA or eSpeak notation) for any language.

## Architecture

### Key Components

1. **CLI Layer** (`src/espeak-ng.c`)
   - Entry point for command-line text processing
   - Handles option parsing (voice selection, output format, file I/O)
   - Key flow: parse args → init library → translate text → output phonemes

2. **Translation Engine** (`src/libespeak-ng/translate.c`, `translate.h`)
   - Core G2P pipeline: text → letter groups → pronunciation rules → phonemes
   - **Critical data**: `Translator` struct manages language-specific rules & dictionaries
   - Entry point: `Lookup()` function for word-level translation

3. **Dictionary & Rules** (`dictsource/` files)
   - Format: `{language}_list` (word exceptions), `{language}_rules` (pattern-based rules)
   - Example: `dictsource/en_list` has pronunciations; `dictsource/en_rules` has conditional replacements
   - Compiled at build time into binary format stored in `espeak-ng-data/`

4. **Phoneme System** (`src/libespeak-ng/phoneme.h`, `phoneme.c`)
   - Phonemes defined as enums with 3-letter mnemonics (e.g., `nas`, `vwl`, `frc`)
   - Features encode articulation properties (manner, place, voicing)
   - Phoneme tables precompiled; phoneme list generation in `synthesize.c`

5. **Voice Management** (`src/libespeak-ng/voice.h`, `voices.c`)
   - Each "voice" = language + phoneme table + dictionary combination
   - Voice files in `espeak-ng-data/voices/` reference phoneme tables and dictionaries

### Data Flow

```
Input text (CLI)
  ↓
espeak-ng.c: parse voice/options
  ↓
espeak_api.c: espeak_ng_Initialize() + espeak_TextToPhonemes()
  ↓
translate.c: TranslateWord3() → apply rules via Lookup()
  ↓
dictionary.c: EncodePhonemes() → phoneme sequence
  ↓
Output: IPA/mnemonics + separators
```

## Build & Runtime

### Build Process (CMake)

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

- **Pre-built data**: `espeak-ng-data/` is copied to build directory (no compilation needed unless `COMPILE_INTONATIONS` flag)
- **Binary output**: `build/src/espeak-ng` (symlinked as `speak-ng`)
- **Test data**: Relies on precompiled phoneme tables & dictionaries

### Runtime Data Path

- Critical: `--path` CLI flag must point to directory containing `espeak-ng-data/`
- Subdirs used: `espeak-ng-data/voices/`, `espeak-ng-data/lang/`, compiled dicts

### Key Functions to Know

- `espeak_Initialize(path)` - loads phoneme data & voice definitions
- `espeak_TextToPhonemes(text)` - translates text, returns phoneme buffer
- `TranslateWord3()` - applies language-specific rules to single word
- `EncodePhonemes()` - converts phoneme string to binary format

## Language-Specific Rules

### Dictionary Rule Syntax

Rules in `dictsource/{lang}_rules`:

- **Conditional rules** (prefix with `?`): `?3` = rule applies only to voice variant 3
- **Letter groups** (`.L##`): Define phoneme contexts, e.g., `.L01 l r` = group of consonants
- **Replacement chains** (`.replace`): Unicode normalization
- **Pattern rules**: Match grapheme sequences → apply stress/phoneme substitutions

Example from `en_rules`:
```
.L01 l r          // group: liquids
.group a           // process letter 'a'
  a        a       // a → /a/ (bare context)
  _)a'     a#      // apostrophe after = /a:/ (stressed)
```

### Dictionary List Entries

Words in `dictsource/{lang}_list`:

```
word     phoneme_sequence     [flags]
```

Special prefixes:
- `_` = letter name (e.g., `_a eI` for letter "A")
- `?3` = conditional (variant-specific pronunciation)
- `$nounf`, `$alt6` = flags for stress/vowel treatment

## Conventions & Patterns

1. **Voice Selection**: `-v en`, `-v pt-br`, `-v ja` — always language code
2. **Phoneme Output**: 
   - `-x` = eSpeak notation (3-letter mnemonics)
   - `--ipa` = IPA; `--sep=" "` = space-separated; `--tie="-"` = tie bar for digraphs
3. **Trace Debugging**: `-X` flag shows rule application step-by-step
4. **Multi-language Files**: Most language files in `dictsource/` — check if your language exists
5. **Error Handling**: Exit codes via `espeak_ERROR` enum; messages to stderr

## Common Development Tasks

### Adding Support for a New Language

1. Create `dictsource/{lang}_rules` and `dictsource/{lang}_list`
2. Define phoneme table in `phsource/phonemes.txt` or reference existing
3. Add voice profile in `espeak-ng-data/voices/{lang}` (references phoneme table)
4. Rebuild; test with `./build/src/espeak-ng --path=./build -v {lang} --ipa "test"`

### Debugging Rule Application

Use trace mode to see which rules match:
```bash
./build/src/espeak-ng --path=./build -v en -X "word"
```

Output shows: grapheme → matched pattern → applied phoneme transformation

### Testing a Single Word

```bash
echo "hello" | ./build/src/espeak-ng --path=./build -v en --ipa --stdin
```

## File Organization

- `src/espeak-ng.c` — CLI entry
- `src/libespeak-ng/` — Core library (phonemes, translation, dictionaries)
- `src/libespeak-ng/espeak_api.c` — Public API bridge
- `dictsource/` — Language rule & word list source files
- `espeak-ng-data/` — Pre-compiled phoneme data, voice configs, language dicts
- `cmake/` — Build system modules (data.cmake handles data distribution)

## Critical Constraints

- **No Audio**: All synthesis, MBROLA, intonation code removed; G2P-only
- **Data Immutable at Runtime**: Phoneme tables, dictionaries compiled at build time
- **UTF-8 Throughout**: All text processing assumes UTF-8 encoding
- **Single-threaded for G2P**: FIFO command queue exists but not used for CLI tool
