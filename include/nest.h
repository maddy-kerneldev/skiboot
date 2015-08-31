/* Copyright 2015 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __NEST_H
#define __NEST_H

/*
 * Power8 has Nest Instrumentation support with which per-chip
 * utilisation metrics like memory bandwidth, Xlink/Alink bandwidth and
 * many other component metrics can be obtained. These Nest
 * counters can be programmed via scoms or HW PORE Engine,
 * called PORE_SLW_IMA.
 *
 * PORE_SLW_IMA:
 * PORE_SLW_IMA is a firmware that runs on PORE Engine.
 * This firmware programs the nest counter and moves counter values to
 * per chip HOMER region in a fixed offset for each unit. Engine
 * has a control block structure for communication with Hyperviosr(Host OS).
 *
 * PORE -- Power On Reset Engine
 * SLW  -- Sleep/Winkle
 * IMA  -- In Memory Accumulation.
 */

/*
 * Control Block structure offset in HOMER IMA Region
 */
#define CB_STRUCT_OFFSET	0x39FC00
#define CB_STRUCT_CMD		0x39FC08
#define CB_STRUCT_SPEED		0x39FC10
#define SLW_IMA_PAUSE		0x2
#define SLW_IMA_RESUME		0x1
#define SLW_IMA_NOP		0
/*
 * Control Block Structure:
 *
 * Name          Producer        Consumer        Values  Desc
 * IMARunStatus   IMA Code       Hypervisor      0       Initializing
 *                               (Host OS)       1       Running
 *                                               2       Paused
 *
 * IMACommand     Hypervisor     IMA Code        0       NOP
 *                                               1       Resume
 *                                               2       Pause
 *                                               3       Clear and Restart
 *
 * IMACollection Hypervisor      IMA Code        0       128us
 * Speed					 1       256us
 *                                               2       1ms
 *                                               3       4ms
 *                                               4       16ms
 *                                               5       64ms
 *                                               6       256ms
 *                                               7       1000ms
 */
struct ima_chip_cb
{
        u64 ima_chip_run_status;
        u64 ima_chip_command;
        u64 ima_chip_collection_speed;
};

#define SLW_IMA_SAMPLE_RATE_64MS	0x5

/*
 * In Memory Accumulation modes
 */
#define IMA_CHIP_PRODUCTION_MODE	0x1

/*
 * PORE_SLW_IMA reserved memory (in HOMER region)
 */
#define SLW_IMA_OFFSET		0x00320000
#define SLW_IMA_TOTAL_SIZE	0x80000

/*
 * Counter Storage size (exposed as part of DT)
 */
#define SLW_IMA_SIZE		0x30000

/*
 * PTS Scoms and values
 */
#define IMA_PTS_SCOM		0x00068009
#define IMA_PTS_ENABLE		0x00F0000000000000
#define IMA_PTS_DISABLE		0x00E0000000000000
#define IMA_PTS_START		0x1
#define IMA_PTS_STOP		0
#define IMA_PTS_ERROR		-1

/*
 * Catalog structures.
 * Catalog is a meta data file provided as part of FW lid.
 * This file contains information about the various events the
 * HW supports under the "24x7" umbrella. Events are classified under
 * 3 different Domains.
 *	Domain 1  -- Chip Events (PORE_SLW_IMA)
 *	Domain 2  -- Core Events (24x7 Core IMA)
 *	Domain 3  -- per-Thread PMU Events
 */

#define CATALOG_MAGIC 0x32347837 /* "24x7" in ASCII */
#define DOMAIN_CHIP	1
#define DOMAIN_CORE	2

struct nest_catalog_page_0 {
	u32	magic;
	u32	length; /* In 4096 byte pages */
	u64	version;
	u8	build_time_stamp[16]; /* "YYYYMMDDHHMMSS\0\0" */
	u8	reserved2[32];
	u16	schema_data_offs; /* in 4096 byte pages */
	u16	schema_data_len; /* in 4096 byte pages */
	u16	schema_entry_count;
	u8	reserved3[2];
	u16	event_data_offs;
	u16	event_data_len;
	u16	event_entry_count;
	u8	reserved4[2];
	u16	group_data_offs; /* in 4096 byte pages */
	u16	group_data_len; /* in 4096 byte pages */
	u16	group_entry_count;
	u8	reserved5[2];
	u16	formula_data_offs; /* in 4096 byte pages */
	u16	formula_data_len; /* in 4096 byte pages */
	u16	formula_entry_count;
	u8	reserved6[2];
	u32	core_event_offset;
	u32	thread_event_offset;
	u32	chip_event_offset;
	u32	core_group_offset;
	u32	thread_group_offset;
	u32	chip_group_offset;
} __packed;

struct nest_catalog_group_data {
	u16 length; /* in bytes, must be a multiple of 16 */
	u8 reserved1[2];
	u32 flags;
	u8 domain;
	u8 reserved2[1];
	u16 event_group_record_start_offs;
	u16 event_group_record_len;
	u8 group_schema_index;
	u8 event_count;
	u16 event_index[16];
	u16 group_name_len;
	u8 remainder[1];
} __packed;

struct nest_catalog_events_data {
	u16 length; /* in bytes, must be a multiple of 16 */
	u16 formula_index;
	u8 domain;
	u8 reserved2[1];
	u16 event_group_record_offs;
	u16 event_group_record_len; /* in bytes */
	u16 event_counter_offs;
	u32 flags;
	u16 primary_group_ix;
	u16 group_count;
	u16 event_name_len;
	u8 remainder[1];
} __packed;


#define CHIP_EVENTS_SUPPORTED	1
#define CHIP_EVENTS_NOT_SUPPORTED	0

/*
 * Just for optimisation, save only relevant addrs
 */
struct nest_catalog_desc {
	char *catalog;
	char *group_entry;
	char *event_entry;
	char *thread_event_entry;
	char *core_event_entry;
	char *chip_event_entry;
	char *thread_group_entry;
	char *core_group_entry;
	char *chip_group_entry;
};

#define CATALOG(x)		(x)->catalog
#define GROUP_ENTRY(x)		(x)->group_entry
#define EVENT_ENTRY(x)		(x)->event_entry
#define CORE_EVENT_ENTRY(x)	(x)->core_event_entry
#define CHIP_EVENT_ENTRY(x)	(x)->chip_event_entry
#define CORE_GROUP_ENTRY(x)	(x)->core_group_entry
#define CHIP_GROUP_ENTRY(x)	(x)->chip_group_entry

/*Size of Nest Catalog LID (256KBytes) */
#define NEST_CATALOG_SIZE             0x40000

/* dimm information for utilisation metrics */
#define MURANO_CENTAUR_DIMM	24000
#define VENICE_CENTAUR_DIMM	27000

/*
 * Function prototypes
 */
int preload_catalog_lid(void);
int load_catalog_lid(int loaded);
void nest_pmu_init(int loaded);

#endif	/* __NEST_H__ */
