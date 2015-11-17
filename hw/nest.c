/* Copyright 2015 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
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

#define MAX_NAME_SIZE   64
#define MEM_BW_OFFSET	26

static int dt_create_nest_unit_events(struct dt_node *pt, int index, u32 offset,
                                        const char *name, const char *scale,
                                        const char *unit)
{
        struct dt_node *type;

        /*
         * Create an event node to pass event information.
         * "reg" property is must for event and rest of the properties
         * such as id, scale, unit are optional.
         */
        type = dt_new_addr(pt, name, offset);
        if (!type) {
		prlog(PR_ERR, "nest-counters: %s: type = NULL\n", __FUNCTION__);
                return -1;
	}

        /*
         * "reg" property:
         *
         * event offset where counter data gets accumulated
         */
        dt_add_property_cells(type, "reg", offset, sizeof(u64));

        /*
         * "id" property:
         *
         * event id to be appended to the event name. In some units like abus,
         * we have events such as abus0, abus1 and abus2. Since having numbers
         * in the dt node name are not suggested, we pass these numbers as
         * id peroperty.
         */
        if (index >= 0)
                 dt_add_property_cells(type, "id", index);

        /*
         * "unit" and "scale" property:
         *
         * unit and scale properties, when used on raw counter value,
         * provide metric information.
         */
        if (unit)
                dt_add_property_string(type, "unit", unit);

        if (scale)
                dt_add_property_string(type, "scale", scale);

        return 0;
}

static int detect_nest_units(struct dt_node *pt)
{
	char name[] = "mem_bw";
	struct dt_node *type;
	const char *unit = "GB", *scale = "0.00128";
	int rc=0;

	type = dt_new(pt, name);
	if (!type) {
		prerror("nest_counters: mem_be type creation failed\n");
		return -1;
	}

	dt_add_property_cells(type, "#address-cells", 1);
	dt_add_property_cells(type, "#size-cells", 1);
	dt_add_property(type, "ranges", NULL, 0);

	rc = dt_create_nest_unit_events(type, -1, MEM_BW_OFFSET, name, scale, unit);
	if (rc)
		return -1;

	return 0;
}

void nest_pmu_init()
{
        struct proc_chip *chip;
	struct dt_node *dev, *chip_dev;
	u64 addr = 0;

        dev = dt_new(dt_root, "nest-counters");
        if (!dev) {
                prerror("nest-counters: device node creation failed\n");
                return;
        }

        dt_add_property_strings(dev, "compatible", "ibm,opal-in-memory-counters");
        dt_add_property_cells(dev, "#address-cells", 2);
        dt_add_property_cells(dev, "#size-cells", 2);
        dt_add_property(dev, "ranges", NULL, 0);

	for_each_chip(chip) {
		addr = chip->homer_base + P8_HOMER_SENSOR_DATA_OFFSET;
		chip_dev = dt_new_addr(dev, "chip", addr);
		if (!chip_dev) {
			prerror("nest-counters:chip node creation failed\n");
			goto fail;
		}

                dt_add_property_cells(chip_dev, "ibm,chip-id", chip->id);
                dt_add_property_cells(chip_dev, "#address-cells", 1);
                dt_add_property_cells(chip_dev, "#size-cells", 1);
                dt_add_property_cells(chip_dev, "ranges", 0, hi32(addr),
                                                lo32(addr), 128);


		if (detect_nest_units(chip_dev)) {
			prerror("nest-counters: unit detect failed \n");
			goto fail;
		}

                switch (chip->type) {
                case PROC_CHIP_P8_MURANO:
                        dt_add_property_cells(chip_dev,"max-dimm-rate",
                                                        MURANO_CENTAUR_DIMM);
                        break;
                case PROC_CHIP_P8_VENICE:
                        dt_add_property_cells(chip_dev,"max-dimm-rate",
                                                        VENICE_CENTAUR_DIMM);
                        break;
                case PROC_CHIP_P8_NAPLES:
                        dt_add_property_cells(chip_dev,"max-dimm-rate",
                                                        VENICE_CENTAUR_DIMM);
                        break;
                default:
                        prerror("nest-counters: Unknown chip type, skipping dimm file\n");
                        break;
                }
	}

	return;

fail:
        prlog(PR_ERR, "%s: failed to created dts\n",__FUNCTION__);
        dt_free(dev);
        return;
}
