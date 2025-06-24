# autoignore

Simple and efficient .gitignore generator from templates.

## Features

- Generate .gitignore files from predefined templates
- Combine multiple templates
- List available templates
- User and system-wide template locations
- Clean GNU/UNIX style interface with colored output
- Append to existing .gitignore files

## Installation

### Build from source

Requirements:
- C++20 compatible compiler
- Meson build system
- Ninja (recommended)

```bash
git clone https://github.com/AnmiTaliDev/autoignore.git
cd autoignore
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir
```

## Usage

### Basic usage

Generate .gitignore for C++ project:
```bash
autoignore cpp
```

Combine multiple templates:
```bash
autoignore cpp cmake ides
```

### Options

```
Usage: autoignore [OPTIONS] [TEMPLATES...]

Options:
  -l, --list      List available templates
  -o, --output    Output file (default: .gitignore)
  -a, --append    Append to existing file instead of overwriting
  -v, --verbose   Verbose output
  -h, --help      Show this help message
```

### Examples

List all available templates:
```bash
autoignore -l
```

Generate with custom output file:
```bash
autoignore -o .gitignore_cpp cpp
```

Append to existing .gitignore:
```bash
autoignore -a python
```

Verbose output:
```bash
autoignore -v cpp qt cmake
```


## Template locations

Templates are searched in the following order:

1. `~/.local/share/autoignore/template/` (user templates)
2. `/usr/local/share/autoignore/template/` (local installation)
3. `/usr/share/autoignore/template/` (system installation)

User templates take precedence over system templates.

## Adding custom templates

Create `.gitignore` files in your user template directory:

```bash
mkdir -p ~/.local/share/autoignore/template
echo "*.custom" > ~/.local/share/autoignore/template/mytemplate.gitignore
autoignore mytemplate
```

Template files should be named `{name}.gitignore`.

## License

Apache License 2.0

## Author

AnmiTaliDev