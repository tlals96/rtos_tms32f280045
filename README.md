# rtos_tms32f280045
> Using RTOS Kernel (SYS/BIOS)

<!-- [![Build Status][travis-image]][travis-url] -->



## IDE, Library Install URL
Windows : 
[IDE][ccs9_3-download], 
[Library][c2000ware-url],
[RTOS][rtos-download]

Compiler version : TI v20.2.1.LTS

Compiled in CCS v9.3.0, SYS/BIOS v6.83.00.18

## Examples
 `ti\ ..\ bios_6_76_04_02\resources\tiTargetExamples\28xSpecificExamples` 
 
 Import the project you want

_Refer to [Resource Explorer][resource-explorer] for more examples and instructions_

## Development environment setting

`Build, composite` in `CCS 9.3.0` in Windows 10 development environment

### How to set Linker command file, Compiler version
![](./images/ccs_general_setting.png)

Choose according to the processor (`General > Project > Tool-chain > Linker command file`)

Select the compiler version you use (`General > Project > Tool-chain > Compiler version`)

### How to set the build command
![](./images/ccs_build_setting.png)

Write down the number of logical processors on the development PC (`Build > Builder > Build Command`)
> ${CCS_UTILS_DIR}/bin/gmake -k `-j 12`

## Update history

* 0.0.1
    * Revise : Document update


<!-- Markdown link & img dfn's -->
[c2000ware-url]: https://www.ti.com/tool/C2000WARE
[ccs9_3-download]: https://software-dl.ti.com/ccs/esd/documents/ccs_downloads.html#code-composer-studio-version-9-downloads
[rtos-download]: https://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/bios/sysbios/index.html
[travis-image]: https://img.shields.io/travis/tlals96/f280045_DMA_ADC/master.svg?style=flat-square
[travis-url]: https://app.travis-ci.com/github/tlals96/f280045_DMA_ADC
[resource-explorer]: https://dev.ti.com/tirex/explore/node?compilers=ccs&devices=F280045&node=AL.a.macE.c7rSiGSB7klQ__gYkahfz__LATEST
[general-setting]: https://https://img.shields.io/travis/tlals96/f280045_DMA_ADC/images/ccs_general_setting.png?retina