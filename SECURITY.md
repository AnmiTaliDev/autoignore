# Security Policy

## Supported versions

| Version | Supported |
|---------|-----------|
| latest (main) | Yes |

## Reporting a vulnerability

If you discover a security vulnerability in autoignore, please **do not open a public issue**.

Instead, report it privately by emailing the maintainer or using [GitHub's private vulnerability reporting](https://github.com/AnmiTaliDev/autoignore/security/advisories/new).

Please include:
- A description of the vulnerability
- Steps to reproduce
- Potential impact
- Any suggested fix, if you have one

You can expect an acknowledgment within 72 hours. We will work with you to understand and address the issue before any public disclosure.

## Scope

autoignore reads template files from disk and writes `.gitignore` files. Relevant security considerations include:

- **Path traversal** — template names must not escape the template directory
- **Symlink attacks** — malicious templates in shared directories
- **Output path** — the `-o` flag writes to a user-specified path

If you find any issue in these areas, please report it.
