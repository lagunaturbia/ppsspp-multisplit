# Upstream engine

The bundled engine reports PPSSPP revision `cdf563b8`. The full matching upstream Git commit resolves to `cdf563b85c42c275ec2835a0883f48d63a9c9406`.

- Source: https://github.com/hrydgard/ppsspp/tree/cdf563b85c42c275ec2835a0883f48d63a9c9406
- Clone recursively and check out that commit; initialize its pinned submodules.
- Upstream libretro build files: https://github.com/hrydgard/ppsspp/tree/cdf563b85c42c275ec2835a0883f48d63a9c9406/libretro
- Original embedded binary SHA-256: `c792d088a9ceb4db6ed6b6d1002a94efe0bc52fcfdcb356f9945083d20ca9e0f`.
- Validated worker SHA-256: `4e1ae8643d9626640b8b9b8f5b8d9bb3c9deee6f9a5ae885a311844348c3bfc5`.

The exact compiler configuration of the prebuilt upstream engine was not recovered. The matching commit is identified by its runtime version, and the binary is unchanged from earlier local builds. A bit-for-bit upstream rebuild has not been verified. Before public binary distribution, verify corresponding upstream source/build provenance and any third-party notices and source-distribution obligations. The upstream GPL license and notices must be retained.

The bridge-only build preserves both exact original payloads. The full build accepts an upstream DLL explicitly or the released MultiSplit DLL/ZIP and recompiles the worker as well as the bridge. A freshly compiled worker is not necessarily byte-identical to the released one.
