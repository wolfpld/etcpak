# etcpak 1.0 #
(Updated 2022-06-04)

## The fastest ETC compressor on the planet ##

etcpak is an extremely fast [Ericsson Texture Compression](http://en.wikipedia.org/wiki/Ericsson_Texture_Compression) and S3 Texture Compression (DXT1/DXT5) utility. Currently it's best suited for rapid assets preparation during development, when graphics quality is not a concern, but it's also used in production builds of applications used by millions of people.

## Compression times ##

Benchmark performed on an AMD Ryzen 9 7950X, using a real-life RGBA 16K × 16K atlas.

ETC1: ST: **797 Mpx/s**, MT: **9613 Mpx/s**  
ETC2: ST: **335 Mpx/s**, MT: **6016 Mpx/s**  
DXT1: ST: **4819 Mpx/s**, MT: **10174 Mpx/s**  
DXT5: ST: **2287 Mpx/s**, MT: **8038 Mpx/s**

The same benchmark performed on Intel i7 1185G7:

ETC1: ST: **516 Mpx/s**, MT: **2223 Mpx/s**  
ETC2: ST: **238 Mpx/s**, MT: **984 Mpx/s**  
DXT1: ST: **2747 Mpx/s**, MT: **9054 Mpx/s**  
DXT5: ST: **1331 Mpx/s**, MT: **5483 Mpx/s**

ARM benchmark performed on Odroid C2, using the same atlas:

ETC1: ST: **23.6 Mpx/s**, MT: **90.6 Mpx/s**  
ETC2: ST: **12.3 Mpx/s**, MT: **48.4 Mpx/s**  
DXT1: ST: **93.2 Mpx/s**, MT: **346 Mpx/s**  
DXT5: ST: **68.5 Mpx/s**, MT: **256 Mpx/s**

[Why there's no image quality metrics? / Quality comparison.](http://i.imgur.com/FxlmUOF.png)

## Decompression times ##

etcpak can also decompress ETC1, ETC2, DXT1 and DXT5 textures. Timings on Ryzen 7950X (all single-threaded):

ETC1: **604 Mpx/s**  
ETC2: **599 Mpx/s**  
DXT1: **1489 Mpx/s**  
DXT5: **1077 Mpx/s**

i7 1185G7:

ETC1: **383 Mpx/s**  
ETC2: **378 Mpx/s**  
DXT1: **1033 Mpx/s**  
DXT5: **788 Mpx/s**

Odroid C2:

ETC1: **48.9 Mpx/s**  
ETC2: **44.4 Mpx/s**  
DXT1: **104 Mpx/s**  
DXT5: **84.5 Mpx/s**

To give some perspective here, Nvidia in-driver ETC2 decoder can do only 42.5 Mpx/s.

## Quality comparison ##

Original image:

![](examples/kodim23.png)

Compressed image:

ETC1:

![](examples/etc1.png "ETC1 mode")

ETC2:

![](examples/etc2.png "ETC2 mode")

DXT1:

![](examples/dxtc.png "DXT1 mode")
