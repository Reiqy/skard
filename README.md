# Skard

[![CI](https://github.com/Reiqy/skard/actions/workflows/ci.yml/badge.svg)](https://github.com/Reiqy/skard/actions/workflows/ci.yml)

> [!Warning]
> This project is still in development. Breaking changes can happen unannounced. Use at your own risk.

Skard is (or rather will be) an interpreted statically type-checked language. It draws inspiration from C#, Rust and Lua.

```rs
fn main() {
    if ((10 > 5) && (10 <= -14)) {
        print ("Hello, Skard! %n", 1 + (2 - 3) * 4 + 5 / (6 - 8) * 9)
    } else if (true || false) {
        print ("%s %n", "Hello!", 1 + 5 * 5)
    } else {
        print ("%n", 0)
    }

    print ("%n", 42)
}
```

## Build instructions

Skard uses CMake. Currently, it produces a single executable. Details will be added.

## Formatting

Skard uses `clang-format` with the repository's `.clang-format` configuration. After
installing `clang-format`, enable the pre-commit hook once per checkout:

```sh
git config core.hooksPath .githooks
```

To format the source tree manually, run `cmake --build build --target format`. The
CI workflow checks formatting with `format-check`.
