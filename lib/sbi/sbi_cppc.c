/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 */

#include <sbi/sbi_error.h>
#include <sbi/sbi_cppc.h>

static const struct sbi_cppc_device *cppc_dev = NULL;

const struct sbi_cppc_device *sbi_cppc_get_device(void)
{
	return cppc_dev;
}

void sbi_cppc_set_device(const struct sbi_cppc_device *dev)
{
	if (!dev || cppc_dev)
		return;

	cppc_dev = dev;
}

static int sbi_cppc_probe_mandatory(unsigned long reg)
{
	switch (reg) {
	case SBI_CPPC_HIGHEST_PERF:
	case SBI_CPPC_NOMINAL_PERF:
	case SBI_CPPC_LOW_NON_LINEAR_PERF:
	case SBI_CPPC_LOWEST_PERF:
	case SBI_CPPC_PERF_LIMITED:
		return 32;
	}

	return 0;
}

static bool sbi_cppc_is_reserved(unsigned long reg)
{
	if ((reg > SBI_CPPC_ACPI_LAST && reg < SBI_CPPC_TRANSITION_LATENCY) ||
	    reg > SBI_CPPC_NON_ACPI_LAST)
		return true;

	return false;
}

static bool sbi_cppc_readable(unsigned long reg)
{
	/* there are no write-only cppc registers currently */
	return true;
}

static bool sbi_cppc_writable(unsigned long reg)
{
	switch (reg) {
	case SBI_CPPC_HIGHEST_PERF:
	case SBI_CPPC_NOMINAL_PERF:
	case SBI_CPPC_LOW_NON_LINEAR_PERF:
	case SBI_CPPC_LOWEST_PERF:
	case SBI_CPPC_GUARANTEED_PERF:
	case SBI_CPPC_CTR_WRAP_TIME:
	case SBI_CPPC_REFERENCE_CTR:
	case SBI_CPPC_DELIVERED_CTR:
	case SBI_CPPC_REFERENCE_PERF:
	case SBI_CPPC_LOWEST_FREQ:
	case SBI_CPPC_NOMINAL_FREQ:
	case SBI_CPPC_TRANSITION_LATENCY:
		return false;
	}

	return true;
}

int sbi_cppc_probe(unsigned long reg)
{
	int ret;

	if (!cppc_dev || !cppc_dev->cppc_probe)
		return SBI_EFAIL;

	/* Check whether register is reserved */
	if (sbi_cppc_is_reserved(reg))
		return SBI_ERR_INVALID_PARAM;

	/* Return width for mandatory registers */
	ret = sbi_cppc_probe_mandatory(reg);
	if (ret)
		return ret;

	ret = cppc_dev->cppc_probe(reg);
	if (ret >= 0)
		return ret;

	switch (reg) {
	case SBI_CPPC_GUARANTEED_PERF:
	case SBI_CPPC_MIN_PERF:
	case SBI_CPPC_MAX_PERF:
	case SBI_CPPC_PERF_REDUC_TOLERANCE:
	case SBI_CPPC_TIME_WINDOW:
	case SBI_CPPC_ENABLE:
	case SBI_CPPC_AUTO_SEL_ENABLE:
	case SBI_CPPC_AUTO_ACT_WINDOW:
	case SBI_CPPC_ENERGY_PERF_PREFERENCE:
	case SBI_CPPC_REFERENCE_PERF:
	case SBI_CPPC_LOWEST_FREQ:
	case SBI_CPPC_NOMINAL_FREQ:
	case SBI_CPPC_TRANSITION_LATENCY:
		return 32;
	}

	return SBI_ERR_FAILED;
}

int sbi_cppc_read(unsigned long reg, uint64_t *val)
{
	int ret;

	if (!cppc_dev || !cppc_dev->cppc_read)
		return SBI_EFAIL;

	/* Check whether register is implemented */
	ret = sbi_cppc_probe(reg);
	if (ret < 0)
		return ret;
	else if (!ret)
		return SBI_ERR_NOT_SUPPORTED;

	/* Check whether the register is write-only */
	if (!sbi_cppc_readable(reg))
		return SBI_ERR_DENIED;

	return cppc_dev->cppc_read(reg, val);
}

int sbi_cppc_write(unsigned long reg, uint64_t val)
{
	int ret;

	if (!cppc_dev || !cppc_dev->cppc_write)
		return SBI_EFAIL;

	/* Check whether register is implemented */
	ret = sbi_cppc_probe(reg);
	if (ret < 0)
		return ret;
	else if (!ret)
		return SBI_ERR_NOT_SUPPORTED;

	/* Check whether the register is read-only */
	if (!sbi_cppc_writable(reg))
		return SBI_ERR_DENIED;

	return cppc_dev->cppc_write(reg, val);
}
