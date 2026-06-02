# Twit 🐦

A simple, fast compiled programming language powered by LLVM.

## Features
- Simple syntax inspired by C and Python
- Static typing
- Native compilation via LLVM
- `print <<` and `input >>` built-in I/O
- Garbage Collection (Boehm GC)
- `import math` support

## Installation

### Linux
```bash
git clone https://github.com/onecu-21/twit
cd twit
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo cp twitc /usr/local/bin/twit
```

### Windows
Use WSL (Windows Subsystem for Linux) 🙂

### macOS
Untested... I think it will work... **maybe**...

## Usage

```bash
twit hello.twit -o hello
./hello
```

## Syntax

### Variables
```
new int x = 5
new float y = 3.14
new double z = 3.141592653589793
new long l = 9999999999
new bool b = true
new string s = "hello"
new int arr[10]
```

### Functions
```
function add(int a, int b) = int {
    return a + b
}
```

### Print & Input
```
print << "hello world"
print << x

new int x
input >> x
```

### Operators
```
x++  x--  ++x  --x
x += 1  x -= 1  x *= 2  x /= 2
x % 2
```

### Control Flow
```
if (x > 0) {
    print << "positive"
} else if (x == 0) {
    print << "zero"
} else {
    print << "negative"
}

while (x > 0) {
    x--
}

for (new int i = 0; i < 10; i++) {
    if (i == 5) { break }
    if (i == 3) { continue }
    print << i
}
```

### Import
```
import math

function main() = int {
    new double x = sqrt(2.0)
    new double y = pow(2.0, 10.0)
    print << x
    print << y
    return 0
}
```

### Available math functions
- `sqrt`, `sin`, `cos`, `tan`
- `log`, `log2`, `floor`, `ceil`, `round`
- `pow`, `fmod`, `fabs`

### Structs
```
struct Point {
    int x
    int y
}
```

## License
Apache 2.0