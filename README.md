# Eye

**🚧 Work in progress 🚧**

Eye is a statically typed compiled language implemented using LLVM, with a syntax inspired by TypeScript.

```typescript
interface Counter {
  inc: () => i64,
  reset: () => void
}

function makeCounter(initialValue: i64): Counter {
  let counter = initialValue;
  let inc = (): i64 => {
    counter = counter + 1;
    return counter;
  };
  let reset = (): void => {
    counter = 0;
  };
  return { inc: inc, reset: reset };
}

function main(): i64 {
  let counter = makeCounter(10);
  let inc = counter.inc;
  let reset = counter.reset;
  print(inc());
  print(inc());
  reset();
  print(inc());
  print(inc());
  return 0;
}
```

Roadmap is available in [`TODO.md`](TODO.md).

## Build

```bash
make all
```

Make sure you have LLVM installed.

On macOS 13, I have LLVM installed through [Brew](https://formulae.brew.sh/formula/llvm), with following environment variables set:

```bash
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export SDKROOT="$(xcrun --show-sdk-path)"
export LIBRARY_PATH="$LIBRARY_PATH:$SDKROOT/usr/lib"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"
```

## Run example programs

```bash
# compile and run
make program SOURCE_FILE=snapshots/closure.eye && ./program

# compile and run (debug)
DEBUG=1 make program SOURCE_FILE=snapshots/closure.eye && ./program
```

## Tests

Testing is done by checking the expected change in snapshots.

```bash
make snapshot
```

## Name

The name "Eye" is inspired by the [IJ bay in Amsterdam](https://en.wikipedia.org/wiki/IJ_(Amsterdam)).