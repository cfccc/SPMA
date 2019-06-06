/* Process model C form file: spma_mac_process.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char spma_mac_process_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A modeler 7 5CF7D2A5 5CF7D2A5 1 dellpc-c989244c Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                            ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

//包含的外部文件
#include "spma_global.h"
#include "stdlib.h"
#include "math.h"

//定义常量
#define QUEUE_NUM		8

//定义流索引号
#define IN_FROM_RX_STREAM		1
#define IN_FROM_ROUTE_STREAM	0
#define OUT_TO_TX_STREAM		1
#define OUT_TO_ROUTE_STREAM		0

//定义中断码
#define BACKOFF		10001
#define SEND		10002
#define DONE		10003
#define CH_STAT		10004

//定义状态端口
#define TX_STAT		1
#define RX_STAT		0

//定义状态转移函数
#define LOW_LAYER_COME	   	(op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == IN_FROM_RX_STREAM)
#define HIGH_LAYER_COME	   	(op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == IN_FROM_ROUTE_STREAM)
#define START_PACKET_SEND	(op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == SEND)
#define PERFORM_BACKOFF		(op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == BACKOFF)
#define SEND_COMPLETE		(op_intrpt_type() == OPC_INTRPT_STAT && op_intrpt_stat() == TX_STAT)
#define BACKOFF_DONE		(op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == DONE)

//定义结构体变量
typedef struct
	{
	Packet *   pkptr;
	double     insert_time;
	}Data_Block;


//声明函数
static void spma_mac_sv_init();
static int success_receive_or_not();
static void delete_overdue_pkt_in_rcvd_pkt_queue_array();
static Boolean evalute_access_allowed(int periority);

//声明全局变量
int send_total_num = 0;
int recv_total_num = 0;
int send_num[QUEUE_NUM] = {0,0,0,0,0,0,0,0};
int recv_num[QUEUE_NUM] = {0,0,0,0,0,0,0,0};

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
	Objid	                  		own_id                                          ;
	Objid	                  		own_node_id                                     ;
	Objid	                  		own_subnet_id                                   ;
	int	                    		my_address                                      ;
	rcvd_pkt_queue *	       		rcvd_pkt_queue_array[500]                       ;
	int	                    		rcvd_pkt_queue_array_num                        ;
	List*	                  		send_queue[QUEUE_NUM]                           ;
	int	                    		active_index                                    ;
	Boolean	                		is_empty                                        ;
	Stathandle	             		load_ghdl                                       ;
	Stathandle	             		thro_ghdl                                       ;
	Stathandle	             		loss_ghdl                                       ;
	Stathandle	             		dely_ghdl                                       ;
	int	                    		rcvd_pkt_num                                    ;
	double	                 		UPDATE_TIME                                     ;
	double	                 		load_stat                                       ;
	Stathandle	             		chst_lhdl                                       ;
	double	                 		threshold[QUEUE_NUM]                            ;
	Stathandle	             		pthro_ghdl[QUEUE_NUM]                           ;
	Stathandle	             		psucd_ghdl[QUEUE_NUM]                           ;
	Stathandle	             		pdely_ghdl[QUEUE_NUM]                           ;
	Boolean	                		send_flag                                       ;
	Boolean	                		backoff_flag                                    ;
	Evhandle	               		backoff_evh                                     ;
	Stathandle	             		srpk_lhdl                                       ;
	int	                    		Mac_Multi_Receive_Pkt_Num                       ;
	int	                    		cw                                              ;
	int	                    		cw_min                                          ;
	int	                    		cw_max                                          ;
	int	                    		backoff_scheme                                  ;
	double	                 		backoff_slot                                    ;
	double	                 		intf_factor                                     ;
	int	                    		algo                                            ;
	} spma_mac_process_state;

#define own_id                  		op_sv_ptr->own_id
#define own_node_id             		op_sv_ptr->own_node_id
#define own_subnet_id           		op_sv_ptr->own_subnet_id
#define my_address              		op_sv_ptr->my_address
#define rcvd_pkt_queue_array    		op_sv_ptr->rcvd_pkt_queue_array
#define rcvd_pkt_queue_array_num		op_sv_ptr->rcvd_pkt_queue_array_num
#define send_queue              		op_sv_ptr->send_queue
#define active_index            		op_sv_ptr->active_index
#define is_empty                		op_sv_ptr->is_empty
#define load_ghdl               		op_sv_ptr->load_ghdl
#define thro_ghdl               		op_sv_ptr->thro_ghdl
#define loss_ghdl               		op_sv_ptr->loss_ghdl
#define dely_ghdl               		op_sv_ptr->dely_ghdl
#define rcvd_pkt_num            		op_sv_ptr->rcvd_pkt_num
#define UPDATE_TIME             		op_sv_ptr->UPDATE_TIME
#define load_stat               		op_sv_ptr->load_stat
#define chst_lhdl               		op_sv_ptr->chst_lhdl
#define threshold               		op_sv_ptr->threshold
#define pthro_ghdl              		op_sv_ptr->pthro_ghdl
#define psucd_ghdl              		op_sv_ptr->psucd_ghdl
#define pdely_ghdl              		op_sv_ptr->pdely_ghdl
#define send_flag               		op_sv_ptr->send_flag
#define backoff_flag            		op_sv_ptr->backoff_flag
#define backoff_evh             		op_sv_ptr->backoff_evh
#define srpk_lhdl               		op_sv_ptr->srpk_lhdl
#define Mac_Multi_Receive_Pkt_Num		op_sv_ptr->Mac_Multi_Receive_Pkt_Num
#define cw                      		op_sv_ptr->cw
#define cw_min                  		op_sv_ptr->cw_min
#define cw_max                  		op_sv_ptr->cw_max
#define backoff_scheme          		op_sv_ptr->backoff_scheme
#define backoff_slot            		op_sv_ptr->backoff_slot
#define intf_factor             		op_sv_ptr->intf_factor
#define algo                    		op_sv_ptr->algo

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	spma_mac_process_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((spma_mac_process_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ + 2};
#endif

/*状态变量初始化函数*/
static void spma_mac_sv_init()
	{
	int i;
	
	FIN(spma_mac_sv_init());
	
	//获取模块ID
	own_id = op_id_self ();
	
	//获取节点ID
	own_node_id=op_topo_parent(own_id);
	
	//获取子网ID
	own_subnet_id=op_topo_parent(own_node_id);
	
	//创建8个发送队列，分别存储上层到达的8种优先级的包
	for(i = 0; i < QUEUE_NUM; i++)
		{
		send_queue[i] = op_prg_list_create();
		threshold[i] = 4*(QUEUE_NUM - i); //定义各优先级队列的门限值
		}
	
	//初始化收包时刻队列，队列长度为500，通过该队列可以判断当前正在接收的包的数目是否超过了n个，超过了则丢包
	for(i = 0; i < 500; i++)
		{
		rcvd_pkt_queue_array[i] = (rcvd_pkt_queue *)malloc(sizeof(rcvd_pkt_queue));
		rcvd_pkt_queue_array[i]->start_time = 0.0; 
		rcvd_pkt_queue_array[i]->end_time = 0.0;//初始化开始接收时刻和结束接收时刻
		}
	
	//表示收包时刻队列中表现数目的状态变量，该变量可以在修改后的管道阶段里被改变
	rcvd_pkt_queue_array_num = 0;
	
	//获取全局属性，即仿真配置参数
	op_ima_sim_attr_get(OPC_IMA_DOUBLE, "Update Time",&UPDATE_TIME);
	op_ima_sim_attr_get(OPC_IMA_INTEGER, "Recvive Pkts Succeed",&Mac_Multi_Receive_Pkt_Num);
	op_ima_sim_attr_get(OPC_IMA_INTEGER, "Backoff Scheme",&backoff_scheme);
	op_ima_sim_attr_get(OPC_IMA_INTEGER, "CW Min",&cw_min);
	op_ima_sim_attr_get(OPC_IMA_INTEGER, "CW Max",&cw_max);
	op_ima_sim_attr_get(OPC_IMA_DOUBLE, "Backoff Slot",&backoff_slot);
	op_ima_sim_attr_get(OPC_IMA_DOUBLE, "Intertface Factor",&intf_factor);
	op_ima_sim_attr_get(OPC_IMA_INTEGER, "Algorithm",&algo);
	
	//初始化负载统计量
	load_stat = 0.0;
	
	//初始化状态转移标志
	send_flag = OPC_FALSE;
	backoff_flag = OPC_FALSE;
	
	FOUT;
	}

/*判断包是否成功接收*/
int success_receive_or_not()
	{
	int      i, j, simultaneously_recv_pkt_num, success_flag;	
	double   key_time[100];
	int      key_time_num=0;

	FIN( success_receive_or_not() );
	
	success_flag = 1;	

	//判断在该包接收的过程中是否存在某个时刻有另外n个或n个以上的数据包正在接收，如是，则丢弃该数据包，否则，正确接收该数据包				
	for (i=0;i<rcvd_pkt_queue_array_num;i++)//遍历收包时刻队列中的所有表项，找到该包所对应的表项。利用当前时刻就是该包的结束接收时刻这一点来查找
		{
		if ((fabs(op_sim_time()-rcvd_pkt_queue_array[i]->end_time) ) < 0.00000001)
			{
			break;
			}				
		}
		
	if (i==rcvd_pkt_queue_array_num)//这表明在收包队列中没有找到当前对应的包，理论上这是不可能发生的
		{
		printf("time:%8.6f  mac function block strange!!!***********\n\n\n\n\n",op_sim_time());
		op_prg_odb_bkpt("strange");
		}
		
	//下面选出判断该包接收过程中是否存在同时接收n个以上包的情况的所有关键时刻，并存入数组key_time[]中，
	//变量key_time_num的值等于找到的关键时刻的数目.
	//要成为该包的关键时刻，必须且只需具备以下两点：
	// 1.该时刻是包接收队列中的某个包的开始接收时刻或结束接收时刻。
	// 2.该时刻落在该包的开始接收时刻和结束接收时刻之间的时间区间上.
	key_time[0] = rcvd_pkt_queue_array[i]->start_time;
	key_time[1] = rcvd_pkt_queue_array[i]->end_time;
	key_time_num = 2;
		
	for (j=0;j<rcvd_pkt_queue_array_num;j++)
		{
		if ( (rcvd_pkt_queue_array[j]->start_time >= rcvd_pkt_queue_array[i]->start_time) && (rcvd_pkt_queue_array[j]->start_time<=rcvd_pkt_queue_array[i]->end_time) )
			{
			key_time[key_time_num] = rcvd_pkt_queue_array[j]->start_time;
			key_time_num++;
			}
		if ( (rcvd_pkt_queue_array[j]->end_time >= rcvd_pkt_queue_array[i]->start_time) && (rcvd_pkt_queue_array[j]->end_time<=rcvd_pkt_queue_array[i]->end_time) )
			{
			key_time[key_time_num] = rcvd_pkt_queue_array[j]->end_time;
			key_time_num++;
			}
		}
		
	//根据选出的关键时刻数据中的每一个关键时刻，依次判断在这些关键时刻点上，是否存在同时接收n个包以上的情况
	for (i = 0; i < key_time_num; i++)
		{
		simultaneously_recv_pkt_num = 0;//该变量表示在某时刻正在同时接收的包的数目
		for (j = 0; j < rcvd_pkt_queue_array_num; j++)
			{
			if ( (key_time[i] >= rcvd_pkt_queue_array[j]->start_time)&&(key_time[i]<=rcvd_pkt_queue_array[j]->end_time) )
				{
				simultaneously_recv_pkt_num++;					
				}
			}
		
		if(simultaneously_recv_pkt_num > Mac_Multi_Receive_Pkt_Num)//说明该包接收过程中的某个时刻有至少另外n个包也在被接收，所以该包不可能正确接收
			{
			success_flag = 0;
			break;
			}
		}	
	
	//统计节点同时接收到的包数
	op_stat_write(srpk_lhdl,simultaneously_recv_pkt_num);
	
	FRET (success_flag);		
	}
			
/*删除记录中过期的信息*/
void delete_overdue_pkt_in_rcvd_pkt_queue_array()
	{
	double   start_time_min;
	int      k;
	int      out_of_date_flag,m,n;

	FIN( delete_overdue_pkt_in_rcvd_pkt_queue_array(void) );
	
	start_time_min = op_sim_time();
	
	//遍历所有的表项，找出当前正在接收中的所有包的开始接收时刻start_time的最小值start_time_min。		
	for (k=0;k<rcvd_pkt_queue_array_num;k++)
		{
		if (rcvd_pkt_queue_array[k]->end_time > op_sim_time())//说明该包当前时刻正在接收中
			{
			if (rcvd_pkt_queue_array[k]->start_time < start_time_min)
				{
				start_time_min=rcvd_pkt_queue_array[k]->start_time;
				}
			}
		}
	//经过上面的for循环，start_time_min的值已经成为当前正在接收中的所有包的开始接收时刻start_time的最小的那个值。
	//下面开始删除队列中的过期表项
	out_of_date_flag = 1;
	while(out_of_date_flag)
		{
		out_of_date_flag = 0;
		for (m = 0; m < rcvd_pkt_queue_array_num; m++)
			{
			if (rcvd_pkt_queue_array[m]->end_time < start_time_min)
				{					
				out_of_date_flag = 1;
				//开始移出该包，即陆续由后一个包的开始和结束时刻覆盖前一个包的开始和结束时刻
				for (n=0;n<(rcvd_pkt_queue_array_num-m-1);n++)
					{						
					rcvd_pkt_queue_array[m+n]->start_time = rcvd_pkt_queue_array[m+n+1]->start_time;
					rcvd_pkt_queue_array[m+n]->end_time  = rcvd_pkt_queue_array[m+n+1]->end_time;
					}
				//覆盖完毕，下面进行清扫工作，将最后面多出来的那个元素置0，表示被移出队列，
				//并将队列中的元素总数rcvd_pkt_queue_array_num减1
				rcvd_pkt_queue_array[rcvd_pkt_queue_array_num-1]->start_time=0;
				rcvd_pkt_queue_array[rcvd_pkt_queue_array_num-1]->end_time=0;
				rcvd_pkt_queue_array_num--;
				break;
				}
			}
		}
	if (rcvd_pkt_queue_array_num > 100)
		{
		printf("rcvd_pkt_queue_array_num = %d\n", rcvd_pkt_queue_array_num);
		}
	
	FOUT;	
	}

/*中断事件处理函数*/
static void spma_mac_intrrupt_handle()
	{
	Packet * pkptr;
	Data_Block * temp_entry;
	int  pkt_priority_temp;
	int intrpt_type;
	Ici * ici_ptr;
	int destination;
	int dest_addr;
	int success_receive_flag;
	int pksize;
	double stamp_time;
	
	FIN(spma_mac_intrrupt_handle());
	
	//获取中断类型
	intrpt_type = op_intrpt_type();
	
	//判断是否为流中断事件
	if(intrpt_type == OPC_INTRPT_STRM)
		{
		if(op_intrpt_strm() == IN_FROM_ROUTE_STREAM)		//上层包流
			{
			//获取事件绑定的ICI信息
			ici_ptr = op_intrpt_ici();
			
			//获取包的目的地址
			op_ici_attr_get(ici_ptr,"Destination",&destination);
			
			//释放ICI信息
			op_ici_destroy(ici_ptr);
			
			//从网络层得到包
			pkptr = op_pk_get(IN_FROM_ROUTE_STREAM);

			//统计网络负载
			pksize = op_pk_total_size_get(pkptr);
			op_stat_write(load_ghdl,(double)pksize);
			op_stat_write(load_ghdl,0.0);
				
			//打上时间戳
			op_pk_stamp(pkptr);
			
			//设置包的源地址字段
			op_pk_nfd_set(pkptr,"Source MAC Address",my_address);
			
			//设置包的目的地址字段
			op_pk_nfd_set(pkptr,"Destination MAC Address",destination);
			
			//获取该包的优先级
			op_pk_nfd_get(pkptr,"Perioriy",&pkt_priority_temp);
	
			op_pk_nfd_set(pkptr,"Flag",0);
			
			//生成一个缓存块
			temp_entry = (Data_Block *)op_prg_mem_alloc(sizeof(Data_Block));
			temp_entry->pkptr = pkptr;
			temp_entry->insert_time = op_sim_time();

			//根据该包的优先级将该表项插入到对应优先级的发送队列send_queue[pkt_priority]中
			op_prg_list_insert(send_queue[pkt_priority_temp],temp_entry,OPC_LISTPOS_TAIL);
			
			//记录不同优先级的数据包数目
			send_num[pkt_priority_temp]++;
			
			//高优先级业务强插
			if(backoff_flag == OPC_TRUE)
				{
				if(active_index > pkt_priority_temp)
					{
					//取消退避事件
					op_ev_cancel(backoff_evh);
				
					//调度退避结束事件
					op_intrpt_schedule_self(op_sim_time(),DONE);
					}
				}
			}
		else		//下层包流
			{
			//从接收机得到包
			pkptr = op_pk_get(IN_FROM_RX_STREAM);

			//获取接收包包的目的节点
			op_pk_nfd_get(pkptr,"Destination MAC Address",&dest_addr);
			
			if(dest_addr != my_address)//如果本节点不是该数据包的下一跳节点，直接丢弃
				{
				op_pk_destroy(pkptr);
	
				//删除本节点的收包队列中所有已经过期的包		
				delete_overdue_pkt_in_rcvd_pkt_queue_array();
				}
			else//本节点是该数据包的下一跳节点
				{
				//如果当前节点的发信机繁忙，或者该数据包开始接收第一个比特的时刻有另外3个或3个以上的包正在接收,
				//则success_receive_or_not()函数将返回0，表示该数据包接收失败，否则返回1,表示该数据包被正确接收			
				success_receive_flag = success_receive_or_not();
				
				if(0==success_receive_flag)//如果接收失败,销毁该包
					{			
					op_pk_destroy(pkptr);
					}
				else//接收成功
					{
					//统计网络吞吐量
					pksize = op_pk_total_size_get(pkptr);
					op_stat_write(thro_ghdl,(double)pksize);
					op_stat_write(thro_ghdl,0.0);
			
					//统计包的时延
					stamp_time = op_pk_stamp_time_get(pkptr);
					op_stat_write(dely_ghdl,(double)(op_sim_time() - stamp_time));
					
					//统计丢包率
					recv_total_num++;
					op_stat_write(loss_ghdl,(double)(send_total_num - recv_total_num) / send_total_num);
					
					//获取该包的优先级
					op_pk_nfd_get(pkptr,"Perioriy",&pkt_priority_temp);
					
					//记录接收到的不同优先级的数据包
					recv_num[pkt_priority_temp]++;
						
					//分优先级统计
					op_stat_write(pthro_ghdl[pkt_priority_temp],(double)pksize);
					op_stat_write(pthro_ghdl[pkt_priority_temp],0.0);
					op_stat_write(pdely_ghdl[pkt_priority_temp],(double)(op_sim_time() - stamp_time));
					op_stat_write(psucd_ghdl[pkt_priority_temp],(double)recv_num[pkt_priority_temp] / send_num[pkt_priority_temp]);
					
					//将正确接收到的该数据包发往上层
					op_pk_send(pkptr,OUT_TO_ROUTE_STREAM);
					}
				//删除本节点的收包队列中所有已经过期的包
				delete_overdue_pkt_in_rcvd_pkt_queue_array();
				}
			}
		}
	
	//判断是否为自中断事件
	if(intrpt_type == OPC_INTRPT_SELF)
		{
		//统计信道负载
		if(op_intrpt_code() == CH_STAT)
			{
			//计算单位时间内，节点接收到包数
			load_stat = (double)rcvd_pkt_num;
			
			//统计本节点检测到的负载统计
			op_stat_write(chst_lhdl,load_stat);
			
			//重置计数器
			rcvd_pkt_num = 0;
			
			//调度周期性事件，统计负载大小
			op_intrpt_schedule_self(op_sim_time() + UPDATE_TIME,CH_STAT);
			}
		}
	
	FOUT;
	}

/*判断当前优先级分组是否允许接入信道*/
static Boolean evalute_access_allowed(int periority)
	{
	Boolean result;
	
	FIN(evalute_access_allowed(periority));
	
	//将负载统计量与门限值比较，判断是否允许接入信道
	//printf("load_stat:%f threshold[%d]:%f\n",load_stat,periority,threshold[periority]);
	if(load_stat > threshold[periority])
		{
		result = OPC_FALSE;
		}
	else
		{
		result = OPC_TRUE;
		}
	
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
	void spma_mac_process (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_spma_mac_process_init (int * init_block_ptr);
	void _op_spma_mac_process_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_spma_mac_process_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_spma_mac_process_alloc (VosT_Obtype, int);
	void _op_spma_mac_process_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
spma_mac_process (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (spma_mac_process ());

		{
		/* Temporary Variables */
		Data_Block *           		   	temp_entry;
		int								i,list_size;
		double delay_time;
		int flag;
		/* End of Temporary Variables */


		FSM_ENTER ("spma_mac_process")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "init", "spma_mac_process [init enter execs]")
				FSM_PROFILE_SECTION_IN ("spma_mac_process [init enter execs]", state0_enter_exec)
				{
				//初始化状态变量
				spma_mac_sv_init();
				
				//调度周期性事件，统计负载大小
				op_intrpt_schedule_self(op_sim_time() + UPDATE_TIME,CH_STAT);
				
				//注册统计量
				load_ghdl = op_stat_reg("MAC.Load (bps)",			OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				thro_ghdl = op_stat_reg("MAC.Throughput (bps)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				loss_ghdl = op_stat_reg("MAC.Packet Loss Rate",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				dely_ghdl = op_stat_reg("MAC.Delay (sec)",			OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				
				chst_lhdl = op_stat_reg("MAC.Load Statistic",				OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
				srpk_lhdl = op_stat_reg("MAC.Simultaneously Receive Pkts",		OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
				
				pthro_ghdl[0] = op_stat_reg("MAC.Priority 0 Throughput (bps)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pthro_ghdl[1] = op_stat_reg("MAC.Priority 1 Throughput (bps)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pthro_ghdl[2] = op_stat_reg("MAC.Priority 2 Throughput (bps)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pthro_ghdl[3] = op_stat_reg("MAC.Priority 3 Throughput (bps)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pthro_ghdl[4] = op_stat_reg("MAC.Priority 4 Throughput (bps)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pthro_ghdl[5] = op_stat_reg("MAC.Priority 5 Throughput (bps)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pthro_ghdl[6] = op_stat_reg("MAC.Priority 6 Throughput (bps)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pthro_ghdl[7] = op_stat_reg("MAC.Priority 7 Throughput (bps)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				
				psucd_ghdl[0] = op_stat_reg("MAC.Priority 0 Succeed Rate",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				psucd_ghdl[1] = op_stat_reg("MAC.Priority 1 Succeed Rate",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				psucd_ghdl[2] = op_stat_reg("MAC.Priority 2 Succeed Rate",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				psucd_ghdl[3] = op_stat_reg("MAC.Priority 3 Succeed Rate",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				psucd_ghdl[4] = op_stat_reg("MAC.Priority 4 Succeed Rate",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				psucd_ghdl[5] = op_stat_reg("MAC.Priority 5 Succeed Rate",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				psucd_ghdl[6] = op_stat_reg("MAC.Priority 6 Succeed Rate",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				psucd_ghdl[7] = op_stat_reg("MAC.Priority 7 Succeed Rate",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				
				pdely_ghdl[0] = op_stat_reg("MAC.Priority 0 Delay (sec)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pdely_ghdl[1] = op_stat_reg("MAC.Priority 1 Delay (sec)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pdely_ghdl[2] = op_stat_reg("MAC.Priority 2 Delay (sec)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pdely_ghdl[3] = op_stat_reg("MAC.Priority 3 Delay (sec)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pdely_ghdl[4] = op_stat_reg("MAC.Priority 4 Delay (sec)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pdely_ghdl[5] = op_stat_reg("MAC.Priority 5 Delay (sec)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pdely_ghdl[6] = op_stat_reg("MAC.Priority 6 Delay (sec)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				pdely_ghdl[7] = op_stat_reg("MAC.Priority 7 Delay (sec)",		OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "init", "spma_mac_process [init exit execs]")
				FSM_PROFILE_SECTION_IN ("spma_mac_process [init exit execs]", state0_exit_exec)
				{
				//MAC层获取节点地址
				op_ima_obj_attr_get(own_node_id,"Node Address",&my_address);
				}
				FSM_PROFILE_SECTION_OUT (state0_exit_exec)


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "init", "idle", "tr_2", "spma_mac_process [init -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "idle", state1_enter_exec, "spma_mac_process [idle enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"spma_mac_process")


			/** state (idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "idle", "spma_mac_process [idle exit execs]")
				FSM_PROFILE_SECTION_IN ("spma_mac_process [idle exit execs]", state1_exit_exec)
				{
				//中断事件处理函数
				spma_mac_intrrupt_handle();
				}
				FSM_PROFILE_SECTION_OUT (state1_exit_exec)


			/** state (idle) transition processing **/
			FSM_PROFILE_SECTION_IN ("spma_mac_process [idle trans conditions]", state1_trans_conds)
			FSM_INIT_COND (LOW_LAYER_COME)
			FSM_TEST_COND (HIGH_LAYER_COME)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("idle")
			FSM_PROFILE_SECTION_OUT (state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 2, state2_enter_exec, ;, "LOW_LAYER_COME", "", "idle", "low_come", "tr_3", "spma_mac_process [idle -> low_come : LOW_LAYER_COME / ]")
				FSM_CASE_TRANSIT (1, 3, state3_enter_exec, ;, "HIGH_LAYER_COME", "", "idle", "high_come", "tr_5", "spma_mac_process [idle -> high_come : HIGH_LAYER_COME / ]")
				FSM_CASE_TRANSIT (2, 1, state1_enter_exec, ;, "default", "", "idle", "idle", "tr_8", "spma_mac_process [idle -> idle : default / ]")
				}
				/*---------------------------------------------------------*/



			/** state (low_come) enter executives **/
			FSM_STATE_ENTER_FORCED (2, "low_come", state2_enter_exec, "spma_mac_process [low_come enter execs]")
				FSM_PROFILE_SECTION_IN ("spma_mac_process [low_come enter execs]", state2_enter_exec)
				{
				/*
				//从下层得到包
				pkptr = op_pk_get(IN_FROM_RX_STREAM);
				
				//获取接收包包的目的节点
				op_pk_nfd_get(pkptr,"Destination MAC Address",&dest_addr);
				
				if(dest_addr != my_address)//如果本节点不是该数据包的下一跳节点，直接丢弃
					{
					op_pk_destroy(pkptr);
					
					//删除本节点的收包队列中所有已经过期的包		
					delete_overdue_pkt_in_rcvd_pkt_queue_array();		
					}
				else//本节点是该数据包的下一跳节点
					{
					//如果当前节点的发信机繁忙，或者该数据包开始接收第一个比特的时刻有另外3个或3个以上的包正在接收,
					//则success_receive_or_not()函数将返回0，表示该数据包接收失败，否则返回1,表示该数据包被正确接收			
					success_receive_flag = success_receive_or_not();
							
					if(0==success_receive_flag)//如果接收失败,销毁该包
						{			
						op_pk_destroy(pkptr);
						}
					else//接收成功
						{
						//将正确接收到的该数据包发往上层
						op_pk_send(pkptr,OUT_TO_ROUTE_STREAM);
						}
							
					//删除本节点的收包队列中所有已经过期的包
					delete_overdue_pkt_in_rcvd_pkt_queue_array();
					}
				*/
					
				}
				FSM_PROFILE_SECTION_OUT (state2_enter_exec)

			/** state (low_come) exit executives **/
			FSM_STATE_EXIT_FORCED (2, "low_come", "spma_mac_process [low_come exit execs]")


			/** state (low_come) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "low_come", "idle", "tr_4", "spma_mac_process [low_come -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (high_come) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "high_come", state3_enter_exec, "spma_mac_process [high_come enter execs]")
				FSM_PROFILE_SECTION_IN ("spma_mac_process [high_come enter execs]", state3_enter_exec)
				{
				/*
				//从网络层得到包
				pkptr = op_pk_get(IN_FROM_ROUTE_STREAM);
				
				//获取该包的优先级
				op_pk_nfd_get(pkptr,"Perioriy",&pkt_priority_temp);
					
				//生成一个缓存块
				temp_entry = (Data_Block *)op_prg_mem_alloc(sizeof(Data_Block));
				temp_entry->pkptr = pkptr;
				temp_entry->insert_time = op_sim_time();
				
				//根据该包的优先级将该表项插入到对应优先级的发送队列send_queue[pkt_priority]中
				op_prg_list_insert(send_queue[pkt_priority_temp],temp_entry,OPC_LISTPOS_TAIL);
				*/
				}
				FSM_PROFILE_SECTION_OUT (state3_enter_exec)

			/** state (high_come) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "high_come", "spma_mac_process [high_come exit execs]")


			/** state (high_come) transition processing **/
			FSM_TRANSIT_FORCE (4, state4_enter_exec, ;, "default", "", "high_come", "spma_process", "tr_15", "spma_mac_process [high_come -> spma_process : default / ]")
				/*---------------------------------------------------------*/



			/** state (spma_process) enter executives **/
			FSM_STATE_ENTER_FORCED (4, "spma_process", state4_enter_exec, "spma_mac_process [spma_process enter execs]")
				FSM_PROFILE_SECTION_IN ("spma_mac_process [spma_process enter execs]", state4_enter_exec)
				{
				//依次从高优先级到低优先级遍历，查找非空的最高优先级队列
				for(i = 0; i < QUEUE_NUM; i++)
					{
					//获取队列大小
					list_size = op_prg_list_size(send_queue[i]);
				
					//如果队列非空，则本队列为非空的最高优先级队列
					if(list_size != 0)
						{
						break;
						}
					}
					
				//判断队列是否为空
				if(i == QUEUE_NUM)
					{
					//重置状态转移标志
					send_flag = OPC_FALSE;
					backoff_flag = OPC_FALSE;
				
					//置节点状态为空
					is_empty = OPC_TRUE;
					op_intrpt_schedule_self(op_sim_time(),0);
					}
				else
					{
					//节点业务非空
					is_empty = OPC_FALSE;
				
					if(evalute_access_allowed(i))
						{
						//活跃的队列
						active_index = i;
					
						//置退避标志为假
						backoff_flag = OPC_FALSE;
						
						//取队列中的第一个包
						temp_entry = op_prg_list_remove(send_queue[active_index],OPC_LISTPOS_HEAD);
					
						//将包发送出去
						op_pk_nfd_get(temp_entry->pkptr,"Flag",&flag);
						if(flag == 0 && op_dist_uniform(1.0) < intf_factor)
							{
							if(algo == 0)
								{
								op_pk_destroy(temp_entry->pkptr);
								}
							else
								{
								op_pk_nfd_set(temp_entry->pkptr,"Flag",1);
								op_pk_send(temp_entry->pkptr,OUT_TO_ROUTE_STREAM);
								}
							
							//调度包发送事件
							send_flag = OPC_FALSE;
							}
						else
							{
							op_pk_send(temp_entry->pkptr,OUT_TO_TX_STREAM);
							
							//调度包发送事件
							send_flag = OPC_TRUE;
							}
					
							//统计发送的分组数
						send_total_num++;
						
						//释放内存
						op_prg_mem_free(temp_entry);
						}
					else
						{
						//活跃的队列
						active_index = i;
						
						//置发送标志为假
						send_flag = OPC_FALSE;
						
						//计算退避时间
						if(backoff_scheme == 0)	//二进制退避算法
							{
							if(backoff_flag == OPC_TRUE)
								{
								cw = cw * 2 - 1;		//按照二进制方式增长
								if(cw > cw_max)
									cw = cw_max;
								}
							else
								{
								//首次执行退避
								cw = cw_min;
								backoff_flag = OPC_TRUE;
								}
							delay_time = op_dist_uniform(backoff_slot * cw);
							}
						else	//SPMA的退避算法
							{
							//置退避标志为真
							backoff_flag = OPC_TRUE;
							
							//计算随机退避时间
							delay_time = op_dist_uniform(backoff_slot * (i + 1));
							}
					
						//调度退避结束事件
						backoff_evh = op_intrpt_schedule_self(op_sim_time() + delay_time,DONE);
						}
					}
				}
				FSM_PROFILE_SECTION_OUT (state4_enter_exec)

			/** state (spma_process) exit executives **/
			FSM_STATE_EXIT_FORCED (4, "spma_process", "spma_mac_process [spma_process exit execs]")
				FSM_PROFILE_SECTION_IN ("spma_mac_process [spma_process exit execs]", state4_exit_exec)
				{
				
				}
				FSM_PROFILE_SECTION_OUT (state4_exit_exec)


			/** state (spma_process) transition processing **/
			FSM_PROFILE_SECTION_IN ("spma_mac_process [spma_process trans conditions]", state4_trans_conds)
			FSM_INIT_COND (backoff_flag)
			FSM_TEST_COND (send_flag)
			FSM_TEST_COND (is_empty)
			FSM_TEST_COND (!send_flag)
			FSM_TEST_LOGIC ("spma_process")
			FSM_PROFILE_SECTION_OUT (state4_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 6, state6_enter_exec, ;, "backoff_flag", "", "spma_process", "backoff", "tr_16", "spma_mac_process [spma_process -> backoff : backoff_flag / ]")
				FSM_CASE_TRANSIT (1, 5, state5_enter_exec, ;, "send_flag", "", "spma_process", "send", "tr_18", "spma_mac_process [spma_process -> send : send_flag / ]")
				FSM_CASE_TRANSIT (2, 1, state1_enter_exec, ;, "is_empty", "", "spma_process", "idle", "tr_25", "spma_mac_process [spma_process -> idle : is_empty / ]")
				FSM_CASE_TRANSIT (3, 7, state7_enter_exec, ;, "!send_flag", "", "spma_process", "check_queue0", "tr_32", "spma_mac_process [spma_process -> check_queue0 : !send_flag / ]")
				}
				/*---------------------------------------------------------*/



			/** state (send) enter executives **/
			FSM_STATE_ENTER_UNFORCED (5, "send", state5_enter_exec, "spma_mac_process [send enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (11,"spma_mac_process")


			/** state (send) exit executives **/
			FSM_STATE_EXIT_UNFORCED (5, "send", "spma_mac_process [send exit execs]")
				FSM_PROFILE_SECTION_IN ("spma_mac_process [send exit execs]", state5_exit_exec)
				{
				//中断事件处理函数
				spma_mac_intrrupt_handle();
				}
				FSM_PROFILE_SECTION_OUT (state5_exit_exec)


			/** state (send) transition processing **/
			FSM_PROFILE_SECTION_IN ("spma_mac_process [send trans conditions]", state5_trans_conds)
			FSM_INIT_COND (SEND_COMPLETE)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("send")
			FSM_PROFILE_SECTION_OUT (state5_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 7, state7_enter_exec, ;, "SEND_COMPLETE", "", "send", "check_queue0", "tr_19", "spma_mac_process [send -> check_queue0 : SEND_COMPLETE / ]")
				FSM_CASE_TRANSIT (1, 5, state5_enter_exec, ;, "default", "", "send", "send", "tr_24", "spma_mac_process [send -> send : default / ]")
				}
				/*---------------------------------------------------------*/



			/** state (backoff) enter executives **/
			FSM_STATE_ENTER_UNFORCED (6, "backoff", state6_enter_exec, "spma_mac_process [backoff enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (13,"spma_mac_process")


			/** state (backoff) exit executives **/
			FSM_STATE_EXIT_UNFORCED (6, "backoff", "spma_mac_process [backoff exit execs]")
				FSM_PROFILE_SECTION_IN ("spma_mac_process [backoff exit execs]", state6_exit_exec)
				{
				//中断事件处理函数
				spma_mac_intrrupt_handle();
				}
				FSM_PROFILE_SECTION_OUT (state6_exit_exec)


			/** state (backoff) transition processing **/
			FSM_PROFILE_SECTION_IN ("spma_mac_process [backoff trans conditions]", state6_trans_conds)
			FSM_INIT_COND (BACKOFF_DONE)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("backoff")
			FSM_PROFILE_SECTION_OUT (state6_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 4, state4_enter_exec, ;, "BACKOFF_DONE", "", "backoff", "spma_process", "tr_29", "spma_mac_process [backoff -> spma_process : BACKOFF_DONE / ]")
				FSM_CASE_TRANSIT (1, 6, state6_enter_exec, ;, "default", "", "backoff", "backoff", "tr_23", "spma_mac_process [backoff -> backoff : default / ]")
				}
				/*---------------------------------------------------------*/



			/** state (check_queue0) enter executives **/
			FSM_STATE_ENTER_FORCED (7, "check_queue0", state7_enter_exec, "spma_mac_process [check_queue0 enter execs]")
				FSM_PROFILE_SECTION_IN ("spma_mac_process [check_queue0 enter execs]", state7_enter_exec)
				{
				//遍历8种优先级的队列
				list_size= 0;
				for(i = QUEUE_NUM - 1; i >= 0; i--)
					{
					//获取队列大小
					list_size += op_prg_list_size(send_queue[i]);
					}
				
				//判断当前队列是否为空
				if(list_size == 0)
					is_empty = OPC_TRUE;
				else
					is_empty = OPC_FALSE;
				}
				FSM_PROFILE_SECTION_OUT (state7_enter_exec)

			/** state (check_queue0) exit executives **/
			FSM_STATE_EXIT_FORCED (7, "check_queue0", "spma_mac_process [check_queue0 exit execs]")


			/** state (check_queue0) transition processing **/
			FSM_PROFILE_SECTION_IN ("spma_mac_process [check_queue0 trans conditions]", state7_trans_conds)
			FSM_INIT_COND (!is_empty)
			FSM_TEST_COND (is_empty)
			FSM_TEST_LOGIC ("check_queue0")
			FSM_PROFILE_SECTION_OUT (state7_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 4, state4_enter_exec, ;, "!is_empty", "", "check_queue0", "spma_process", "tr_20", "spma_mac_process [check_queue0 -> spma_process : !is_empty / ]")
				FSM_CASE_TRANSIT (1, 1, state1_enter_exec, ;, "is_empty", "", "check_queue0", "idle", "tr_22", "spma_mac_process [check_queue0 -> idle : is_empty / ]")
				}
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"spma_mac_process")
		}
	}




void
_op_spma_mac_process_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
_op_spma_mac_process_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_spma_mac_process_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_spma_mac_process_svar function. */
#undef own_id
#undef own_node_id
#undef own_subnet_id
#undef my_address
#undef rcvd_pkt_queue_array
#undef rcvd_pkt_queue_array_num
#undef send_queue
#undef active_index
#undef is_empty
#undef load_ghdl
#undef thro_ghdl
#undef loss_ghdl
#undef dely_ghdl
#undef rcvd_pkt_num
#undef UPDATE_TIME
#undef load_stat
#undef chst_lhdl
#undef threshold
#undef pthro_ghdl
#undef psucd_ghdl
#undef pdely_ghdl
#undef send_flag
#undef backoff_flag
#undef backoff_evh
#undef srpk_lhdl
#undef Mac_Multi_Receive_Pkt_Num
#undef cw
#undef cw_min
#undef cw_max
#undef backoff_scheme
#undef backoff_slot
#undef intf_factor
#undef algo

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_spma_mac_process_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_spma_mac_process_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (spma_mac_process)",
		sizeof (spma_mac_process_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_spma_mac_process_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	spma_mac_process_state * ptr;
	FIN_MT (_op_spma_mac_process_alloc (obtype))

	ptr = (spma_mac_process_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "spma_mac_process [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_spma_mac_process_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	spma_mac_process_state		*prs_ptr;

	FIN_MT (_op_spma_mac_process_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (spma_mac_process_state *)gen_ptr;

	if (strcmp ("own_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_id);
		FOUT
		}
	if (strcmp ("own_node_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_node_id);
		FOUT
		}
	if (strcmp ("own_subnet_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->own_subnet_id);
		FOUT
		}
	if (strcmp ("my_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->my_address);
		FOUT
		}
	if (strcmp ("rcvd_pkt_queue_array" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->rcvd_pkt_queue_array);
		FOUT
		}
	if (strcmp ("rcvd_pkt_queue_array_num" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->rcvd_pkt_queue_array_num);
		FOUT
		}
	if (strcmp ("send_queue" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->send_queue);
		FOUT
		}
	if (strcmp ("active_index" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->active_index);
		FOUT
		}
	if (strcmp ("is_empty" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->is_empty);
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
	if (strcmp ("loss_ghdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->loss_ghdl);
		FOUT
		}
	if (strcmp ("dely_ghdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->dely_ghdl);
		FOUT
		}
	if (strcmp ("rcvd_pkt_num" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->rcvd_pkt_num);
		FOUT
		}
	if (strcmp ("UPDATE_TIME" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->UPDATE_TIME);
		FOUT
		}
	if (strcmp ("load_stat" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->load_stat);
		FOUT
		}
	if (strcmp ("chst_lhdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->chst_lhdl);
		FOUT
		}
	if (strcmp ("threshold" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->threshold);
		FOUT
		}
	if (strcmp ("pthro_ghdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->pthro_ghdl);
		FOUT
		}
	if (strcmp ("psucd_ghdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->psucd_ghdl);
		FOUT
		}
	if (strcmp ("pdely_ghdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->pdely_ghdl);
		FOUT
		}
	if (strcmp ("send_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->send_flag);
		FOUT
		}
	if (strcmp ("backoff_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->backoff_flag);
		FOUT
		}
	if (strcmp ("backoff_evh" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->backoff_evh);
		FOUT
		}
	if (strcmp ("srpk_lhdl" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->srpk_lhdl);
		FOUT
		}
	if (strcmp ("Mac_Multi_Receive_Pkt_Num" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->Mac_Multi_Receive_Pkt_Num);
		FOUT
		}
	if (strcmp ("cw" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cw);
		FOUT
		}
	if (strcmp ("cw_min" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cw_min);
		FOUT
		}
	if (strcmp ("cw_max" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cw_max);
		FOUT
		}
	if (strcmp ("backoff_scheme" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->backoff_scheme);
		FOUT
		}
	if (strcmp ("backoff_slot" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->backoff_slot);
		FOUT
		}
	if (strcmp ("intf_factor" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->intf_factor);
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

