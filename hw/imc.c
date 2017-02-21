/* Copyright 2016 IBM Corp.
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

#include <skiboot.h>
#include <xscom.h>
#include <imc.h>
#include <chip.h>
#include <libxz/xz.h>

/*
 * Nest IMC PMU names along with their bit values as represented in the
 * imc_chip_avl_vector(in struct imc_chip_cb, look at include/imc.h).
 * nest_pmus[] is an array containing all the possible nest IMC PMU node names.
 */
char const *nest_pmus[] = {
	"powerbus0",
	"mcs0",
	"mcs1",
	"mcs2",
	"mcs3",
	"mcs4",
	"mcs5",
	"mcs6",
	"mcs7",
	"mba0",
	"mba1",
	"mba2",
	"mba3",
	"mba4",
	"mba5",
	"mba6",
	"mba7",
	"cen0",
	"cen1",
	"cen2",
	"cen3",
	"cen4",
	"cen5",
	"cen6",
	"cen7",
	"xlink0",
	"xlink1",
	"xlink2",
	"mcd0",
	"mcd1",
	"phb0",
	"phb1",
	"phb2",
	"resvd",
	"nx",
	"capp0",
	"capp1",
	"vas",
	"int",
	"alink0",
	"alink1",
	"alink2",
	"nvlink0",
	"nvlink1",
	"nvlink2",
	"nvlink3",
	"nvlink4",
	"nvlink5",
	/* reserved bits : 48 - 64 */
};


static struct imc_chip_cb *get_imc_cb(void)
{
	uint64_t cb_loc;
	struct proc_chip *chip = get_chip(this_cpu()->chip_id);

	cb_loc = chip->homer_base + P9_CB_STRUCT_OFFSET;
	return (struct imc_chip_cb *)cb_loc;
}

/*
 * Decompresses the blob obtained from the IMA_CATALOG sub-partition
 * in "buf" of size "size", assigns the uncompressed device tree
 * binary to "fdt" and returns.
 * Returns 0 on success and -1 on error.
 */
static int decompress_subpartition(char *buf, size_t size, void **fdt)
{
	struct xz_dec *s;
	struct xz_buf b;
	void *data;
	int ret = 0;

	/* Initialize the xz library first */
	xz_crc32_init();
	s = xz_dec_init(XZ_SINGLE, 0);
	if (s == NULL)
	{
		prerror("IMC: initialization error for xz\n");
		return -1;
	}

	/* Allocate memory for the uncompressed data */
	data = malloc(IMC_DTB_SIZE);
	if (!data) {
		prerror("IMC: memory allocation error\n");
		ret = -1;
		goto err;
	}

	/*
	 * Source address : buf
	 * Source size : size
	 * Destination address : data
	 * Destination size : IMC_DTB_SIZE
	 */
	b.in = buf;
	b.in_pos = 0;
	b.in_size = size;
	b.out = data;
	b.out_pos = 0;
	b.out_size = IMC_DTB_SIZE;

	/* Start decompressing */
	ret = xz_dec_run(s, &b);
	if (ret != XZ_STREAM_END) {
		prerror("IMC: failed to decompress subpartition\n");
		free(data);
		ret = -1;
	}
	*fdt = data;

err:
	/* Clean up memory */
	xz_dec_end(s);
	return ret;
}

/*
 * Remove the PMU device nodes from the ncoming new subtree, if they are not
 * available in the hardware. The availability is described by the
 * control block's imc_chip_avl_vector.
 * Each bit represents a device unit. If the device is available, then
 * the bit is set else its unset.
 */
static int disable_unavailable_units(struct dt_node *dev)
{
	uint64_t avl_vec;
	struct imc_chip_cb *cb;
	struct dt_node *target;
	int i;

	/* Fetch the IMC control block structure */
	cb = get_imc_cb();

	avl_vec = be64_to_cpu(cb->imc_chip_avl_vector);
	for (i = 0; i < MAX_AVL; i++) {
		if (!(PPC_BITMASK(i, i) & avl_vec)) {
			/* Check if the device node exists */
			target = dt_find_by_name(dev, nest_pmus[i]);
			if (!target)
				continue;
			/* Remove the device node */
			dt_free(target);
		}
	}

	return 0;
}

/*
 * Fetch the IMA_CATALOG partition and find the appropriate sub-partition
 * based on the platform's PVR.
 * Decompress the sub-partition and link the imc device tree to the
 * existing device tree.
 */
void imc_init(void)
{
	char *buf = NULL;
	size_t size = IMC_DTB_SIZE;
	uint32_t pvr = mfspr(SPR_PVR);
	int ret;
	struct dt_node *dev;
	void *fdt = NULL;
	struct dt_fixup_p parent;

	/* Enable only for power 9 */
	if (proc_gen != proc_gen_p9)
		return;

	buf = malloc(IMC_DTB_SIZE);
	if (!buf) {
		prerror("IMC: Memory allocation Failed\n");
		return;
	}

	ret = start_preload_resource(RESOURCE_ID_CATALOG,
					pvr, buf, &size);
	if (ret != OPAL_SUCCESS)
		goto err;

	ret = wait_for_resource_loaded(RESOURCE_ID_CATALOG,
					  pvr);
	if (ret != OPAL_SUCCESS) {
		prerror("IMC Catalog load failed\n");
		return;
	}

	/* Decompress the subpartition now */
	ret = decompress_subpartition(buf, size, &fdt);
	if (ret < 0)
		goto err;

	/* Create a device tree entry for imc counters */
	dev = dt_new_root("imc-counters");
	if (!dev)
		goto err;

	/* Attach the new fdt to the imc-counters node */
	ret = dt_expand_node(dev, fdt, 0);
	if (ret < 0) {
		dt_free(dev);
		goto err;
	}

	ret = dt_fixup_populate_llist(dev, &parent, "events");
	if (ret < 0) {
		dt_free(dev);
		goto err;
	}

	/* Fixup the phandle for the IMC device tree */
	dt_fixup_phandle(dev, &parent);
	dt_fixup_list_free(&parent);

	/* Check availability of the Nest PMU units from the availability vector */
	disable_unavailable_units(dev);

	if (!dt_attach_root(dt_root, dev)) {
		dt_free(dev);
		goto err;
	}

	return;
err:
	prerror("IMC Devices not added\n");
	free(buf);
}
