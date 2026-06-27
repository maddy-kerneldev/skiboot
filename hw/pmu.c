// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
/*
 * Performance Monitoring Unit (PMU) Device Tree Generation
 *
 * Copyright 2026 IBM Corp.
 */

#define pr_fmt(fmt)  "PMU: " fmt
#include <skiboot.h>
#include <device.h>
#include <chip.h>
#include <pmu.h>

/* Helper function to add PMC register nodes */
static void add_pmc_node(struct dt_node *parent, const char *name,
			 uint32_t sprn, uint32_t reg_width,
			 const char *privilege, uint32_t programmable,
			 const char *event)
{
	struct dt_node *pmc;

	pmc = dt_new(parent, name);
	if (!pmc) {
		prerror("Failed to create PMC node %s\n", name);
		return;
	}

	dt_add_property_cells(pmc, "sprn", sprn);
	dt_add_property_cells(pmc, "register-width", reg_width);
	dt_add_property_string(pmc, "privilege", privilege);
	dt_add_property_cells(pmc, "programmable", programmable);
	dt_add_property_string(pmc, "event", event);
	dt_add_property_string(pmc, "status", "okay");
}

/* Helper function to add MMCR register nodes */
static void add_mmcr_node(struct dt_node *parent, const char *name,
			  uint32_t sprn, uint32_t reg_width,
			  const char *privilege)
{
	struct dt_node *mmcr;

	mmcr = dt_new(parent, name);
	if (!mmcr) {
		prerror("Failed to create MMCR node %s\n", name);
		return;
	}

	dt_add_property_cells(mmcr, "sprn", sprn);
	dt_add_property_cells(mmcr, "register-width", reg_width);
	dt_add_property_string(mmcr, "privilege", privilege);
	dt_add_property_string(mmcr, "status", "okay");
}

/* Helper function to add event code format field nodes */
static void add_evt_code_field(struct dt_node *parent, const char *name,
			       const char *description, uint32_t bits_start,
			       uint32_t bits_end, uint32_t length, uint32_t mmcr,
			       int has_target_fields, uint32_t target_field_start,
			       uint32_t target_field_end, int has_target_field_base,
			       uint32_t target_field_base, uint32_t target_field_shift)
{
	struct dt_node *field;

	field = dt_new(parent, name);
	if (!field) {
		prerror("Failed to create event code field %s\n", name);
		return;
	}

	dt_add_property_string(field, "description", description);
	dt_add_property_cells(field, "bits", bits_start, bits_end);
	dt_add_property_cells(field, "length", length);
	dt_add_property_cells(field, "mmcr", mmcr);

	if (has_target_fields)
		dt_add_property_cells(field, "target_fields", target_field_start, target_field_end);

	if (has_target_field_base) {
		dt_add_property_cells(field, "target_field_base", target_field_base);
		dt_add_property_cells(field, "target_field_shift", target_field_shift);
	}
}

/* Helper function to add event nodes */
static void add_event_node(struct dt_node *parent, const char *name,
			   uint64_t event_code_high, uint64_t event_code_low,
			   const char *category, const char *event_class,
			   const char *description)
{
	struct dt_node *event;

	event = dt_new(parent, name);
	if (!event) {
		prerror("Failed to create event node %s\n", name);
		return;
	}

	if (event_code_high != 0)
		dt_add_property_u64s(event, "event_code", event_code_high, event_code_low);
	else
		dt_add_property_cells(event, "event_code", event_code_low);

	dt_add_property_string(event, "event-category", category);
	dt_add_property_string(event, "event-class", event_class);
	dt_add_property_string(event, "description", description);
	dt_add_property_string(event, "status", "okay");
}

/* Create PMU device tree nodes */
void pmu_init(void)
{
	struct dt_node *pmus, *pmu_node, *sprs, *pmcs, *mmcr;
	struct dt_node *evt_code_format, *mmcr2_node, *mmcr0_setters;
	struct dt_node *constraints, *pmc_constraints, *sample, *threshold;
	struct dt_node *cache, *grouping, *ebb, *bhrb, *l1_qualifier;
	struct dt_node *fab_match, *radix_scope, *nc, *alternatives;
	struct dt_node *events, *fcs, *fcp, *fch;
	struct dt_node *pmc1ce, *pmcjce, *fc56;
	struct dt_node *restricted_5, *restricted_6;
	struct dt_node *thresh_sel, *thresh_cmp, *thresh_ctl;
	struct dt_node *cache_group, *cache_pmc4, *l2l3_group;

	prlog(PR_INFO, "Initializing PMU device tree nodes\n");

	/* Create root pmus node */
	pmus = dt_new(dt_root, "pmus");
	if (!pmus) {
		prerror("Failed to create pmus root node\n");
		return;
	}

	dt_add_property_cells(pmus, "#address-cells", 1);
	dt_add_property_cells(pmus, "#size-cells", 0);

	/* Create pmu_dts@0 node */
	pmu_node = dt_new_addr(pmus, "pmu_dts", 0);
	if (!pmu_node) {
		prerror("Failed to create pmu_dts@0 node\n");
		return;
	}

	dt_add_property_string(pmu_node, "compatible", "ibm,power-pmu");
	dt_add_property_cells(pmu_node, "reg", 0);
	dt_add_property_string(pmu_node, "pmu-name", "POWER10 PMU");
	dt_add_property_string(pmu_node, "pmu-version", "PowerISA 3.1");
	dt_add_property_string(pmu_node, "platform", "power10");
	dt_add_property_string(pmu_node, "status", "okay");
	dt_add_property_cells(pmu_node, "nr_pmc", 6);
	dt_add_property_cells(pmu_node, "nr_mmcr", 5);

	/* Create sprs node */
	sprs = dt_new(pmu_node, "sprs");
	if (!sprs) {
		prerror("Failed to create sprs node\n");
		return;
	}

	/* Create pmcs node under sprs */
	pmcs = dt_new(sprs, "pmcs");
	if (!pmcs) {
		prerror("Failed to create pmcs node\n");
		return;
	}

	/* Add PMC registers */
	add_pmc_node(pmcs, "pmc1", 787, 32, "hv", 1, "any");
	add_pmc_node(pmcs, "pmc2", 788, 32, "hv", 1, "any");
	add_pmc_node(pmcs, "pmc3", 789, 32, "hv", 1, "any");
	add_pmc_node(pmcs, "pmc4", 790, 32, "hv", 1, "any");
	add_pmc_node(pmcs, "pmc5", 791, 32, "hv", 0, "instructions");
	add_pmc_node(pmcs, "pmc6", 792, 32, "hv", 0, "cycles");

	/* Create mmcr node under sprs */
	mmcr = dt_new(sprs, "mmcr");
	if (!mmcr) {
		prerror("Failed to create mmcr node\n");
		return;
	}

	/* Add MMCR registers */
	add_mmcr_node(mmcr, "mmcr0", 795, 64, "hv");
	add_mmcr_node(mmcr, "mmcr1", 798, 64, "hv");
	add_mmcr_node(mmcr, "mmcr2", 785, 64, "hv");
	add_mmcr_node(mmcr, "mmcr3", 754, 64, "hv");
	add_mmcr_node(mmcr, "mmcra", 0x312, 64, "hv");

	/* Create evt_code_format node */
	evt_code_format = dt_new(pmu_node, "evt_code_format");
	if (!evt_code_format) {
		prerror("Failed to create evt_code_format node\n");
		return;
	}

	dt_add_property_string(evt_code_format, "compatible", "ibm,pmus");

	/* Add event code format fields */
	add_evt_code_field(evt_code_format, "IFM",
			   "Instruction fetch marking mode selector",
			   60, 61, 2, 4, 1, 30, 31, 0, 0, 0);

	add_evt_code_field(evt_code_format, "SRC_MMCR3",
			   "Source selection from MMCR3 register",
			   45, 59, 15, 3, 0, 0, 0, 1, 64, 15);

	add_evt_code_field(evt_code_format, "L2L3_SELECT",
			   "L2/L3 cache event selection field",
			   40, 44, 5, 2, 1, 3, 7, 0, 0, 0);

	add_evt_code_field(evt_code_format, "THRESH_START",
			   "Threshold start value for event filtering",
			   36, 39, 4, 4, 1, 12, 15, 0, 0, 0);

	add_evt_code_field(evt_code_format, "THRESH_STOP",
			   "Threshold stop value for event filtering",
			   32, 35, 4, 4, 1, 8, 11, 0, 0, 0);

	add_evt_code_field(evt_code_format, "THRESH_SEL",
			   "Threshold selection for performance monitoring",
			   29, 31, 3, 4, 1, 16, 18, 0, 0, 0);

	add_evt_code_field(evt_code_format, "RAND_SAMP_ELIG",
			   "Random sampling eligibility control",
			   26, 28, 3, 4, 1, 4, 6, 0, 0, 0);

	add_evt_code_field(evt_code_format, "RAND_SAMP_MODE",
			   "Random sampling mode selector",
			   24, 25, 2, 4, 1, 1, 2, 0, 0, 0);

	add_evt_code_field(evt_code_format, "SDAR_MODE",
			   "SDAR mode control for data address sampling",
			   22, 23, 2, 4, 1, 42, 43, 0, 0, 0);

	add_evt_code_field(evt_code_format, "DC_RLD_QUAL",
			   "Data cache reload event qualifier",
			   21, 21, 1, 1, 1, 47, 47, 0, 0, 0);

	add_evt_code_field(evt_code_format, "IC_RLD_QUAL",
			   "Instruction cache reload event qualifier",
			   20, 20, 1, 1, 1, 46, 46, 0, 0, 0);

	add_evt_code_field(evt_code_format, "PMCx",
			   "Performance monitor counter selector field",
			   16, 19, 4, 1, 0, 0, 0, 0, 0, 0);

	add_evt_code_field(evt_code_format, "PMCxUNIT",
			   "PMC unit selector for event source selection",
			   12, 15, 4, 1, 0, 0, 0, 1, 64, 4);

	add_evt_code_field(evt_code_format, "PMCxCOMB",
			   "PMC combine mode for multi-counter operations",
			   10, 11, 2, 1, 0, 0, 0, 1, 36, 1);

	add_evt_code_field(evt_code_format, "RADIX_SCOPE_QUAL",
			   "Radix page table scope qualifier for events",
			   9, 9, 1, 1, 1, 45, 45, 0, 0, 0);

	add_evt_code_field(evt_code_format, "MARK",
			   "Mark event for instruction sampling",
			   8, 8, 1, 4, 1, 0, 0, 0, 0, 0);

	add_evt_code_field(evt_code_format, "PMCxSEL",
			   "PMC event selector (256 possible events per PMC)",
			   0, 7, 8, 1, 0, 0, 0, 1, 32, 8);

	/* Create mmcr2 node */
	mmcr2_node = dt_new(pmu_node, "mmcr2");
	if (!mmcr2_node) {
		prerror("Failed to create mmcr2 node\n");
		return;
	}

	/* Add fcs, fcp, fch nodes under mmcr2 */
	fcs = dt_new(mmcr2_node, "fcs");
	if (fcs) {
		dt_add_property_cells(fcs, "mmcr", 2);
		dt_add_property_cells(fcs, "pgm_base", 63);
		dt_add_property_cells(fcs, "stride", 9);
		dt_add_property(fcs, "use_stride", NULL, 0);
	}

	fcp = dt_new(mmcr2_node, "fcp");
	if (fcp) {
		dt_add_property_cells(fcp, "mmcr", 2);
		dt_add_property_cells(fcp, "pgm_base", 62);
		dt_add_property_cells(fcp, "stride", 9);
		dt_add_property(fcp, "use_stride", NULL, 0);
	}

	fch = dt_new(mmcr2_node, "fch");
	if (fch) {
		dt_add_property_cells(fch, "mmcr", 2);
		dt_add_property_cells(fch, "pgm_base", 57);
		dt_add_property_cells(fch, "stride", 9);
		dt_add_property(fch, "use_stride", NULL, 0);
	}

	/* Create mmcr0-setters node */
	mmcr0_setters = dt_new(pmu_node, "mmcr0-setters");
	if (!mmcr0_setters) {
		prerror("Failed to create mmcr0-setters node\n");
		return;
	}

	pmc1ce = dt_new(mmcr0_setters, "pmc1ce");
	if (pmc1ce) {
		dt_add_property_cells(pmc1ce, "mmcr", 0);
		dt_add_property_cells(pmc1ce, "bit", 15);
		dt_add_property_cells(pmc1ce, "trigger-pmc", 1);
	}

	pmcjce = dt_new(mmcr0_setters, "pmcjce");
	if (pmcjce) {
		dt_add_property_cells(pmcjce, "mmcr", 0);
		dt_add_property_cells(pmcjce, "bit", 14);
		dt_add_property_cells(pmcjce, "trigger-pmc", 2, 3, 4);
	}

	fc56 = dt_new(mmcr0_setters, "fc56");
	if (fc56) {
		dt_add_property_cells(fc56, "mmcr", 0);
		dt_add_property_cells(fc56, "bit", 4);
		dt_add_property_cells(fc56, "trigger-pmc", 5, 6);
	}

	/* Create constraints node */
	constraints = dt_new(pmu_node, "constraints");
	if (!constraints) {
		prerror("Failed to create constraints node\n");
		return;
	}

	/* Create pmc-constraints node */
	pmc_constraints = dt_new(constraints, "pmc-constraints");
	if (pmc_constraints) {
		dt_add_property_cells(pmc_constraints, "max-counter", 6);

		restricted_5 = dt_new(pmc_constraints, "restricted-counters-5");
		if (restricted_5) {
			dt_add_property_cells(restricted_5, "pmc", 5);
			dt_add_property_u64s(restricted_5, "valid-events", 0x00000000, 0x000500fa);
		}

		restricted_6 = dt_new(pmc_constraints, "restricted-counters-6");
		if (restricted_6) {
			dt_add_property_cells(restricted_6, "pmc", 6);
			dt_add_property_u64s(restricted_6, "valid-events", 0x00000000, 0x000600f4);
		}
	}

	/* Add sample constraint */
	sample = dt_new(constraints, "sample");
	if (sample) {
		dt_add_property_cells(sample, "condition", 1);
		dt_add_property_cells(sample, "event-mask", 0x1f);
		dt_add_property_cells(sample, "event-shift", 24);
		dt_add_property_u64s(sample, "constraint-mask", 0x00000000, 0x001f0000);
		dt_add_property_cells(sample, "constraint-shift", 16);
	}

	/* Add threshold constraint */
	threshold = dt_new(constraints, "threshold");
	if (threshold) {
		dt_add_property(threshold, "supported", NULL, 0);

		thresh_sel = dt_new(threshold, "thresh-sel");
		if (thresh_sel) {
			dt_add_property_cells(thresh_sel, "condition", 2);
			dt_add_property_cells(thresh_sel, "event-mask", 0x7);
			dt_add_property_cells(thresh_sel, "event-shift", 29);
			dt_add_property_u64s(thresh_sel, "constraint-mask", 0x000007ff, 0x00000000);
			dt_add_property_cells(thresh_sel, "constraint-shift", 32);
		}

		thresh_cmp = dt_new(threshold, "thresh-cmp");
		if (thresh_cmp) {
			dt_add_property_cells(thresh_cmp, "condition", 2);
			dt_add_property_cells(thresh_cmp, "event-mask", 0x7ff);
			dt_add_property_cells(thresh_cmp, "event-shift", 40);
			dt_add_property_u64s(thresh_cmp, "constraint-mask", 0x003ff800, 0x00000000);
			dt_add_property_cells(thresh_cmp, "constraint-shift", 43);
		}

		thresh_ctl = dt_new(threshold, "thresh-ctl");
		if (thresh_ctl) {
			dt_add_property_cells(thresh_ctl, "condition", 2);
			dt_add_property_cells(thresh_ctl, "event-mask", 0xff);
			dt_add_property_cells(thresh_ctl, "event-shift", 32);
			dt_add_property_u64s(thresh_ctl, "constraint-mask", 0x000007f8, 0x00000000);
			dt_add_property_cells(thresh_ctl, "constraint-shift", 35);
		}
	}

	/* Add cache constraint */
	cache = dt_new(constraints, "cache");
	if (cache) {
		dt_add_property_cells(cache, "supported-units", 6, 7, 8, 9);
		dt_add_property_cells(cache, "cache-selector-mask", 0x7);
		dt_add_property(cache, "require-cache-selector-zero", NULL, 0);

		cache_group = dt_new(cache, "cache-group");
		if (cache_group) {
			dt_add_property_cells(cache_group, "condition", 5);
			dt_add_property_cells(cache_group, "event-mask", 0xff);
			dt_add_property_cells(cache_group, "event-shift", 0);
			dt_add_property_u64s(cache_group, "constraint-mask", 0x07f80000, 0x00000000);
			dt_add_property_cells(cache_group, "constraint-shift", 55);
		}

		cache_pmc4 = dt_new(cache, "cache-pmc4");
		if (cache_pmc4) {
			dt_add_property_cells(cache_pmc4, "condition", 5);
			dt_add_property_u64s(cache_pmc4, "constraint-mask", 0x00400000, 0x00000000);
			dt_add_property_cells(cache_pmc4, "constraint-shift", 54);
		}

		l2l3_group = dt_new(cache, "l2l3-group");
		if (l2l3_group) {
			dt_add_property_cells(l2l3_group, "condition", 5);
			dt_add_property_cells(l2l3_group, "event-mask", 0x1f);
			dt_add_property_cells(l2l3_group, "event-shift", 0);
			dt_add_property_u64s(l2l3_group, "constraint-mask", 0x0f800000, 0x00000000);
			dt_add_property_cells(l2l3_group, "constraint-shift", 50);
		}
	}

	/* Add grouping constraint */
	grouping = dt_new(constraints, "grouping");
	if (grouping) {
		dt_add_property_cells(grouping, "group-constraint-mask", 0x0);
		dt_add_property_cells(grouping, "group-constraint-value", 0x0);
	}

	/* Add ebb constraint */
	ebb = dt_new(constraints, "ebb");
	if (ebb) {
		dt_add_property_cells(ebb, "condition", 0);
		dt_add_property(ebb, "require-pmc", NULL, 0);
		dt_add_property_cells(ebb, "event-mask", 0x1);
		dt_add_property_cells(ebb, "event-shift", 63);
		dt_add_property_u64s(ebb, "constraint-mask", 0x00000000, 0x01000000);
		dt_add_property_cells(ebb, "constraint-shift", 24);
	}

	/* Add bhrb constraint */
	bhrb = dt_new(constraints, "bhrb");
	if (bhrb) {
		dt_add_property_cells(bhrb, "condition", 4);
		dt_add_property(bhrb, "requires-ebb", NULL, 0);
		dt_add_property_cells(bhrb, "event-mask", 0x3);
		dt_add_property_cells(bhrb, "event-shift", 60);
		dt_add_property_u64s(bhrb, "constraint-mask", 0x00000000, 0x06000000);
		dt_add_property_cells(bhrb, "constraint-shift", 25);
	}

	/* Add l1-qualifier constraint */
	l1_qualifier = dt_new(constraints, "l1-qualifier");
	if (l1_qualifier) {
		dt_add_property_cells(l1_qualifier, "condition", 3);
		dt_add_property_cells(l1_qualifier, "event-mask", 0x3);
		dt_add_property_cells(l1_qualifier, "event-shift", 20);
		dt_add_property_u64s(l1_qualifier, "constraint-mask", 0x00000000, 0x00c00000);
		dt_add_property_cells(l1_qualifier, "constraint-shift", 22);
	}

	/* Add fab-match constraint */
	fab_match = dt_new(constraints, "fab-match");
	if (fab_match) {
		dt_add_property_cells(fab_match, "condition", 0);
		dt_add_property_cells(fab_match, "event-mask", 0xff);
		dt_add_property_cells(fab_match, "event-shift", 32);
		dt_add_property_u64s(fab_match, "constraint-mask", 0xff000000, 0x00000000);
		dt_add_property_cells(fab_match, "constraint-shift", 56);
	}

	/* Add radix-scope constraint */
	radix_scope = dt_new(constraints, "radix-scope");
	if (radix_scope) {
		dt_add_property_cells(radix_scope, "condition", 6);
		dt_add_property_cells(radix_scope, "event-mask", 0x1);
		dt_add_property_cells(radix_scope, "event-shift", 9);
		dt_add_property_u64s(radix_scope, "constraint-mask", 0x00000000, 0x00200000);
		dt_add_property_cells(radix_scope, "constraint-shift", 21);
	}

	/* Add nc constraint */
	nc = dt_new(constraints, "nc");
	if (nc) {
		dt_add_property_cells(nc, "condition", 1);
		dt_add_property_u64s(nc, "mask", 0x00000000, 0x00008000);
		dt_add_property_cells(nc, "shift", 12);
		dt_add_property_cells(nc, "increment", 1);
	}

	/* Add alternatives constraint */
	alternatives = dt_new(constraints, "alternatives");
	if (alternatives) {
		dt_add_property(alternatives, "supported", NULL, 0);
	}

	/* Create events node */
	events = dt_new(pmu_node, "events");
	if (!events) {
		prerror("Failed to create events node\n");
		return;
	}

	/* Add all event definitions */
	add_event_node(events, "cycles", 0, 0x600f4, "core", "primary",
		       "Number of processor cycles");
	add_event_node(events, "dispatch-stall-cycles", 0, 0x100f8, "pipeline", "primary",
		       "Cycles in which instruction dispatch is stalled");
	add_event_node(events, "execution-stall-cycles", 0, 0x30008, "pipeline", "primary",
		       "Cycles where execution units are stalled");
	add_event_node(events, "instructions", 0, 0x500fa, "core", "primary",
		       "Number of instructions completed");
	add_event_node(events, "branches", 0, 0x4d05e, "branch", "primary",
		       "Number of branch instructions completed");
	add_event_node(events, "branch-misses", 0, 0x400f6, "branch", "primary",
		       "Number of mispredicted branch instructions completed");
	add_event_node(events, "branch-finished", 0, 0x2f04a, "branch", "primary",
		       "Number of branch instructions finished execution");
	add_event_node(events, "branch-mispredict-finished", 0, 0x3e098, "branch", "primary",
		       "Number of mispredicted branch instructions finished");
	add_event_node(events, "l1-load-misses", 0, 0x400f0, "cache", "primary",
		       "Number of completed load operations that missed in L1 cache");
	add_event_node(events, "l1-loads", 0, 0x100fc, "cache", "primary",
		       "All L1 D cache load references counted at finish, gated by reject");
	add_event_node(events, "l1-load-misses-alt", 0, 0x3e054, "cache", "alternative",
		       "Load Missed L1");
	add_event_node(events, "l1-store-misses", 0, 0x300f0, "cache", "primary",
		       "Store Missed L1");
	add_event_node(events, "l1-prefetch-misses", 0, 0x1002c, "cache", "primary",
		       "L1 cache data prefetches");
	add_event_node(events, "l1-icache-misses", 0, 0x200fc, "cache", "primary",
		       "Demand iCache Miss");
	add_event_node(events, "l1-instruction-hit", 0, 0x04080, "cache", "primary",
		       "Instruction fetches from L1");
	add_event_node(events, "l1-instruction-miss", 0x003f0000, 0x0001c040, "cache", "primary",
		       "Instruction Demand sectors wriittent into IL1");
	add_event_node(events, "icache-prefetch-requests", 0, 0x040a0, "cache", "primary",
		       "Instruction prefetch written into IL1");
	add_event_node(events, "l3-data-hit", 0x00134000, 0x0001c040, "cache", "primary",
		       "The data cache was reloaded from local core's L3 due to a demand load");
	add_event_node(events, "l3-data-miss", 0, 0x300fe, "cache", "primary",
		       "Demand LD - L3 Miss (not L2 hit and not L3 hit)");
	add_event_node(events, "l2-store-accesses", 0x00000100, 0x00046080, "cache", "primary",
		       "All successful D-side store dispatches for this thread");
	add_event_node(events, "l2-store-misses", 0, 0x26880, "cache", "primary",
		       "All successful D-side store dispatches for this thread that were L2 Miss");
	add_event_node(events, "l3-prefetch-misses", 0x00001000, 0x00016080, "cache", "primary",
		       "Total HW L3 prefetches(Load+store)");
	add_event_node(events, "dtlb-misses", 0, 0x300fc, "tlb", "primary",
		       "Data PTEG reload");
	add_event_node(events, "itlb-misses", 0, 0x400fc, "tlb", "primary",
		       "ITLB Reloaded");
	add_event_node(events, "cycles-alt", 0, 0x0001e, "core", "alternative",
		       "Alternate count of processor cycles");
	add_event_node(events, "instructions-alt", 0, 0x00002, "core", "alternative",
		       "Alternate count of completed instructions");

	prlog(PR_INFO, "PMU device tree nodes created successfully\n");
}
