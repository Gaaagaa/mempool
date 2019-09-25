
### 分片类型表:

| step  |    8 |   16 |   32 |   64 |  128 |  256 |   512 |  1024 |  2048 |  4096 |   X   |
| :---: | ---: | ---: | ---: | ---: | ---: | ---: | ----: | ----: | ----: | ----: | :---: |
| size  |    8 |  144 |  288 |  576 | 1152 | 2304 |  4608 |  9216 | 18432 | 36864 |   X   |
| size  |   16 |  160 |  320 |  640 | 1280 | 2560 |  5120 | 10240 | 20480 | 40960 |   X   |
| size  |   24 |  176 |  352 |  704 | 1408 | 2816 |  5632 | 11264 | 22528 | 45056 |   X   |
| size  |   32 |  192 |  384 |  768 | 1536 | 3072 |  6144 | 12288 | 24576 | 49152 |   X   |
| size  |   40 |  208 |  416 |  832 | 1664 | 3328 |  6656 | 13312 | 26624 | 53248 |   X   |
| size  |   48 |  224 |  448 |  896 | 1792 | 3584 |  7168 | 14336 | 28672 | 57344 |   X   |
| size  |   56 |  240 |  480 |  960 | 1920 | 3840 |  7680 | 15360 | 30720 | 61440 |   X   |
| size  |   64 |  256 |  512 | 1024 | 2048 | 4096 |  8192 | 16384 | 32768 | 65536 |   X   |
| size  |   72 |      |      |      |      |      |       |       |       |       |   X   |
| size  |   80 |      |      |      |      |      |       |       |       |       |   X   |
| size  |   88 |      |      |      |      |      |       |       |       |       |   X   |
| size  |   96 |      |      |      |      |      |       |       |       |       |   X   |
| size  |  104 |      |      |      |      |      |       |       |       |       |   X   |
| size  |  112 |      |      |      |      |      |       |       |       |       |   X   |
| size  |  120 |      |      |      |      |      |       |       |       |       |   X   |
| size  |  128 |      |      |      |      |      |       |       |       |       |   X   |
| ----- | ---- | ---- | ---- | ---- | ---- | ---- | ----- | ----- | ----- | ----- | ----- |
| count |   16 |    8 |    8 |    8 |    8 |    8 |     8 |     8 |     8 |     8 |  88   |

### 分类方案表:

|  NO. | slice size (B) | chunk size (KB) | capacity | unused (B) |
| ---: | -------------: | --------------: | -------: | ---------: |
| [01] |              8 |             260 |    26616 |          0 |
| [02] |             16 |             284 |    16152 |          0 |
| [03] |             24 |             268 |    10552 |          0 |
| [04] |             32 |             292 |     8792 |          0 |
| [05] |             40 |             320 |     7800 |          0 |
| [06] |             48 |             320 |     6552 |          0 |
| [07] |             56 |             344 |     6072 |          0 |
| [08] |             64 |             344 |     5336 |          0 |
| [09] |             72 |             388 |     5368 |          0 |
| [10] |             80 |             412 |     5144 |          0 |
| [11] |             88 |             320 |     3640 |          0 |
| [12] |             96 |             376 |     3928 |          0 |
| [13] |            104 |             304 |     2936 |          0 |
| [14] |            112 |             416 |     3736 |          0 |
| [15] |            120 |             388 |     3256 |          0 |
| [16] |            128 |             320 |     2520 |          0 |
| [17] |            144 |             332 |     2328 |          0 |
| [18] |            160 |             500 |     3160 |          0 |
| [19] |            176 |             516 |     2968 |          0 |
| [20] |            192 |             332 |     1752 |          0 |
| [21] |            208 |             320 |     1560 |          0 |
| [22] |            224 |             528 |     2392 |          0 |
| [23] |            240 |             520 |     2200 |          0 |
| [24] |            256 |             764 |     3032 |          0 |
| [25] |            288 |             460 |     1624 |          0 |
| [26] |            320 |             712 |     2264 |          0 |
| [27] |            352 |             296 |      856 |          0 |
| [28] |            384 |             564 |     1496 |          0 |
| [29] |            416 |             872 |     2136 |          0 |
| [30] |            448 |             320 |      728 |          0 |
| [31] |            480 |             644 |     1368 |          0 |
| [32] |            512 |            1008 |     2008 |          0 |
| [33] |            576 |             700 |     1240 |          0 |
| [34] |            640 |             296 |      472 |          0 |
| [35] |            704 |             744 |     1079 |          2 |
| [36] |            768 |             740 |      984 |          0 |
| [37] |            832 |             852 |     1046 |          4 |
| [38] |            896 |             300 |      342 |          4 |
| [39] |            960 |             684 |      728 |          0 |
| [40] |           1024 |             472 |      471 |          2 |
| [41] |           1152 |             532 |      472 |          0 |
| [42] |           1280 |             268 |      214 |          4 |
| [43] |           1408 |             292 |      212 |          8 |
| [44] |           1536 |             320 |      213 |          6 |
| [45] |           1664 |             768 |      472 |          0 |
| [46] |           1792 |             820 |      468 |          8 |
| [47] |           1920 |             764 |      407 |          2 |
| [48] |           2048 |            1024 |      511 |        946 |
| [49] |           2304 |             768 |      341 |          6 |
| [50] |           2560 |             528 |      211 |         10 |
| [51] |           2816 |             944 |      343 |          2 |
| [52] |           3072 |            1024 |      341 |        262 |
| [53] |           3328 |             696 |      214 |          4 |
| [54] |           3584 |             732 |      209 |         14 |
| [55] |           3840 |             788 |      210 |         12 |
| [56] |           4096 |            1024 |      255 |       3506 |
| [57] |           4608 |             968 |      215 |          2 |
| [58] |           5120 |            1016 |      203 |        538 |
| [59] |           5632 |             996 |      181 |         70 |
| [60] |           6144 |            1016 |      169 |       1630 |
| [61] |           6656 |            1008 |      155 |        122 |
| [62] |           7168 |            1016 |      145 |        654 |
| [63] |           7680 |             968 |      129 |        174 |
| [64] |           8192 |            1020 |      127 |       3762 |
| [65] |           9216 |            1000 |      111 |        722 |
| [66] |          10240 |            1012 |      101 |       1766 |
| [67] |          11264 |            1024 |       93 |        758 |
| [68] |          12288 |            1024 |       85 |       3846 |
| [69] |          13312 |             976 |       75 |        794 |
| [70] |          14336 |            1024 |       73 |       1822 |
| [71] |          15360 |             976 |       65 |        814 |
| [72] |          16384 |            1012 |       63 |       3890 |
| [73] |          18432 |             992 |       55 |       1858 |
| [74] |          20480 |            1024 |       51 |       3914 |
| [75] |          22528 |             992 |       45 |       1878 |
| [76] |          24576 |            1012 |       42 |       3932 |
| [77] |          26624 |            1016 |       39 |       1890 |
| [78] |          28672 |            1012 |       36 |       3944 |
| [79] |          30720 |             992 |       33 |       1902 |
| [80] |          32768 |             996 |       31 |       3954 |
| [81] |          36864 |            1012 |       28 |       3960 |
| [82] |          40960 |            1004 |       25 |       3966 |
| [83] |          45056 |            1016 |       23 |       3970 |
| [84] |          49152 |            1012 |       21 |       3974 |
| [85] |          53248 |             992 |       19 |       3978 |
| [86] |          57344 |            1012 |       18 |       3980 |
| [87] |          61440 |            1024 |       17 |       3982 |
| [88] |          65536 |             964 |       15 |       3986 |
