"""Repository rule to vend prebuilt Mesa/GL binaries."""

_PACKAGES = [
    {
        "name": "libgl1",
        "url": "https://deb.debian.org/debian/pool/main/libg/libglvnd/libgl1_1.7.0-1+b2_amd64.deb",
        "sha256": "87fa2f6e5abaed4ed385fac879c8dd735af719ee2300222d901793c66e041678",
    },
    {
        "name": "libglvnd0",
        "url": "https://deb.debian.org/debian/pool/main/libg/libglvnd/libglvnd0_1.7.0-1+b2_amd64.deb",
        "sha256": "887f74008166549ce9e100c906aa937e95d6e5ce1c8d86efe8c95fd953359b9c",
    },
    {
        "name": "libglx-mesa0",
        "url": "https://deb.debian.org/debian/pool/main/m/mesa/libglx-mesa0_25.2.6-1_amd64.deb",
        "sha256": "e2d2a113932b31b8b9d3c483595d337b69bccd6651372f66a820422f56aef83e",
    },
    {
        "name": "libgl1-mesa-dri",
        "url": "https://deb.debian.org/debian/pool/main/m/mesa/libgl1-mesa-dri_25.2.6-1_amd64.deb",
        "sha256": "70aed9cc7a049d36a9e8c0df6f2eeb48ea065cce5d4ff150975f5f6c8b72fad1",
    },
    {
        "name": "mesa-common-dev",
        "url": "https://deb.debian.org/debian/pool/main/m/mesa/mesa-common-dev_25.2.6-1_amd64.deb",
        "sha256": "666682528aa85886ff086cd7a522a3c2d9732888b7db841373d87d8b9a952c26",
    },
]

_EXTRACT_SCRIPT = """
import io
import os
import sys
import tarfile

def _extract_deb(deb_path, out_dir):
    with open(deb_path, "rb") as deb_file:
        magic = deb_file.read(8)
        if magic != b"!<arch>\\n":
            raise RuntimeError(f"{deb_path} is not an ar archive")
        while True:
            header = deb_file.read(60)
            if len(header) < 60:
                break
            name = header[:16].decode("utf-8").strip()
            size = int(header[48:58].decode("utf-8").strip())
            payload = deb_file.read(size)
            if size % 2 == 1:
                deb_file.read(1)
            if name.startswith("data.tar"):
                if name.endswith("xz"):
                    mode = "r:xz"
                elif name.endswith("gz"):
                    mode = "r:gz"
                elif name.endswith("bz2"):
                    mode = "r:bz2"
                else:
                    mode = "r:"
                with tarfile.open(fileobj=io.BytesIO(payload), mode=mode) as tar:
                    tar.extractall(out_dir)
                return
    raise RuntimeError(f"No data.tar.* member found inside {deb_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit(f"Usage: {sys.argv[0]} <deb> <output>")
    _extract_deb(sys.argv[1], sys.argv[2])
"""

_BUILD_FILE = """
package(default_visibility = ["//visibility:public"])

cc_import(
    name = "libGL_import",
    shared_library = "usr/lib/x86_64-linux-gnu/libGL.so.1.7.0",
)

cc_import(
    name = "libGLX_mesa_import",
    shared_library = "usr/lib/x86_64-linux-gnu/libGLX_mesa.so.0.0.0",
)

cc_import(
    name = "libGLdispatch_import",
    shared_library = "usr/lib/x86_64-linux-gnu/libGLdispatch.so.0.0.0",
)

cc_library(
    name = "gl",
    hdrs = glob(["usr/include/**/*.h"]),
    includes = ["usr/include"],
    deps = [
        ":libGL_import",
        ":libGLX_mesa_import",
        ":libGLdispatch_import",
    ],
)
"""

def _libgl_repository_impl(ctx):
    ctx.file("extract_deb.py", _EXTRACT_SCRIPT, executable = False)
    script_path = str(ctx.path("extract_deb.py"))

    for pkg in _PACKAGES:
        output = pkg["name"] + ".deb"
        ctx.download(
            url = pkg["url"],
            sha256 = pkg["sha256"],
            output = output,
        )
        deb_path = str(ctx.path(output))
        result = ctx.execute([
            "python3",
            script_path,
            deb_path,
            str(ctx.path(".")),
        ])
        if result.return_code:
            fail("Failed to extract %s: %s" % (pkg["name"], result.stderr))

    ctx.file("BUILD.bazel", _BUILD_FILE)

libgl_repository = repository_rule(
    implementation = _libgl_repository_impl,
    attrs = {},
)
