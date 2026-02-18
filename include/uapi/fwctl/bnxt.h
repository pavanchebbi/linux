/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Copyright (c) 2026, Broadcom Inc
 */

#ifndef _UAPI_FWCTL_BNXT_H_
#define _UAPI_FWCTL_BNXT_H_

#include <linux/types.h>

#define FWCTL_BNXT_HWRM_DFLT_TIMEOUT    500 /* ms */

enum fwctl_bnxt_commands {
	FWCTL_BNXT_INLINE_COMMANDS = 0,
	FWCTL_BNXT_QUERY_COMMANDS,
	FWCTL_BNXT_SEND_COMMANDS,
};

/**
 * struct fwctl_info_bnxt - ioctl(FWCTL_INFO) out_device_data
 * @uctx_caps: The command capabilities driver accepts.
 *
 * Return basic information about the FW interface available.
 */
struct fwctl_info_bnxt {
	__u32 uctx_caps;
};

/**
 * struct fwctl_rpc_bnxt - describe the fwctl message for bnxt
 * @req: FW (HWRM) command input structure
 * @req_len: length of @req
 * @timeout: in ms to override the driver's default, 0 otherwise
 * @reserved: must be 0
 * @reserved1: must be 0
 */
struct fwctl_rpc_bnxt {
	__aligned_u64 req;
	__u32 req_len;
	__u32 timeout;
	__u32 reserved[2];
	__aligned_u64 reserved1;
};
#endif
