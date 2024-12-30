# jsonchrist

A powerful JSON processor and formatter with advanced features.

## Features

- 🌳 Tree View: Hierarchical visualization of JSON structure
- 🎨 Pretty Print: Format JSON with customizable indentation
- 📦 Compact Mode: Minify JSON by removing whitespace
- 🔍 Validation: Check JSON syntax with detailed error reporting
- 📊 Statistics: Analyze JSON structure and content
- 🎯 Path Flattening: Convert nested JSON to flat key-value pairs
- 🔄 Stream View: Show JSON parsing events
- ✨ Syntax Highlighting: Colorized JSON output
- 📝 Edit Mode: Generate editable node structure
- 🔎 Index: Create searchable value index
- 🔄 Format Conversion: Convert JSON to CSV/TSV formats

## Installation

```bash
git clone https://github.com/yourusername/jsonchrist.git
cd jsonchrist
make
```

## Usage

```bash
./jsonchrist [options] input.json
```

### Options

- `--tree`           Output hierarchical tree structure
- `--pretty`         Output formatted JSON (default)
- `--compact`        Output compact JSON
- `--flatten`        Output flattened key-value pairs
- `--stream`         Output parsing events stream
- `--validate`       Validate JSON and show errors
- `--stats`          Output JSON statistics
- `--highlight`      Output syntax-highlighted JSON
- `--edit`          Output editable node structure
- `--index`         Output searchable index
- `--no-color`      Disable colored output
- `--indent N`      Set indentation level (default: 4)
- `-o, --output FILE` Write output to FILE
- `--convert FORMAT` Convert JSON to another format (csv, tsv)
- `-h, --help`      Display help message

### Examples

```bash
# View JSON as a tree structure
./jsonchrist --tree input.json

# Format JSON with 2-space indentation
./jsonchrist --pretty --indent 2 input.json

# Validate JSON and show statistics
./jsonchrist --validate --stats input.json

# Convert JSON to CSV
./jsonchrist --convert csv input.json > output.csv

# Convert JSON to TSV
./jsonchrist --convert tsv input.json > output.tsv
```

## Building from Source

Requirements:
- GCC or compatible C compiler
- GNU Make
- C11 standard library

Build commands:
```bash
make        # Build the application
make clean  # Clean build files
make test   # Run tests
```

## License

[Your chosen license]

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. 
