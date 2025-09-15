# Skard

> [!Warning]
> This project is still in development. Breaking changes can happen unannounced. Use at your own risk.

Skard is (or rather will be) an interpreted statically type-checked language. It draws inspiration from C#, Rust and Lua.

```rs
fn main() {
    if ((10 > 5) && (10 <= -14)) {
        print 1 + (2 - 3) * 4 + 5 / (6 - 8) * 9
    } else if (true || false) {
        print 1 + 5 * 5
    } else {
        print 0
    }
    
    print 42
}
```

## Build instructions

Skard uses CMake. Currently, it produces a single executable. Details will be added.
