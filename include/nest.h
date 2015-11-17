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
 */

#define MAX_PSTATES 256
struct occ_pstate_entry {
        s8 id;
        u8 flags;
        u8 vdd;
        u8 vcs;
        u32 freq_khz;
};

struct occ_pstate_table {
        u8 valid;
        u8 version;
        u8 throttle;
        s8 pstate_min;
        s8 pstate_nom;
        s8 pstate_max;
        u8 spare1;
        u8 spare2;
        u64 reserved;
        struct occ_pstate_entry pstates[MAX_PSTATES];
	u8 pad[112];
};

struct occ_sensor_table {
	/* Config */
	u8 valid;
	u8 version;
	u16 core_mask;
	/* System sensors */
	u16 ambient_temperature;
	u16 power;
	u16 fan_power;
	u16 io_power;
	u16 storage_power;
	u16 gpu_power;
	u16 fan_speed;
	/* Processor Sensors */
	u16 pwr250us;
	u16 pwr250usvdd;
	u16 pwr250usvcs;
	u16 pwr250usmem;
	u64 chip_bw;
	/* Core sensors*/
	u16 core_temp[12];
	u64 count;
	u32 chip_energy;
	u32 system_energy;
	u8  pad[54];
}__attribute__ ((aligned (128)));

/* dimm information for utilisation metrics */
#define MURANO_CENTAUR_DIMM     24
#define VENICE_CENTAUR_DIMM     27

#define P8_HOMER_SAPPHIRE_DATA_OFFSET   0x1F8000


#define P8_HOMER_SENSOR_DATA_OFFSET	P8_HOMER_SAPPHIRE_DATA_OFFSET + \
					sizeof(struct occ_pstate_table)

#define chip_occ_data(chip) \
                ((struct occ_pstate_table *)(chip->homer_base + \
                                P8_HOMER_SAPPHIRE_DATA_OFFSET))
#define occ_sensor_data(chip) \
		((struct occ_sensor_table *) (chip->homer_base + \
		P8_HOMER_SENSOR_DATA_OFFSET))

void nest_pmu_init(void);
