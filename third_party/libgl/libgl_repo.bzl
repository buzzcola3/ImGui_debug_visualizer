"""Repository rule to vend prebuilt Mesa/GL binaries."""

_PACKAGES = {
    "amd64": [
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
    ],
    "arm64": [
        {
            "name": "libgl1",
            "url": "https://deb.debian.org/debian/pool/main/libg/libglvnd/libgl1_1.7.0-1+b2_arm64.deb",
            "sha256": "7943f99962aa3316515760bf44e66f0c15a3c813ad750b784b43e319fa2e6003",
        },
        {
            "name": "libglvnd0",
            "url": "https://deb.debian.org/debian/pool/main/libg/libglvnd/libglvnd0_1.7.0-1+b2_arm64.deb",
            "sha256": "29ec7b8cf1c9a47055de643eb0a1921854e762b4789a7f3bc3b0dea498fd4da9",
        },
        {
            "name": "libglx-mesa0",
            "url": "https://deb.debian.org/debian/pool/main/m/mesa/libglx-mesa0_25.2.6-1_arm64.deb",
            "sha256": "fae96e5967f012bd22928d2ada5bd17709882d8ede8d1d1caaa8b68253bc6897",
        },
        {
            "name": "libgl1-mesa-dri",
            "url": "https://deb.debian.org/debian/pool/main/m/mesa/libgl1-mesa-dri_25.2.6-1_arm64.deb",
            "sha256": "9f800e662fe5722313c3290fcac4c62ea9780b6e7f8dfbc5ea7ccf4f8d49a851",
        },
        {
            "name": "mesa-common-dev",
            "url": "https://deb.debian.org/debian/pool/main/m/mesa/mesa-common-dev_25.2.6-1_arm64.deb",
            "sha256": "eab68adf321e3c9279d5504c2d5e075aa28b1d119cea708d0491609211a9ca30",
        },
    ],
}

_LIB_DIRS = {
    "amd64": "usr/lib/x86_64-linux-gnu",
    "arm64": "usr/lib/aarch64-linux-gnu",
}

_CPU_CONSTRAINTS = {
    "amd64": "@@platforms//cpu:x86_64",
    "arm64": "@@platforms//cpu:aarch64",
}

_SHARED_LIBS = [
    ("libGL", "libGL.so.1.7.0"),
    ("libGLX_mesa", "libGLX_mesa.so.0.0.0"),
    ("libGLdispatch", "libGLdispatch.so.0.0.0"),
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

def _build_file_content():
    lines = [
        'package(default_visibility = ["//visibility:public"])',
        "",
    ]

    for arch in sorted(_PACKAGES.keys()):
        lines.extend([
            "config_setting(",
            '    name = "is_{arch}",'.format(arch = arch),
            '    constraint_values = ["{constraint}"],'.format(constraint = _CPU_CONSTRAINTS[arch]),
            ")",
            "",
        ])

    for arch in sorted(_PACKAGES.keys()):
        lib_dir = _LIB_DIRS[arch]
        for stem, filename in _SHARED_LIBS:
            lines.extend([
                "cc_import(",
                '    name = "{stem}_import_{arch}",'.format(stem = stem, arch = arch),
                '    shared_library = "{lib_dir}/{filename}",'.format(lib_dir = lib_dir, filename = filename),
                ")",
                "",
            ])

    lines.extend([
        "cc_library(",
        '    name = "gl",',
        '    hdrs = glob(["usr/include/**/*.h"]),',
        '    includes = ["usr/include"],',
        '    deps = select({',
    ])

    for arch in sorted(_PACKAGES.keys()):
        lines.append('        ":is_{arch}": ['.format(arch = arch))
        for stem, _ in _SHARED_LIBS:
            lines.append('            ":{stem}_import_{arch}",'.format(stem = stem, arch = arch))
        lines.append('        ],')

    lines.extend([
        '        "//conditions:default": [],',
        '    }),',
        ")",
        "",
    ])

    return "\n".join(lines)

def _libgl_repository_impl(ctx):
    ctx.file("extract_deb.py", _EXTRACT_SCRIPT, executable = False)
    script_path = str(ctx.path("extract_deb.py"))

    for arch in sorted(_PACKAGES.keys()):
        for pkg in _PACKAGES[arch]:
            output = "{}_{}.deb".format(pkg["name"], arch)
            ctx.download(
                url = pkg["url"],
                sha256 = pkg["sha256"],
                output = output,
            )
            result = ctx.execute([
                "python3",
                script_path,
                str(ctx.path(output)),
                str(ctx.path(".")),
            ])
            if result.return_code:
                fail("Failed to extract %s (%s): %s" % (pkg["name"], arch, result.stderr))

    ctx.file("BUILD.bazel", _build_file_content())

libgl_repository = repository_rule(
    implementation = _libgl_repository_impl,
    attrs = {},
)
