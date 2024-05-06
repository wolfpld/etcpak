project('etcpak', 'c', 'cpp', default_options: ['cpp_std=c++20', 'b_lto=true'])

sources = [
    'Application.cpp',
    'bc7enc.cpp',
    'bcdec.c',
    'Bitmap.cpp',
    'BitmapDownsampled.cpp',
    'BlockData.cpp',
    'ColorSpace.cpp',
    'DataProvider.cpp',
    'Debug.cpp',
    'Dither.cpp',
    'Error.cpp',
    'mmap.cpp',
    'ProcessDxtc.cpp',
    'ProcessRGB.cpp',
    'System.cpp',
    'Tables.cpp',
    'TaskDispatch.cpp',
    'Timing.cpp'
]

libpng = [
    'libpng/arm_init.c',
    'libpng/filter_neon_intrinsics.c',
    'libpng/filter_sse2_intrinsics.c',
    'libpng/intel_init.c',
    'libpng/palette_neon_intrinsics.c',
    'libpng/png.c',
    'libpng/pngerror.c',
    'libpng/pngget.c',
    'libpng/pngmem.c',
    'libpng/pngpread.c',
    'libpng/pngread.c',
    'libpng/pngrio.c',
    'libpng/pngrtran.c',
    'libpng/pngrutil.c',
    'libpng/pngset.c',
    'libpng/pngtrans.c',
    'libpng/pngwio.c',
    'libpng/pngwrite.c',
    'libpng/pngwtran.c',
    'libpng/pngwutil.c'
]

lz4 = [
    'lz4/lz4.c'
]

compiler = meson.get_compiler('cpp')

tracy = dependency('tracy', static: true)
zlib_ng = dependency('zlib-ng', static: true)

deps = [
    tracy,
    zlib_ng
]

if get_option('tracy_enable')
    if get_option('buildtype') != 'debugoptimized'
        warning('Profiling builds should set --buildtype=debugoptimized')
    endif
    add_project_arguments('-DTRACY_ENABLE', language: ['c', 'cpp'])
    frame_pointer_args = compiler.get_supported_arguments([
        '-fno-omit-frame-pointer'
    ])
    add_project_arguments(frame_pointer_args, language: ['c', 'cpp'])
endif

add_project_arguments(['-DNO_GZIP', '-DPNG_INTEL_SSE'], language: ['c', 'cpp'])

if get_option('buildtype') != 'debug'
    add_project_arguments('-DNDEBUG', language: ['c', 'cpp'])
endif

if compiler.get_id() == 'msvc'
    add_project_arguments(['-DNOMINMAX', '-D__SSE4_1__', '-D__AVX2__', '/arch:AVX2', '/GL'], language: ['c', 'cpp'])
    add_project_link_arguments('/LTCG:incremental', language: ['c', 'cpp'])
    sources += [ 'getopt/getopt.c' ]
else
    cpu = host_machine.cpu_family()
    if cpu == 'arm' or cpu == 'aarch64'
        add_project_arguments('-mcpu=native', language: ['c', 'cpp'])
    else
        add_project_arguments('-march=native', language: ['c', 'cpp'])
    endif
endif

executable('etcpak',
    sources,
    libpng,
    lz4,
    dependencies: deps
)
