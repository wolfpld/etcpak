# etcpak 1.0 #
(Updated 2022-06-04)

## The fastest ETC compressor on the planet ##

etcpak is an extremely fast [Ericsson Texture Compression](http://en.wikipedia.org/wiki/Ericsson_Texture_Compression) and S3 Texture Compression (DXT1/DXT5) utility. Currently it's best suited for rapid assets preparation during development, when graphics quality is not a concern, but it's also used in production builds of applications used by millions of people.

## Compression times ##

Benchmark performed on an AMD Ryzen 9 5950X, using a real-life RGBA 16K × 16K atlas.

ETC1: ST: **407 Mpx/s**, MT: **6730 Mpx/s**  
ETC2: ST: **222 Mpx/s**, MT: **3338 Mpx/s**  
DXT1: ST: **2688 Mpx/s**, MT: **8748 Mpx/s**  
DXT5: ST: **1281 Mpx/s**, MT: **6667 Mpx/s**

The same benchmark performed on Intel i7 1185G7:

ETC1: ST: **463 Mpx/s**, MT: **2189 Mpx/s**  
ETC2: ST: **229 Mpx/s**, MT: **1003 Mpx/s**  
DXT1: ST: **2647 Mpx/s**, MT: **9173 Mpx/s**  
DXT5: ST: **1294 Mpx/s**, MT: **5517 Mpx/s**

ARM benchmark performed on Odroid C2, using the same atlas:

ETC1: ST: **23.6 Mpx/s**, MT: **90.6 Mpx/s**  
ETC2: ST: **12.3 Mpx/s**, MT: **48.4 Mpx/s**  
DXT1: ST: **93.2 Mpx/s**, MT: **346 Mpx/s**  
DXT5: ST: **68.5 Mpx/s**, MT: **256 Mpx/s**

[Why there's no image quality metrics? / Quality comparison.](http://i.imgur.com/FxlmUOF.png)

## Decompression times ##

etcpak can also decompress ETC1, ETC2, DXT1 and DXT5 textures. Timings on Ryzen 5950X (all single-threaded):

ETC1: **385 Mpx/s**  
ETC2: **358 Mpx/s**  
DXT1: **653 Mpx/s**  
DXT5: **515 Mpx/s**

i7 1185G7:

ETC1: **344 Mpx/s**  
ETC2: **324 Mpx/s**  
DXT1: **1012 Mpx/s**  
DXT5: **720 Mpx/s**

Odroid C2:

ETC1: **48.9 Mpx/s**  
ETC2: **44.4 Mpx/s**  
DXT1: **104 Mpx/s**  
DXT5: **84.5 Mpx/s**

To give some perspective here, Nvidia in-driver ETC2 decoder can do only 42.5 Mpx/s.

## Quality comparison ##

Original image:

![](http://1.bp.blogspot.com/-kqFgRVL0uKY/UbSclN-fZdI/AAAAAAAAAxU/Fy87I8P4Yxs/s1600/kodim23.png)

Compressed image:

ETC1:
![](http://i.imgur.com/xmdht4u.png "ETC1 mode")
ETC2:
![](http://i.imgur.com/v7Dw2Yz.png "ETC2 mode")

## More information ##

[etcpak 0.6](http://zgredowo.blogspot.com/2018/07/etcpak-06.html)  
[etcpak 0.5](http://zgredowo.blogspot.com/2016/01/etcpak-05.html)  
[etcpak 0.4](http://zgredowo.blogspot.com/2016/01/etcpak-04.html)  
[etcpak 0.3](http://zgredowo.blogspot.com/2014/05/etcpak-03.html)  
[etcpak 0.2.2](http://zgredowo.blogspot.com/2014/03/etcpack-022.html)  
[etcpak 0.2.1](http://zgredowo.blogspot.com/2013/08/etcpak-021.html)   
[etcpak 0.2](http://zgredowo.blogspot.com/2013/07/etcpak-02.html)  
[etcpak 0.1](http://zgredowo.blogspot.com/2013/06/fastest-etc-compressor-on-planet.html)
