/* dra_bkgnoise.ps.c */                                                       
/* Default background noise model for radio link Transceiver Pipeline */

/****************************************/
/*		  Copyright (c) 1993-2002		*/
/*		by OPNET Technologies, Inc.		*/
/*		(A Delaware Corporation)		*/
/*	7255 Woodmont Av., Suite 250  		*/
/*     Bethesda, MD 20814, U.S.A.       */
/*			All Rights Reserved.		*/
/****************************************/

#include "opnet.h"
//要用malloc和free，所以要包含stdlib.h
#include "stdlib.h"

#include "spma_global.h"


/***** constants *****/

#define BOLTZMANN			1.379E-23
#define BKG_TEMP			290.0
#define AMB_NOISE_LEVEL		1.0E-26


/***** procedure *****/

#if defined (__cplusplus)
extern "C"
#endif
void
//dra_bkgnoise_mt_airspace_ad_hoc_my (OP_SIM_CONTEXT_ARG_OPT_COMMA Packet * pkptr)
dra_bkgnoise_spma (OP_SIM_CONTEXT_ARG_OPT_COMMA Packet * pkptr)
	{
	double		rx_noisefig, rx_temp, rx_bw;
	double		bkg_temp, bkg_noise, amb_noise;
	
	
		//RSK
	double                tx_delay_my;
	Objid                 mac_id;
	Objid                 own_id;
	Objid                 own_node_id;
	int *                 p_to_mac_svar_num; 
	int *                 p_total_num; 
	rcvd_pkt_queue **     p_to_mac_svar;
	int *                 my_address;
	

	/** Compute noise sources other than transmission interference. **/
	FIN_MT (dra_bkgnoise (pkptr));
	
	/* Get receiver noise figure. */
	rx_noisefig = op_td_get_dbl (pkptr, OPC_TDA_RA_RX_NOISEFIG);

	/* Calculate effective receiver temperature. */
	rx_temp = (rx_noisefig - 1.0) * 290.0;

	/* Set the effective background temperature. */
	bkg_temp = BKG_TEMP;

	/* Get receiver channel bandwidth (in Hz). */
	rx_bw = op_td_get_dbl (pkptr, OPC_TDA_RA_RX_BW);

	/* Calculate in-band noise from both background and thermal sources. */
	bkg_noise = (rx_temp + bkg_temp) * rx_bw * BOLTZMANN;

	/* Calculate in-band ambient noise. */
	amb_noise = rx_bw * AMB_NOISE_LEVEL;

	/* Put the sum of both noise sources in the packet transmission data attr.*/
	op_td_set_dbl (pkptr, OPC_TDA_RA_BKGNOISE, (amb_noise + bkg_noise));

	
	tx_delay_my = op_td_get_dbl (pkptr, OPC_TDA_RA_TX_DELAY);
	own_id=op_td_get_int(pkptr,OPC_TDA_RA_RX_CH_OBJID);
	own_node_id=op_topo_parent(op_topo_parent(op_topo_parent(own_id)));
	mac_id=op_id_from_name(own_node_id,OPC_OBJTYPE_PROC,"mac");

	p_to_mac_svar_num = op_ima_obj_svar_get(mac_id,"rcvd_pkt_queue_array_num");
	p_total_num = op_ima_obj_svar_get(mac_id,"rcvd_pkt_num");
	my_address = op_ima_obj_svar_get(mac_id,"my_address");
//	p_to_mac_svar=(rcvd_pkt_queue **)malloc(sizeof(rcvd_pkt_queue *));
	p_to_mac_svar = op_ima_obj_svar_get(mac_id,"rcvd_pkt_queue_array");
	
	( *(p_to_mac_svar+(*p_to_mac_svar_num) ) ) -> start_time = op_sim_time();
	( *(p_to_mac_svar+(*p_to_mac_svar_num) ) ) -> end_time = op_sim_time()+tx_delay_my;
//	free(p_to_mac_svar);

	* p_to_mac_svar_num = * p_to_mac_svar_num + 1;
	* p_total_num = * p_total_num + 1;
	
	FOUT
	}
