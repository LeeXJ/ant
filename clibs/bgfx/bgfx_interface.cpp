#include "bgfx_interface.h"

extern "C"
bgfx_interface_vtbl_t* bgfx_inf_ = bgfx_get_interface(BGFX_API_VERSION);
