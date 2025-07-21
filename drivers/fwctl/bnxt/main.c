// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025, Broadcom Corporation
 */

#include <linux/kernel.h>
#include <linux/auxiliary_bus.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/fwctl.h>
#include <uapi/fwctl/fwctl.h>
#include <uapi/fwctl/bnxt.h>
#include <linux/bnxt/hsi.h>
#include <linux/bnxt/ulp.h>

struct bnxtctl_uctx {
	struct fwctl_uctx uctx;
	u32 uctx_caps;
};

struct bnxtctl_dev {
	struct fwctl_device fwctl;
	struct bnxt_aux_priv *aux_priv;
	void *dma_virt_addr[MAX_NUM_DMA_INDICATIONS];
	dma_addr_t dma_addr[MAX_NUM_DMA_INDICATIONS];
};

DEFINE_FREE(bnxtctl, struct bnxtctl_dev *, if (_T) fwctl_put(&_T->fwctl))

static int bnxtctl_open_uctx(struct fwctl_uctx *uctx)
{
	struct bnxtctl_uctx *bnxtctl_uctx =
		container_of(uctx, struct bnxtctl_uctx, uctx);

	bnxtctl_uctx->uctx_caps = BIT(FWCTL_BNXT_QUERY_COMMANDS) |
				  BIT(FWCTL_BNXT_SEND_COMMAND);
	return 0;
}

static void bnxtctl_close_uctx(struct fwctl_uctx *uctx)
{
}

static void *bnxtctl_info(struct fwctl_uctx *uctx, size_t *length)
{
	struct bnxtctl_uctx *bnxtctl_uctx =
		container_of(uctx, struct bnxtctl_uctx, uctx);
	struct fwctl_info_bnxt *info;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return ERR_PTR(-ENOMEM);

	info->uctx_caps = bnxtctl_uctx->uctx_caps;

	*length = sizeof(*info);
	return info;
}

static bool bnxtctl_validate_rpc(struct bnxt_en_dev *edev,
				 struct bnxt_fw_msg *hwrm_in,
				 enum fwctl_rpc_scope scope)
{
	struct input *req = (struct input *)hwrm_in->msg;

	guard(mutex)(&edev->en_dev_lock);
	if (edev->flags & BNXT_EN_FLAG_ULP_STOPPED)
		return false;

	switch (le16_to_cpu(req->req_type)) {
	case HWRM_FUNC_VF_CFG:
	case HWRM_FUNC_RESET:
	case HWRM_FUNC_CFG:
	case HWRM_PORT_PHY_CFG:
	case HWRM_PORT_MAC_CFG:
	case HWRM_PORT_CLR_STATS:
	case HWRM_QUEUE_PRI2COS_CFG:
	case HWRM_QUEUE_COS2BW_CFG:
	case HWRM_QUEUE_DSCP2PRI_CFG:
	case HWRM_QUEUE_ADPTV_QOS_RX_FEATURE_CFG:
	case HWRM_QUEUE_ADPTV_QOS_TX_FEATURE_CFG:
	case HWRM_QUEUE_ADPTV_QOS_RX_TUNING_CFG:
	case HWRM_VNIC_RSS_CFG:
	case HWRM_TUNNEL_DST_PORT_ALLOC:
	case HWRM_TUNNEL_DST_PORT_FREE:
	case HWRM_QUEUE_ADPTV_QOS_TX_TUNING_CFG:
	case HWRM_PORT_TX_FIR_CFG:
	case HWRM_FW_SET_STRUCTURED_DATA:
	case HWRM_PORT_PRBS_TEST:
	case HWRM_PORT_EP_TX_CFG:
	case HWRM_CFA_REDIRECT_TUNNEL_TYPE_INFO:
	case HWRM_CFA_FLOW_FLUSH:
	case HWRM_CFA_L2_FILTER_ALLOC:
	case HWRM_CFA_NTUPLE_FILTER_FREE:
	case HWRM_CFA_REDIRECT_TUNNEL_TYPE_ALLOC:
	case HWRM_CFA_REDIRECT_TUNNEL_TYPE_FREE:
	case HWRM_FW_LIVEPATCH:
	case HWRM_FW_RESET:
	case HWRM_FW_SYNC:
	case HWRM_FW_SET_TIME:
	case HWRM_PORT_CFG:
	case HWRM_FUNC_PTP_PIN_CFG:
	case HWRM_FUNC_PTP_CFG:
	case HWRM_FUNC_PTP_EXT_CFG:
	case HWRM_FUNC_SYNCE_CFG:
	case HWRM_MFG_OTP_CFG:
	case HWRM_MFG_TESTS:
	case HWRM_UDCC_CFG:
	case HWRM_DBG_SERDES_TEST:
	case HWRM_DBG_LOG_BUFFER_FLUSH:
	case HWRM_DBG_DUMP:
	case HWRM_DBG_ERASE_NVM:
	case HWRM_DBG_CFG:
	case HWRM_DBG_COREDUMP_LIST:
	case HWRM_DBG_COREDUMP_INITIATE:
	case HWRM_DBG_COREDUMP_RETRIEVE:
	case HWRM_DBG_CRASHDUMP_HEADER:
	case HWRM_DBG_CRASHDUMP_ERASE:
	case HWRM_DBG_PTRACE:
	case HWRM_DBG_TOKEN_CFG:
	case HWRM_NVM_DEFRAG:
	case HWRM_NVM_FACTORY_DEFAULTS:
	case HWRM_NVM_FLUSH:
	case HWRM_NVM_INSTALL_UPDATE:
	case HWRM_NVM_MODIFY:
	case HWRM_NVM_VERIFY_UPDATE:
	case HWRM_NVM_ERASE_DIR_ENTRY:
	case HWRM_NVM_MOD_DIR_ENTRY:
	case HWRM_NVM_FIND_DIR_ENTRY:
	case HWRM_NVM_RAW_DUMP:
		return scope >= FWCTL_RPC_CONFIGURATION;

	case HWRM_VER_GET:
	case HWRM_FW_GET_STRUCTURED_DATA:
	case HWRM_ERROR_RECOVERY_QCFG:
	case HWRM_FUNC_QCAPS:
	case HWRM_FUNC_QCFG:
	case HWRM_FUNC_QSTATS:
	case HWRM_PORT_QSTATS:
	case HWRM_PORT_PHY_QCFG:
	case HWRM_PORT_MAC_QCFG:
	case HWRM_PORT_PHY_QCAPS:
	case HWRM_PORT_PHY_I2C_READ:
	case HWRM_PORT_PHY_MDIO_READ:
	case HWRM_QUEUE_PRI2COS_QCFG:
	case HWRM_QUEUE_COS2BW_QCFG:
	case HWRM_QUEUE_DSCP2PRI_QCFG:
	case HWRM_VNIC_RSS_QCFG:
	case HWRM_QUEUE_GLOBAL_QCFG:
	case HWRM_QUEUE_ADPTV_QOS_RX_FEATURE_QCFG:
	case HWRM_QUEUE_ADPTV_QOS_TX_FEATURE_QCFG:
	case HWRM_QUEUE_QCAPS:
	case HWRM_QUEUE_ADPTV_QOS_RX_TUNING_QCFG:
	case HWRM_QUEUE_ADPTV_QOS_TX_TUNING_QCFG:
	case HWRM_TUNNEL_DST_PORT_QUERY:
	case HWRM_PORT_QSTATS_EXT:
	case HWRM_PORT_TX_FIR_QCFG:
	case HWRM_FW_LIVEPATCH_QUERY:
	case HWRM_FW_QSTATUS:
	case HWRM_FW_HEALTH_CHECK:
	case HWRM_FW_GET_TIME:
	case HWRM_PORT_DSC_DUMP:
	case HWRM_PORT_EP_TX_QCFG:
	case HWRM_PORT_QCFG:
	case HWRM_PORT_MAC_QCAPS:
	case HWRM_TEMP_MONITOR_QUERY:
	case HWRM_REG_POWER_QUERY:
	case HWRM_CORE_FREQUENCY_QUERY:
	case HWRM_STAT_QUERY_ROCE_STATS:
	case HWRM_STAT_QUERY_ROCE_STATS_EXT:
	case HWRM_CFA_REDIRECT_QUERY_TUNNEL_TYPE:
	case HWRM_CFA_FLOW_INFO:
	case HWRM_CFA_ADV_FLOW_MGNT_QCAPS:
	case HWRM_FUNC_RESOURCE_QCAPS:
	case HWRM_FUNC_BACKING_STORE_QCAPS:
	case HWRM_FUNC_BACKING_STORE_QCFG:
	case HWRM_FUNC_QSTATS_EXT:
	case HWRM_FUNC_PTP_PIN_QCFG:
	case HWRM_FUNC_PTP_EXT_QCFG:
	case HWRM_FUNC_BACKING_STORE_QCFG_V2:
	case HWRM_FUNC_BACKING_STORE_QCAPS_V2:
	case HWRM_FUNC_SYNCE_QCFG:
	case HWRM_FUNC_TTX_PACING_RATE_PROF_QUERY:
	case HWRM_PCIE_QSTATS:
	case HWRM_MFG_OTP_QCFG:
	case HWRM_MFG_FRU_EEPROM_READ:
	case HWRM_MFG_GET_NVM_MEASUREMENT:
	case HWRM_STAT_GENERIC_QSTATS:
	case HWRM_PORT_PHY_FDRSTAT:
	case HWRM_UDCC_QCAPS:
	case HWRM_UDCC_QCFG:
	case HWRM_UDCC_SESSION_QCFG:
	case HWRM_UDCC_SESSION_QUERY:
	case HWRM_UDCC_COMP_QCFG:
	case HWRM_UDCC_COMP_QUERY:
	case HWRM_QUEUE_ADPTV_QOS_RX_QCFG:
	case HWRM_QUEUE_ADPTV_QOS_TX_QCFG:
	case HWRM_TF_RESC_USAGE_QUERY:
	case HWRM_TFC_RESC_USAGE_QUERY:
	case HWRM_DBG_READ_DIRECT:
	case HWRM_DBG_READ_INDIRECT:
	case HWRM_DBG_RING_INFO_GET:
	case HWRM_DBG_QCAPS:
	case HWRM_DBG_QCFG:
	case HWRM_DBG_USEQ_FLUSH:
	case HWRM_DBG_USEQ_QCAPS:
	case HWRM_DBG_SIM_CABLE_STATE:
	case HWRM_DBG_TOKEN_QUERY_AUTH_IDS:
	case HWRM_NVM_GET_VARIABLE:
	case HWRM_NVM_GET_DEV_INFO:
	case HWRM_NVM_GET_DIR_ENTRIES:
	case HWRM_NVM_GET_DIR_INFO:
	case HWRM_NVM_READ:
	case HWRM_SELFTEST_QLIST:
	case HWRM_SELFTEST_RETRIEVE_SERDES_DATA:
		return scope >= FWCTL_RPC_DEBUG_READ_ONLY;

	case HWRM_PORT_PHY_I2C_WRITE:
	case HWRM_MFG_FRU_WRITE_CONTROL:
	case HWRM_MFG_FRU_EEPROM_WRITE:
	case HWRM_DBG_WRITE_DIRECT:
	case HWRM_NVM_SET_VARIABLE:
	case HWRM_NVM_WRITE:
	case HWRM_NVM_RAW_WRITE_BLK:
	case HWRM_PORT_PHY_MDIO_WRITE:
		return scope >= FWCTL_RPC_DEBUG_WRITE;

	default:
		return false;
	}
}

static int bnxt_fw_setup_input_dma(struct bnxtctl_dev *bnxt_dev,
				   struct device *dev,
				   int num_dma,
				   struct fwctl_dma_info_bnxt *msg,
				   struct bnxt_fw_msg *fw_msg)
{
	u8 i, num_allocated = 0;
	void *dma_ptr;
	int rc = 0;

	for (i = 0; i < num_dma; i++) {
		if (msg->len == 0 || msg->len > MAX_DMA_MEM_SIZE) {
			rc = -EINVAL;
			goto err;
		}
		bnxt_dev->dma_virt_addr[i] = dma_alloc_coherent(dev->parent,
								msg->len,
								&bnxt_dev->dma_addr[i],
								GFP_KERNEL);
		if (!bnxt_dev->dma_virt_addr[i]) {
			rc = -ENOMEM;
			goto err;
		}
		num_allocated++;
		if (msg->dma_direction == DEVICE_WRITE) {
			if (copy_from_user(bnxt_dev->dma_virt_addr[i],
					   u64_to_user_ptr(msg->data),
					   msg->len)) {
				rc = -EFAULT;
				goto err;
			}
		}
		dma_ptr = fw_msg->msg + msg->offset;

		if ((PTR_ALIGN(dma_ptr, 8) == dma_ptr) &&
		    msg->offset < fw_msg->msg_len) {
			__le64 *dmap = dma_ptr;

			*dmap = cpu_to_le64(bnxt_dev->dma_addr[i]);
		} else {
			rc = -EINVAL;
			goto err;
		}
		msg += 1;
	}

	return 0;
err:
	for (i = 0; i < num_allocated; i++)
		dma_free_coherent(dev->parent,
				  msg->len,
				  bnxt_dev->dma_virt_addr[i],
				  bnxt_dev->dma_addr[i]);

	return rc;
}

static void *bnxtctl_fw_rpc(struct fwctl_uctx *uctx,
			    enum fwctl_rpc_scope scope,
			    void *in, size_t in_len, size_t *out_len)
{
	struct bnxtctl_dev *bnxtctl =
		container_of(uctx->fwctl, struct bnxtctl_dev, fwctl);
	struct bnxt_aux_priv *bnxt_aux_priv = bnxtctl->aux_priv;
	struct fwctl_dma_info_bnxt *dma_buf = NULL;
	struct device *dev = &uctx->fwctl->dev;
	struct fwctl_rpc_bnxt *msg = in;
	struct bnxt_fw_msg rpc_in;
	int i, rc, err = 0;

	rpc_in.msg = kzalloc(msg->req_len, GFP_KERNEL);
	if (!rpc_in.msg)
		return ERR_PTR(-ENOMEM);

	if (copy_from_user(rpc_in.msg, u64_to_user_ptr(msg->req),
			   msg->req_len)) {
		dev_dbg(dev, "Failed to copy in_payload from user\n");
		err = -EFAULT;
		goto free_msg_out;
	}

	if (!bnxtctl_validate_rpc(bnxt_aux_priv->edev, &rpc_in, scope)) {
		err = -EPERM;
		goto free_msg_out;
	}

	rpc_in.msg_len = msg->req_len;
	rpc_in.resp = kzalloc(*out_len, GFP_KERNEL);
	if (!rpc_in.resp) {
		err = -ENOMEM;
		goto free_msg_out;
	}

	rpc_in.resp_max_len = *out_len;
	if (!msg->timeout)
		rpc_in.timeout = DFLT_HWRM_CMD_TIMEOUT;
	else
		rpc_in.timeout = msg->timeout;

	if (msg->num_dma) {
		if (msg->num_dma > MAX_NUM_DMA_INDICATIONS) {
			dev_err(dev, "DMA buffers exceed the number supported\n");
			err = -EINVAL;
			goto free_msg_out;
		}

		dma_buf = kcalloc(msg->num_dma, sizeof(*dma_buf), GFP_KERNEL);
		if (!dma_buf) {
			err = -ENOMEM;
			goto free_msg_out;
		}

		if (copy_from_user(dma_buf, u64_to_user_ptr(msg->payload),
				   msg->num_dma * sizeof(*dma_buf))) {
			dev_dbg(dev, "Failed to copy payload from user\n");
			err = -EFAULT;
			goto free_dmabuf_out;
		}

		rc = bnxt_fw_setup_input_dma(bnxtctl, dev, msg->num_dma,
					     dma_buf, &rpc_in);
		if (rc) {
			err = -EIO;
			goto free_dmabuf_out;
		}
	}

	rc = bnxt_send_msg(bnxt_aux_priv->edev, &rpc_in);
	if (rc) {
		err = -EIO;
		goto free_dma_out;
	}

	for (i = 0; i < msg->num_dma; i++) {
		if (dma_buf[i].dma_direction == DEVICE_READ) {
			if (copy_to_user(u64_to_user_ptr(dma_buf[i].data),
					 bnxtctl->dma_virt_addr[i],
					 dma_buf[i].len)) {
				dev_dbg(dev, "Failed to copy resp to user\n");
				err = -EFAULT;
				break;
			}
		}
	}
free_dma_out:
	for (i = 0; i < msg->num_dma; i++)
		dma_free_coherent(dev->parent, dma_buf[i].len,
				  bnxtctl->dma_virt_addr[i],
				  bnxtctl->dma_addr[i]);
free_dmabuf_out:
	kfree(dma_buf);
free_msg_out:
	kfree(rpc_in.msg);

	if (err) {
		kfree(rpc_in.resp);
		return ERR_PTR(err);
	}

	return rpc_in.resp;
}

static const struct fwctl_ops bnxtctl_ops = {
	.device_type = FWCTL_DEVICE_TYPE_BNXT,
	.uctx_size = sizeof(struct bnxtctl_uctx),
	.open_uctx = bnxtctl_open_uctx,
	.close_uctx = bnxtctl_close_uctx,
	.info = bnxtctl_info,
	.fw_rpc = bnxtctl_fw_rpc,
};

static int bnxtctl_probe(struct auxiliary_device *adev,
			 const struct auxiliary_device_id *id)
{
	struct bnxt_aux_priv *aux_priv =
		container_of(adev, struct bnxt_aux_priv, aux_dev);
	struct bnxtctl_dev *bnxtctl __free(bnxtctl) =
		fwctl_alloc_device(&aux_priv->edev->pdev->dev, &bnxtctl_ops,
				   struct bnxtctl_dev, fwctl);
	int rc;

	if (!bnxtctl)
		return -ENOMEM;

	bnxtctl->aux_priv = aux_priv;

	rc = fwctl_register(&bnxtctl->fwctl);
	if (rc)
		return rc;

	auxiliary_set_drvdata(adev, no_free_ptr(bnxtctl));
	return 0;
}

static void bnxtctl_remove(struct auxiliary_device *adev)
{
	struct bnxtctl_dev *ctldev = auxiliary_get_drvdata(adev);

	fwctl_unregister(&ctldev->fwctl);
	fwctl_put(&ctldev->fwctl);
}

static const struct auxiliary_device_id bnxtctl_id_table[] = {
	{ .name = "bnxt_en.fwctl", },
	{}
};
MODULE_DEVICE_TABLE(auxiliary, bnxtctl_id_table);

static struct auxiliary_driver bnxtctl_driver = {
	.name = "bnxt_fwctl",
	.probe = bnxtctl_probe,
	.remove = bnxtctl_remove,
	.id_table = bnxtctl_id_table,
};

module_auxiliary_driver(bnxtctl_driver);

MODULE_IMPORT_NS("FWCTL");
MODULE_DESCRIPTION("BNXT fwctl driver");
MODULE_AUTHOR("Pavan Chebbi <pavan.chebbi@broadcom.com>");
MODULE_AUTHOR("Andy Gospodarek <gospo@broadcom.com>");
MODULE_LICENSE("GPL");
