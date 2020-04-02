# uDepot Hopscotch

[uDepot](https://www.usenix.org/conference/fast19/presentation/kourtis)'s index structure is based on [hopscotch hashing](https://en.wikipedia.org/wiki/Hopscotch_hashing), which linearly probes entries but bounds probe distance within a neighborhood. Its modified hopscotch hash-table effectively performs key-value addressing on top of fast NVMe devices and table resizing itself. In this project, we implement the modified hopscotch hash-table and evaluate its performance under specific configurations.

## Description

An implementation for a modified hopscotch hash-table introduced in uDepot paper (FAST '19)

## Authors

Jinwook Bae (jinwook.bae@dgist.ac.kr)

