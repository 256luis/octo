# The Octo Programming Language
Octo is a statically, strongly-typed programming language that transpiles to C.
```rust
extern func printf(format: &char, ..) -> i32;

func fib(n: i64) -> i64
{
    if n == 0 return 0;
    if n == 1 return 1;

    return fib(n - 1) + fib(n - 2);
}

func main() -> i32
{
    let n = 5;
    let nth_fib = fib(n);
    printf("fib(%d) = %d", n, nth_fib);
}

```
Note: Currently, the compiler calls `gcc` to build the generated C code but it is trivial to change it to a C compiler on your system.

## Development Progress
| | Tokenizing + Parsing | Semantic Analysis | Code Generation |
|-|:-:|:-:|:-:|
| Function declaration | ✅ | ✅ | ✅ |
| Function call | ✅ | ✅ | ✅ |
| Variadic functions | ✅ | ✅ | ✅ |
| Variable declaration | ✅ | ✅ | ✅ |
| Variable reassignment | ✅ | ✅ | ✅ |
| Type inference | ⬛ | ✅ | ✅ |
| Pointers | ✅ | ✅ | ✅ |
| Arrays | ⚠️ | ⚠️ | ⚠️ |
| Modules | ❌| ❌ | ❌ |
| If-statements | ✅ | ✅ | ✅ |
| If-expressions | ❌ | ❌ | ❌ |
| Switch-statements | ❌ | ❌ | ❌ |
| Switch-expressions | ❌ | ❌ | ❌ |
| While-loops | ✅ | ✅ | ✅ |
| For-loops | ✅ | ❌ | ❌ |
| Structs | ❌ | ❌ | ❌ |
| Unions | ❌ | ❌ | ❌ |
| Enums | ❌ | ❌ | ❌ |
| Tagged unions | ❌ | ❌ | ❌ |
| Compile-time function execution | ❌ | ❌ | ❌ |
| Generics | ❌ | ❌ | ❌ |
| Closures | ❌ | ❌ | ❌ |
| Out-of-order declarations | ⬛ | ❌ | ❌ |

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
## How to write Octo
### Variables
Variables declarations are in the form `let <identifier>: <type> = <rvalue>;`.
```rust
let age: i32 = 10;
```
The type can also be omitted, making use of the compiler's basic type inference.
```rust
let height = 175; // type of `height` is `i32`
```
Reassignments are in the form `<identifier> = <rvalue>;`.
```rust
age = 23;
```
### Functions
Function declarations are in the form:
```rust
func <identifier>(<arg-identifier>: <arg-type>) -> <return-type>
{
    <body>
}

func add(a: i32, b: i32) -> i32
{
    return a + b;
}
```
Functions that don't return anything have to be explicitly declared as returning `void`.
```rust
func say_hello() -> void
{
    // ...
}
```
Functions that need to be linked later (like when interoperating with C libraries) can be declared `extern` and do not need function bodies.
```rust
extern func puts(message: &char) -> i32;
```
Functions can be called the usual way, like in other languages.
```rust
puts("this is the message");
say_hello();
let result = add(10, 20);
```
### Control flow
Octo currently supports `if`-statements and `while`-loops which are used the same way as in other languages.
```rust
let number = 10;
if number > 10
{
    puts("wow that is a big number");
}
else
{
    puts("mid sized number");
}

while number > 0
{
    puts("this will be printed 10 times");
    number = number - 1;
}
```
The expression beside the `while` and `if` keywords must evaluate to a `bool`. Otherwise, the compiler will throw an error.

### Pointers
Pointers in are declared using the `&` operator. To get the address of a variable, simply prefix the variable's identifier with the `&` operator.
```rust
let my_var = 10;
let my_ptr = &my_var;
```
In the above example, `my_ptr` is of type `&i32`.

Pointers can be dereferenced with the `*` operator.
```rust
let deref = *my_ptr;
```

### Types
Octo contains the following built-in types:
| Symbol | Size in bytes | Description |
|:-:|:-:|:-:|
| `void` | N/A | type representing no type |
| `i8` | 1 | 8-bit signed integer |
| `i16` | 2 | 16-bit signed integer |
| `i32` | 4 | 32-bit signed integer, the default integer type |
| `i64` | 8 | 64-bit signed integer |
| `u8` | 1 | 8-bit unsigned integer |
| `u16` | 2 | 16-bit unsigned integer |
| `u32` | 4 | 32-bit unsigned integer |
| `u64` | 8 | 64-bit unsigned integer |
| `f32` | 4 | single-precision floating point, the default floating point type |
| `f64` | 8 | double-precision floating point |
| `char` | 1 | character |
| `bool` | 1 | boolean |
| `&T` | 8 | pointer to `T` |
| `[n]T` | 8 | array of `T` with size `n` |

Octo is strict with types. Implicit casts from signed to unsigned types and the reverse are not allowed. Implicit casts from integer to floating point types and the reverse are also not allowed. Numeric types (floats and integers) can only be implicitly casted from lower size to higher size (e.g. `i16 -> i32`, `f32 -> f64`) but the reverse is not allowed.
```rust
let my_int = 123;          // type of my_int is i32
let unsigned: u32 = my_int // error: implicit cast from 'i32' to 'u32' is not allowed
let bigger: i64 = my_int   // OK
let float: f32 = my_int    // error: implicit cast from 'i32' to 'f32' is not allowed
```
Implicit casting of pointers is only allowed when casting `&void` to non-`&void` and non-`&void` to `&void`
```rust
let ptr: &void = &my_int;
```
Also unlike C, `char`s are not treated as `u8`s. Meaning operations that are normally allowed on `u8`s (e.g. addition, subtraction, multiplication, etc.) may not be possible on `char`s without first converting them to integers.
```rust
'a' + 10 // ERROR
```
Octo also supports null-terminated string literals which are no different from string literals in C.
