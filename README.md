# ADM interop libraries

Official [ADM](https://github.com/admlang/adm) libraries that run other languages inside an ADM
program. Each library embeds one engine, is published as `adm.interop.<language>` and is signed by
the `admlang` publisher.

| Library | Language | Engine | Status |
|---------|---|---|---|
| [`adm.interop.js`](js/) | JavaScript | QuickJS-ng 0.17.0 | Run scripts and ES modules, exchange values, call in both directions |
| [`adm.interop.lua`](lua/) | Lua | Lua 5.4.8 | Run scripts and modules, exchange values, call in both directions |
|-|Python|-|In progress|
|-|Java|-|In progress|
|-|WebAssembly|-|In progress|

## Install

Install a library into your project with `adm get`, for example:

```bash
adm get admlang:adm.interop.js
```

Each library's README covers its use.

## License

The engines keep their own licenses, recorded beside their sources (`js/quickjs/LICENSE`, `lua/lua/LICENSE`).
