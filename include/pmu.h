// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
/* Copyright 2026 IBM Corp. */

#ifndef __PMU_H
#define __PMU_H

/*
 * Performance Monitoring Unit (PMU) Device Tree Generation
 *
 * This module creates device tree nodes for the PMU subsystem,
 * including PMC (Performance Monitor Counter) registers, MMCR
 * (Monitor Mode Control Register) registers, event code formats,
 * constraints, and predefined events.
 */

/* Initialize PMU device tree nodes */
void pmu_init(void);

#endif /* __PMU_H */
