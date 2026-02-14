# eSpeak NG - Grapheme-to-Phoneme Converter

A compact, open-source **Grapheme-to-Phoneme (G2P)** converter based on [eSpeak NG](https://github.com/espeak-ng/espeak-ng). This project strips away all audio synthesis and playback functionality, retaining only the core text-to-phoneme conversion engine.

Supports **[100+ languages and accents](docs/languages.md)**.

## Features

- **G2P conversion** — converts text to phoneme representations
- **IPA output** — International Phonetic Alphabet transcription (`--ipa`)
- **Phoneme mnemonics** — eSpeak native phoneme notation (`-x`)
- **Trace mode** — step-by-step rule application debugging (`-X`)
- **Customizable separators** — between phonemes (`--sep`, `--tie`)
- **Multi-language** — supports 100+ languages via voice selection (`-v`)
- **Dictionary compilation** — compile custom pronunciation dictionaries (`--compile`)
- **Self-contained** — includes all pre-compiled phoneme data; no external dependencies
- **Compact** — ~19MB total data for all languages
- Written in C

## Building

### Requirements

- CMake ≥ 3.8
- C compiler (GCC or Clang)

### Build Steps

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The build automatically copies the pre-compiled phoneme data to the build directory.

### Verify Installation

```bash
./build/src/espeak-ng --path=./build -v en --ipa "Hello World"
# Output: həlˈəʊ wˈɜːld
```

## Usage

### Basic G2P (eSpeak notation)

```bash
espeak-ng --path=<data-dir> -v <language> -x "text"
```

### IPA Output

```bash
espeak-ng --path=<data-dir> -v en --ipa "Hello World"
# həlˈəʊ wˈɜːld

espeak-ng --path=<data-dir> -v pt-br --ipa "Olá Mundo"
# olˈa mˈũŋdʊ
```

### IPA with Phoneme Separators

```bash
espeak-ng --path=<data-dir> -v pt-br --ipa --sep=" " "Olá Mundo"
# o l ˈa  m ˈũ ŋ d ʊ
```

### Trace Mode (rule debugging)

```bash
espeak-ng --path=<data-dir> -v en -X "Testing"
# Shows step-by-step rule application
```

### From File

```bash
espeak-ng --path=<data-dir> -v en --ipa -f input.txt
```

### From stdin

```bash
echo "Hello" | espeak-ng --path=<data-dir> -v en --ipa --stdin
```

### Compile Custom Dictionary

```bash
espeak-ng --path=<data-dir> -v en --compile
```

### List Available Voices

```bash
espeak-ng --path=<data-dir> --voices
espeak-ng --path=<data-dir> --voices=pt    # Portuguese voices only
```

## Command-Line Options

| Option | Description |
|--------|-------------|
| `-v <voice>` | Select voice/language (e.g., `en`, `pt-br`, `fr`) |
| `-x` | Output phonemes in eSpeak notation |
| `-X` | Output phonemes with translation trace |
| `--ipa` | Output in International Phonetic Alphabet |
| `--sep=<char>` | Separate phonemes with given character |
| `--tie=<char>` | Tie character for multi-letter phonemes |
| `-f <file>` | Read text from file |
| `--stdin` | Read text from stdin |
| `--path=<dir>` | Path to directory containing `espeak-ng-data` |
| `--phonout=<file>` | Write phoneme output to file |
| `--compile` | Compile pronunciation dictionary |
| `--voices[=<lang>]` | List available voices |
| `--version` | Show version |
| `-h`, `--help` | Show help |

## Project Structure

```
meu-g2p/
├── CMakeLists.txt          # Build configuration
├── cmake/                  # CMake modules
├── dictsource/             # Dictionary source files
├── espeak-ng-data/         # Pre-compiled phoneme data + dictionaries
│   ├── phontab             # Phoneme table (compiled)
│   ├── phondata            # Phoneme data (compiled)
│   ├── phonindex           # Phoneme index (compiled)
│   ├── intonations         # Intonation patterns (compiled)
│   ├── *_dict              # Language dictionaries (compiled)
│   ├── lang/               # Language definitions
│   └── voices/             # Voice configurations
├── src/
│   ├── espeak-ng.c         # CLI application
│   └── libespeak-ng/       # Core G2P library
└── docs/                   # Documentation
```

## Supported Languages

See [docs/languages.md](docs/languages.md) for the full list of 100+ supported languages.

## Origin

This project is a fork of [eSpeak NG](https://github.com/espeak-ng/espeak-ng), modified to function exclusively as a G2P converter. All audio synthesis, WAV generation, and playback capabilities have been removed.

## License

Released under the [GPL version 3](COPYING) or later.
