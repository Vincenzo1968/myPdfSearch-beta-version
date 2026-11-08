# MyPdfSearch

**A C-based PDF parser for text extraction and direct keyword searching.**

MyPdfSearch is a command-line program written in C. It can extract text from PDF documents and search for words directly inside PDF files, without requiring the user to export the complete text first.

## Project status

**Beta release — version 0.1.1**

MyPdfSearch is under active development. This beta is made available for testing and feedback. PDF parsing, text extraction, font and character-encoding support, and the handling of complex document structures are still being improved. Some PDF files may not yet be processed correctly.

MyPdfSearch is a text parser, not a PDF page renderer. It does not reproduce the original visual layout of a page. The reading order of text in complex layouts (for example, newspapers with multiple columns, mixed text and images, or unusual writing modes) may require further improvement. It is not an OCR application, so image-only pages without an embedded text layer cannot be read as text.

## Features

- Extract text from a PDF document.
- Extract text from a selected page range.
- Write extracted text to standard output or to an output file.
- Search for one or more words directly in a PDF file.
- Search PDF files in a directory, including subdirectories by default.
- Optionally limit a directory search to the specified directory.
- Optionally select text according to glyph row order.
- Handle Unicode text mappings and several PDF font-encoding mechanisms; international-text support is still being tested and improved.

## Requirements

The current build uses:

- GCC or another compatible C compiler (GCC is the tested compiler).
- GNU Autotools (for global system installation via `./configure && make && sudo make install`).
- GNU Make.
- zlib.
- OpenSSL 3.x.

### Build on Ubuntu

Install the required development packages:

```sh
sudo apt update
sudo apt install build-essential zlib1g-dev libssl-dev
```

Build and install the program globally from the project directory:

```sh
./configure
make -j2
sudo make install
```

The executable is named `mypdfsearch` and can be launched from any directory.

To display the available command-line options:

```sh
mypdfsearch --help
```

The beta has been built and tested on Ubuntu 24.04 LTS with GCC 13.3.0. Other platforms have not yet been fully validated for this release.

## Usage

### Extract text from a PDF

```sh
mypdfsearch --extracttextfrom="/path/to/document.pdf"
```

Extract only a page range:

```sh
mypdfsearch --extracttextfrom="/path/to/document.pdf" --frompage=1 --topage=5
```

Write the extracted text to a file:

```sh
mypdfsearch --extracttextfrom="/path/to/document.pdf" --outputfile="extracted.txt"
```

### Search a PDF file

```sh
mypdfsearch --path="/path/to/document.pdf" --words="word1 word2"
```

### Search a directory of PDF files

```sh
mypdfsearch --path="/path/to/pdf-directory" --words="word1 word2"
```

By default, directory searches include subdirectories. Use `--nosubdirs` to search only the specified directory:

```sh
mypdfsearch --path="/path/to/pdf-directory" --words="word1 word2" --nosubdirs
```

### Select text by glyph row order

```sh
mypdfsearch --extracttextfrom="/path/to/document.pdf" --sortglyphsbyrow
```

Without `--sortglyphsbyrow`, text is selected according to the order in which glyphs appear in the PDF content.

For the complete and current list of options, run `mypdfsearch --help`.

## Validation and Robustness

MyPdfSearch undergoes rigorous regression testing to ensure cross-platform consistency and syntactic reliability. 

### CMap & Unicode Encoding Verification
The engine has been validated against complex multi-lingual document structures, including Right-to-Left (RTL) scripts such as Arabic and Hebrew, East Asian CJK character sets via N-gram tokenization, and ancient/vertical writing systems.

### Isartor PDF/A Test Suite Compliance
The parsing engine was stress-tested against the industry-standard **Isartor Test Suite (PDF/A-1b Compliance)**, analyzing 205 structural compliance files. The parser demonstrated enterprise-grade robustness with **0 crashes (no segmentation faults)** and **0 memory leaks**, while successfully catching and isolating deliberate syntax violations:
- **File Header Corruptions:** Properly intercepted and rejected invalid `%PDF-1.x` headers.
- **Trailer & XRef Anomalies:** Safely caught corrupted cross-reference tables and malformed trailer dictionaries.
- **Stream Object Violations:** Fully compliant with ISO specifications by identifying and blocking invalid whitespace characters following the `stream` keyword token.

## Feedback and bug reports

This is a beta release. Bug reports, PDF test cases that can be shared legally, and suggestions for improvement are welcome. Please include the command used, the relevant output, and a short description of the expected result. Do not upload documents containing private or confidential information.

## License

The original MyPdfSearch source code is distributed under the **GNU General Public License, version 3 or (at your option) any later version**. See `COPYING` for the complete license text.

Some bundled character-mapping data are derived from third-party resources and are covered by separate notices. The Adobe CMap mapping tables are accompanied by the BSD 3-Clause license notice, and Unicode-derived data are accompanied by the Unicode License V3 notice. See the files in `LICENSES/` for details. These third-party notices apply to the corresponding data, not to the original MyPdfSearch source code.

## Author

Vincenzo Lo Cicero\
vincenzolocicero68@gmail.com
