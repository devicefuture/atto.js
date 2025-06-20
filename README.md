# `atto.js`
Embed the [atto programming language](https://atto.devicefuture.org) into your JavaScript projects!

Want to see it for yourself? Check out the [editor demo](https://attojs.devicefuture.org/examples/editor/)!

## Developing locally
To clone this repo, you'll need to use the `--recurse-submodules` flag on `git clone`:

```bash
git clone --recures-submodules https://github.com/devicefuture/atto.js
```

## Building
[catto](https://github.com/devicefuture/catto) is the C implementation of the atto programming language, and atto.js uses it as a dependency, which is running inside a WebAssembly module. To build catto and produce a binary for WebAssembly, run:

```bash
./build.sh
```