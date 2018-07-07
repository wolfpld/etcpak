# etcpak 0.5 #
(Updated 2016-01-27)

## The fastest ETC compressor on the planet ##

etcpak is an extremely fast [Ericsson Texture Compression](http://en.wikipedia.org/wiki/Ericsson_Texture_Compression) utility. Currently it's best suited for rapid assets preparation during development, when graphics quality is not a concern, but it's also used in production builds of applications used by millions of people.

## Compression times ##

In the following test a [8192x8192 RGB texture](https://bitbucket.org/wolfpld/etcpak/downloads/8192.png) was compressed on an [Intel Xeon E3-1230 v3](http://ark.intel.com/products/75054/Intel-Xeon-Processor-E3-1230-v3-8M-Cache-3_30-GHz).

| Tool | Image decode | Compression | Total time | Throughput |
|------|--------------|-------------|------------|------------|
|PVRTexToolCLI 3.5 (SDK 4.0)|||**51.69 s**|1.3 Mpx/s|
|mali etcpack 4.2|5.12 s ᵃ|86.31 s|**91.43 s**|0.777 Mpx/s|
|etc2comp (effort=0, format=ETC1)|6 s ᵃ|34.3 s|**40.3 s**|1.96 Mpx/s|
|crunch (rg-etc1) 1.04|1.96 s ᵇ|5.82 s ᵇ|**8.872 s** ᵇ|11.53 Mpx/s|
|*etcpak 0.5 (ETC1)*|*1.21 s*|*0.077 s*|** *1.26 s* **|872.6 Mpx/s|
|*etcpak 0.5 (ETC2 RGB)*|*1.21 s*|*0.117 s*|** *1.26 s* **|573.5 Mpx/s|
|*etcpak 0.5 (ARM64ᶜ)*|*4.66 s*|*2.975 s*|** *5.14 s* **|22.6 Mpx/s|

ᵃ Total time minus compression time.

ᵇ crunch times multiplied by 4, as it only supports 4096x4096 textures.

ᶜ [Odroid-C2](http://www.hardkernel.com/main/products/prdt_info.php?g_code=G145457216438)

[Why there's no image quality metrics? / Quality comparison.](http://i.imgur.com/FxlmUOF.png)  
[Workload distribution.](https://i.imgur.com/9ZUy4KP.png)

## Quality comparison ##

Original image:

![](http://1.bp.blogspot.com/-kqFgRVL0uKY/UbSclN-fZdI/AAAAAAAAAxU/Fy87I8P4Yxs/s1600/kodim23.png)

Compressed image:

ETC1:
![](http://i.imgur.com/xmdht4u.png "ETC1 mode")
ETC2:
![](http://i.imgur.com/v7Dw2Yz.png "ETC2 mode")

## More information ##

[etcpak 0.5](http://zgredowo.blogspot.com/2016/01/etcpak-05.html)  
[etcpak 0.4](http://zgredowo.blogspot.com/2016/01/etcpak-04.html)  
[etcpak 0.3](http://zgredowo.blogspot.com/2014/05/etcpak-03.html)  
[etcpak 0.2.2](http://zgredowo.blogspot.com/2014/03/etcpack-022.html)  
[etcpak 0.2.1](http://zgredowo.blogspot.com/2013/08/etcpak-021.html)   
[etcpak 0.2](http://zgredowo.blogspot.com/2013/07/etcpak-02.html)  
[etcpak 0.1](http://zgredowo.blogspot.com/2013/06/fastest-etc-compressor-on-planet.html)
