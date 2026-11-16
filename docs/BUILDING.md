# Building PartyFrames Light (SE/AE)

Use Windows x64, Visual Studio 2022 Build Tools, CMake 3.24+ and Ninja. The release uses C++23, the release MSVC dynamic runtime (/MD), and alandtse CommonLibSSE-NG with both SE and AE enabled.

Clone https://github.com/alandtse/CommonLibVR into `external/CommonLibSSE-NG` (despite the upstream repository name, its NG branch supports SE/AE). The release was built against commit `12d61f3c39f7d4ba1b1ab543df2bca0e613dc747`. Obtain its declared dependencies through vcpkg using `x64-windows-static-md`; PartyFrames also requires nlohmann-json, spdlog and directxtk. The checked-in headers cover the consumed Magelight, Canine Drives, SIF and JsonCpp interfaces. Provider DLLs are not linked or bundled.

From an x64 VS developer command prompt, configure with dependency paths for your machine:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPARTYLIGHT_RUNTIME=flatrim -DCOMMONLIB_DIR=C:/path/to/CommonLibSSE-NG -DDEPS=C:/path/to/vcpkg_installed/x64-windows-static-md
cmake --build build
ctest --test-dir build --output-on-failure
```

By default CommonLib builds from source. Alternatively pass `-DCOMMONLIB_LIBRARY=C:/path/to/CommonLibSSE.lib` for an existing library compiled with matching headers, SE+AE targets and /MD CRT. Do not use an SE-only, VR-only or Debug library for the public SE/AE target. `COMMONLIB_LIBRARY` is ignored by the experimental internal VR target; no VR binary is released.

Copy `build/PartyLight.dll` into `package/SKSE/Plugins/`, then archive the contents of `package/`, not its parent folder. Runtime log/config and asset names intentionally remain PartyLight for update compatibility.

For UI checks, install Node.js and Playwright with Chromium. Set `PARTYLIGHT_PLAYWRIGHT` to your Playwright module if it is not on Node's module search path, then run `node verify-ui.cjs`. The script uses sample actors and writes screenshots into `verification/`. To regenerate the optional overhead icons, run `tools/build-overhead.cjs` with Playwright, then `tools/build-overhead-swf.py` with Python and Pillow. The generated SWF and rules are already included.

Live Skyrim tests are still required for engine integration; unit/browser checks do not substitute for those.
