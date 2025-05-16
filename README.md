# The Octo Programming Language
Octo is a programming language I'm writing for educational purposes.

```
import std;

func main() -> void
{
    std::println("Hello, Octo!");
}
```

## Development Progress
| | Tokenizing + Parsing | Semantic Analysis | Code Generation |
|-|:-:|:-:|:-:|
| Function declaration | ✅ | ✅ | ❌ |
| Function call | ✅ | ✅ | ❌ |
| Variable declaration | ✅ | ✅ | ❌ |
| Variable reassignment | ✅ | ✅ | ❌ |
| Type inference | ⬛ | ✅ | ❌ |
| Modules | ❌| ❌ | ❌ |
| If-statements | ❌ | ❌ | ❌ |
| If-expressions | ❌ | ❌ | ❌ |
| Switch-statements | ❌ | ❌ | ❌ |
| Switch-expressions | ❌ | ❌ | ❌ |
| Loops | ❌ | ❌ | ❌ |
| User-defined types | ❌ | ❌ | ❌ |
| Compile-time function execution | ❌ | ❌ | ❌ |
| Generics | ❌ | ❌ | ❌ |
| Closures | ❌ | ❌ | ❌ |

## Building from source
This project uses CMake as its build system.

Clone the repository with the `--recurse-submodules` flag to also clone the submodules.
```
$ git clone --recurse-submodules https://github.com/256luis/octo.git
```
Create a build directory (or don't) and call `cmake`. This will generate the build script of your choosing (Make, Ninja, MSBuild, etc.)
```
$ mkdir build
$ cd build
$ cmake ..
```
Finally, run the generated build script. The compiled binary will be in the bin folder.
