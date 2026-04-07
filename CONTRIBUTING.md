# Contributing to autoignore

Thank you for your interest in contributing!

## Ways to contribute

- Add new `.gitignore` templates
- Improve existing templates
- Fix bugs in the C++ source
- Improve documentation
- Report issues

---

## Adding a template

Templates live in `template/` and are named `{name}.gitignore` (lowercase).

1. Fork the repository and create a branch:
   ```bash
   git checkout -b add-mytool-template
   ```

2. Create the template file:
   ```bash
   touch template/mytool.gitignore
   ```

3. Add patterns with a short comment header:
   ```gitignore
   # MyTool
   # Build artifacts and generated files for MyTool projects

   build/
   *.generated
   .mytool-cache/
   ```

4. Verify it works:
   ```bash
   meson setup builddir && meson compile -C builddir
   ./builddir/autoignore mytool
   ```

5. Commit and open a pull request:
   ```bash
   git commit -m "feat: add MyTool gitignore template"
   ```

### Template guidelines

- **Lowercase names** — `rust.gitignore`, not `Rust.gitignore`
- **Focused scope** — only include files the tool/language actually generates
- **No broad wildcards** — prefer `*.pyc` over `*`
- **Add comments** for non-obvious patterns

---

## Reporting bugs

Open an issue with:
- Description of the problem
- Steps to reproduce
- Expected vs actual behavior
- OS, compiler, and version of autoignore

---

## Code contributions

For changes to the C++ source:

1. Target C++20
2. Follow the existing code style
3. Build and test locally before submitting
4. Keep commits focused and descriptive

---

## License

By contributing, you agree your contributions will be licensed under the [Apache License 2.0](LICENSE).
