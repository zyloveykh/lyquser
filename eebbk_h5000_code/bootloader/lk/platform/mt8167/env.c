/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include <sys/types.h>
#include <stdint.h>
#include <printf.h>
#include <malloc.h>
#include <string.h>

#include <platform/env.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/mt_typedefs.h>
#include <platform/errno.h>

#ifdef MTK_EMMC_SUPPORT
#define ENV_PART "para"
#else
#define ENV_PART PART_MISC
#endif

#define MODULE_NAME "LK_ENV"
env_t g_env;
static int env_valid;
static char *env_buffer;
static char env_get_char(int index);
static char *env_get_addr(int index);
static int envmatch(uchar *s1, int i2);
static int write_env_area(char *env_buf);
static int read_env_area(char *env_buf);

#ifndef min
#define min(x, y)   (x < y ? x : y)
#endif

void env_init(void)
{
	int ret, i;
	env_buffer = (char *)malloc(CFG_ENV_SIZE);
	memset(env_buffer, 0x00, CFG_ENV_SIZE);
	g_env.env_data = env_buffer + CFG_ENV_DATA_OFFSET;
	ret = read_env_area(env_buffer);
	int checksum = 0;
	if (ret < 0) {
		printf("[%s]read_env_area fail, ret = %x\n", MODULE_NAME, ret);
		env_valid = 0;
		goto end;
	}

	memcpy(g_env.sig, env_buffer, sizeof(g_env.sig));
	memcpy(g_env.sig_1, env_buffer + CFG_ENV_SIG_1_OFFSET, sizeof(g_env.sig_1));

	if (!strcmp(g_env.sig, ENV_SIG) && !strcmp(g_env.sig_1, ENV_SIG)) {
		g_env.checksum = *((int *)env_buffer + CFG_ENV_CHECKSUM_OFFSET / 4);
		for (i = 0; i < (int)(CFG_ENV_DATA_SIZE); i++) {
			checksum += g_env.env_data[i];
		}
		if (checksum != g_env.checksum) {
			printf("[%s]checksum mismatch s %d d %d!\n", MODULE_NAME, g_env.checksum, checksum);
			env_valid = 0;
			goto end;
		} else {
			printf("[%s]ENV initialize sucess\n", MODULE_NAME);
			env_valid = 1;
		}

	} else {
		printf("[%s]ENV SIG Wrong\n", MODULE_NAME);
		env_valid = 0;
		goto end;
	}
end:
	if (!env_valid) {
		memset(env_buffer, 0x00, CFG_ENV_SIZE);
	}

}

char *get_env(char *name)
{
	int i, nxt;
	printf("[%s]get_env %s\n", MODULE_NAME, name);
	if (!env_valid)
		return NULL;

	for (i = 0; env_get_char(i) != '\0'; i = nxt + 1) {
		int val;

		for (nxt = i; env_get_char(nxt) != '\0'; ++nxt) {
			if (nxt >= CFG_ENV_SIZE) {
				return (NULL);
			}
		}
		if ((val = envmatch((uchar *) name, i)) < 0)
			continue;
		return ((char *)env_get_addr(val));
	}

	return (NULL);
}

static char env_get_char(int index)
{
	return *(g_env.env_data + index);
}

static char *env_get_addr(int index)
{
	return (g_env.env_data + index);

}

static int envmatch(uchar *s1, int i2)
{

	while (*s1 == env_get_char(i2++))
		if (*s1++ == '=')
			return (i2);
	if (*s1 == '\0' && env_get_char(i2 - 1) == '=')
		return (i2);
	return (-1);
}

int set_env(char *name, char *value)
{
	int len, oldval;
	uchar *env, *nxt = NULL;

	int ret;

	uchar *env_data = (uchar *) g_env.env_data;

	printf("[%s]set_env %s %s\n", MODULE_NAME, name, value);

	oldval = -1;

	if (!env_valid) {
		env = env_data;
		goto add;
	}

	for (env = env_data; *env; env = nxt + 1) {
		for (nxt = env; *nxt; ++nxt);
		if ((oldval = envmatch((uchar *) name, env - env_data)) >= 0)
			break;
	}

	if (oldval > 0) {
		if (*++nxt == '\0') {
			if (env > env_data) {
				env--;
			} else {
				*env = '\0';
			}
		} else {
			for (;;) {
				*env = *nxt++;
				if ((*env == '\0') && (*nxt == '\0'))
					break;
				++env;
			}
		}
		*++env = '\0';
	}

	for (env = env_data; *env || *(env + 1); ++env);
	if (env > env_data)
		++env;

add:
	/*
	 * Overflow when:
	 * "name" + "=" + "val" +"\0\0"  > ENV_SIZE - (env-env_data)
	 */
	len = strlen(name) + 2;
	/* add '=' for first arg, ' ' for all others */
	len += strlen(value) + 1;

	if (len > (&env_data[CFG_ENV_DATA_SIZE] - env)) {
		printf("## Error: environment overflow, \"%s\" deleted\n", name);
		return -1;
	}
	while ((*env = *name++) != '\0')
		env++;

	*env = '=';

	while ((*++env = *value++) != '\0');

	/* end is marked with double '\0' */
	*++env = '\0';
	memset(env, 0x00, CFG_ENV_DATA_SIZE - (env - env_data));
	ret = write_env_area(env_buffer);
	if (ret < 0) {
		printf("[%s]write env fail\n", MODULE_NAME);
		memset(env_buffer, 0x00, CFG_ENV_SIZE);
		return -1;
	}
	env_valid = 1;
	return 0;

}

void print_env()
{
	int i, nxt;
	uchar *env = (uchar *) g_env.env_data;
	if (!env_valid) {
		printf("[%s]no valid env\n", MODULE_NAME);
		return;
	}
	printf("[%s]env:\n", MODULE_NAME);
	for (i = 0; env_get_char(i) != '\0'; i = nxt + 1) {
		for (nxt = i; env_get_char(nxt) != '\0'; ++nxt) {
			if (nxt >= (int)(CFG_ENV_DATA_SIZE)) {
				return;
			}
		}
		printf("%s\n", env + i);
	}
}

static int write_env_area(char *env_buf)
{
	part_t *part;
	part_dev_t *dev;
	size_t count;
	long len;
	int i, checksum = 0;
	unsigned long long start_addr;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(ENV_PART);
	if (!part) {
		return -ENOENT;
	}

	start_addr = (u64) part->start_sect * dev->blkdev->blksz;

	count = min(sizeof(ENV_SIG), sizeof(g_env.sig));
	memcpy(env_buf, ENV_SIG, count);
	memcpy(env_buf + CFG_ENV_SIG_1_OFFSET, ENV_SIG, count);

	for (i = 0; i < (int)CFG_ENV_DATA_SIZE; i++) {
		checksum += *(env_buf + CFG_ENV_DATA_OFFSET + i);
	}
	printf("checksum %d\n", checksum);

	*((int *)env_buf + CFG_ENV_CHECKSUM_OFFSET / 4) = checksum;

	len = dev->write(dev, (uchar *) env_buf, start_addr + CFG_ENV_OFFSET, CFG_ENV_SIZE, part->part_id);
	if (len < 0) {
		return -EIO;
	}

	return 0;

}

static int read_env_area(char *env_buf)
{
	part_t *part;
	part_dev_t *dev;
	long len;
	unsigned long long start_addr;

	dev = mt_part_get_device();
	if (!dev) {
		return -ENODEV;
	}

	part = mt_part_get_partition(ENV_PART);
	if (!part) {
		return -ENOENT;
	}

	start_addr = (u64) part->start_sect * dev->blkdev->blksz;

	len = dev->read(dev, start_addr + CFG_ENV_OFFSET, (uchar *) env_buf, CFG_ENV_SIZE, part->part_id);
	if (len < 0) {
		return -EIO;
	}
	return 0;
}
