# cppxx

**cppxx** (C++ PlusPlus) is a lightweight, TOML-based build system for C++ projects.
It aims to be simpler and more declarative than tools like CMake, while still supporting modern project features like external dependencies, per-target configuration, and flexible build modes.

---

## 📦 Features

- Simple, expressive `TOML` configuration
- Project types: `interface`, `executable`, `static`, `dynamic`
- Git-based third-party dependencies
- Public/private visibility for includes, flags, and links
- Supports globbing (`*.cpp`) and environment variables
- Generates `compile_commands.json` for tooling
- Workspace-aware builds and automatic dependency resolution
- Cached dependencies into `CPPXX_CACHE` directory. **Must be defined in the environment variables.**

---

## 🗂️ Project Structure

Your workspace is defined in a single TOML file (usually `cppxx.toml`) like so:

```toml
[workspace]
title = "cppxx"
version = "v0.1.0"
author = "aufam"
````

### 📁 Projects

Each project is defined using `[project.<name>]` sections.

#### Example:

```toml
[project.myapp]
type = "executable"
sources = ["cmd/main.cpp"]
depends_on = ["core", "mylib"]
```

---

## 🔧 Project Types

| Type         | Description                                              |
| ------------ | -------------------------------------------------------- |
| `interface`  | Header-only or virtual project. **May contain sources.** |
| `executable` | Produces an executable.                                  |
| `static`     | Builds a static library.                                 |
| `dynamic`    | Builds a shared library.                                 |

* If `type` is not specified, it defaults to `"interface"`.
* Only `interface` projects can be referenced via `depends_on`.

---

## 📁 Fields Overview

| Field          | Type                    | Description                                                |
| -------------- | ----------------------- | ---------------------------------------------------------- |
| `sources`      | List                    | Source files relative to workspace dir. Supports globbing. |
| `include_dirs` | Flat list or scoped map | Header include paths.                                      |
| `flags`        | Flat list or scoped map | Compiler flags.                                            |
| `link_flags`   | Flat list or scoped map | Linker flags.                                              |
| `depends_on`   | Flat list or scoped map | Only allows `interface` project names.                     |

### Example (Scoped visibility):

```toml
flags = { public = ["-DDEBUG"], private = ["-Wall", "-Wextra"] }
include_dirs = ["include/"] # Flat list is treated as private (unless interface)
```

---

## 🌐 Git Dependencies

External dependencies can be defined using:

```toml
[project.fmt]
git.url = "fmtlib/fmt"
git.tag = "11.2.0"
include_dirs = ["include/"]
flags = ["-DFMT_HEADER_ONLY=1"]
```

* Sources and includes are treated relative to the workspace, except for `git` projects.
* Dependencies are cloned and extracted into `CPPXX_CACHE` directory.

---

## 🧠 Environment Variables

You can reference environment variables using the syntax:

```toml
flags = ["-fmacro-prefix-map=${PWD:-.}=cppxx"]
```

If an environment variable is missing, the default value after `:-` is used.

---

## 🚀 CLI Usage

```bash
cppxx [OPTIONS...] [targets]
```

### Options:

| Short | Long                 | Description                                           |
| ----- | -------------------- | ----------------------------------------------------- |
| `-t`  | `--targets`          | Specify target(s) to build                            |
| `-o`  | `--out`              | Specify output file (only for single target)          |
| `-m`  | `--mode`             | Set build mode: `debug` or `release` (default: debug) |
| `-c`  | `--clear`            | Clear the specified targets                           |
| `-g`  | `--compile-commands` | Generate `compile_commands.json`                      |
| `-i`  | `--info`             | Print workspace info as JSON                          |
| `-h`  | `--help`             | Print help                                            |

---

## 🧪 Example Workspace

```toml
[workspace]
title = "myapp"
version = "0.1.0"
author = "me"

[project.myapp]
type = "executable"
sources = ["cmd/*.cpp"]
depends_on = ["core", "fmt"]

[project.core]
sources = ["src/*.cpp"]
include_dirs = ["include/"]
flags = ["-Wall"]
depends_on = ["fmt"]

[project.fmt]
git.url = "fmtlib/fmt"
git.tag = "10.1.1"
include_dirs = ["include/"]
```

---

## 📜 License

MIT License

---

