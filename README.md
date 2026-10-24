# Infinite Layout Studio

C++ -> WebAssembly generator of website layouts. Every layout is a function of a seed, is built as
real HTML + CSS you can copy or download, and a small on-device AI learns which layouts you like.

Tick "Add JavaScript" to include a mobile menu, light/dark toggle and scroll animations. Export as one file or as separate index.html, style.css and script.js.

## Deploy on GitHub
1. Create a repo and upload this folder's contents to the `main` branch (keep the `src`, `web` and `.github` folders).
2. Settings -> Pages -> Source: **GitHub Actions**.
3. The workflow builds the C++ with Emscripten and publishes `web/` at `https://<user>.github.io/<repo>/`.

## Run locally
```
bash build.sh
python3 -m http.server -d web 8000
```

## Extend it
Add new sections in `body()` and styles in `css()` (`src/main.cpp`), randomize them in `make()`,
and add a slot for them in `feat()` so the AI can learn them.
