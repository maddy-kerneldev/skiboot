/* Copyright 2015 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * Locking notes:
 */

#include <skiboot.h>
#include <device.h>
#include <nest.h>
#include <chip.h>
#include <cpu.h>
#include <xscom.h>
#include <timebase.h>
#include <opal-api.h>
#include <opal-internal.h>
#include <io.h>
#include <platform.h>

/*
 * Pointer holding the Catalog file in memory
 */
struct nest_catalog_desc *catalog_desc;

/*
 * Catalog Page-0 (each page is 4K bytes long) contains
 * information about event, group, formula structure offsets
 * in the catalog. Cache these offsets in struct nest_catalog_page_0
 * to help the search later.
 */
struct nest_catalog_page_0 *page0_desc;

int preload_catalog_lid()
{
	size_t size = NEST_CATALOG_SIZE;
	int loaded;

        catalog_desc = malloc(sizeof(struct nest_catalog_desc));
	if (!catalog_desc) {
		prerror("nest-counters: No mem for nest_catalog_desc structure\n");
		return OPAL_NO_MEM;
	}

	catalog_desc->catalog = malloc(NEST_CATALOG_SIZE);
	if (!catalog_desc->catalog) {
		prerror("nest-counters: No mem for catalog lid \n");
		free(catalog_desc);
		return OPAL_NO_MEM;
	}

	loaded = start_preload_resource(RESOURCE_ID_CATALOG,
						RESOURCE_SUBID_NONE,
						catalog_desc->catalog, &size);

	return loaded;
}

int load_catalog_lid(int loaded)
{
	if (loaded == OPAL_SUCCESS)
		loaded = wait_for_resource_loaded(RESOURCE_ID_CATALOG,
							RESOURCE_SUBID_NONE);

	if (loaded != OPAL_SUCCESS) {
		prerror("nest-counters: Error loading catalog lid\n");
		return OPAL_RESOURCE;
	}

	/*
	 * Now that we have loaded the catalog, check for the
	 * catalog magic.
	 */
	page0_desc = (struct nest_catalog_page_0 *)CATALOG(catalog_desc);
	if (page0_desc->magic != CATALOG_MAGIC) {
		prerror("nest-counters: Error catalog magic number mismatch\n");
		return OPAL_RESOURCE;
	}

	/*
	 * Check for Chip event support in this catalog.
	 */
	if (page0_desc->chip_group_offset == 0) {
		prerror("nest-counters: Not Supported \n");
		return OPAL_UNSUPPORTED;
	}

	/*
	 * Lets save some entry points to help out search
	 */
	GROUP_ENTRY(catalog_desc) = CATALOG(catalog_desc) +
					(page0_desc->group_data_offs * 4096);
	EVENT_ENTRY(catalog_desc) = CATALOG(catalog_desc) +
					(page0_desc->event_data_offs * 4096);
	CHIP_EVENT_ENTRY(catalog_desc) = EVENT_ENTRY(catalog_desc) +
					(page0_desc->chip_event_offset);
	CHIP_GROUP_ENTRY(catalog_desc) = GROUP_ENTRY(catalog_desc) +
					(page0_desc->chip_group_offset);
	CORE_EVENT_ENTRY(catalog_desc) = EVENT_ENTRY(catalog_desc) +
					(page0_desc->core_event_offset);
	CORE_GROUP_ENTRY(catalog_desc) = GROUP_ENTRY(catalog_desc) +
					(page0_desc->core_group_offset);

	return OPAL_SUCCESS;
}

void nest_pmu_init(int loaded)
{
	struct proc_chip *chip;
	struct dt_node *dev, *chip_dev;
	u64 addr = 0;

	if (load_catalog_lid(loaded) != OPAL_SUCCESS) {
		prerror("nest-counters: Catalog failed to load\n");
		return;
	}

	/*
	 * Now that we have catalog loaded and verified for nest counter
	 * support, lets create device tree entries.
	 *
	 * Top level device node called "nest-counters" created under
	 * "/" root folder to contain all the nest unit information
	 */
	dev = dt_new(dt_root, "nest-counters");
	if (!dev) {
		prerror("nest-counters: device node creation failed\n");
		return;
	}

	dt_add_property_strings(dev, "compatible", "ibm,opal-in-memory-counters");
	dt_add_property_cells(dev, "#address-cells", 2);
	dt_add_property_cells(dev, "#size-cells", 2);
	dt_add_property(dev, "ranges", NULL, 0);

	/*
	 * Top level device node "nest-counters" will have per-chip nodes.
	 * Each chip node will have slw ima offset and the nest pmu
	 * units details.
	 *
	 * pore_slw_ima firmware will program nest counters with
	 * pre-defined set of events (provided in catalog) and accumulate
	 * counter data in a fixed homer offset called
	 * "SLW 24x7 Counters Data Area (per chip)". This offset detail
	 * provide in the range field.
	 *
	 * For homer memory layout refer "p8_homer_map.h" in hostboot git tree
	 * of open-power github
	 */
	for_each_chip(chip) {
		addr = chip->homer_base + SLW_IMA_OFFSET;
		chip_dev = dt_new_addr(dev, "chip", addr);
		if (!chip_dev) {
			prerror("nest-counters:chip node creation failed\n");
			goto fail;
		}

		/*
		 *TODO: Need to fix phandle property to point to "reserved-
		 * memory" node for HOMER.
		 */
		dt_add_property_cells(chip_dev, "ibm,chip-id", chip->id);
		dt_add_property_cells(chip_dev, "#address-cells", 1);
		dt_add_property_cells(chip_dev, "#size-cells", 1);
		dt_add_property_cells(chip_dev, "ranges", 0, hi32(addr),
						lo32(addr), SLW_IMA_SIZE);
	}

	return;

fail:
	dt_free(dev);
	return;
}
