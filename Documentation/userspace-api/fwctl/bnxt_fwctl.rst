.. SPDX-License-Identifier: GPL-2.0

=================
fwctl bnxt driver
=================

:Author: Pavan Chebbi

Overview
========

BNXT driver makes a fwctl service available through an auxiliary_device.
The bnxt_fwctl driver binds to this device and registers itself with the
fwctl subsystem.

The bnxt_fwctl driver is agnostic to the device firmware internals. It
uses the Upper Layer Protocol (ULP) conduit provided by bnxt to send
HardWare Resource Manager (HWRM) commands to firmware.

These commands can query or change firmware driven device configurations
and read/write registers that are useful for debugging.

bnxt_fwctl User API
===================

Each RPC request contains a message request structure (HWRM input),
its length, optional request timeout, and dma buffers' information
if the command needs any DMA. The request is then put together with
the request data and sent through bnxt's message queue to the firmware,
and the results are returned to the caller.

A typical user application can send a FWCTL_INFO command using ioctl()
to discover bnxt_fwctl's RPC capabilities as shown below:

        ioctl(fd, FWCTL_INFO, &fwctl_info_msg);

where fwctl_info_msg (of type struct fwctl_info) describes bnxt_info_msg
(of type struct fwctl_info_bnxt). fwctl_info_msg is set up as follows:

        size = sizeof(struct fwctl_info);
        flags = 0;
        device_data_len = sizeof(bnxt_info_msg);
        out_device_data = (__aligned_u64)&bnxt_info_msg;

The uctx_caps of bnxt_info_msg represents the capabilities as described
in fwctl_bnxt_commands of include/uapi/fwctl/bnxt.h

The FW RPC itself, FWCTL_RPC can be sent using ioctl() as:

        ioctl(fd, FWCTL_RPC, &fwctl_rpc_msg);

where fwctl_rpc_msg (of type struct fwctl_rpc) encapsulates fwctl_rpc_bnxt
(see bnxt_rpc_msg below). fwctl_rpc_bnxt members are set up as per the
requirements of specific HWRM commands described in include/bnxt/hsi.h.
An example for HWRM_VER_GET is shown below:

        struct fwctl_rpc_bnxt bnxt_rpc_msg;
        struct hwrm_ver_get_output resp;
        struct fwctl_rpc fwctl_rpc_msg;
        struct hwrm_ver_get_input req;

        req.req_type = HWRM_VER_GET;
        req.hwrm_intf_maj = HWRM_VERSION_MAJOR;
        req.hwrm_intf_min = HWRM_VERSION_MINOR;
        req.hwrm_intf_upd = HWRM_VERSION_UPDATE;
        req.cmpl_ring = -1;
        req.target_id = -1;

        bnxt_rpc_msg.req_len = sizeof(struct hwrm_ver_get_input);
        bnxt_rpc_msg.num_dma = 0;
        bnxt_rpc_msg.req = (__aligned_u64)&req;

        fwctl_rpc_msg.size = sizeof(struct fwctl_rpc);
        fwctl_rpc_msg.scope = FWCTL_RPC_DEBUG_READ_ONLY;
        fwctl_rpc_msg.in_len = sizeof(bnxt_rpc_msg) + sizeof(req);
        fwctl_rpc_msg.out_len = sizeof(struct hwrm_ver_get_output);
        fwctl_rpc_msg.in = (__aligned_u64)&bnxt_rpc_msg;
        fwctl_rpc_msg.out = (__aligned_u64)&resp;
