#ifndef __BRUBECK_HISTO_H__
#define __BRUBECK_HISTO_H__

struct brubeck_histo {
	value_t *values;
	uint32_t count;
	uint16_t alloc, size;
};

struct brubeck_histo_sample {
	value_t sum;
	value_t min;
	value_t max;
	value_t mean;
	value_t median;
	value_t count;

	value_t percentile[7];
};

enum { PC_5, PC_10, PC_25, PC_75, PC_90, PC_95, PC_99 };

void brubeck_histo_push(struct brubeck_histo *histo, value_t value, value_t sample_rate);
void brubeck_histo_sample(
		struct brubeck_histo_sample *sample,
		struct brubeck_histo *histo);

void brubeck_histo_empty(struct brubeck_histo *histo);

#endif
