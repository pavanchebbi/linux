.. SPDX-License-Identifier: GPL-2.0

================
fwctl bnxt driver
================

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
==================

Each RPC request contains a message request structure (HWRM input), its
length, optional request timeout, and dma buffers' information if the
command needs any DMA. The request is then put together with the request
data and sent through bnxt's message queue to the firmware, and the results
are returned to the caller.

A typical user application would send a FWCTL_RPC using ioctl() for a FW
command as below:

        ioctl(fd, FWCTL_RPC, &rpc_msg);

where rpc_msg (struct fwctl_rpc) is an encapsulation of fwctl_rpc_bnxt
(which contains the HWRM command description) and its response.
