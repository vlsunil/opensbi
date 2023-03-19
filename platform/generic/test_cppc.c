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
	unsigned int	desired_perf;
	unsigned int	perf_limited;
	unsigned int	transition_latency;
};

static unsigned long cppc_offset;

static int sbi_cppc_test_probe(unsigned long reg)
{
	switch (reg) {
	case SBI_CPPC_REFERENCE_CTR:
	case SBI_CPPC_DELIVERED_CTR:
		return 64;
	case SBI_CPPC_DESIRED_PERF:
	case SBI_CPPC_PERF_LIMITED:
	case SBI_CPPC_TRANSITION_LATENCY:
		return 32;
	default:
		/* Unimplemented */
		return 0;
	}
}

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
	case SBI_CPPC_TRANSITION_LATENCY:
		*val = cppc->transition_latency;
		break;
	default:
		/*
		 * Common read should have checked for unimplemented,
		 * reserved or write-only registers.
		 */
		ret = SBI_ERR_FAILED;
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
		/*
		 * Common read should have checked for unimplemented,
		 * reserved or write-only registers.
		 */
		ret = SBI_ERR_FAILED;
	}

	return ret;
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
			cppc->desired_perf = 5;
			cppc->perf_limited = 0;
			cppc->transition_latency = 20000;
		}
	}

	sbi_cppc_test_enable();

	return 0;
}
