Exploring LLVM Passes: Simple Pass to convert Division to Shifts rights

Building the Pass:
LLVM_VERSION:20.0 on Target x86_64-apple-darwin23.3.0
```
mkdir build && cd build
cmake -DCMAKE_CXX_COMPILER=$LLVM_DIR/clang++ -DCMAKE_C_COMPILER=$LLVM_DIR/clang ../
make
```

Generating LLVM IR:

```
clang++ -S -emit-llvm -Xclang -disable-O0-optnone test.cpp -o test.ll
opt -load-pass-plugin /path/to/libMS.dylib -passes=div-shift test.ll -o mod.bc
llvm-dis mod.bc -o mod.ll  
```
