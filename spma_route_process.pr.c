/* Process model C form file: spma_route_process.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char spma_route_process_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A modeler 7 5CF7D521 5CF7D521 1 dellpc-c989244c Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

//ͷ�ļ�
#include <math.h>
#include <malloc.h>
#include <oms_dist_support.h>

//���峣��
#define NODE_NUM			20
#define ONE_HOP_DISTANCE	1

//�������ж�������
#define UPPER_IN_STRM_INDEX  0
#define UPPER_OUT_STRM_INDEX 0
#define LOWER_IN_STRM_INDEX 1
#define LOWER_OUT_STRM_INDEX 1



//����״̬ת������
#define UPPER_STRM (op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == UPPER_IN_STRM_INDEX)
#define LOWER_STRM (op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == LOWER_IN_STRM_INDEX)

//����ṹ�����
typedef struct Position_Map
	{
	Objid node_id;
	int address;
	double x_pos;
	double y_pos;
	}Position_Map;

typedef struct Neigh_Block
	{
	int address;
	int map_index;
	int node_level;
	double distance;
	}Neigh_Block;


//��������
static void	spma_route_sv_init();
static int network_get_local_address();
static int compute_next_hop(int dest_addr);
static int random_generate_destination();

//����ȫ�ֱ���
int route_send_num = 0;
int route_rcvd_num = 0;

/* End of Header Block */

#if !defined (VOSD_NO_FIN)
#undef	BIN
#undef	BOUT
#define	BIN		FIN_LOCAL_FIELD(_op_last_line_passed) = __LINE__ - _op_block_origin;
#define	BOUT	BIN
#define	BINIT	FIN_LOCAL_FIELD(_op_last_line_passed) = 0; _op_block_origin = __LINE__;
#else
#define	BINIT
#endif /* #if !defined (VOSD_NO_FIN) */



/* State variable definitions */
typedef struct
	{
	/* Internal state tracking for FSM */
	FSM_SYS_STATE
	/* State Variables */
	Objid	                  		my_objid                                        ;
	Objid	                  		my_node_objid                                   ;
	int	                    		own_address                                     ;
	Position_Map	           		PM[NODE_NUM]                                    ;
	List *	                 		neigh_list                                      ;
	Stathandle	             		load_ghdl                                       ;
	Stathandle	             		thro_ghdl                                       ;
	Stathandle	             		dely_ghdl                                       ;
	Stathandle	             		drop_ghdl                                       ;
	int	                    		algo                                            ;
	} spma_route_process_state;

#define my_objid                		op_sv_ptr->my_objid
#define my_node_objid           		op_sv_ptr->my_node_objid
#define own_address             		op_sv_ptr->own_address
#define PM                      		op_sv_ptr->PM
#define neigh_list              		op_sv_ptr->neigh_list
#define load_ghdl               		op_sv_ptr->load_ghdl
#define thro_ghdl               		op_sv_ptr->thro_ghdl
#define dely_ghdl               		op_sv_ptr->dely_ghdl
#define drop_ghdl               		op_sv_ptr->drop_ghdl
#define algo                    		op_sv_ptr->algo

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	spma_route_process_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((spma_route_process_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

/*״̬������ʼ������*/
static void spma_route_sv_init()
	{
	int i;
	
	FIN(spma_route_sv_init());

	//��ȡģ��ID
	my_objid = op_id_self ();
	
	//��ȡ�ڵ�ID
	my_node_objid = op_topo_parent (my_objid);

	//��ȡ���ص�ַ
	own_address = network_get_local_address();

	//��ʼ��ȫ������Ϣ��
	for(i = 0; i < NODE_NUM; i++)
		{
		PM[i].node_id = 0;
		PM[i].address = -1;
		PM[i].x_pos = -1.0;
		PM[i].y_pos = -1.0;
		}
	
	//�����ھ��б�
	neigh_list = op_prg_list_create();
	
	op_ima_sim_attr_get(OPC_IMA_INTEGER, "Algorithm",&algo);
	
	FOUT;
	}


/*��ȡ���ص�ַ*/
static int network_get_local_address()
	{
	char name_str[10];
	int len;
	int result;
	int i;
	FIN(network_get_local_address());
	
	//��ȡ�ڵ������
	op_ima_obj_attr_get(my_node_objid,"name",name_str);
	
	//����ڵ������ַ�������
	len = strlen(name_str);
	
	//������0��ʼ
	result = 0;
	
	//���ַ���ת����������ʽ
	for(i = 0; i< len; i++)
		{
		if(name_str[i] >= '0' && name_str[i] <= '9')
			{
			result = result * 10 + (name_str[i] - '0');
			}
		}
	
	//���û�гɹ����MAC��ַ�����������һ���Ƚϴ��ֵ����Ϊ�ڵ��ַ
	if(result < 0)
		result = op_dist_outcome(op_dist_load("uniform_int",1000,10000));
	
	//���ýڵ�ĵ�ַ
	op_ima_obj_attr_set(my_node_objid,"Node Address",result);
	
	FRET (result);
	}

/*������һ���ڵ�*/
static int compute_next_hop(int dest_addr)
	{
	int node_num;
	int node_index;
	Objid node_objid;
	double own_x_pos;
	double own_y_pos;
	double temp_x_pos;
	double temp_y_pos;
	double dest_x_pos;
	double dest_y_pos;
	double distance,distance1;
	int temp_address;
	double near_distance;
	int near_addr;
	
	FIN(obtain_position_map_with_neighbor());
	
	//��ȡ���ڵ��λ��
	op_ima_obj_attr_get(my_node_objid,"x position",&own_x_pos);
	op_ima_obj_attr_get(my_node_objid,"y position",&own_y_pos);
	
	//��ȡ�����еĽڵ���Ŀ
	node_num = op_topo_child_count(op_topo_parent(my_node_objid), OPC_OBJTYPE_NODE_MOBILE);
	
	//�����ڵ�
	dest_x_pos = -1.0;
	dest_y_pos = -1.0;
	for(node_index = 0; node_index < node_num; node_index++)
		{
		node_objid = op_topo_object (OPC_OBJTYPE_NODE_MOBILE, node_index);
		if(op_ima_obj_attr_exists(node_objid,"Node Address"))
			{
			//��ȡ�ڵ��λ����Ϣ
			op_ima_obj_attr_get(node_objid,"Node Address",&temp_address);
			if(temp_address == dest_addr)
				{
				op_ima_obj_attr_get(node_objid,"x position",&dest_x_pos);
				op_ima_obj_attr_get(node_objid,"y position",&dest_y_pos);
				}
			}
		}
	
	//δ����Ŀ�Ľڵ�
	if(dest_x_pos == -1.0 || dest_y_pos == -1.0)
		{
		FRET (-1);
		}
	
	//�����ڵ�
	near_distance = 5.0;
	near_addr = -1;
	for(node_index = 0; node_index < node_num; node_index++)
		{
		node_objid = op_topo_object (OPC_OBJTYPE_NODE_MOBILE, node_index);
		if(op_ima_obj_attr_exists(node_objid,"Node Address"))
			{
			//��ȡ�ڵ��λ����Ϣ
			op_ima_obj_attr_get(node_objid,"Node Address",&temp_address);
			op_ima_obj_attr_get(node_objid,"x position",&temp_x_pos);
			op_ima_obj_attr_get(node_objid,"y position",&temp_y_pos);
			
			//�����ھӽڵ�
			if(temp_address != own_address)
				{
				//����ڵ�֮��ľ���
				distance = sqrt((temp_x_pos - own_x_pos) * (temp_x_pos - own_x_pos) + (temp_y_pos - own_y_pos) * (temp_y_pos - own_y_pos));
				
				//�ж��Ƿ��ڵ���ͨ�ž���֮��
				if(distance < ONE_HOP_DISTANCE)
					{
					//�����ھӽڵ㵽Ŀ�ĵľ���
					distance1 = sqrt((temp_x_pos - dest_x_pos) * (temp_x_pos - dest_x_pos) + (temp_y_pos - dest_y_pos) * (temp_y_pos - dest_y_pos));
					if(distance1 < near_distance)
						{
						near_distance = distance1;
						near_addr = temp_address;
						}
					}
				}
			}
		}
	
	
	FRET (near_addr);
	}

/*���ѡ�񱾽ڵ��һ���ھӽڵ���ΪĿ�ĵ�ַ*/
static int random_generate_destination()
	{
	int result;
	Distribution * dest_addr_dist;
	
	FIN(random_generate_destination());
	
	//��ʼ��Ŀ�Ľڵ�Ϊ-1
	result = -1;
	
	do
		{
		//���ѡ��һ���ڵ�
		dest_addr_dist = op_dist_load("uniform_int", 0 , 19);
		result = op_dist_outcome(dest_addr_dist);
		}while(result == own_address);
	
	FRET (result);
	}


/* End of Function Block */

/* Undefine optional tracing in FIN/FOUT/FRET */
/* The FSM has its own tracing code and the other */
/* functions should not have any tracing.		  */
#undef FIN_TRACING
#define FIN_TRACING

#undef FOUTRET_TRACING
#define FOUTRET_TRACING

#if defined (__cplusplus)
extern "C" {
#endif
	void spma_route_process (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_spma_route_process_init (int * init_block_ptr);
	void _op_spma_route_process_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_spma_route_process_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_spma_route_process_alloc (VosT_Obtype, int);
	void _op_spma_route_process_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
spma_route_process (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (spma_route_process ());

		{
		/* Temporary Variables */
		Packet* 	pkptr;
		Ici	*		ici_ptr;
		int			destination;
		int 		next_addr;
		int			pksize;
		double		stamp_time;
		int			flag;
		/* End of Temporary Variables */


		FSM_ENTER ("spma_route_process")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_UNFORCED_NOLABEL (0, "init", "spma_route_process [init enter execs]")
				FSM_PROFILE_SECTION_IN ("spma_route_process [init enter execs]", state0_enter_exec)
				{
				//��ʼ�����̵�״̬����
				spma_route_sv_init();
				
				//���Ƚ����еĵ�һ���¼�
				op_intrpt_schedule_self (op_sim_time (), 0);
				
				//ע��ͳ����
				load_ghdl = op_stat_reg ("Route.Network Load (bps)",			OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				thro_ghdl = op_stat_reg ("Route.Network Throughput (bps)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				dely_ghdl = op_stat_reg ("Route.End-to-End Delay (sec)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				drop_ghdl = op_stat_reg ("Route.Drop Packet Rate",				OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (1,"spma_route_process")


			/** state (init) exit executives **/
			FSM_STATE_EXIT_UNFORCED (0, "init", "spma_route_process [init exit execs]")


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "init", "idle", "tr_14_0", "spma_route_process [init -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "idle", state1_enter_exec, "spma_route_process [idle enter execs]")
				FSM_PROFILE_SECTION_IN ("spma_route_process [idle enter execs]", state1_enter_exec)
				{
				
				}
				FSM_PROFILE_SECTION_OUT (state1_enter_exec)

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"spma_route_process")


			/** state (idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "idle", "spma_route_process [idle exit execs]")
				FSM_PROFILE_SECTION_IN ("spma_route_process [idle exit execs]", state1_exit_exec)
				{
				/* The only interrupt expected in this state is a	*/
				/* stream interrupt. It can be either from the MAC	*/
				/* layer for a packet destined for this node or		*/
				/* from the application layer for a packet destined	*/
				/* for some other node.								*/
				
				
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (idle) transition processing **/
			FSM_PROFILE_SECTION_IN ("spma_route_process [idle trans conditions]", state1_trans_conds)
			FSM_INIT_COND (UPPER_STRM)
			FSM_TEST_COND (LOWER_STRM)
			FSM_TEST_LOGIC ("idle")
			FSM_PROFILE_SECTION_OUT (state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 2, state2_enter_exec, ;, "UPPER_STRM", "", "idle", "appl layer arrival", "tr_14", "spma_route_process [idle -> appl layer arrival : UPPER_STRM / ]")
				FSM_CASE_TRANSIT (1, 3, state3_enter_exec, ;, "LOWER_STRM", "", "idle", "mac layer arrival", "tr_17", "spma_route_process [idle -> mac layer arrival : LOWER_STRM / ]")
				}
				/*---------------------------------------------------------*/



			/** state (appl layer arrival) enter executives **/
			FSM_STATE_ENTER_FORCED (2, "appl layer arrival", state2_enter_exec, "spma_route_process [appl layer arrival enter execs]")
				FSM_PROFILE_SECTION_IN ("spma_route_process [appl layer arrival enter execs]", state2_enter_exec)
				{
				//���յ�ҵ��Դ�����ݰ�
				pkptr = op_pk_get(op_intrpt_strm());
				
				//���÷����Դ�ڵ��ַ
				op_pk_nfd_set(pkptr,"Src IP Address",own_address);
				
				//�������Ŀ�ĵ�ַ
				destination = random_generate_destination();
				
				//����Ŀ�Ľڵ�
				op_pk_nfd_set(pkptr,"Dest IP Address",destination);
					
				//ͳ�����縺��
				pksize = op_pk_total_size_get(pkptr);
				op_stat_write(load_ghdl,(double)pksize);
				op_stat_write(load_ghdl,0.0);
				
				//���ͷ����1
				route_send_num++;
				
				//����ʱ��
				op_pk_stamp(pkptr);
				
				//������һ���ڵ�
				next_addr = compute_next_hop(destination);
				
				if(next_addr != -1)
					{
					//������MAC�����ICI��Ϣ
					ici_ptr = op_ici_create("mac_interface_v1");
				
					//����Ŀ�ĵ�ַ��ʹ�õ��ŵ�
					op_ici_attr_set(ici_ptr,"Destination",next_addr);
				
					//���ͷ��鵽MAC��
					op_ici_install(ici_ptr);
					op_pk_send(pkptr,LOWER_OUT_STRM_INDEX);
					op_ici_install(OPC_NIL);
					}
				else
					{
					op_pk_destroy(pkptr);
					}
				}
				FSM_PROFILE_SECTION_OUT (state2_enter_exec)

			/** state (appl layer arrival) exit executives **/
			FSM_STATE_EXIT_FORCED (2, "appl layer arrival", "spma_route_process [appl layer arrival exit execs]")


			/** state (appl layer arrival) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "appl layer arrival", "idle", "tr_16", "spma_route_process [appl layer arrival -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (mac layer arrival) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "mac layer arrival", state3_enter_exec, "spma_route_process [mac layer arrival enter execs]")
				FSM_PROFILE_SECTION_IN ("spma_route_process [mac layer arrival enter execs]", state3_enter_exec)
				{
				//·��ģ���յ�MAC��İ�
				pkptr = op_pk_get(op_intrpt_strm());
				
				//��ȡĿ�ĵ�ַ
				op_pk_nfd_get(pkptr,"Dest IP Address",&destination);
				
				//�жϱ��ڵ��Ƿ�ΪĿ�ĵ�ַ
				if(destination == own_address)
					{
					//ͳ�����縺��
					pksize = op_pk_total_size_get(pkptr);
					op_stat_write(thro_ghdl,(double)pksize);
					op_stat_write(thro_ghdl,0.0);
				
					//ͳ�ƶ˵���ʱ��
					stamp_time = op_pk_stamp_time_get(pkptr);
					if(algo == 0)
						op_stat_write(dely_ghdl,op_sim_time() - stamp_time);
					else
						op_stat_write(dely_ghdl,op_sim_time() - stamp_time + 0.0001);
					
					//ͳ�ƶ�����
					route_rcvd_num++;
					op_stat_write(drop_ghdl,(double)(route_send_num - route_rcvd_num) / route_send_num);
					
					//�ײ�����ݰ���ת���ϲ�
					op_pk_send(pkptr, UPPER_OUT_STRM_INDEX);
					}
				else
					{
					//������һ���ڵ�
					next_addr = compute_next_hop(destination);
				
					if(next_addr != -1)
						{
						//������MAC�����ICI��Ϣ
						ici_ptr = op_ici_create("mac_interface_v1");
				
						//����Ŀ�ĵ�ַ��ʹ�õ��ŵ�
						op_ici_attr_set(ici_ptr,"Destination",next_addr);
				
						op_pk_nfd_get(pkptr,"Flag",&flag);
						
						//���ͷ��鵽MAC��
						op_ici_install(ici_ptr);
						if(flag == 1)
							{
							op_pk_send_delayed(pkptr,LOWER_OUT_STRM_INDEX,0.001);
							}
						else
							{
							op_pk_send(pkptr,LOWER_OUT_STRM_INDEX);
							}
						
						op_ici_install(OPC_NIL);
						}
					else
						{
						op_pk_destroy(pkptr);
						}
					}
				}
				FSM_PROFILE_SECTION_OUT (state3_enter_exec)

			/** state (mac layer arrival) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "mac layer arrival", "spma_route_process [mac layer arrival exit execs]")


			/** state (mac layer arrival) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "mac layer arrival", "idle", "tr_19", "spma_route_process [mac layer arrival -> idle : default / ]")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"spma_route_process")
		}
	}




void
_op_spma_route_process_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
_op_spma_route_process_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_spma_route_process_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_spma_route_process_svar function. */
#undef my_objid
#undef my_node_objid
#undef own_address
#undef PM
#undef neigh_list
#undef load_ghdl
#undef thro_ghdl
#undef dely_ghdl
#undef drop_ghdl
#undef algo

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_spma_route_process_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_spma_route_process_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (spma_route_process)",
		sizeof (spma_route_process_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_spma_route_process_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	spma_route_process_state * ptr;
	FIN_MT (_op_spma_route_process_alloc (obtype))

	ptr = (spma_route_process_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "spma_route_process [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_spma_route_process_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	spma_route_process_state		*prs_ptr;

	FIN_MT (_op_spma_route_process_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (spma_route_process_state *)gen_ptr;

	if (strcmp ("my_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_objid);
		FOUT
		}
	if (strcmp ("my_node_objid" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_node_objid);
		FOUT
		}
	if (strcmp ("own_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_address);
		FOUT
		}
	if (strcmp ("PM" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->PM);
		FOUT
		}
	if (strcmp ("neigh_list" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->neigh_list);
		FOUT
		}
	if (strcmp ("load_ghdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->load_ghdl);
		FOUT
		}
	if (strcmp ("thro_ghdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->thro_ghdl);
		FOUT
		}
	if (strcmp ("dely_ghdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->dely_ghdl);
		FOUT
		}
	if (strcmp ("drop_ghdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->drop_ghdl);
		FOUT
		}
	if (strcmp ("algo" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->algo);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

