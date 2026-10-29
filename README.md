# Taint

Taint ([pick your favorite definition, they all fit](https://www.merriam-webster.com/dictionary/taint)) is a
stripped-down fork/wrapper of Tint from Google's [Dawn](https://dawn.googlesource.com/dawn/) repo with a sane (read:
ungoogled) build system and [a (soon-to-be) stable C API](include/taint.h).

It is focused entirely on SPIR-V to WGSL conversion and nothing else. No other functionality will be provided without a
very good reason.

Upstream is tracked on a whenever-I'm-feeling-like-it basis. The [`pull-upstream.sh`](pull-upstream.sh) script is used
to fetch and marge the latest changes from Dawn's main branch. It uses
[git-subtree](https://www.atlassian.com/git/tutorials/git-subtree) and can be ran locally, though every run will take
a long time because Dawn is huge.

## Building

Taint uses the [Meson](https://mesonbuild.com/) build system. It depends on `SPIRV-Headers` and `SPIRV-Tools`. It can be
used as a meson subproject, or installed into the system. When installed, a `pkgconfig` file is provided for integration
with other build systems.

Unfortunately, Tint is naughty and likes to play with SPIRV-Tools-opt's private parts. For that reason, we pull in 
our mesonified fork of `SPIRV-Tools` and link it statically. It's based on a stable version of SPIRV-Tools (currently
`v2026.4`), and may need to be occasionally updated manually if a new version of Tint breaks. Thank google for not 
giving a shit; at least we don't pin random git commits here.

## B-but why not CMake??

Because I hate CMake, I don't use CMake, and I want nothing to do with CMake. You're free to contribute a CMake build
system if you're willing to maintain it. Should be easier than attempting to use than whatever horrors google came up
with. Before you do that though, I'd recommend seeing if pkgconfig works for your needs.
