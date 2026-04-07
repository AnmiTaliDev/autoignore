# autoignore

A `.gitignore` generator.

## Installation

### Build from source

**Requirements:** C++ compiler, Meson, Ninja

```bash
git clone https://github.com/AnmiTaliDev/autoignore.git
cd autoignore
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir
```

## Usage

```
autoignore [OPTIONS] [TEMPLATES...]
```

### Options
```
  -l, --list              List available templates
  -s, --search <query>    Search templates by name
  -i, --interactive       Select templates interactively
  -d, --detect            Auto-detect templates from project files
  -o, --output <file>     Output file (default: .gitignore)
  -a, --append            Append to existing file
  -p, --preview           Preview output without writing
  -v, --verbose           Verbose output
  -h, --help              Show this help
```
### Examples

```bash
# Generate .gitignore for a C++ project with CMake
autoignore cpp cmake

# Combine multiple templates
autoignore python django vscode

# Append to existing file
autoignore -a nodejs

# List available templates
autoignore -l

# Write to a custom path
autoignore -o /tmp/my.gitignore rust cargo

# Let autoignore detect the project type automatically
autoignore

# Interactive selector
autoignore -i
```

## Template locations

Templates are searched in order:

1. `~/.local/share/autoignore/template/` — user templates
2. `/usr/local/share/autoignore/template/` — local installation
3. `/usr/share/autoignore/template/` — system installation

User templates take precedence over system templates.

## Custom templates

```bash
mkdir -p ~/.local/share/autoignore/template
echo "*.myext" > ~/.local/share/autoignore/template/mytool.gitignore
autoignore mytool
```

Template files must be named `{name}.gitignore`.

## Shell completions

Completions are installed automatically with `meson install`. To install manually:

```bash
# Bash
cp completion/autoignore.bash /etc/bash_completion.d/autoignore

# Zsh
cp completion/_autoignore /usr/share/zsh/site-functions/_autoignore

# Fish
cp completion/autoignore.fish ~/.config/fish/completions/autoignore.fish
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

[Apache License 2.0](LICENSE)
