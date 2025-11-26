load("//third_party/libgl:libgl_repo.bzl", "libgl_repository")

def _libgl_extension_impl(module_ctx):
    libgl_repository(name = "mesa_gl")

libgl_extension = module_extension(
    implementation = _libgl_extension_impl,
)
