/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 */

#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_cppc.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_timer.h>

struct perf_channel {
	unsigned int	highest_perf;
	unsigned int	nominal_perf;
	unsigned int	lowest_nonlinear_perf;
	unsigned int	lowest_perf;
	unsigned int	desired_perf;
	unsigned int	perf_limited;
	unsigned int	reference_perf;
	unsigned int	lowest_freq;
	unsigned int	nominal_freq;
	unsigned int	transition_latency;
};

static unsigned long cppc_offset;

static int sbi_cppc_test_read(unsigned long reg, uint64_t *val)
{
	struct sbi_scratch *scratch;
	struct perf_channel *cppc;
	unsigned long hartid;
	int ret = SBI_SUCCESS;

	hartid = current_hartid();

	scratch = sbi_hartid_to_scratch(hartid);
	cppc = sbi_scratch_offset_ptr(scratch, cppc_offset);

	switch (reg) {
	case SBI_CPPC_HIGHEST_PERF:
		*val = cppc->highest_perf;
		break;
	case SBI_CPPC_NOMINAL_PERF:
		*val = cppc->nominal_perf;
		break;
	case SBI_CPPC_LOW_NON_LINEAR_PERF:
		*val = cppc->lowest_nonlinear_perf;
		break;
	case SBI_CPPC_LOWEST_PERF:
		*val = cppc->lowest_perf;
		break;
	case SBI_CPPC_DESIRED_PERF:
		*val = cppc->desired_perf;
		break;
	case SBI_CPPC_REFERENCE_CTR:
		*val = sbi_timer_value();
		break;
	case SBI_CPPC_DELIVERED_CTR:
		/*
		 * Can't use CYCLE CSR properly in qemu, so just return
		 * TIME itself so that delta(delivered) / delta(ref) = 1
		 */
		*val = sbi_timer_value();
		break;
	case SBI_CPPC_PERF_LIMITED:
		*val = cppc->perf_limited;
		break;
	case SBI_CPPC_REFERENCE_PERF:
		*val = cppc->reference_perf;
		break;
	case SBI_CPPC_LOWEST_FREQ:
		*val = cppc->lowest_freq;
		break;
	case SBI_CPPC_NOMINAL_FREQ:
		*val = cppc->nominal_freq;
		break;
	case SBI_CPPC_TRANSITION_LATENCY:
		*val = cppc->transition_latency;
		break;
	default:
		return -1;
	}

	return ret;
}

static int sbi_cppc_test_write(unsigned long reg, uint64_t val)
{
	struct sbi_scratch *scratch;
	struct perf_channel *cppc;
	unsigned long hartid;
	int ret = SBI_SUCCESS;

	hartid = current_hartid();

	scratch = sbi_hartid_to_scratch(hartid);
	cppc = sbi_scratch_offset_ptr(scratch, cppc_offset);

	switch (reg) {
	case SBI_CPPC_DESIRED_PERF:
		cppc->desired_perf = val;
		break;
	case SBI_CPPC_PERF_LIMITED:
		cppc->perf_limited = val;
		break;
	default:
		return -1;
	}

	return ret;
}

static int sbi_cppc_test_probe(unsigned long reg)
{
	switch (reg) {
	case SBI_CPPC_DESIRED_PERF:
	case SBI_CPPC_PERF_LIMITED:
	case SBI_CPPC_HIGHEST_PERF:
	case SBI_CPPC_NOMINAL_PERF:
	case SBI_CPPC_LOW_NON_LINEAR_PERF:
	case SBI_CPPC_LOWEST_PERF:
	case SBI_CPPC_REFERENCE_PERF:
	case SBI_CPPC_LOWEST_FREQ:
	case SBI_CPPC_NOMINAL_FREQ:
	case SBI_CPPC_TRANSITION_LATENCY:
		return 32;
	case SBI_CPPC_REFERENCE_CTR:
	case SBI_CPPC_DELIVERED_CTR:
		return 64;
	case SBI_CPPC_GUARANTEED_PERF:
	case SBI_CPPC_MIN_PERF:
	case SBI_CPPC_MAX_PERF:
	case SBI_CPPC_PERF_REDUC_TOLERANCE:
	case SBI_CPPC_TIME_WINDOW:
	case SBI_CPPC_CTR_WRAP_TIME:
	case SBI_CPPC_ENABLE:
	case SBI_CPPC_AUTO_SEL_ENABLE:
	case SBI_CPPC_AUTO_ACT_WINDOW:
	case SBI_CPPC_ENERGY_PERF_PREFERENCE:
		return 0;
	}

	return -1;
}

static struct sbi_cppc_device sbi_system_cppc_test = {
	.name		= "cppc-test",
	.cppc_read	= sbi_cppc_test_read,
	.cppc_write	= sbi_cppc_test_write,
	.cppc_probe	= sbi_cppc_test_probe,
};

static void sbi_cppc_test_enable(void)
{
	sbi_cppc_set_device(&sbi_system_cppc_test);
}

/*
 * Allocate scratch space as the channel memory to emulate the SBI CPPC
 * extension.
 */
int test_cppc_init()
{
	u32 i;
	struct sbi_scratch *rscratch;
	struct perf_channel *cppc;

	cppc_offset = sbi_scratch_alloc_offset(sizeof(*cppc));
	if (!cppc_offset)
		return SBI_ENOMEM;

	/* Initialize hart state data for every hart */
	for (i = 0; i <= sbi_scratch_last_hartid(); i++) {
		rscratch = sbi_hartid_to_scratch(i);
		if (!rscratch)
			continue;

		cppc = sbi_scratch_offset_ptr(rscratch,
					      cppc_offset);
		if (cppc) {
			sbi_memset(cppc, 0, sizeof(*cppc));

			/* Initialize sample values */
			cppc->highest_perf = 6;
			cppc->nominal_perf = 5;
			cppc->lowest_nonlinear_perf = 2;
			cppc->lowest_perf = 1;
			cppc->desired_perf = cppc->nominal_perf;
			cppc->perf_limited = 0;
			cppc->reference_perf = 20;
			cppc->lowest_freq = 20;
			cppc->nominal_freq = 100;
			cppc->transition_latency = 20000;
		}
	}

	sbi_cppc_test_enable();

	return 0;
}
