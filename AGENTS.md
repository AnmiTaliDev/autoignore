# Agent Instructions for autoignore

This file provides context for AI coding assistants working on this repository.

## Project overview

**autoignore** is a C++20 command-line tool that generates `.gitignore` files from named templates. Version 2.0.0, licensed Apache-2.0.

## Build

```bash
meson setup builddir
meson compile -C builddir
```

The binary ends up at `builddir/autoignore`. Always build and run it to verify changes.

## Key conventions

- **C++20**, warning level 3 (`-Wall -Wextra -Wpedantic` equivalent)
- Template files are named `{name}.gitignore` — always **lowercase**
- Template search order: `~/.local/share/autoignore/template/` → `/usr/local/share/autoignore/template/` → `/usr/share/autoignore/template/`
- No external runtime dependencies — only the standard library and `stdc++fs`

## Adding a template

Create `template/{name}.gitignore` with a comment header and relevant patterns. Name must be lowercase. Verify with:

```bash
meson compile -C builddir && ./builddir/autoignore {name}
```

## CLI flags

| Flag | Description |
|------|-------------|
| `-l`, `--list` | List available templates |
| `-s`, `--search <query>` | Search templates by name |
| `-i`, `--interactive` | Interactive selector |
| `-d`, `--detect` | Auto-detect from project files |
| `-o`, `--output <file>` | Output path (default: `.gitignore`) |
| `-a`, `--append` | Append instead of overwrite |
| `-p`, `--preview` | Preview without writing |
| `-v`, `--verbose` | Verbose output |

## What to avoid

- Do not add external dependencies
- Do not change the template naming convention
- Do not lower the warning level or disable warnings
- Do not modify `builddir/` — it is a build artifact directory
