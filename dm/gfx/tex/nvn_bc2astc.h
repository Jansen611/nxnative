#pragma once
#include <stddef.h>
#include <stdint.h>

int nvn_bc_astc_fmt(uint32_t nvn_fmt, uint32_t *mtl_pf, int *kind);
uint32_t nvn_bc2astc_block(int kind, const uint8_t *bc, uint8_t *astc);
int nvn_bc2astc(uint32_t nvn_fmt, const uint8_t *src, size_t pitch, uint32_t w, uint32_t h, uint8_t *dst, size_t dpitch);
void nvn_astc_zero_block(uint8_t out[16]);