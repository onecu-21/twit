# Twit 🐦
A simple, fast compiled programming language powered by LLVM.
## Features
- Simple syntax inspired by C and Python
- Static typing
- Native compilation via LLVM
- `print <<` and `input >>` built-in I/O
## Official Homepage
```
twit.onecu.dev
```
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
### MacOS
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
```
### Functions
```
function add(int x, int y) = int {
    return x + y
}
```
### Print & Input
```
print << "hello world"
print << x
new int x
input >> x
```
### Control Flow
```
if (x > 0) {
    print << "positive"
} else {
    print << "negative"
}
while (x > 0) {
    print << x
}
for (new int i = 0; i < 10; i + 1) {
    print << i
}
```
## License
Apache 2.0
