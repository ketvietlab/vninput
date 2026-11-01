# Third-party notices

- **Qt 6** — Core, Gui, Widgets; Test for development only. Qt is provided by the
  developer's selected Qt installation. License information:
  https://www.qt.io/licensing/ and https://doc.qt.io/qt-6/licensing.html.
- **Két Design System 0.1.29** — semantic color values adapted in `theme.cpp` and
  `controls.hpp`. Copyright (c) 2026 KETVIET JSC, Vietnam. MIT license reproduced
  in [third_party/licenses/ket-design-system.txt](third_party/licenses/ket-design-system.txt).
  Source: https://github.com/ketvietlab/ketjs/tree/develop/packages/design-system.
- **VNInput logo** — generated asset, copied unchanged from
  `design/public/brand/vninput-logo.png` into `resources/branding/vninput-logo.png`.
  Prompt/provenance: [design/public/brand/README.md](design/public/brand/README.md).
  The file is covered by the repository MIT License, but the VNInput name and logo
  are trademarks of KETVIET JSC; the MIT License grants no trademark rights.
- Fonts are resolved from the operating system. No Apple font files are included
  in this repository or distribution assets.

The web mockup under `design/` has its own package manifest and lockfile. Those
packages are not runtime dependencies of the C++ application.
