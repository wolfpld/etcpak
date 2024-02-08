project('etcpak', 'c', 'cpp', default_options: ['cpp_std=c++20'])

sources = [
    'Application.cpp',
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

zlib = [
    'zlib/adler32.c',
    'zlib/compress.c',
    'zlib/crc32.c',
    'zlib/deflate.c',
    'zlib/gzlib.c',
    'zlib/infback.c',
    'zlib/inffast.c',
    'zlib/inflate.c',
    'zlib/inftrees.c',
    'zlib/trees.c',
    'zlib/uncompr.c',
    'zlib/zutil.c'
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

deps = [
    tracy
]

if get_option('tracy_enable')
    add_project_arguments('-DTRACY_ENABLE', language: ['c', 'cpp'])
    if get_option('buildtype') != 'debugoptimized'
        message('Profiling builds should set --buildtype=debugoptimized')
    endif
endif

executable('etcpak',
    sources,
    zlib,
    libpng,
    lz4,
    include_directories: ['zlib'],
    dependencies: deps
)
