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
  -M, --max-depth <num>   Maximum directory depth for auto-detection (default: 3)
  -o, --output <file>     Output file (default: .gitignore)
  -a, --append            Append to existing file
  -p, --preview           Preview output without writing
  -u, --dedup             Deduplicate repeated patterns
  -v, --verbose           Verbose output
  -h, --help              Show this help
```

### Examples

```bash
# Generate .gitignore for a C++ project with CMake
autoignore cpp cmake

# Combine multiple templates with deduplication
autoignore -u nodejs react vite

# Append to existing file
autoignore -a nodejs

# List available templates
autoignore -l

# Write to a custom path
autoignore -o /tmp/my.gitignore rust cargo

# Let autoignore detect the project type automatically
autoignore -d

# Interactive selector
autoignore -i
```

## Template locations

Templates are searched in order:

1. `$AUTOIGNORE_PATH` (colon-separated custom paths)
2. `./template/` - current project template directory
3. Relative to executable (`<exe_dir>/template/` and `<exe_dir>/../share/autoignore/template/`)
4. `$XDG_DATA_HOME/autoignore/template/` (default: `~/.local/share/autoignore/template/`)
5. `$XDG_DATA_DIRS/autoignore/template/` (default: `/usr/local/share/...`, `/usr/share/...`)

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
