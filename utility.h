#pragma once

#define FREQ_TO_US(freq) \
	((uint32_t) (1e6/(freq)))

#define US_TO_FREQ(us) \
	((float) (1e6/(us)))
