
/******************************************************
Copyright (c) 2007
Rancore Technologies (P) Ltd.
All Rights Reserved
*******************************************************/

/*************************************************************************************************************************************
* FILE NAME 	:    	RtShUserDataIntf.cpp
*
* DESCRIPTION : 		Member Functions for RtShUserDataIntf Class are defined here.
*
* DOCUMENTS 	:			A reference to the applicable design documents and coding guidelines document used.
*
* AUTHOR 			: 		
*
* DATE 				: 		
*
***************************************************************************************************************************************/

#ifdef __RT_DIA_ENABLE_SH__

#include "RtShUserDataIntfInclude.hpp"

RtShUserDataIntf* RtShUserDataIntf ::  mp_self_ref = NULL;

//UDI_PHASE2
RtRcT rtPrepOrigOrdList(RtShUDIUserCntxt* ap_cntxt,RtShUDIOrderedArr&		ar_ordered_arr, RtShUDISessCase  a_sess_case, RtSipMethodType a_sip_method)
{
	ar_ordered_arr.num_ordered_elem	=	(ap_cntxt->case_ord_list[a_sess_case][a_sip_method]).size();

	list<RtShUDIOrderedListElem>::iterator  l_itr;
	RtU32T	l_count = 0;
	for(l_itr = (ap_cntxt->case_ord_list[a_sess_case][a_sip_method]).begin(); l_itr !=	(ap_cntxt->case_ord_list[a_sess_case][a_sip_method]).end(); l_itr++)
	{
		ar_ordered_arr.ordered_arr_elem[l_count].udi_ref		= l_itr->udi_ref;
		ar_ordered_arr.ordered_arr_elem[l_count].app_enum		= l_itr->data_ref_appl_info.app_enum;
		l_count++;
	}
	
	return RT_SUCCESS;
}

RtRcT rtPrepRegenOrdList(RtShUDIUserCntxt* ap_cntxt,RtShUDIOrderedArr&		ar_ordered_arr, RtShUDISessCase  a_sess_case, RtSipMethodType a_sip_method)
{
	if(ap_cntxt->regen_ord_list)
	{
		(RtShUDIDataPoolMgr :: rtGetInstance())->rtCreateCaseOrderedListForUser(ap_cntxt);
		
		ap_cntxt->regen_ord_list	= false;
	}
	
	ar_ordered_arr.num_ordered_elem	=	(ap_cntxt->case_ord_list[a_sess_case][a_sip_method]).size();

	list<RtShUDIOrderedListElem>::iterator  l_itr;
	RtU32T	l_count = 0;
	for(l_itr = (ap_cntxt->case_ord_list[a_sess_case][a_sip_method]).begin(); l_itr !=	(ap_cntxt->case_ord_list[a_sess_case][a_sip_method]).end(); l_itr++)
	{
		ar_ordered_arr.ordered_arr_elem[l_count].udi_ref		= l_itr->udi_ref;
		ar_ordered_arr.ordered_arr_elem[l_count].app_enum		= l_itr->data_ref_appl_info.app_enum;
		l_count++;
	}
	
	return RT_SUCCESS;
}


RtRcT rtGetCntxtForUserIden(RtS8T* ap_user_id,RtU32T& ar_cntxt_indx,RtU32T& ar_num_wrkr_thrd)//C function to called by sh callback in PNR
{
	(RtShUserDataIntf::rtGetInstance())->rtGetNumWrkr(ar_num_wrkr_thrd);
	return ((RtShUserDataIntf::rtGetInstance())->rtFindCntxtForUserIden(ap_user_id,ar_cntxt_indx));

}
/*
	following function is callback to be registered with RtCacheKeepr class to be called when cache timeout happens
*/
void RtCacheKeeperCallBackImpl(const RtCacheKeeperBuffer* ap_cache_buffer,const RtS64T* ap_cache_id,const RtTransAddr* ap_call_back_trans_addr)
{
	RtTransMsg l_trans_msg;
	memset(&l_trans_msg,0,sizeof(RtTransMsg));
	
	l_trans_msg.msg_type  										= RT_M_SH_UDI_APPL_MSG;
	l_trans_msg.msg_hdr.src_addr 							= *ap_call_back_trans_addr;
	l_trans_msg.msg_hdr.dest_addr 						= *ap_call_back_trans_addr;
	
	RtShUDIProcReq* lp_proc_req 							= (RtShUDIProcReq*)l_trans_msg.msg_buffer;
	
	lp_proc_req->opcode 											= RT_O_TIMEOUT_CACHE;
	lp_proc_req->req_body.cache_msg.index 		= ap_cache_buffer->index;
	lp_proc_req->req_body.cache_msg.cache_id	= *ap_cache_id;
	
	//printf("\nRtCacheKeeperCallBackImpl for user_indx=%u,cache_id = %lld EE_id[%d]",ap_cache_buffer->index, *ap_cache_id, l_trans_msg.msg_hdr.dest_addr.ee_id);//KLOCK_FIX_18062012 ap_cache_id added *
	
	//SHASHI_06072012 Load test don't discard flag set to free context and pools
	if(RT_SUCCESS !=	RtMglIntf::rtGetInstance()->rtSendSockMsg(sizeof(RtTransMsg), &l_trans_msg, true))
	{
		RtSysAgent::rtGetInstance()->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiSendToWrkrFailed);
		printf("\nRtCacheKeeperCallBackImpl::ERROR in rtSendSockMsg for user_indx=%u,cache_id = %lld EE_id[%d]",ap_cache_buffer->index,*ap_cache_id, l_trans_msg.msg_hdr.dest_addr.ee_id);
	}
}


/*******************************************************************************
 *
 * FUNCTION NAME : RtShUserDataIntf().
 *
 * DESCRIPTION   : This is the constructor of the class. .rtInitialize() function is called inside
 * 								the constructor to initialize all the data that is needed during system startup
 *
 * INPUT         : RtShUDIInitializeData*
 *
 * OUTPUT        : a_max_data_ref_id , a_max_data_repository
 *
 * RETURN        : none. 
 *
 ******************************************************************************/
RtShUserDataIntf :: RtShUserDataIntf()
{

//SHASHI_24032012 commented to remove cyclic dependency
// 	if(RT_SUCCESS != rtInitialize(a_max_data_ref_id, a_max_data_repository) )
//   {
//     printf("\nERROR::RtShUserDataIntf::RtShUserDataIntf()-rtInitilaize returned FAILURE (EXCEPTION THROW)" );
// 		fflush(NULL);
//     
//     throw RT_SH_UDI_EXCEP_INITIALIZE_FAILED;
//   } 
     mp_user_cntxt_mgr = NULL;

 
}

/*******************************************************************************
 *
 * FUNCTION NAME : ~RtShUserDataIntf().
 *
 * DESCRIPTION   : This is the destructor of the class.
 *
 * INPUT         : none. 
 *
 * OUTPUT        : none 
 *
 * RETURN        : none. 
 *
 ******************************************************************************/ 
RtShUserDataIntf :: ~RtShUserDataIntf()
{
	//nothing
}
 
/*******************************************************************************
 *
 * FUNCTION NAME : rtCreate
 *
 * DESCRIPTION   : This function creates the singleton instance of this class
 *
 * INPUT         : a_max_data_ref_id , a_max_data_repository
 *
 * OUTPUT        : none 
 *
 * RETURN        : static *RtShUserDataIntf. 
 *
 ******************************************************************************/
RtShUserDataIntf* RtShUserDataIntf :: rtCreate()
{
	printf("\nRtShUserDataIntf::rtCreate() ENTER ");                                                                      

	try
  {
    if(mp_self_ref == NULL)
    {
      return(mp_self_ref = new RtShUserDataIntf());

    }
    else
    {
      return NULL;	
    }
  }
  catch(...)
  {
    printf("\nERROR::RtShUserDataIntf::rtCreate() - RtShUserDataIntf() could not be created due to Memory problem or not able to read from files %s,%d\n",__FILE__,__LINE__);                                                                      
    fflush(NULL);
		return NULL;
  }
    
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtGetInstance()
 *
 * DESCRIPTION   : This function returns the  singleton instance of this class
 *
 * INPUT         : none. 
 *
 * OUTPUT        : none 
 *
 * RETURN        : static *RtShUserDataIntf. 
 *
 ******************************************************************************/
RtShUserDataIntf* RtShUserDataIntf :: rtGetInstance()
{
	return mp_self_ref;
}
/*******************************************************************************
 *
 * FUNCTION NAME : rtInit()
 *
 * DESCRIPTION   : This function returns the  singleton instance of this class
 *
 * INPUT         : none. 
 *
 * OUTPUT        : none 
 *
 * RETURN        : static *RtShUserDataIntf. 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf :: rtInit()
{
		printf("\nRtShUserDataIntf :: rtInit() ENTER ..");
	  fflush(NULL);
		mp_sys_agent	= RtSysAgent::rtGetInstance();
		if(NULL ==	mp_sys_agent)
		{
			printf("RtShUserDataIntf :: rtInit()  RtSysAgent::rtGetInstance() returned NULL");
			fflush(NULL);

			return RT_FAILURE;
		}
		if(mp_sys_agent->rtGetRtBoolT(RT_PRV_UDI_INTF_ENABLED))
		{
			mp_prov_rest_mgr = RtProvRestMgr::rtCreate();
			if(NULL	==	mp_prov_rest_mgr)
			{
				printf("\n ERROR RtShUserDataIntf :: rtInit()  RtProvRestMgr::rtCreate() returned NULL");
				fflush(NULL);

				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"ERROR RtShUserDataIntf()::rtInit()  RtShProvIntf::rtCreate() returned NULL");			

				return 	RT_FAILURE;	
			}

			if( mp_prov_rest_mgr->rtInitialize() != RT_SUCCESS)
			{
				printf("\n ERROR RtShUserDataIntf :: rtInit()  initialization of mp_prov_rest_mgr failed");
				fflush(NULL);

				return RT_FAILURE;
			}
		}
		else
		{
			printf("\nRtShUserDataIntf :: rtInit()  mp_sys_agent->rtGetRtBoolT(RT_PRV_UDI_INTF_ENABLED) returned %d - no need to create Prov Interface ",mp_sys_agent->rtGetRtBoolT(RT_PRV_UDI_INTF_ENABLED));
			fflush(NULL);

			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"RtShUserDataIntf :: rtInit() ALERT mp_sys_agent->rtGetRtBoolT(RT_PRV_UDI_INTF_ENABLED) returned %d - no need to create Prov Interface ",mp_sys_agent->rtGetRtBoolT(RT_PRV_UDI_INTF_ENABLED));			
		}
		printf("\nRtShUserDataIntf :: rtInit() LEAVE success ..");
	  fflush(NULL);
	
		return RT_SUCCESS;
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtInitialze()
 *
 * DESCRIPTION   : HP_CR(DONE): This function is used to Create and Initialize RtShUDIUserCntxtMgr 
 *								 RtShUDIDataPoolMgr, RtShUDIWrkr, RtCacheKeeper etc							 
 * INPUT         : a_max_data_ref_id , a_max_data_repository ,	a_proc_cur_state
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT. 
 *
 ******************************************************************************/
//RtRcT RtShUserDataIntf :: rtInitialize(RtU8T 		a_max_data_ref_id,RtU8T 	a_max_data_repository,RtShUDIAppRspCallBackT a_rsp_call_bk_fun,RtS8T* a_comp_name)
//Tag_amit_18032013 - Application provisioning callback function also registered here 
RtRcT RtShUserDataIntf :: rtInitialize(RtU8T 		a_max_data_ref_id,RtU8T 	a_max_data_repository,RtShUDIAppRspCallBackT a_rsp_call_bk_fun,RtShUDIAppProvRspCallBackT a_prov_rsp_callbk_fun,RtShUDISubsRspCallBackT a_subs_rsp_callback, RtS8T* a_comp_name)
{
	//HP_CR(DONE):: give logs (DEBUG + CRITICAL + others) in this functions
	printf("\nRtShUserDataIntf :: rtInitialize ENTER with a_max_data_ref_id =%d,a_max_data_repository=%d,comp_name=%s",
				a_max_data_ref_id,a_max_data_repository,a_comp_name);
	fflush(NULL);
	
	RtRcT 				l_ret_val 		= RT_FAILURE;
	RtEEId  			l_ee_id 			= RT_EE_SH_UDI_WRKR_BASE;				
	pthread_t 		l_thread_id;
	RtEEInfo  		l_ee_info;
	m_cur_max_usr_cntxt_size =0; //initiliaze
	m_max_data_ref				= a_max_data_ref_id;
	m_max_data_repository = a_max_data_repository;

	mp_app_rsp_callback				=	a_rsp_call_bk_fun;
	mp_subs_rsp_callback			=	a_subs_rsp_callback;
	
	//Tag_amit_18032013 - Application provisioning callback function also registered here 	
	mp_app_prov_rsp_callback	=	a_prov_rsp_callbk_fun;
		
	//Tag_hotfix_genesysPF_152
	m_udi_init_complete = false;
	
	m_nxt_free_srvc_token = m_max_data_ref; //like 30
	
	m_data_ref_conf_bitmap.reset();
	
	m_is_registration_ceased = false;
	
	/* Initialize lock for user_id vs context_index map*/
	for(RtU32T	l_cnt = 0; l_cnt	< RT_SH_UDI_MAX_NUM_USER_IDEN_BLOCKS;l_cnt++)
	{
		pthread_rwlock_init(&m_user_iden_vs_cntxt_lock_arr[l_cnt],NULL);
	}

	/* Initialize lock for service_indication vs service_token map*/
	
	pthread_mutex_init(&m_srvc_ind_vs_srvc_token_lock, NULL);
	
	
	//now creating UDI Entity classes
	//HP_CR(DONE): nowhere in code SYS patameter - RT_SH_UDI_MAX_DATA_REF shal be used
  //HP_CR(DONE): create two API's to get m_max_data_ref and m_max_data_repository values
	

	RtU32T  l_num_user_cntxt			=  mp_sys_agent->rtGetRtU32T(RT_SH_UDI_MAX_NUM_USR_CNTXT);


	/* 
			creation of  Cachekeeper
	*/	
	
	//HP_CR(DONE): create and initialize mp_cache_keeper;
	RtU32T		l_num_cache_ids 						=  l_num_user_cntxt + RT_SH_UDI_NUM_EXTRA_CACHE_ID;
	RtU32T    l_num_cache_keeper_threads  =  mp_sys_agent->rtGetRtU32T(RT_SH_UDI_NUM_CACHE_KEEPER_THRDS);
	RtBoolT   l_cache_ckpt_reqd   				=  mp_sys_agent->rtGetRtBoolT(RT_SH_UDI_CACHE_KEEPER_CKPT_RQD);
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"RtShUserDataIntf()::rtInitialize()  creating RtCacheKeeper with num_cache_id=%u,num_cache_keeper_thrds=%u,chkpt_reqd=%d",
	l_num_cache_ids, l_num_cache_keeper_threads, l_cache_ckpt_reqd);		
	
	mp_cache_keeper = RtCacheKeeper::rtCreate(l_num_cache_keeper_threads,l_num_cache_ids/l_num_cache_keeper_threads,l_cache_ckpt_reqd);
	if(NULL ==	mp_cache_keeper)
	{
		printf("RtShUserDataIntf :: rtInitialize()  RtCacheKeeper::rtCreate() returned NULL");
		fflush(NULL);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf()::rtInitialize()  RtCacheKeeper::rtCreate() returned NULL");		
		
		return 	RT_FAILURE;	
	}
	
	if(RT_SUCCESS != mp_cache_keeper->rtInitCacheKeeper(RtCacheKeeperCallBackImpl))
	{
		//HP_CR(Done): give prints, CRITICAL logs
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf()::rtInitialize()  mp_cache_keeper->rtInitCacheKeeper() returned FAILURE");		
		
		return RT_FAILURE;
	}

	/* 
			creation of  RtShUDIUserCntxtMgr
	*/	


	mp_user_cntxt_mgr = RtShUDIUserCntxtMgr::rtCreate(l_num_user_cntxt,(m_max_data_ref+m_max_data_repository));
	if(NULL ==	mp_user_cntxt_mgr)
	{
		printf("RtShUserDataIntf :: rtInitialize()  RtShUDIUserCntxtMgr::rtCreate() returned NULL");
		fflush(NULL);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf()::rtInitialize()  RtShUDIUserCntxtMgr::rtCreate() returned NULL");			
		
		return 	RT_FAILURE;	
	}
	

	//mp_data_pool_mgr = RtShUDIDataPoolMgr::rtCreate() does initialization implicitly
	mp_data_pool_mgr	= RtShUDIDataPoolMgr::rtCreate(m_max_data_ref, m_max_data_repository,a_comp_name);
	if(NULL ==	mp_data_pool_mgr)
	{
		printf("RtShUserDataIntf :: rtInitialize()  RtShUDIDataPoolMgr::rtCreate() returned NULL");
		fflush(NULL);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf()::rtInitialize()  RtShUDIDataPoolMgr::rtCreate() returned NULL");				
		
		return 	RT_FAILURE;	
	}

// Now we can initialize RtShUDIUserCntxtMgr
	if(RT_SUCCESS !=	mp_user_cntxt_mgr->rtInitialize())
	{
		printf("RtShUserDataIntf :: rtInitialize()  RtShUDIUserCntxtMgr::rtInitialize() FAILED");
		fflush(NULL);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf()::rtInitialize()  rtInitialize()  RtShUDIUserCntxtMgr::rtInitialize() FAILED");			
		
		return 	RT_FAILURE;	
	}

	/* Create and initialize the RtUDICdrCntxtMgr */
	mp_udi_cdr_cntxt_mgr = RtUDICdrCntxtMgr::rtCreate();
	if(NULL == mp_udi_cdr_cntxt_mgr)
	{
		printf("RtShUserDataIntf :: rtInitialize()  RtUDICdrCntxtMgr::rtCreate() returned NULL");
		fflush(NULL);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf()::rtInitialize()  RtUDICdrCntxtMgr::rtCreate() returned NULL");			
		
		return 	RT_FAILURE;	
	}
	if( mp_udi_cdr_cntxt_mgr->rtInitialize() != RT_SUCCESS)
	{
		printf("RtShUserDataIntf :: rtInitialize()  initialization of mp_udi_cdr_cntxt_mgr failed");
		fflush(NULL);

		return RT_FAILURE;
	}
	

	/* Create and initialize the UDICdrMgr */
	
  mp_udi_cdr_mgr = RtUDICdrMgr::rtCreate();
	
	if(NULL == mp_udi_cdr_mgr)
	{
		printf("RtShUserDataIntf :: rtInitialize()  RtUDICdrMgr::rtCreate() returned NULL");
		fflush(NULL);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf()::rtInitialize()  RtUDICdrMgr::rtCreate() returned NULL");			
		
		return 	RT_FAILURE;	
	}
	
	if( mp_udi_cdr_mgr->rtInitialize() != RT_SUCCESS)
	{
		printf("RtShUserDataIntf :: rtInitialize()  initialization of mp_udi_cdr_mgr failed");
		fflush(NULL);

		return RT_FAILURE;
	}
	
	if((mp_udi_cdr_mgr->rtThreadStart()) != RT_SUCCESS)
	{
		printf("\nERROR::RtUDICdrMgr :: rtInitialise()-LEAVE - mp_udi_cdr_mgr->rtThreadStart() returned failure [RETVAL=RT_FAILURE]");

		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			"RtUDICdrMgr :: rtInitialise()-LEAVE - mp_udi_cdr_mgr->rtThreadStart() returned failure[RETVAL=RT_FAILURE]");

		return RT_FAILURE;
	}

	/*
		SHASHI creation and initialization of provisioning interface
	*/
	
	// if(mp_sys_agent->rtGetRtBoolT(RT_PRV_UDI_INTF_ENABLED))
// 	{
// 		mp_prov_rest_mgr = RtProvRestMgr::rtCreate();
// 		if(NULL	==	mp_prov_rest_mgr)
// 		{
// 			printf("RtShUserDataIntf :: rtInitialize()  RtProvRestMgr::rtCreate() returned NULL");
// 			fflush(NULL);
// 
// 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 			"RtShUserDataIntf()::rtInitialize()  RtShProvIntf::rtCreate() returned NULL");			
// 
// 			return 	RT_FAILURE;	
// 		}
// 
// 		if( mp_prov_rest_mgr->rtInitialize() != RT_SUCCESS)
// 		{
// 			printf("RtShUserDataIntf :: rtInitialize()  initialization of mp_prov_rest_mgr failed");
// 			fflush(NULL);
// 
// 			return RT_FAILURE;
// 		}
// 	}
// 	else
// 	{
// 		printf("RtShUserDataIntf :: rtInitialize()  mp_sys_agent->rtGetRtBoolT(RT_PRV_UDI_INTF_ENABLED) returned %d - no need to create Prov Interface ",mp_sys_agent->rtGetRtBoolT(RT_PRV_UDI_INTF_ENABLED));
// 		fflush(NULL);
// 
// 		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 		"RtShUserDataIntf :: rtInitialize()  mp_sys_agent->rtGetRtBoolT(RT_PRV_UDI_INTF_ENABLED) returned %d - no need to create Prov Interface ",mp_sys_agent->rtGetRtBoolT(RT_PRV_UDI_INTF_ENABLED));			
// 	}
	
	
	//if(l_cache_ckpt_reqd)//JFC
	// {
// 		
// 		RtCacheKeeperCkptInitData  l_ckpt_init_data;
// 		memset(&l_ckpt_init_data,0,sizeof(RtCacheKeeperCkptInitData));
// 		
// 		//SHASHI:: changes are made in XML READER AND RtAppIntfParam IN basecomp...
//   	
// 		l_ckpt_init_data.shm_key = mp_base_comp_conf_param->app_intf_param.cache_keeper_data_sync_data.shm_key;
//   	printf("\n RtShUserDataIntf::rtInitialize() - Referential Data shm_key=%d",l_ckpt_init_data.shm_key);
//     	fflush(NULL);
//   	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
//     	"RtShUserDataIntf::rtInitialize() - Referential Data shm_key=%d", l_ckpt_init_data.shm_key);
// 
//   	l_ckpt_init_data.l_ds_init.init_state                    = RT_DATA_SYNC_INVALID_STATE;
//   	l_ckpt_init_data.l_ds_init.data_sync_mode                = RtDataSyncDataModeT(mp_sys_agent->rtGetRtU32T(RT_SH_UDI_DATA_SYNC_MODE));
//   	l_ckpt_init_data.l_ds_init.num_ds_active_wrkr            = mp_sys_agent->rtGetRtU32T(RT_SH_UDI_DATA_SYNC_ACTIVE_WRKR);
//   	l_ckpt_init_data.l_ds_init.active_wrkr_base_eeid         = RT_EE_CACHE_KEEPER_DS_ACTIVE_WRKR_BASE;
//   	l_ckpt_init_data.l_ds_init.num_ds_standby_wrkr           = mp_sys_agent->rtGetRtU32T(RT_SH_UDI_DATA_SYNC_STANDBY_WRKR);
//   	l_ckpt_init_data.l_ds_init.standby_wrkr_base_eeid        = RT_EE_CACHE_KEEPER_DS_STANDBY_WRKR_BASE;
//   	l_ckpt_init_data.l_ds_init.is_rep_buf_match_req          = true;
//   	l_ckpt_init_data.l_ds_init.shm_key                       = l_ckpt_init_data.shm_key ;
//   	l_ckpt_init_data.l_ds_init.max_sections                  = l_num_cache_ids;
//   	l_ckpt_init_data.l_ds_init.ipc_mode                      = RT_DATA_SYNC_USE_MGL_MESSAGING;
//   	
// 		if(mp_sys_agent->rtGetRtBoolT(RT_UDI_TO_USE_SHM))
//     	l_ckpt_init_data.l_ds_init.app_data_storage            = RT_APP_DATA_STORAGE_SH_MEM;
//   	else
//     	l_ckpt_init_data.l_ds_init.app_data_storage            = RT_APP_DATA_STORAGE_IN_MEM;
// 
//   	if(l_ckpt_init_data.l_ds_init.ipc_mode == RT_DATA_SYNC_USE_MGL_MESSAGING)
//   	{
//     	l_ckpt_init_data.l_ds_init.u.messaging_struct.mate_comp_type  = mp_base_comp_conf_param->bci_conf_param.mate_process_type;
//     	l_ckpt_init_data.l_ds_init.u.messaging_struct.mate_comp_id    = mp_base_comp_conf_param->bci_conf_param.mate_process_inst_id;
//   	}
//   	else
//   	{
//     	printf("\RtShUserDataIntf::rtInitialize():- ,Invalid IPC Mode=%d for DataSync\n", l_ckpt_init_data.l_ds_init.ipc_mode);
//     	fflush(NULL);
//     	return RT_FAILURE;
//   	}
// 
// 
//   	RtProcCurrentState    l_proc_cur_state;
//   	bzero(&l_proc_cur_state, sizeof(RtProcCurrentState));
//   	l_proc_cur_state = lp_avsv_mgr->rtGetProcCurrentState();
// 
//   	printf("\RtShUserDataIntf::rtInitialize():- procStartUpType=%d,is_node_reboot=%d",l_proc_cur_state.procStartUpType,l_proc_cur_state.is_node_reboot);
//   	fflush(NULL);
// 
//   	//HP_CPSV_CHANGES - 05052009 
//   	if(   (l_proc_cur_state.procStartUpType == RT_PROC_AUTO_RESTART ||
//         	 l_proc_cur_state.procStartUpType == RT_PROC_ADMIN_START) 
//                             	 &&
//         	(!l_proc_cur_state.is_node_reboot)
//     	 )
//   	{
//     	l_ckpt_init_data.l_ds_init.shm_init_mode  = RT_DATA_SYNC_SHM_ATTACH;
// 			l_ckpt_init_data.attach_only							= true;
//   	}
//   	else
//   	{
//     	l_ckpt_init_data.l_ds_init.shm_init_mode  = RT_DATA_SYNC_SHM_CREATE;
// 			l_ckpt_init_data.attach_only							= false;
//   	}
// 	
// 	
// 		l_ckpt_init_data.logger_mod				= RT_SH_UDI_MOD;
// 		l_ckpt_init_data.p_logger_mod_str = "RT_SH_UDI_MOD";
// 		
// 		if(RT_SUCCESS != mp_cache_keeper->rtInitCacheKeeperWithCkptInitData(&l_ckpt_init_data);
// 		{
// 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
// 			"rtInitialize()::mp_cache_keeper->rtInitCacheKeeperWithCkptInitData FAILED app_data_storage=%d,data_sync_mode=%d",
// 			l_ckpt_init_data.l_ds_init.app_data_storage,l_ckpt_init_data.l_ds_init.data_sync_mode);
// 
// 			return RT_FAILURE;
// 		}
// 		
// 	}


	/* 
			creation of RtUserData class
	*/	
// 	mp_user_data_class = RtUserData::rtCreate();
// 	if(NULL ==	mp_user_data_class)
// 	{
// 		printf("RtShUserDataIntf :: rtInitialize()  RtUserData::rtCreate() returned NULL");
// 		fflush(NULL);
// 		
// 		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
// 		"RtShUserDataIntf()::rtInitialize()  RtUserData::rtCreate() returned NULL");		
// 		
// 		return 	RT_FAILURE;	
// 	}


	/* 
			creation of  worker threads
	*/	

	
	 m_num_wrkr	=  mp_sys_agent->rtGetRtU32T(RT_SH_UDI_NUM_WRKR_THRDS);
	//HP_CR(DONE):: debug logs printing m_num_wrkr
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"RtShUserDataIntf::rtInitialize()::Max No Of Wrkr [%u]",m_num_wrkr);

	mp_wrkr_arr = new RtShUDIWrkr*[m_num_wrkr];//allocating memory to the array of wrkr class instances
	
	for(RtU32T l_cnt=0; l_cnt < m_num_wrkr; l_cnt++)
	{ 
		l_ee_id = RT_EE_SH_UDI_WRKR_BASE	+ l_cnt;
		mp_wrkr_arr[l_cnt] = new RtShUDIWrkr(l_ee_id);

		if( mp_wrkr_arr[l_cnt]->rtInitialize() != RT_SUCCESS)
		{
			printf("RtShUserDataIntf :: rtInitialize()  initialization of %dth wrkr thread failed",l_ee_id);
			fflush(NULL);

			return RT_FAILURE;
		}
	
//SHASHI
		l_ret_val = mp_wrkr_arr[l_cnt]->rtThreadStart();
		if (l_ret_val != RT_SUCCESS)
		{
			printf("\nRtShUserDataIntf :: rtInitialize(),mp_wrkr_arr[l_cnt]->rtThreadStart() returned =%d for entiy Id = %d",l_ret_val,l_ee_id);
			return RT_FAILURE;
		}	

		l_ret_val = mp_wrkr_arr[l_cnt]->rtGetThreadId(l_thread_id);
		if (l_ret_val != RT_SUCCESS)
		{
			printf("\nRtShUserDataIntf :: rtInitialize(),mp_wrkr_arr[l_cnt]->rtGetThreadId returned =%d for entiy Id = %d",l_ret_val,l_ee_id);
			return RT_FAILURE;
		}	

		l_ret_val = gp_mgl_intf->rtSetThreadEntity(l_thread_id,l_ee_id);
		if(l_ret_val != RT_SUCCESS)
		{
			printf("\nRtShUserDataIntf :: rtInitialize(),mp_mgl_intf->rtSetThreadEntity() returned %d; l_thread_id=%ld,a_ee_id=%d\n",l_ret_val,l_thread_id,l_ee_id);
			fflush(NULL);
			return RT_FAILURE;
		}

		l_ee_info.ee_ptr = mp_wrkr_arr[l_cnt];

		l_ret_val = gp_mgl_intf->rtRegExecEntity(l_ee_id,rtEntitySend,l_ee_info);
		if(l_ret_val != RT_SUCCESS)
		{
			printf("\nRtShUserDataIntf :: rtInitialize(),mp_mgl_intf->rtRegExecEntity() returned =%d for entiy Id = %d",l_ret_val,l_ee_id);
			return RT_FAILURE;
		}
		
		printf("\nRtShUserDataIntf :: rtInitialize(), Successfully started UDI worker thread with entity id =%d\n",l_ee_id);
//SHASHI			
	}//registration with MGL layer has been done in rtInitialize of RtShUDIWrkr class.
	 
	 /*------------------------------------------------------------------------------
	 -
	 -  Sunny:-1.Creation of UdiDbWorker which are for DB related Work.
	 -         2.RT_EE_DB_UDI_WRKR_BASE should be define in the RtExecEntit
	 -		     3.RT_DB_UDI_NUM_WRKR_THRDS should be define in the Pam file.
	 ------------------------------------------------------------------------------*/

	l_ee_id = RT_EE_DB_UDI_WRKR_BASE;
	m_num_dbi_wrkr	=  mp_sys_agent->rtGetRtU32T(RT_DB_UDI_NUM_WRKR_THRDS);
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,"RtShUserDataIntf::rtInitialize()::Max No Of DB_UDI_Wrkr [%u] and Base l_ee_id=%d",m_num_dbi_wrkr,l_ee_id);
	
	mp_dbi_wrkr_arr = new RtDbUDIWrkr*[m_num_dbi_wrkr];
	for(RtU32T l_cnt=0 ; l_cnt < m_num_dbi_wrkr; l_cnt++,++l_ee_id)
	{
		//l_ee_id = RT_EE_DB_UDI_WRKR_BASE + l_cnt;
		mp_dbi_wrkr_arr[l_cnt] = new RtDbUDIWrkr(l_ee_id);
		
		if( mp_dbi_wrkr_arr[l_cnt]->rtInitialize() != RT_SUCCESS)
		{
			printf("RtShUserDataIntf :: rtInitialize()  initialization of %dth db_udi_wrkr thread failed",l_ee_id);
			fflush(NULL);

			return RT_FAILURE;
		}
		l_ret_val = mp_dbi_wrkr_arr[l_cnt]->rtThreadStart();
		if (l_ret_val != RT_SUCCESS)
		{
			printf("\nRtShUserDataIntf :: rtInitialize(),mp_dbi_wrkr_arr[l_cnt]->rtThreadStart() returned =%d for entiy Id = %d",l_ret_val,l_ee_id);
			return RT_FAILURE;
		}	

		l_ret_val = mp_dbi_wrkr_arr[l_cnt]->rtGetThreadId(l_thread_id);
		if (l_ret_val != RT_SUCCESS)
		{
			printf("\nRtShUserDataIntf :: rtInitialize(),mp_dbi_wrkr_arr[l_cnt]->rtGetThreadId returned =%d for entiy Id = %d",l_ret_val,l_ee_id);
			return RT_FAILURE;
		}	

		l_ret_val = gp_mgl_intf->rtSetThreadEntity(l_thread_id,l_ee_id);
		if(l_ret_val != RT_SUCCESS)
		{
			printf("\nRtShUserDataIntf :: rtInitialize(),mp_mgl_intf->rtSetThreadEntity() returned %d; l_thread_id=%ld,a_ee_id=%d\n",l_ret_val,l_thread_id,l_ee_id);
			fflush(NULL);
			return RT_FAILURE;
		}

		l_ee_info.ee_ptr = mp_dbi_wrkr_arr[l_cnt];

		l_ret_val = gp_mgl_intf->rtRegExecEntity(l_ee_id,rtEntitySend,l_ee_info);
		if(l_ret_val != RT_SUCCESS)
		{
			printf("\nRtShUserDataIntf :: rtInitialize(),mp_mgl_intf->rtRegExecEntity() returned =%d for entiy Id = %d",l_ret_val,l_ee_id);
			return RT_FAILURE;
		}
		
		printf("\nRtShUserDataIntf :: rtInitialize(), Successfully started DB_UDI worker thread with entity id =%d\n",l_ee_id);fflush(NULL);
			
	}
	mp_udi_db_hdlr = new RtUdiDbHandler();
	if(mp_udi_db_hdlr == NULL)
	{
		printf("\nERROR::RtShUserDataIntf :: rtInitialize()-LEAVE -  new RtUdiDbHandler() returned NULL[RETVAL=RT_FAILURE]");
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			"RtShUserDataIntf :: rtInitialize()-LEAVE -  new RtUdiDbHandler() returned NULL[RETVAL=RT_FAILURE]");

		return RT_FAILURE;
	}
	l_ret_val = mp_udi_db_hdlr->rtInit();
	if(l_ret_val != RT_SUCCESS)
	{
		printf("\nERROR::RtShUserDataIntf :: rtInitialize()-LEAVE - mp_udi_db_hdlr->rtInit() returned l_rval=%d",
		l_ret_val);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			"RtShUserDataIntf :: rtInitialize()-LEAVE -  mp_udi_db_hdlr->rtInit() returned l_rval=%d ",
			l_ret_val);

		return RT_FAILURE;
	}
	 /*------------------------------------------------------------------------------
	 - Sunny Changes Till Here
	 ------------------------------------------------------------------------------*/


	
	if(mp_sys_agent->rtGetRtBoolT(RT_UDI_IMMEDIATE_REGEN_ORD_LIST))
	{
		m_immediate_regen = true;
		mp_prep_ord_list	= rtPrepOrigOrdList;
	}
	else
	{
		m_immediate_regen = false;
		mp_prep_ord_list	= rtPrepRegenOrdList;
	}
	
	/* registration of avsv callbacks */
	
  /*
    Get the INSTANCE of RtAvsvMgr
  */
  RtAvsvMgr* lp_avsv_mgr = RtAvsvMgr::rtGetInstance();
  if(lp_avsv_mgr == NULL)
  {
    printf("\nERROR::RtShUserDataIntf::rtInitialize():- RtAvsvMgr::rtGetInstance() return NULL");
    mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
      "ERROR::RtShUserDataIntf::rtInitialize():-RtAvsvMgr::rtGetInstance() return NULL [RETVAL=RT_FAILURE]");
    
    return RT_FAILURE;
  }
  
  
  
	l_ret_val = lp_avsv_mgr->rtSetModCb(RT_PROC_ACTIVE_NEW_ASSIGN, rtUDIAvsvActiveNewAssignmentCallback);
	if(RT_SUCCESS != l_ret_val)
	{
		printf("\n ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_ACTIVE_NEW_ASSIGN(=%d), rtUDIAvsvActiveNewAssignmentCallback)", RT_PROC_ACTIVE_NEW_ASSIGN);
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
		"ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_ACTIVE_NEW_ASSIGN(=%d), rtUDIAvsvActiveNewAssignmentCallback)", RT_PROC_ACTIVE_NEW_ASSIGN);
		return RT_FAILURE;
	}

	l_ret_val = lp_avsv_mgr->rtSetModCb(RT_PROC_ACTIVE_FAIL_OVER, rtUDIAvsvActiveFailOverCallback);
	if(RT_SUCCESS != l_ret_val)
	{
		printf("\n ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_ACTIVE_FAIL_OVER(=%d), rtUDIAvsvActiveFailOverCallback)", RT_PROC_ACTIVE_FAIL_OVER);
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
  		"ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_ACTIVE_FAIL_OVER(=%d), rtUDIAvsvActiveFailOverCallback)", RT_PROC_ACTIVE_FAIL_OVER);
		return RT_FAILURE;
	}

	l_ret_val = lp_avsv_mgr->rtSetModCb(RT_PROC_ACTIVE_SWITCH_OVER, rtUDIAvsvActiveSwitchOverCallback);
	if(RT_SUCCESS != l_ret_val)
	{
		printf("\n ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_ACTIVE_SWITCH_OVER(=%d), rtUDIAvsvActiveSwitchOverCallback)", RT_PROC_ACTIVE_SWITCH_OVER);
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
  		"ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_ACTIVE_SWITCH_OVER(=%d), rtUDIAvsvActiveSwitchOverCallback)", RT_PROC_ACTIVE_SWITCH_OVER);
		return RT_FAILURE;
	}

	l_ret_val = lp_avsv_mgr->rtSetModCb(RT_PROC_STANDBY, rtUDIAvsvStandbyCallback);
	if(RT_SUCCESS != l_ret_val)
	{
		printf("\n ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_STANDBY(=%d), rtUDIAvsvStandbyCallback)", RT_PROC_STANDBY);
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
  		"ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_STANDBY(=%d), rtUDIAvsvStandbyCallback)", RT_PROC_STANDBY);
		return RT_FAILURE;
	}

	//Tag_amit_15092012 -- handling is done for QUIESCED callback
	// RT_PROC_QUIESCED callback should be of lower priority (should be called at the end).
	l_ret_val = lp_avsv_mgr->rtSetModCb(RT_PROC_QUIESCED, rtUDIAvsvQuiescedCallback, true);
	if(RT_SUCCESS != l_ret_val)
	{
		printf("\n ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_QUIESCED(=%d), rtUDIAvsvQuiescedCallback)", RT_PROC_QUIESCED);
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
  		"ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_QUIESCED(=%d), rtUDIAvsvQuiescedCallback)", RT_PROC_QUIESCED);
		return RT_FAILURE;
	}

	l_ret_val = lp_avsv_mgr->rtSetModCb(RT_PROC_QUIESCING, rtUDIAvsvQuiescingCallback);
	if(RT_SUCCESS != l_ret_val)
	{
		printf("\n ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_QUIESCING(=%d), rtUDIAvsvQuiescingCallback)", RT_PROC_QUIESCING);
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
  		"ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_QUIESCING(=%d), rtUDIAvsvQuiescingCallback)", RT_PROC_QUIESCING);
		return RT_FAILURE;
	}

	l_ret_val = lp_avsv_mgr->rtSetModCb(RT_PROC_CSI_STILL_ACTIVE, rtUDIAvsvCsiStillActiveCallback);
	if(RT_SUCCESS != l_ret_val)
	{
		printf("\n ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_CSI_STILL_ACTIVE(=%d), rtUDIAvsvCsiStillActiveCallback)", RT_PROC_CSI_STILL_ACTIVE);
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
  		"ERROR::RtShUserDataIntf::rtInitialize() - lp_avsv_mgr->rtSetModCb() FAILED for (RT_PROC_CSI_STILL_ACTIVE(=%d), rtUDIAvsvCsiStillActiveCallback)", RT_PROC_CSI_STILL_ACTIVE);
		return RT_FAILURE;
	}
	
	//TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES
// 	m_delta_stack_wait_time = 5;
// 	if((getenv("RT_DELTA_STACK_WAIT_TIME") != NULL))
// 	{
// 	
// 		m_delta_stack_wait_time = atoi(getenv("RT_DELTA_STACK_WAIT_TIME"));
// 				
// 	}
// 	printf("\n m_delta_stack_wait_time =%u\n",m_delta_stack_wait_time);
	
	
	printf("\nRtShUserDataIntf :: rtInitialize(), Successfully started UDI worker thread with entity id =%d\n",l_ee_id);
	
	//Tag_hotfix_genesysPF_152
	m_udi_init_complete = true;
	
	return RT_SUCCESS;
}

/*******************************************************************************
*
* FUNCTION NAME : rtGetMaxDataRef()
*
* DESCRIPTION   :  Return m_max_data_ref
*                 
* INPUT         : a_max_data_ref 
*
* OUTPUT        : none 
*
* RETURN        : void. 
*
******************************************************************************/
void  RtShUserDataIntf :: rtGetMaxDataRef(RtU8T&  a_max_data_ref)
{
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
  "RtShUserDataIntf :: rtGetMaxDataRef()--ENTER");
  
  a_max_data_ref = m_max_data_ref;  
  
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
  "RtShUserDataIntf :: rtGetMaxDataRef()--EXIT MaxDataRef[%u]",a_max_data_ref);   

}

 /*******************************************************************************
*
* FUNCTION NAME : rtGetMaxDataRepository()
*
* DESCRIPTION   :  Return m_max_data_repository
*                 
* INPUT         : a_max_data_repository 
*
* OUTPUT        : none 
*
* RETURN        : void. 
*
******************************************************************************/
void  RtShUserDataIntf :: rtGetMaxDataRepository(RtU8T&  a_max_data_repository)
{
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
  "RtShUserDataIntf :: rtGetMaxDataRepository()--ENTER");
  
  a_max_data_repository = m_max_data_repository;  
  
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
  "RtShUserDataIntf :: rtGetMaxDataRepository()--EXIT MaxDataRepository[%u]",a_max_data_repository);    

}

/*******************************************************************************
*
* FUNCTION NAME : rtGetNumWrkr()
*
* DESCRIPTION   :  Return m_max_data_repository
*                 
* INPUT         : ar_num_wrkr 
*
* OUTPUT        : none 
*
* RETURN        : void. 
*
******************************************************************************/
void  RtShUserDataIntf :: rtGetNumWrkr(RtU32T&  ar_num_wrkr)
{
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
  "RtShUserDataIntf :: rtGetNumWrkr()--ENTER");
  
  ar_num_wrkr = m_num_wrkr; 
  
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
  "RtShUserDataIntf :: rtGetNumWrkr()--EXIT num_wrkr[%u]",ar_num_wrkr);   

}


/*******************************************************************************
 *
 * FUNCTION NAME :	rtRegisterForDataRef()
 *
 * DESCRIPTION   :	This function is invoked by application to register for a particular data reference.
 *									It provides new srv token for data ref =0.constructs srv-ind vs srv-token map and 
 *									app_enum vs udiref map
 *
 * INPUT         :	RtShUDIRegData*
 *
 * OUTPUT        :	RtShUDIRef - valid when data reference is ZERO and function returns RT_SUCCESS
 *
 * RETURN        :	RtRcT 
  									RT_SUCCESS
	 									RT_SH_UDI_ERR_DUPLICATE_REGISTRATION
										RT_SH_UDI_ERR_INVALID_DATA_REF

 *
 ******************************************************************************/
RtRcT RtShUserDataIntf :: rtRegisterForDataRef(RtShUDIRegData* ap_reg_data,RtShUDIRef& ar_udi_ref)
{
	//HP_CR(DONE):: %c or ?? to be used for udi_ref
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER rtRegisterForDataRef() with  ref_data_val = %u,max_data_size= %u,",
		ap_reg_data->ref_data_val,ap_reg_data->max_data_size);
	
	RtRcT l_ret_val = RT_SUCCESS;
	
	if( true == m_is_registration_ceased)
	{
		//HP_CR(DONE):: give CRITICAL LOgs
	  
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf:: Registration ceased Could not register for ref_data_val [%u]",ap_reg_data->ref_data_val);
		return RT_SH_UDI_ERR_REGISTRATION_CEASED;
	
	}

	//take W lock on m_srvc_ind_vs_srvc_token_lock
	
	pthread_mutex_lock(&m_srvc_ind_vs_srvc_token_lock);

	if(RT_DIA_SH_REPOSITORY_DATA == ap_reg_data->ref_data_val) 
	{ //means that registration called for data repository
		
		RtShSrvcIndTokenData	l_srvc_token_data;
		string l_srvc_ind(ap_reg_data->data_ref_appl_info.app_srvc_indic);
		memset(&l_srvc_token_data,0,sizeof(RtShSrvcIndTokenData));
		
		l_srvc_token_data.srvc_token	= ++m_nxt_free_srvc_token;
		l_srvc_token_data.app_info.app_enum		= ap_reg_data->data_ref_appl_info.app_enum;
		strcpy(l_srvc_token_data.app_info.app_srvc_indic, ap_reg_data->data_ref_appl_info.app_srvc_indic);
		
		RtSrvcIndVsSrvcTokenDataMapPair l_pair;
		l_pair = m_srvc_ind_vs_srvc_token_map.insert(make_pair(l_srvc_ind, l_srvc_token_data));
		
		if(l_pair.second)
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
				"rtRegisterForDataRef() :: Success inserting srvc_ind = %s, srvc_token= %u in srvc_ind_vs_srvc_token_map",
				l_srvc_ind.c_str(),l_srvc_token_data.srvc_token);

			RtApplEnumVsUDIRefMapPair l_enum_vs_udi_pair;
			l_enum_vs_udi_pair	= m_app_enum_vs_udi_ref_map.insert(make_pair(ap_reg_data->data_ref_appl_info.app_enum, m_nxt_free_srvc_token));

			if (l_enum_vs_udi_pair.second)
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtRegisterForDataRef() :: Success inserting app_enum = %d, udiref= %u in srvc_ind_vs_srvc_token_map",
					ap_reg_data->data_ref_appl_info.app_enum,m_nxt_free_srvc_token);

				m_data_ref_conf_bitmap.set(m_nxt_free_srvc_token);

				ar_udi_ref = m_nxt_free_srvc_token;


				l_ret_val = mp_data_pool_mgr->rtSetRegData(ar_udi_ref,ap_reg_data);
				if(RT_SUCCESS !=	l_ret_val)
				{
					//HP_CR(DONE): critical logs
			  	
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					"rtRegisterForDataRef() ::rtSetRegData() FAILED Udi_ref [%d] Ret_val[%d]",ar_udi_ref, l_ret_val);
					
				}
				else
				{
					//HP_CR(DONE): debug logs
					
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"rtRegisterForDataRef() ::rtSetRegData() SUCCESS Udi_ref [%d] Ret_val[%d]",ar_udi_ref, l_ret_val);					
				}
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					"rtRegisterForDataRef() :: ERROR Failed inserting app_enum [%d], udi_ref[%d] in srvc_ind_vs_srvc_token_map",
					ap_reg_data->data_ref_appl_info.app_enum,m_nxt_free_srvc_token);
			
				m_srvc_ind_vs_srvc_token_map.erase(l_srvc_ind);
				--m_nxt_free_srvc_token;
				l_ret_val = RT_SH_UDI_ERR_DUPLICATE_REGISTRATION;
			}

		}
		else
		{//most probably due to duplicate registration
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtRegisterForDataRef() :: ERROR Failed inserting srvc_ind = %s, srvc_token= %u in srvc_ind_vs_srvc_token_map",
				l_srvc_ind.c_str(),l_srvc_token_data.srvc_token);
			
			--m_nxt_free_srvc_token;
			l_ret_val = RT_SH_UDI_ERR_DUPLICATE_REGISTRATION;
		}
	}
	else if( ((ap_reg_data->ref_data_val >=1) && (ap_reg_data->ref_data_val <=9))
													||
							(20 == ap_reg_data->ref_data_val)	
													||					
							(ap_reg_data->ref_data_val > m_max_data_ref))
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"rtRegisterForDataRef() :: ERROR invalid Data ref [%u] received in udi_reg_data",
			ap_reg_data->ref_data_val);

		l_ret_val = RT_SH_UDI_ERR_INVALID_DATA_REF;
	}
	else
	{//non-transparent data

		if (true	==	m_data_ref_conf_bitmap.test(ap_reg_data->ref_data_val ))
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
				"rtRegisterForDataRef() :: Duplicate registration received for data_ref=%u",ap_reg_data->ref_data_val);

			l_ret_val = RT_SH_UDI_ERR_DUPLICATE_REGISTRATION;
		}
		else
		{

			ar_udi_ref = ap_reg_data->ref_data_val;

			m_data_ref_conf_bitmap.set(ap_reg_data->ref_data_val);

			l_ret_val = mp_data_pool_mgr->rtSetRegData(ap_reg_data->ref_data_val,ap_reg_data);
			if(RT_SUCCESS !=	l_ret_val)
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtRegisterForDataRef() ::rtSetRegData() FAILED Udi_ref [%u] Ret_val[%d]",ap_reg_data->ref_data_val, l_ret_val);				
			}
			else
			{
				//HP_CR(DONE): debug logs
				
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"rtRegisterForDataRef() ::rtSetRegData() SUCCESS Udi_ref [%d] Ret_val[%d]",ar_udi_ref, l_ret_val);					
			}
		}

	}

	pthread_mutex_unlock(&m_srvc_ind_vs_srvc_token_lock);
	
	//HP_CR(DONE):: gove ref_data_val and ar_udi_ref in logs
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	" rtRegisterForDataRef():EXIT   For Ref_data_val[%u], Udi_ref[%u] with l_ret_val = %d",
	ap_reg_data->ref_data_val, ar_udi_ref, l_ret_val);

	return l_ret_val;
}


/*******************************************************************************
 *
 * FUNCTION NAME : rtCeaseRegisterForDataRef()
 *
 * DESCRIPTION   : This function is called by service application to indicate
 *                 end of data reference registration process
 *
 * INPUT         : none 
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf :: rtCeaseRegisterForDataRef()
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtCeaseRegisterForDataRef() m_is_registration_ceased =%d",m_is_registration_ceased);
	
	RtRcT		l_ret_val = RT_SUCCESS;
	
	pthread_mutex_lock(&m_srvc_ind_vs_srvc_token_lock);
	
	if(m_is_registration_ceased)
	{
		l_ret_val = RT_SH_UDI_ERR_DUPLICATE_CEASE_OF_REGISTRATION;
	}
	else
	{
		//HP_CR(DONE):: check on l_ret_val 
		l_ret_val = mp_data_pool_mgr->rtEndOfSetRegData();
		if(RT_SUCCESS == l_ret_val)
		{
			m_is_registration_ceased = true;
			
		}
		else
		{
	  	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"RtShUserDataIntf :: rtCeaseRegisterForDataRef() FAILED");			
		}
	}
	
	pthread_mutex_unlock(&m_srvc_ind_vs_srvc_token_lock);
	
	//******************************************************************
	// JFT calling for UDI order list testing //
	//******************************************************************/
		
// 		RtTransMsg l_trans_msg;
// 		memset(&l_trans_msg,0,sizeof(RtTransMsg));
// 
// 		l_trans_msg.msg_type  										= 99;
// 
//     l_trans_msg.msg_hdr.dest_addr.ee_id	      =	RT_EE_DB_UDI_WRKR_BASE;
// 
// 
//     mp_dbi_wrkr_arr[0]->rtPutMsg(&l_trans_msg);
	
	
	
	// ******************************************************************
	// JFT calling for UDI order list testing //
	//******************************************************************/
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtCeaseRegisterForDataRef() with ret_val[%d]",l_ret_val);

	return l_ret_val;
}


/*******************************************************************************
 *
 * FUNCTION NAME : rtEraseUserIdenVsCntxtMapEntry()
 *
 * DESCRIPTION   : Called whenever entry from m_user_iden_vs_cntxt_map is to be erased
 *								 when block no is known
 *
 * INPUT         : RtS8T* ap_user_identity,RtU32T a_block_no
 *
 * OUTPUT        : none 
 *
 * RETURN        : None
 *
 ******************************************************************************/
void RtShUserDataIntf :: rtEraseUserIdenVsCntxtMapEntry(RtS8T* ap_user_identity, RtU32T a_block_no)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtEraseUserIdenVsCntxtMapEntry() with ap_user_identity=%s,a_block_no =%u",
		ap_user_identity,a_block_no);
	

	
	pthread_rwlock_wrlock(&m_user_iden_vs_cntxt_lock_arr[a_block_no]);

	if(!( m_user_iden_vs_cntxt_map[a_block_no].erase(ap_user_identity)))
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"rtEraseUserIdenVsCntxtMapEntry() : ERROR Failed erasing user_id =%s user_iden_vs_cntxt_map block =%u",
			ap_user_identity,a_block_no);		
	}
	else
	{
		//HP_CR(DONE): DEBUG logs
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"rtEraseUserIdenVsCntxtMapEntry() : SUCCESS User_id [%s] user_iden_vs_cntxt_map block [%u]",
		ap_user_identity,a_block_no);		
	}
	
	pthread_rwlock_unlock(&m_user_iden_vs_cntxt_lock_arr[a_block_no]);

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtEraseUserIdenVsCntxtMapEntry() User_id [%s] user_iden_vs_cntxt_map block [%u]",
		ap_user_identity,a_block_no);
}


/*******************************************************************************
 *
 * FUNCTION NAME : rtEraseUserIdenVsCntxtMapEntry()
 *
 * DESCRIPTION   : Called whenever entry from m_user_iden_vs_cntxt_map is to be erased
 *								 when block no is NOT known
 *
 * INPUT         : RtS8T* ap_user_identity
 *
 * OUTPUT        : none 
 *
 * RETURN        : None
 *
 ******************************************************************************/
void RtShUserDataIntf :: rtEraseUserIdenVsCntxtMapEntry(RtS8T* ap_user_identity)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtEraseUserIdenVsCntxtMapEntry() with user_identity=%s",ap_user_identity);

	RtU32T	l_block_no;
	
	rtDecideBlock(ap_user_identity,l_block_no);
	
	rtEraseUserIdenVsCntxtMapEntry(ap_user_identity, l_block_no);
	

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtEraseUserIdenVsCntxtMapEntry() with user_identity=%s",ap_user_identity);

}

/*******************************************************************************
 *
 * FUNCTION NAME : rtFindCntxtForUserIden()
 *
 * DESCRIPTION   : Called whenever enquiry needs to be done whether context for
 *                 user-identity exists or not
 *
 * INPUT         : RtS8T* ap_user_identity,RtU32T& ar_cntxt_indx
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT
 *
 ******************************************************************************/
RtRcT	RtShUserDataIntf :: rtFindCntxtForUserIden(RtS8T* ap_user_identity,RtU32T& ar_cntxt_indx)	
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtFindCntxtForUserIden() with user_identity=%s",ap_user_identity);

	RtRcT 	l_ret_val = RT_FAILURE;
	RtShUDIUserIdenVsCntxtMapItr	map_itr;
	
	RtU32T	l_block_no	=	0;
	rtDecideBlock(ap_user_identity,l_block_no);
	
	pthread_rwlock_rdlock((&m_user_iden_vs_cntxt_lock_arr[l_block_no]));
	
	map_itr = m_user_iden_vs_cntxt_map[l_block_no].find(ap_user_identity);
	if(map_itr	!=	m_user_iden_vs_cntxt_map[l_block_no].end())
	{
		ar_cntxt_indx = map_itr->second;
		l_ret_val = RT_SUCCESS;

		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
			"rtFindCntxtForUserIden() ENTRY EXIST FOR user_identity=%s,block no =%u,cntxt_index =%u",
			ap_user_identity, l_block_no, ar_cntxt_indx);
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
			"rtFindCntxtForUserIden() ENTRY DOES NOT EXIST user_identity=%s,block no =%u",
			ap_user_identity, l_block_no);		
	}	
		
	
	pthread_rwlock_unlock((&m_user_iden_vs_cntxt_lock_arr[l_block_no]));
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"Exit RtShUserDataIntf :: rtFindCntxtForUserIden() with user_identity=%s,ret_val =%d",ap_user_identity, l_ret_val);
	return l_ret_val;
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtInsertUserIdenVsCntxtMapEntry()
 *
 * DESCRIPTION   : Called whenever entry in m_user_iden_vs_cntxt_map needs to be inserted 
 *								  when block no is known
 *
 * INPUT         : RtS8T* ap_user_identity,RtU32T a_cntxt_indx,RtU32T a_block_no
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::	rtInsertUserIdenVsCntxtMapEntry(RtS8T* ap_user_identity,RtU32T a_cntxt_indx,RtU32T a_block_no)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtInsertUserIdenVsCntxtMapEntry() with user_identity=%s,cntxt_index=%u,block_no=%u",
		ap_user_identity, a_cntxt_indx, a_block_no);
	
	RtRcT 	l_ret_val = RT_SUCCESS;
	//- take W lock on m_user_iden_vs_cntxt_lock_arr[a_block]
	pthread_rwlock_wrlock((&m_user_iden_vs_cntxt_lock_arr[a_block_no]));
	
	//HP_CR(DONE): make string from ap_user_identity and pass in insert()
	
	string l_user_identity(ap_user_identity);
	
	RtShUDIUserIdenVsCntxtMapPair l_map_pair;
	l_map_pair	= m_user_iden_vs_cntxt_map[a_block_no].insert(make_pair(l_user_identity, a_cntxt_indx));
	
	if(!l_map_pair.second)
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"rtInsertUserIdenVsCntxtMapEntry() FAILED INSERTING IN MAP with user_identity=%s,cntxt_index=%u,block_no=%u",
			ap_user_identity, a_cntxt_indx, a_block_no);
		
		l_ret_val = RT_FAILURE;
	}
	else
	{
		//HP_CR(DONE): debug logs
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
			"rtInsertUserIdenVsCntxtMapEntry() SUCCESS INSERTING IN MAP with user_identity[%s],cntxt_index[%u],block_no[%u]",
			ap_user_identity, a_cntxt_indx, a_block_no);		
		
	}
	pthread_rwlock_unlock((&m_user_iden_vs_cntxt_lock_arr[a_block_no]));

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtInsertUserIdenVsCntxtMapEntry() with user_identity=%s,cntxt_index=%u,block_no=%u,ret_val=%d",
		ap_user_identity, a_cntxt_indx, a_block_no, l_ret_val);
	
	return l_ret_val;
}


/*******************************************************************************
 *
 * FUNCTION NAME : rtInsertUserIdenVsCntxtMapEntry()
 *
 * DESCRIPTION   : Called whenever entry in m_user_iden_vs_cntxt_map needs to be inserted
 *								  when block no is NOT known
 *
 * INPUT         : RtS8T* ap_user_identity,RtU32T a_cntxt_indx,RtU32T a_block_no
 *
 * OUTPUT        : none 
 *
 * RETURN        : None
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtInsertUserIdenVsCntxtMapEntry(RtS8T* ap_user_identity,RtU32T a_cntxt_indx)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtInsertUserIdenVsCntxtMapEntry() with user_identity=%s,cntxt_index=%u",
		ap_user_identity, a_cntxt_indx);
		
	RtRcT 	l_ret_val = RT_FAILURE;
	RtU32T  l_block_no;
	
	rtDecideBlock(ap_user_identity, l_block_no);
	
	l_ret_val = rtInsertUserIdenVsCntxtMapEntry(ap_user_identity, a_cntxt_indx, l_block_no);
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtInsertUserIdenVsCntxtMapEntry() with user_identity=%s,cntxt_index=%u,block_no=%u,ret_val=%d",
		ap_user_identity, a_cntxt_indx, l_block_no, l_ret_val);
	
	return l_ret_val;
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtGetUserOrderedSrvcList()
 *
 * DESCRIPTION   : This function is called by application to get list of services for a user.
 *									if user data is not in present message is sent to worker to send UDR.
 *
 * INPUT         : user_identity, sess_case(orig/term),a_sip_method,a_appl_token, RtTransAddr  
 *
 * OUTPUT        : RtShUDIOrderedList 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtGetUserOrderedSrvcList(RtS8T*   ap_user_identity,	/* Input */ 
						  RtShUDISessCase   a_sess_case,/* Input */ 									       RtSipMethodType   a_sip_method,/* Input */                                                                           RtShUDIApplToken  a_appl_token,/* Input */										 RtTransAddr*      ap_app_addr, /* Input - incase data is not available with UDI at that instant and reply will be sent later*/ 														RtShUDIOrderedArr&		ar_ordered_arr,   /* Ouput, Valid incase Return value is RT_SUCCESS*/																						     
                                                  RtS8T*                ap_entprise_id,   /* Optional Input it can be NULL */ 
                                                  RtS8T*                ap_cug_id)        /* Optional Input it can be NULL */ 

{

	//Tag_hotfix_genesysPF_152 - START
	RtU8T l_cnt = 0;
	while(!m_udi_init_complete)
	{
		if(l_cnt < 6000)
		{
			poll(0,0,10);
			l_cnt++;
		}
	}
	
	if(!m_udi_init_complete)
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"ENTER/EXIT RtShUserDataIntf :: rtGetUserOrderedSrvcList() with user_identity=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu return FAILURE as udi module not initialized",
			ap_user_identity, a_sess_case, a_sip_method, a_appl_token);
	
		return RT_FAILURE;
	}

	//Tag_hotfix_genesysPF_152 - END

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtGetUserOrderedSrvcList() with user_identity=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu",
		ap_user_identity, a_sess_case, a_sip_method, a_appl_token);

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtGetUserOrderedSrvcList() INfo ap_entprise_id=%s,ap_cug_id=%s,a_appl_token=%lu",
		ap_entprise_id, ap_cug_id, a_appl_token);
		
	RtRcT 						l_func_ret_val  = RT_SUCCESS;
	RtRcT 						l_rval 			= RT_SUCCESS;
	RtU32T						l_block_no;
	RtU32T						l_cntxt_id	   = 0;
	RtU32T            l_cug_cntxt_id = 0;
	RtTransMsg				l_trans_msg;
	RtShUDIUserCntxt* lp_cntxt;
	memset(&l_trans_msg,0,sizeof(RtTransMsg));
		
	//:	Bidhu
	//mp_sys_agent	=	NULL;
	mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetOrderedListCalled);
	
	//SHASHI_26042012: Validation check
	
	if(	(ap_user_identity	!= NULL) 
						&& 
			((a_sip_method / RT_SIP_METHOD_MAX) ==	0)
						&&
			((a_sess_case	/ RT_SH_MAX_SESS_CASE)	==	0)
		)
	{
		//OK let us proceed
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"rtGetUserOrderedSrvcList() Parameter ValidationFAILED with user_identity=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u",
			ap_user_identity, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id);
			
		return RT_SH_UDI_ERR_BAD_PARAMETER;	
		
	}
	
	RtShUDIProcReq* lp_udi_proc_req = (RtShUDIProcReq*)l_trans_msg.msg_buffer;
	lp_udi_proc_req->opcode = RT_O_SH_UDI_APPL_MSG;
	//lp_udi_proc_req->req_body.appl_msg.appl_req.cntxt_indx	= lp_cntxt->session_cookie.self_indx;
	lp_udi_proc_req->req_body.appl_msg.opcode = RT_O_SH_UDI_APPL_DOWNLOAD_REQ;//SHASHI_ADDED
	lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.sess_case 	= a_sess_case;
	lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.sip_method	= a_sip_method;
	lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token	= a_appl_token;
	lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.trans_addr	= *ap_app_addr;
	
	strcpy(lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id,ap_user_identity);//Added by RAJENDER
	strncpy(lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.enterprise_id,ap_entprise_id,sizeof(lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.enterprise_id));//Added by RAJENDER
	strncpy(lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cug_id,ap_cug_id,sizeof(lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cug_id));//Added by RAJENDER
	
	
	
	 /*------------------------------------------------------------------------------------------------------------------
	 -             CUG SRVC DATA HANDLING CHANGES by RAJENDER
	 -  1. Check whether CUG_SRVC_DATA is loaded
	 -  2. If loaded then proceed with user_identity
	 -  3. else user_req will be processed later by CUG UDA handler( In this case UDR is initiated for CUG-SRVC-DATA )
	 --------------------------------------------------------------------------------------------------------------------*/
   l_func_ret_val	=	rtHandleCugSrvcData(ap_entprise_id,ap_cug_id,lp_udi_proc_req);
	
	 
	 if(l_func_ret_val != RT_SUCCESS &&  l_func_ret_val != RT_SH_UDI_WAIT_FOR_RSP)
	 {
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					"rtGetUserOrderedSrvcList() ERROR rtHandleCugSrvcData() failed l_ret=%d for ap_entprise_id=%s ap_cug_id=%s user_identity=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu",
					l_func_ret_val,ap_entprise_id,ap_cug_id,ap_user_identity, a_sess_case, a_sip_method, a_appl_token);
	 }
	 else if(l_func_ret_val == RT_SH_UDI_WAIT_FOR_RSP)
	 {
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtGetUserOrderedSrvcList() rtHandleCugSrvcData() returnd RT_SH_UDI_WAIT_FOR_RSP for ap_entprise_id=%s ap_cug_id=%s So not handling user_identity=%s req NOW ,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu",
					ap_entprise_id,ap_cug_id,ap_user_identity, a_sess_case, a_sip_method, a_appl_token);
	 
	 }
	 else  /* proceed for usr */
	 {
		  l_cug_cntxt_id = lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx;
	    lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx = 0;

			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtGetUserOrderedSrvcList(),rtHandleCugSrvcData() returnd SUCCESS cug cntxt_indx=%u for ap_entprise_id=%s user_identity=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu",
					l_cug_cntxt_id,ap_entprise_id==NULL?" ":ap_entprise_id,ap_user_identity, a_sess_case, a_sip_method, a_appl_token);
	 
		 /*-----------------------------------------------------------------------------------------------------------------------------------------
		 -     MULTI-CUG changes by RAJENDER
		 - 1.Context is created for user_identity,enterprise_id and cug_id combination, We are using '_' as a delimiter while key genration
		 -----------------------------------------------------------------------------------------------------------------------------------------*/
  	 RtS8T l_usr_ent_cug_id[3*RT_MAX_STRING_LEN] = {'\0'};
		 snprintf(l_usr_ent_cug_id,sizeof(l_usr_ent_cug_id),"%s_%s_%s",ap_user_identity,ap_entprise_id,ap_cug_id);

		 
		 l_rval	= rtFindCntxtForUserIden(l_usr_ent_cug_id, l_cntxt_id);	

		 if(RT_SUCCESS ==	l_rval)
		 {
			 if ( RT_SUCCESS != mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt) )
			 {
				 //shall not happen
				 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					 "rtGetUserOrderedSrvcList() mp_user_cntxt_mgr->rtRetrieveCntxtData() FAILED with l_usr_ent_cug_id=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u",
					 l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id);

				 // get new context and set state and processing state 
				 l_rval = mp_user_cntxt_mgr->rtGetNewCntxt(&lp_cntxt);

				 if( RT_SUCCESS != l_rval )
				 {
					 rtEraseUserIdenVsCntxtMapEntry(l_usr_ent_cug_id);

					 l_func_ret_val = RT_SH_UDI_ERR_USER_CNTXT_FULL;

					 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						 "rtGetUserOrderedSrvcList() user context FULL with l_usr_ent_cug_id=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu",
						 l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token);
				 }
				 else
				 {
					 //tag_Db erase first then update cntxt id 
					 RtU32T		l_block_no	= 0;
					 rtDecideBlock(l_usr_ent_cug_id, l_block_no);
					 rtEraseUserIdenVsCntxtMapEntry(l_usr_ent_cug_id,l_block_no);
					 l_cntxt_id												=	lp_cntxt->session_cookie.self_indx;

						string l_user_eid_cid_id(l_usr_ent_cug_id);
						lp_cntxt->alias_ids_list.push_back(l_user_eid_cid_id);

						l_rval  = rtInsertUserIdenVsCntxtMapEntry(l_usr_ent_cug_id,lp_cntxt->session_cookie.self_indx);

						if( RT_SUCCESS != l_rval )
						{

							//shall not happen
							l_func_ret_val = RT_SH_UDI_ERR_IN_INTRNL_MAP;

							mp_user_cntxt_mgr->rtReturnCntxt(lp_cntxt->session_cookie.self_indx);

								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
									"RtShUserDataIntf ::rtInsertUserIdenVsCntxtMapEntry  l_usr_ent_cug_id [%s],a_sess_case[%u],a_sip_method[%d],a_appl_token[%ld],l_cntxt_id[%u]",
									l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id);					
						}
						else
						{

							//l_cntxt_id												=	lp_cntxt->session_cookie.self_indx;
							lp_cntxt->cntxt_state 						= RT_SH_UDI_CNTXT_NON_IDLE;
							lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROC_SRVC_APPL_REQ;
              lp_cntxt->cug_cntxt_id            = l_cug_cntxt_id;

							lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx	= lp_cntxt->session_cookie.self_indx;


							lp_cntxt->processing_req.proc_req_list.push_front(*lp_udi_proc_req);
							lp_cntxt->processing_req.curr_req_iter	= lp_cntxt->processing_req.proc_req_list.begin();

				 		 //	mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

							/*--------------------------------------------------------------------------
							- BTAS ::  Distribution Db worker to process UDR Req
							-
							--------------------------------------------------------------------------*/
        			//l_rval = rtSendToWrkr(&l_trans_msg, l_cntxt_id);	

							l_rval = rtSendToDbWrkr(&l_trans_msg, l_cntxt_id);


							if(RT_SUCCESS != l_rval )
							{
								l_func_ret_val = RT_SH_UDI_ERR_IN_SNDING_WRKR;


								rtEraseUserIdenVsCntxtMapEntry(l_usr_ent_cug_id);
								mp_user_cntxt_mgr->rtReturnCntxt(l_cntxt_id);					


								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
									"RtShUserDataIntf ::rtGetUserOrderedSrvcList(): rtSendToWrkr() FAILED  l_usr_ent_cug_id [%s],a_sess_case[%u],a_sip_method[%d],a_appl_token[%ld],l_cntxt_id[%u]",
									l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id);					

							}
							else
							{
								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
									"rtGetUserOrderedSrvcList() appl msg dispatched to wrkr l_usr_ent_cug_id =%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u",
									l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id);

								mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
								l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
							}
					 }
				 }
			 }
			 else
			 {//user data retrieved
				 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					 "rtGetUserOrderedSrvcList() mp_user_cntxt_mgr->rtRetrieveCntxtData() SUCCESS with l_usr_ent_cug_id=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u",
					 l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id);

				 if(lp_cntxt->cntxt_flag & RT_SH_UDI_UDA_RECEIVED)//SHASHI_04062012 addition of UDA flag
				 {
				 	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					   "rtGetUserOrderedSrvcList() UDA recived Making OrderList with l_usr_ent_cug_id=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u",
					    l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id);

				 
					 //TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES - 27102012
	 // 				l_func_ret_val = rtCheckForMissedTimeoutFromDiaStack(lp_cntxt, ap_user_identity); 

					 //if(RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
					 //{
						 switch(a_sess_case)
						 {
							 case RT_SH_ORIG_CASE:
							 case RT_SH_TERM_CASE:
							 {
								 if( a_sip_method < RT_SIP_METHOD_FOR_CTF)
								 {

									 l_func_ret_val = mp_prep_ord_list(lp_cntxt, ar_ordered_arr, a_sess_case, a_sip_method);

									 //SHASHI_20082012 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetOrderedListRetSucc);

									 if( RT_SUCCESS != mp_cache_keeper->rtReCacheData(lp_cntxt->cache_id) )
									 {
										 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
										 "rtGetUserOrderedSrvcList() rtReCacheData failed in appl_srv_req l_usr_ent_cug_id=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u,cache_id=%lld",
										 l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id,lp_cntxt->cache_id);


										 RtCacheKeeperBuffer l_buffer;
										 bzero(&l_buffer,sizeof(l_buffer));
										 l_buffer.index = lp_cntxt->session_cookie.self_indx;

										 RtTransAddr 	l_self_addr;
										 memset(&l_self_addr,0,sizeof(l_self_addr));
										 RtMglIntf::rtGetInstance()->rtGetSelfAddress(l_self_addr);//SHASHI_20102012
										 l_self_addr.ee_id = RT_EE_SH_UDI_WRKR_BASE  + (l_buffer.index % m_num_wrkr);

										 if( RT_SUCCESS != mp_cache_keeper->rtCacheData(&l_buffer,&l_self_addr,lp_cntxt->cache_id) )
										 {
											 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
												 "rtGetUserOrderedSrvcList() rtReCacheData failed,rtCacheData also failed  in appl_srv_req l_usr_ent_cug_id=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u,cache_id=%lld",
												 l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id,lp_cntxt->cache_id);


										 }
										 else
										 {
											 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
												 "rtGetUserOrderedSrvcList() rtReCacheData failed,rtCacheData success  in appl_srv_req l_usr_ent_cug_id=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u,cache_id=%lld",
												 l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id,lp_cntxt->cache_id);
										 }

									 }//end of if(recache fails) 
									 else
									 {
										 //recache success; nothing to do
									 }

								 }
								 else
								 {
									 l_func_ret_val = RT_SH_UDI_ERR_INVALID_INPUT_DATA;

									 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
										 "rtGetUserOrderedSrvcList() INVALID METHOD NAME in appl_srv_req l_usr_ent_cug_id=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u",
										 l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id);
								 }
							 }break;

							 default:
							 {

								 l_func_ret_val = RT_SH_UDI_ERR_INVALID_INPUT_DATA;

								 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
									 "rtGetUserOrderedSrvcList() INVALID SESSION CASE in appl_srv_req l_usr_ent_cug_id=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u",
									 l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id);
							 }
						 }//end of switch
					 //}
				 }
				 else
				 {

				 	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					   "rtGetUserOrderedSrvcList() UDA NOT RECIVED Inserting in Pending Req for l_usr_ent_cug_id=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u",
					    l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, l_cntxt_id);


					 //TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES - 27102012
					 //l_func_ret_val = rtCheckForMissedTimeoutFromDiaStack(lp_cntxt, ap_user_identity); 

	 // 				if(RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
	 // 				{

						 ////SHASHI_04062012 addition of UDA flag :add req to pending queue
						 lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx	= 	l_cntxt_id;
						 lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);
						 //SHASHI_18102012 : counters
						 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiApplReqPushedInPend);

						 l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
	 // 				}


				 }

	 // 			if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
	 // 			{
					 //TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES
					 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
	 // 			}

			 }//user data retrieved
		 }//rtFindCntxtForUserIden
		 else
		 { //not found
			 l_rval = mp_user_cntxt_mgr->rtGetNewCntxt(&lp_cntxt);

			 if( RT_SUCCESS != l_rval )
			 {
				 l_func_ret_val = RT_SH_UDI_ERR_USER_CNTXT_FULL;

				 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					 "rtGetUserOrderedSrvcList() mp_user_cntxt_mgr->rtGetNewCntxt returned =%d for l_usr_ent_cug_id=%s,a_token=%lu",
					 l_rval, l_usr_ent_cug_id, a_appl_token);
			 }
			 else
			 {
				 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					 "rtGetUserOrderedSrvcList() rtGetNewCntxt success cntxt_id =%u for l_usr_ent_cug_id=%s,a_token=%lu",
					 lp_cntxt->session_cookie.self_indx, l_usr_ent_cug_id, a_appl_token);

				 lp_cntxt->cntxt_state 						 = RT_SH_UDI_CNTXT_NON_IDLE;
				 lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROC_SRVC_APPL_REQ;
         lp_cntxt->cug_cntxt_id            = l_cug_cntxt_id;
				 
				 string l_user_id(l_usr_ent_cug_id); 
				 lp_cntxt->alias_ids_list.push_back(l_user_id);

				 l_rval = rtInsertUserIdenVsCntxtMapEntry(l_usr_ent_cug_id,lp_cntxt->session_cookie.self_indx);

				 if( RT_SUCCESS != l_rval )
				 {

					 //shall not happen
					 l_func_ret_val = RT_SH_UDI_ERR_IN_INTRNL_MAP;

					 mp_user_cntxt_mgr->rtReturnCntxt(lp_cntxt->session_cookie.self_indx);

					 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						 "rtGetUserOrderedSrvcList() rtInsertUserIdenVsCntxtMapEntry FAILED for user_cntxt_id =%u,l_usr_ent_cug_id=%s,a_token=%lu",
						 lp_cntxt->session_cookie.self_indx, l_usr_ent_cug_id, a_appl_token);					
				 }
				 else
				 {
					 lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx	= lp_cntxt->session_cookie.self_indx;

					 lp_cntxt->processing_req.proc_req_list.push_front(*lp_udi_proc_req);
					 lp_cntxt->processing_req.curr_req_iter	= lp_cntxt->processing_req.proc_req_list.begin();

	 //				mp_user_cntxt_mgr->rtReleaseCntxtDataLock(lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);



					 /*--------------------------------------------------------------------------
					 - BTAS ::  Distribution Db worker to process UDR Req
					 -
					 ---------------------------------------------------------------*/
        	 //l_rval = rtSendToWrkr(&l_trans_msg, lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);	

					 l_rval = rtSendToDbWrkr(&l_trans_msg, lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);

					 if(RT_SUCCESS != l_rval )
					 {
						 l_func_ret_val = RT_SH_UDI_ERR_IN_SNDING_WRKR;

						 //HP_CR(DONE):: call rtEraseUserIdenVsCntxtMapEntry , mp_user_cntxt_mgr->rtReturnCntxt(l_cntxt_id);

						 rtEraseUserIdenVsCntxtMapEntry(ap_user_identity);
						 mp_user_cntxt_mgr->rtReturnCntxt(lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);		

						 //HP_CR(DONE):: give critical logs

						 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
							 "RtShUserDataIntf ::rtGetUserOrderedSrvcList(): rtSendToWrkr() FAILED  l_usr_ent_cug_id [%s],a_sess_case[%u],a_sip_method[%d],a_appl_token[%ld],l_cntxt_id[%u]",
							 l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);										
					 }
					 else
					 {
						 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
							 "rtGetUserOrderedSrvcList() appl msg dispatched to wrkr l_usr_ent_cug_id =%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu,l_cntxt_id=%u",
							 l_usr_ent_cug_id, a_sess_case, a_sip_method, a_appl_token, lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);

						 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(lp_udi_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);
						 l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
					 }

				 }

			 }
		 }/* ----- end of usr not found handling case -----  */
	}/* -----  end of handleCugsrvcData ret value success case -----  */
	if(l_func_ret_val	==	RT_SUCCESS)
	{
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetOrderedListRetSucc);
	}
	else if(l_func_ret_val == RT_SH_UDI_WAIT_FOR_RSP)
	{
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetOrderedListWaitRsp);
	}
	else
	{
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetOrderedListRetFail);
	}
   
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtGetUserOrderedSrvcList() LEAVE with l_func_ret_val=%d,num_ordered_elem=%d for uid=%s eid=%s cid=%s,a_sess_case=%u,a_sip_method=%d,a_appl_token=%lu",
		l_func_ret_val,ar_ordered_arr.num_ordered_elem,ap_user_identity,ap_entprise_id,ap_cug_id, a_sess_case, a_sip_method, a_appl_token);
	
	return l_func_ret_val;
}

//TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES - 27102012
// /*******************************************************************************
//  *
//  * FUNCTION NAME : rtCheckForMissedTimeoutFromDiaStack()
//  *
//  * DESCRIPTION   : This function is called by application in order to evaluate whether stack has missed
//  *                 any timeout for UDR/SNR/PNR sent earlier.
//  *
//  * INPUT         : user_identity, sess_case(orig/term),a_sip_method,a_appl_token, RtTransAddr  
//  *
//  * OUTPUT        : 
//  *
//  * RETURN        : RtRcT 
//  *
//  ******************************************************************************/
// RtRcT RtShUserDataIntf ::rtCheckForMissedTimeoutFromDiaStack(RtShUDIUserCntxt*  ap_cntxt, RtS8T* ap_user_identity) 
// {
// 	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
// 		"rtCheckForMissedTimeoutFromDiaStack(ENTER) - Context blocked in UDA/SNA/PUA response/timeout(expected from stack)- appl_srv_req user_identity=%s,l_cntxt_id=%u,cntxt_processing_state=%d",
// 					ap_user_identity, ap_cntxt->session_cookie.self_indx,ap_cntxt->cntxt_processing_state);
// 
// 
// 	RtRcT l_ret_val = RT_SUCCESS;
// 	
// 	if( (RT_SH_UDI_CNTXT_IDLE != ap_cntxt->cntxt_state )
// 				       &&
// 			(
//  			 ( RT_SH_UDI_WAIT_FOR_UDA == ap_cntxt->cntxt_processing_state)
// 				   ||
// 			 ( RT_SH_UDI_WAIT_FOR_PUA == ap_cntxt->cntxt_processing_state)
// 				   ||
// 			 ( RT_SH_UDI_WAIT_FOR_SNA== ap_cntxt->cntxt_processing_state)
// 			     ||
// 				( RT_SH_UDI_WAIT_FOR_UNSUBSCRIBE_SNA== ap_cntxt->cntxt_processing_state)
// 				   ||	
// 				( RT_SH_UDI_WAIT_FOR_UNSUBSCRIBE_SNA_PUA_OUT_OF_SYNC_RECEIVED== ap_cntxt->cntxt_processing_state)	  
// 			)
// 		)
// 	{
// 		//TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES
// 		struct timeval              l_current_time;
// 		gettimeofday(&l_current_time, NULL);
// 
// 		RtU32T l_time_diff = (l_current_time.tv_sec*1000000 + l_current_time.tv_usec) - (ap_cntxt->activity_time.tv_sec*1000000 + ap_cntxt->activity_time.tv_usec);
// 
// 		RtU32T l_sh_msg_timer_dur ;
// 		if(		RT_SH_UDI_WAIT_FOR_UDA == ap_cntxt->cntxt_processing_state)
// 		{
// 			l_sh_msg_timer_dur = mp_sys_agent->rtGetRtU32T(RT_SH_UDR_TIMER_DUR);
// 		}
// 		else if( (RT_SH_UDI_WAIT_FOR_SNA== ap_cntxt->cntxt_processing_state)||(RT_SH_UDI_WAIT_FOR_UNSUBSCRIBE_SNA== ap_cntxt->cntxt_processing_state)||(RT_SH_UDI_WAIT_FOR_UNSUBSCRIBE_SNA_PUA_OUT_OF_SYNC_RECEIVED== ap_cntxt->cntxt_processing_state))
// 		{
// 			l_sh_msg_timer_dur = mp_sys_agent->rtGetRtU32T(RT_SH_SNR_TIMER_DUR);
// 		
// 		}
// 		else
// 		{
// 			l_sh_msg_timer_dur = mp_sys_agent->rtGetRtU32T(RT_SH_PUR_TIMER_DUR);
// 		}
// 
// 		if(l_time_diff > ( l_sh_msg_timer_dur*1000000 + m_delta_stack_wait_time*1000000))
// 		{
// 			//This scenario happens whenever stack doesn't give response/timeout for UDR
// 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 					"rtCheckForMissedTimeoutFromDiaStack() - Context blocked in UDA/SNA/PUA response/timeout(expected from stack)- appl_srv_req user_identity=%s,l_cntxt_id=%u,cntxt_processing_state=%d",
// 					ap_user_identity, ap_cntxt->session_cookie.self_indx,ap_cntxt->cntxt_processing_state);
// 
// 			printf("rtCheckForMissedTimeoutFromDiaStack() - Context blocked in UDA/SNA/PUA response/timeout(expected from stack)- appl_srv_req user_identity=%s,l_cntxt_id=%u,cntxt_processing_state=%d\n",
// 					ap_user_identity, ap_cntxt->session_cookie.self_indx,ap_cntxt->cntxt_processing_state); fflush(NULL);
// 
// 			mp_wrkr_arr[0]->rtFlushAndReturnCntxt(ap_cntxt, RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD);		
// 
// 			l_ret_val = RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD;
// 		}
// 	}
// 
// 	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
// 					"rtCheckForMissedTimeoutFromDiaStack(LEAVE) - Context blocked in UDA/SNA/PUA response/timeout(expected from stack)- appl_srv_req user_identity=%s,l_cntxt_id=%u,cntxt_processing_state=%d,l_ret_val=%d",
// 					ap_user_identity, ap_cntxt->session_cookie.self_indx,ap_cntxt->cntxt_processing_state,l_ret_val);
// 
// 	
// 	
// 	return l_ret_val;
// 
// }
/*******************************************************************************
 *
 * FUNCTION NAME : rtSendToWrkr()
 *
 * DESCRIPTION   : HP_CR(DONE)::This function is used by the UDI interface to forward the 
 *                 application request or provisioning request to the RtShUDIWrkr class when
 *								 not present in UDI.
 *
 * INPUT         : RtTransMsg* ap_trans_msg, RtU32T a_cntxt_id
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtSendToWrkr(RtTransMsg* ap_trans_msg, RtU32T a_cntxt_id)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"ENTER RtShUserDataIntf :: rtSendToWrkr() with ap_trans_msg=%p, cntxt_index=%u",
	ap_trans_msg, a_cntxt_id);

	RtRcT l_ret_val = RT_FAILURE;
	
	if(ap_trans_msg->msg_type != RT_M_SH_INTF_MSG) //FOR UDA case msg_type is set by DbUdiWorkr
	   ap_trans_msg->msg_type = RT_M_SH_UDI_APPL_MSG;
		
	l_ret_val = mp_wrkr_arr[a_cntxt_id % m_num_wrkr]->rtPutMsg(ap_trans_msg);
	
	
	if(RT_SUCCESS !=	l_ret_val)
	{
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiSendToWrkrFailed);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"rtSendToWrkr() ERROR: FAILED sending to wrkr thread with ap_trans_msg=%p, cntxt_index=%u",
		ap_trans_msg, a_cntxt_id);		
	}
	else
	{
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiSendToWrkrSuccess);
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"EXIT RtShUserDataIntf :: rtSendToWrkr() with ap_trans_msg=%p, cntxt_index=%u, ret_val=%d",
	ap_trans_msg, a_cntxt_id, l_ret_val);
	
	return l_ret_val;
}
/*******************************************************************************
 *
 * FUNCTION NAME : rtSendToDbWrkr()
 *
 * DESCRIPTION   : This function is used by the UDI interface to forward the 
 *                 application request  to the RtDbUDIWrkr class 
 *								 To process UDR using DB
 *
 * INPUT         : RtTransMsg* ap_trans_msg, RtU32T a_cntxt_id
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtSendToDbWrkr(RtTransMsg* ap_trans_msg, RtU32T& a_cntxt_id)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	  "RtShUserDataIntf::rtSendToDbWrkr() with ap_trans_msg=%p,msg_type=%d,cntxt_index=%u",
	  ap_trans_msg,ap_trans_msg->msg_type, a_cntxt_id);

  ap_trans_msg->msg_type = RT_M_SH_UDI_APPL_MSG;
			
	RtRcT l_ret_val = mp_dbi_wrkr_arr[a_cntxt_id % m_num_dbi_wrkr]->rtPutMsg(ap_trans_msg);
	
	
	if(RT_SUCCESS !=	l_ret_val)
	{
		/*-------------------------------------------------------------------------------------------------------
		- 
		-    BTAS TBD : New counters for Db Worker are not defined, Using existing counters
		-
		---------------------------------------------------------------------------------------------------------*/
		
		
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiSendToWrkrFailed);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf::rtSendToDbWrkr() ERROR: FAILED sending to wrkr thread with ap_trans_msg=%p, cntxt_index=%u",
		   ap_trans_msg, a_cntxt_id);		
	}
	else
	{
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiSendToWrkrSuccess);
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	   "RtShUserDataIntf :: rtSendToDbWrkr() with msg_type=%d, cntxt_index=%u, ret_val=%d",
	     ap_trans_msg->msg_type, a_cntxt_id, l_ret_val);
	
	return l_ret_val;
}
																							
																														
/*******************************************************************************
 *
 * FUNCTION NAME : rtGetUserData()
 *
 * DESCRIPTION   : This function is called by application service to get udi_ref.
 *									if user_identity is found in map it gives udi_ref ptr.
 *
 * INPUT         : ap_user_identity, a_udi_ref
 *
 * OUTPUT        : ap_udi_ref_data 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtGetUserData(  RtS8T*        						ap_user_identity,	/* Input */
																				 RtShUDIRef								a_udi_ref,				/* Input */
																				 RtS8T*                		ap_entprise_id, 
																				 RtS8T*                		ap_cug_id,
																				 void* 										ap_udi_ref_data) /* Ouput, Valid incase Return value is RT_SUCCESS*/
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"ENTER RtShUserDataIntf :: rtGetUserData() with ap_user_identity=%s, a_udi_ref=%u",
	ap_user_identity, a_udi_ref);
	
	RtRcT 						l_ret_val 	= RT_FAILURE;
	RtU32T						l_cntxt_id	= 0;
	RtShUDIUserCntxt*	lp_cntxt;

	mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetUserDataCalled);	
	//DB_15012013 Validation check	
	if((ap_user_identity	!= NULL)&&(ap_udi_ref_data!= NULL) && (NULL != ap_cug_id) && (NULL != ap_entprise_id))
	{
		//OK let us proceed
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			" RtShUserDataIntf :: rtGetUserData() Parameter ValidationFAILED with with ap_user_identity=%s, a_udi_ref=%u,ap_udi_ref_data=%p",ap_user_identity, a_udi_ref, ap_udi_ref_data);
			
		return RT_FAILURE;	
		
	}
	//DB_15012013 Validation check
	
	//HP_CR(DONE):: validation check on 	a_udi_ref against <= (m_max_data_ref + m_max_data_repository)
	//if failure , return from here with approprate value
	
  RtS8T l_usr_ent_cug_id[3*RT_MAX_STRING_LEN] = {'\0'};
	snprintf(l_usr_ent_cug_id,sizeof(l_usr_ent_cug_id),"%s_%s_%s",ap_user_identity,ap_entprise_id,ap_cug_id);

	if(a_udi_ref <= (m_max_data_ref + m_max_data_repository))
	{
		l_ret_val	= rtFindCntxtForUserIden(l_usr_ent_cug_id, l_cntxt_id);
		if(RT_SUCCESS ==	l_ret_val)
		{
			l_ret_val = mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id,	&lp_cntxt);
			
			if(RT_SUCCESS ==	l_ret_val)//SHASHI_04062012 addition of UDA flag
			{
					if(lp_cntxt->cntxt_flag & RT_SH_UDI_UDA_RECEIVED)
					{
						l_ret_val =mp_data_pool_mgr->rtCopyPoolElem(a_udi_ref,lp_cntxt->p_data_ref_arr[a_udi_ref].elem_indx,ap_udi_ref_data);
						if(RT_SUCCESS ==	l_ret_val)
						{
							mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetUserDataRetSucc);

							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
								"rtGetUserData() mp_data_pool_mgr->rtRetrievePoolElem() success with ap_user_identity=%s, ap_cug_id = %s, ap_eid = %s, a_udi_ref=%u,pool_elem =%u",
								ap_user_identity, ap_cug_id, ap_entprise_id, a_udi_ref,lp_cntxt->p_data_ref_arr[a_udi_ref].elem_indx);	
								
						  //JFT
// 							RtS8T 			l_srvc_indic[RT_DIA_SH_MAX_SRV_INDICATION_LEN];
// 					    mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
// 								"rtGetUserData() Calling PrintSrvcDataa_udi_ref=%u,pool_elem =%u",
// 								a_udi_ref,lp_cntxt->p_data_ref_arr[a_udi_ref].elem_indx);	
// 
// 							memset(l_srvc_indic,0,RT_DIA_SH_MAX_SRV_INDICATION_LEN);
// 
// 						  RtU32T	l_app_enum = 0;
// 					
// 				     ( RtShUDIDataPoolMgr :: rtGetInstance())->rtGetSrvIndAppEnumFromUdiRef(a_udi_ref, l_srvc_indic, l_app_enum);
// 
// 							rtPrintSrvcData(l_app_enum,ap_udi_ref_data);
// 
							//JFT			

						}
						else
						{
							mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetUserDataRetFailed);

							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
								"rtGetUserData() mp_data_pool_mgr->rtRetrievePoolElem() returned FAILURE with ap_user_identity=%s, ap_cug_id = %s, ap_eid = %s, a_udi_ref=%u,pool_elem =%u",
								ap_user_identity, ap_cug_id, ap_entprise_id, a_udi_ref,lp_cntxt->p_data_ref_arr[a_udi_ref].elem_indx);				
						}

						RtRcT l_ret_val_tmp	= mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
						if(RT_SUCCESS ==	l_ret_val_tmp)
						{

							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
								"rtGetUserData() mp_user_cntxt_mgr->rtReleaseCntxtDataLock returned SUCCESS with ap_user_identity=%s, ap_cug_id = %s, ap_eid = %s, a_udi_ref=%u,l_ret_val_tmp=%d",
								ap_user_identity, ap_cug_id, ap_entprise_id, a_udi_ref,l_ret_val_tmp);		
						}
						else
						{
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
								"rtGetUserData() mp_user_cntxt_mgr->rtReleaseCntxtDataLock returned FAILURE with ap_user_identity=%s, ap_cug_id = %s, ap_eid = %s, a_udi_ref=%u,l_ret_val_tmp=%d",
								ap_user_identity, ap_cug_id, ap_entprise_id, a_udi_ref,l_ret_val_tmp);				
						}
				 }
				 else
				 {
						mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetUserDataRetFailed);
						mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
						
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
							"rtGetUserData() mp_user_cntxt_mgr->rtRetrieveCntxtData() returned FAILURE or UDA not received yet with ap_user_identity=%s, ap_cug_id = %s, ap_eid = %s, cntxt_id=%u",
							ap_user_identity, ap_cug_id, ap_entprise_id,l_cntxt_id);

						l_ret_val	=	RT_FAILURE;							
				 
				 }
			}
			else
			{
				mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetUserDataRetFailed);
				
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					"rtGetUserData() mp_user_cntxt_mgr->rtRetrieveCntxtData() returned FAILURE  with ap_user_identity=%s, ap_cug_id = %s, ap_eid = %s ",
					ap_user_identity, ap_cug_id, ap_entprise_id);
				
				l_ret_val	=	RT_FAILURE;							
			}
		}
		else	
		{

			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtGetUserData() ERROR user id is not found in map ap_user_identity=%s, ap_cug_id = %s, ap_eid = %s return RT_SH_UDI_ERR_USER_UNKNOWN",
					ap_user_identity, ap_cug_id, ap_entprise_id);	
			
			l_ret_val	=	RT_SH_UDI_ERR_USER_UNKNOWN;
			mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetUserDataRetFailed);

		}
	}
	else
	{
		l_ret_val = RT_SH_UDI_ERR_INVALID_DATA_REF;
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetUserDataRetFailed);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"rtGetUserData() Invalid Udi_ref User_identity[%s], ap_cug_id = %s, ap_eid = %s, udi_ref[%u]",
			ap_user_identity, ap_cug_id, ap_entprise_id,a_udi_ref);						
	}
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"EXIT RtShUserDataIntf :: rtGetUserData() with ap_user_identity=%s, ap_cug_id = %s, ap_eid = %s, a_udi_ref=%u,ret_val =%d",
	ap_user_identity, ap_cug_id, ap_entprise_id, a_udi_ref,l_ret_val);

	return l_ret_val;
}


/*******************************************************************************
 *
 * FUNCTION NAME : rtProcessShUserDataRcvdinUDA()
 *
 * DESCRIPTION   : This function is called to process the Sh user data received from HSS in UDA
 *
 * INPUT         : RtShUDIUserCntxt* ,RtShUserData* 
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtProcessShUserDataRcvdinUDA(RtShUDIUserCntxt* ap_cntxt,RtShUserData* ap_sh_user_data)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"ENTER RtShUserDataIntf :: rtProcessShUserDataRcvdinUDA() with usr_indx=%u num_repository_data=%d",ap_cntxt->session_cookie.self_indx,ap_sh_user_data->num_repository_data);


	RtRcT 		    			l_fun_ret_val   				= RT_SUCCESS; 
	RtBoolT 						l_processing_successful = true;
	RtShUDIDataRefList	l_data_ref_list;
	RtU8T 							l_udi_ref,l_data_ref;
	l_data_ref_list.clear();

	mp_data_pool_mgr->rtGetRegDataRefList(l_data_ref_list);

	//- iterate through each element of l_data_ref_list

	//for (each list element) and (as long as l_processing_successful == TRUE)
	while((!l_data_ref_list.empty())	&& (l_processing_successful ==	true))
	{
		l_data_ref	= l_data_ref_list.front();
		//HP_CR(DONE):: nowhere in code USE "0", use corresponding enum/hashdef
		if(RT_DIA_SH_REPOSITORY_DATA	==	l_data_ref) //data reference is ZERO
		{
			for(RtU32T l_cnt=0;l_cnt < ap_sh_user_data->num_repository_data;l_cnt++)
			{
				if((ap_sh_user_data->p_reporitory_data_arr[l_cnt].flag & RT_SH_SERVICE_DATA_PRESENT)	&&
						(ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_data_len > 0))
				{						
					rtGetUDIRefFromSrvcInd(ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_indication.val, l_udi_ref);

					l_fun_ret_val = mp_data_pool_mgr->rtStoreDataRepositoryData(l_udi_ref, 
																																(void*) ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_data.p_val,
																																ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_data_len,
																																ap_cntxt->p_data_ref_arr[l_udi_ref].elem_indx,
																																&ap_cntxt->p_data_ref_arr[l_udi_ref].p_data_ptr);
					if (RT_SUCCESS == l_fun_ret_val)
					{
						ap_cntxt->p_data_ref_arr[l_udi_ref].data_state  	|= 	RT_SH_UDI_DATA_STATE_VALID;

						//tag_amit_ut_changes
						ap_cntxt->p_data_ref_arr[l_udi_ref].old_seq_no				= 	ap_cntxt->p_data_ref_arr[l_udi_ref].seq_no;
						ap_cntxt->p_data_ref_arr[l_udi_ref].seq_no						= 	ap_sh_user_data->p_reporitory_data_arr[l_cnt].seq_no;
	

						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
							"rtProcessShUserDataRcvdinUDA() mp_data_pool_mgr->rtStoreDataRepositoryDatawith success, usr_indx=%u,udi_ref=%u",
							ap_cntxt->session_cookie.self_indx, l_udi_ref);							
					}
					else
					{
						l_processing_successful = false;
						break;						
					}					
				}
				else
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
						"rtProcessShUserDataRcvdinUDA() service data is not present or empty,service_data_len=%d,flag=%d,  usr_indx=%u,loop_index=%u",
						ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_data_len,ap_sh_user_data->p_reporitory_data_arr[l_cnt].flag,ap_cntxt->session_cookie.self_indx, l_cnt);							

				}

			}//end of loop

		}
		else
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"rtProcessShUserDataRcvdinUDA()  calling rtCheckNStoreNonTransData in UDA, usr_indx=%u",
				ap_cntxt->session_cookie.self_indx);							

			if(RT_SUCCESS !=	rtCheckNStoreNonTransData(l_data_ref, ap_cntxt, ap_sh_user_data))
			{
				l_processing_successful = false;
			}
			else
			{
				//its OK
				//HP_CR(DONE):debug logs
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"rtProcessShUserDataRcvdinUDA(): rtCheckNStoreNonTransData Success usr_indx=%u udi_ref =%u",
				ap_cntxt->session_cookie.self_indx, l_data_ref);							
			}

		}//end of else

		l_data_ref_list.pop_front();
	}//end of while

	if( ! l_processing_successful )
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
			"rtProcessShUserDataRcvdinUDA() FAILURE in updating data in pool hence deleting all udi ref for usr_indx=%u",
			ap_cntxt->session_cookie.self_indx);							


		for(RtU32T	l_cnt = 0; l_cnt <= (m_max_data_ref + m_max_data_repository);l_cnt++)
		{
			if(RT_SH_UDI_DATA_STATE_VALID & ap_cntxt->p_data_ref_arr[l_cnt].data_state)
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtProcessShUserDataRcvdinUDA() deleting  udi ref = %d for elem_index=%u",
					l_cnt, ap_cntxt->p_data_ref_arr[l_cnt].elem_indx);							
				
				mp_data_pool_mgr->rtReturnPoolElem(l_cnt, ap_cntxt->p_data_ref_arr[l_cnt].elem_indx);

				ap_cntxt->p_data_ref_arr[l_cnt].data_state  	&= ~RT_SH_UDI_DATA_STATE_VALID;
				ap_cntxt->p_data_ref_arr[l_cnt].p_data_ptr		= NULL;
				ap_cntxt->p_data_ref_arr[l_cnt].elem_indx 		= 0;

			}
			else
			{
				//NOTHING
			}
		}
		
	}


	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"EXIT RtShUserDataIntf :: rtProcessShUserDataRcvdinUDA() with usr_indx=%u,ret_val=%d",ap_cntxt->session_cookie.self_indx,l_fun_ret_val);
		
	return l_fun_ret_val;
		
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtProcessShUserDataRcvdinPNR()
 *
 * DESCRIPTION   : This function is called to process the Sh user data received from HSS in PNR
 *
 * INPUT         : RtShUDIUserCntxt* ,RtShUserData* 
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT 
  									RT_SH_UDI_ERR_NO_SUBSCRIPTION_TO_DATA/RT_SH_UDI_ERR_IN_DATA_REF_UPD/
										RT_SH_UDI_ERR_IN_DATA_REF_ADD/RT_SUCCESS
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtProcessShUserDataRcvdinPNR(RtShUDIUserCntxt* ap_cntxt,RtShUserData* ap_sh_user_data)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"ENTER RtShUserDataIntf :: rtProcessShUserDataRcvdinPNR() with usr_indx=%u",ap_cntxt->session_cookie.self_indx);

	RtRcT 		    			l_rval  			  					= RT_SUCCESS; 
	RtRcT 		    			l_func_rval   						= RT_SUCCESS; 
	RtBoolT 						l_processing_successful 	= true;
	RtBoolT							l_update_in_srvc_data			=	false;
	RtBoolT							l_update_in_alias_data		=	false;
	RtBoolT							l_add_impu								=	false;
	
	RtShUDIDataRefList	l_data_ref_list;
	RtU8T 							l_udi_ref,l_data_ref;
	//tag_amit_ut_changes
	RtS8T 	l_srvc_indic[RT_DIA_SH_MAX_SRV_INDICATION_LEN];
	memset(l_srvc_indic,0,RT_DIA_SH_MAX_SRV_INDICATION_LEN);
					
	l_data_ref_list.clear();

	mp_data_pool_mgr->rtGetRegDataRefList(l_data_ref_list);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//NOTE: we have assumed that HSS will send data for those data references ONLY, for which we have subscribed//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	while(!l_data_ref_list.empty()	&&	l_processing_successful)
	{
		l_data_ref	= l_data_ref_list.front();
		
		if(RT_DIA_SH_REPOSITORY_DATA	==	l_data_ref) //data reference is ZERO
		{
			for(RtU32T l_cnt=0; ((l_cnt < ap_sh_user_data->num_repository_data) && (l_processing_successful));l_cnt++)
			{
				
				l_rval	= rtGetUDIRefFromSrvcInd(ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_indication.val, l_udi_ref);
//SHASHI_BUGFIX_AS_1971
/////////////////////////////////
// SHASHI_CR_31072012 removed as RT_SH_UDI_DATA_STATE_SNR_SENT flag is not being used
//((RT_SUCCESS ==	l_rval) && !(ap_cntxt->p_data_ref_arr[l_udi_ref].data_state & RT_SH_UDI_DATA_STATE_SNR_SENT)) )
//////////////////////////////////////
//HP: use mp_sys_agent->rtGetRtBoolT(RT_SH_UDI_SND_ONLY_VALID_DATA_IN_SNR) to check if subscribed or not
//
// RT_SH_UDI_SND_ONLY_VALID_DATA_IN_SNR if true means AS has subscribed only those services which it receives in UDA
//
				if(RT_SUCCESS !=	l_rval) 
				{
					l_func_rval  								= RT_SH_UDI_ERR_NO_SUBSCRIPTION_TO_DATA;		
					l_processing_successful 		= false;

								
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtProcessShUserDataRcvdinPNR() rtGetUDIRefFromSrvcInd() FAILED for usr_indx=%u service_indication=%s",
					ap_cntxt->session_cookie.self_indx,ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_indication.val);
				}
				else
				{
					if( (ap_cntxt->p_data_ref_arr[l_udi_ref].data_state & RT_SH_UDI_DATA_STATE_VALID)
										&&
							!(ap_cntxt->p_data_ref_arr[l_udi_ref].data_state & RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY)
						)
					{
						if( !(ap_sh_user_data->p_reporitory_data_arr[l_cnt].flag & RT_SH_SERVICE_DATA_PRESENT) 
										||
									(ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_data_len == 0)
							) 
						{//possible deletion case
							ap_cntxt->p_data_ref_arr[l_udi_ref].data_state &= ~RT_SH_UDI_DATA_STATE_VALID;

							mp_data_pool_mgr->rtReturnPoolElem(l_udi_ref,ap_cntxt->p_data_ref_arr[l_udi_ref].elem_indx);
							
							//Note: if service is mandatory, it will automatically created in rtCreateCaseOrderedListForUser
							mp_data_pool_mgr->rtCreateCaseOrderedListForUser(ap_cntxt);
							
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
							"rtProcessShUserDataRcvdinPNR() service data is deleted in HSS for usr_indx=%u,udi_ref=%u,elem_indx=%u",
							ap_cntxt->session_cookie.self_indx, l_udi_ref, ap_cntxt->p_data_ref_arr[l_udi_ref].elem_indx);

						}
						else
						{//possible updation case

							/////////////////////////////////////////////////////////////////////////////////////////////////////
							//NOTE:- Addition case is not possible as AS can only subscribe for valid Service Indication in SNR//
							/////////////////////////////////////////////////////////////////////////////////////////////////////
							l_rval = mp_data_pool_mgr->rtUpdDataRepositoryData(l_udi_ref,
																												ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_data.p_val,
																												ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_data_len,
																												ap_cntxt->p_data_ref_arr[l_udi_ref].p_data_ptr);

							if(RT_SUCCESS !=	l_rval)
							{
								//HP_CR(DONE): give INFO logs
								
								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
								"rtProcessShUserDataRcvdinPNR():rtUpdDataRepositoryData() FAILED  usr_indx=%u,udi_ref=%u,elem_indx=%u ",
								ap_cntxt->session_cookie.self_indx, l_udi_ref, ap_cntxt->p_data_ref_arr[l_udi_ref].elem_indx);								
								
								l_processing_successful = false;
								l_func_rval 						= RT_SH_UDI_ERR_IN_DATA_REF_UPD;
							
							}
							else
							{ 
								//tag_amit_ut_changes
								ap_cntxt->p_data_ref_arr[l_udi_ref].old_seq_no	= ap_cntxt->p_data_ref_arr[l_udi_ref].seq_no;
								ap_cntxt->p_data_ref_arr[l_udi_ref].seq_no			= ap_sh_user_data->p_reporitory_data_arr[l_cnt].seq_no;

								//HP_CR(DONE): give seq_no in logs
								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
								"rtProcessShUserDataRcvdinPNR() service data updated in pool for usr_indx=%u,udi_ref=%u,elem_indx=%u",
								ap_cntxt->session_cookie.self_indx, l_udi_ref, ap_cntxt->p_data_ref_arr[l_udi_ref].elem_indx);
							}
						}
						
						l_update_in_srvc_data = true;
					}
					else if(!mp_sys_agent->rtGetRtBoolT(RT_SH_UDI_SND_ONLY_VALID_DATA_IN_SNR))
					{
						//Either service is mandatory or has been created by other AS
						if((ap_sh_user_data->p_reporitory_data_arr[l_cnt].flag & RT_SH_SERVICE_DATA_PRESENT)	
													&&
								(ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_data_len > 0)
							)
						{						


							l_rval = mp_data_pool_mgr->rtStoreDataRepositoryData(l_udi_ref, 
																																		(void*) ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_data.p_val,
																																		ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_data_len,
																																		ap_cntxt->p_data_ref_arr[l_udi_ref].elem_indx,
																																		&ap_cntxt->p_data_ref_arr[l_udi_ref].p_data_ptr);
							if (RT_SUCCESS == l_rval)
							{
								ap_cntxt->p_data_ref_arr[l_udi_ref].data_state  	|= RT_SH_UDI_DATA_STATE_VALID;
								ap_cntxt->p_data_ref_arr[l_udi_ref].data_state 		&= ~RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY;//Tag_db 30092013 Reset as Non Mandatory for particular user.
								
								//tag_amit_ut_changes
								ap_cntxt->p_data_ref_arr[l_udi_ref].old_seq_no		= ap_cntxt->p_data_ref_arr[l_udi_ref].seq_no;
								ap_cntxt->p_data_ref_arr[l_udi_ref].seq_no				= ap_sh_user_data->p_reporitory_data_arr[l_cnt].seq_no;

								//Note: if service is mandatory, it will automatically created in rtCreateCaseOrderedListForUser
								mp_data_pool_mgr->rtCreateCaseOrderedListForUser(ap_cntxt);
							
							
								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
									"rtProcessShUserDataRcvdinPNR() mp_data_pool_mgr->rtStoreDataRepositoryData with success, usr_indx=%u,udi_ref=%u",
									ap_cntxt->session_cookie.self_indx, l_udi_ref);							
							}
							else
							{
								l_func_rval  								= RT_SH_UDI_ERR_IN_DATA_REF_ADD;
								l_processing_successful 		= false;
												
							}					
						}
						else
						{
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
								"rtProcessShUserDataRcvdinPNR() service data is not present or empty, usr_indx=%u,loop_index=%u",
								ap_cntxt->session_cookie.self_indx, l_cnt);							

						}
						
						l_update_in_srvc_data = true;
					}
					else
					{
						l_func_rval  								= RT_SH_UDI_ERR_NO_SUBSCRIPTION_TO_DATA;		
						l_processing_successful 		= false;


						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
						"rtProcessShUserDataRcvdinPNR() service is not subscribed for PNR for usr_indx=%u service_indication=%s",
						ap_cntxt->session_cookie.self_indx,ap_sh_user_data->p_reporitory_data_arr[l_cnt].service_indication.val);
	
					}
				}	
			}//end of for of repository data
		}//end of if(data_ref = 0)
		else
		{
			
			l_udi_ref = l_data_ref;
			
			
			if((RT_DIA_SH_IMS_PUBLIC_ID ==	l_data_ref) &&	(ap_sh_user_data->flag	& RT_SH_PUBLIC_ID_PRESENT))
			{
				///////////////////////////////////
				//Special Handling for Identities//
				///////////////////////////////////

				RtShIMSPublicIdentity* lp_public_id;
				void* lp_pub_id;

 				if(RT_SUCCESS	== mp_data_pool_mgr->rtPointToPoolElem(l_data_ref,ap_cntxt->p_data_ref_arr[l_data_ref].elem_indx, &lp_pub_id))
				{
					lp_public_id	=	(RtShIMSPublicIdentity*)lp_pub_id;
					//- check which flag is set in received PNR data for IMS Public Identities
					//HP_CR: flag for each identity type to be coded in RtUserData
					
					//HP_CR:
					//following coding to be done for each type of public identities
					// - check whether corresponding flag for public identity is set
					// - if set
					//		- update num-XXXXX 
					//		- update each ims_public_id in loop
					//		- give logs
					//	- else
					//		- give logs
					
					//HP_CR: give DEBUG logs in below code
					
					if(ap_sh_user_data->public_id.num_ims_public_id !=	0)
					{
						//- update registration data structure through lp_buffer
						lp_public_id->num_ims_public_id	=	ap_sh_user_data->public_id.num_ims_public_id;
						
						RtU32T	l_cntr;
						for(l_cntr	= 0;	l_cntr	< lp_public_id->num_ims_public_id; ++l_cntr)
						{	
							strcpy(lp_public_id->ims_public_id[l_cntr].val, ap_sh_user_data->public_id.ims_public_id[l_cntr].val);
						}
						//HP_CR(Done): update num_ims_public_id with new receievd value
							
					}
					else // not used if(ap_sh_user_data->public_id.deletion_flag	& RT_SH_DELETE_PUBLIC_ID)
					{
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
								"rtProcessShUserDataRcvdinPNR() num_ims_public_id = 0 hence do nothing , usr_indx=%u",
								ap_cntxt->session_cookie.self_indx);							
					}

					if(ap_sh_user_data->public_id.num_all_public_id !=	0)
					{
						//- update ALL data structure through lp_buffer
						lp_public_id->num_all_public_id	=	ap_sh_user_data->public_id.num_all_public_id;
						
						RtU32T	l_cntr;
						for(l_cntr	= 0;	l_cntr	< lp_public_id->num_all_public_id; l_cntr++)
						{
							strcpy(lp_public_id->all_public_id[l_cntr].val, ap_sh_user_data->public_id.all_public_id[l_cntr].val);
						}
						//HP_CR(Done): update num_XXX_public_id with new receievd value

					}
					else
					{
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
								"rtProcessShUserDataRcvdinPNR() num_all_public_id = 0 hence do nothing , usr_indx=%u",
								ap_cntxt->session_cookie.self_indx);							
					
					}
					
					if(ap_sh_user_data->public_id.num_alias_public_id !=	0)
					{

						
						//HP_CR(Done): call rtEraseUserIdenVsCntxtMapEntry(RtS8T* ap_user_identity); for each entry in ap_cntxt->alias_ids_list
						
						while(!ap_cntxt->alias_ids_list.empty())
						{
							rtEraseUserIdenVsCntxtMapEntry((RtS8T*)ap_cntxt->alias_ids_list.front().c_str());
							ap_cntxt->alias_ids_list.pop_front();
						}
						
						lp_public_id->num_alias_public_id	=	ap_sh_user_data->public_id.num_alias_public_id;
						for(RtU32T l_cnt	= 0;l_cnt < lp_public_id->num_alias_public_id;l_cnt++)
						{
							//- Update ALILAS Id in ap_cntxt->alias_ids_list and also insert in user_id vs cntxt_id map
							string	l_id(ap_sh_user_data->public_id.alias_public_id[l_cnt].val);
							ap_cntxt->alias_ids_list.push_back(l_id);
							rtInsertUserIdenVsCntxtMapEntry(ap_sh_user_data->public_id.alias_public_id[l_cnt].val,ap_cntxt->session_cookie.self_indx);
						
							//- update ALIAS data structure through lp_buffer
							strcpy(lp_public_id->alias_public_id[l_cnt].val, ap_sh_user_data->public_id.alias_public_id[l_cnt].val);
						}
						
						//HP_CR(Done): update num_XXX_public_id with new receievd value

					}
					else
					{
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
								"rtProcessShUserDataRcvdinPNR() num_alias_public_id = 0 hence do nothing , usr_indx=%u",
								ap_cntxt->session_cookie.self_indx);							
					}

					if(ap_sh_user_data->public_id.num_implicit_public_id !=	0)
					{
						lp_public_id->num_implicit_public_id	=	ap_sh_user_data->public_id.num_implicit_public_id;
						for(RtU32T	l_cntr	= 0;	l_cntr	< lp_public_id->num_implicit_public_id; l_cntr++)
						{
							strcpy(lp_public_id->implicit_public_id[l_cntr].val, ap_sh_user_data->public_id.implicit_public_id[l_cntr].val);
						}
						//HP_CR(Done): update num_XXX_public_id with new receievd value
					}
					else
					{
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
								"rtProcessShUserDataRcvdinPNR() num_implicit_public_id = 0 hence do nothing , usr_indx=%u",
								ap_cntxt->session_cookie.self_indx);							
					}

					if(ap_sh_user_data->public_id.num_deleted_public_id !=	0)
					{
						lp_public_id->num_deleted_public_id	=	ap_sh_user_data->public_id.num_deleted_public_id;
						for(RtU32T	l_cntr	= 0;	l_cntr	< lp_public_id->num_deleted_public_id; l_cntr++)
						{
							strcpy(lp_public_id->deleted_public_id[l_cntr].val, ap_sh_user_data->public_id.deleted_public_id[l_cntr].val);
							
							//we should delete deleted ids from map and alias id list if present
							ap_cntxt->alias_ids_list.remove(ap_sh_user_data->public_id.deleted_public_id[l_cntr].val);
							rtEraseUserIdenVsCntxtMapEntry(ap_sh_user_data->public_id.deleted_public_id[l_cntr].val);
						}
						//HP_CR(Done): update num_XXX_public_id with new receievd value
					}
					else
					{
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
								"rtProcessShUserDataRcvdinPNR() num_deleted_public_id = 0 hence do nothing , usr_indx=%u",
								ap_cntxt->session_cookie.self_indx);							
					}
					
					
					if(ap_sh_user_data->public_id.num_reg_public_id !=	0)
					{
						lp_public_id->num_reg_public_id	=	ap_sh_user_data->public_id.num_reg_public_id;
						for(RtU32T	l_cntr	= 0;	l_cntr	< ap_sh_user_data->public_id.num_reg_public_id; l_cntr++)
						{
							strcpy(lp_public_id->reg_public_id[l_cntr].val, ap_sh_user_data->public_id.reg_public_id[l_cntr].val);
						}
						//HP_CR(Done): update num_XXX_public_id with new receievd value
						
					}
					else
					{
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
								"rtProcessShUserDataRcvdinPNR() num_reg_public_id = 0 hence do nothing , usr_indx=%u",
								ap_cntxt->session_cookie.self_indx);							
					}
					
				}
				else
				{
					l_processing_successful = false;
					
					l_func_rval 						= RT_SH_UDI_ERR_NO_SUBSCRIPTION_TO_DATA;
					
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					"rtProcessShUserDataRcvdinPNR() mp_data_pool_mgr->rtCopyPoolElem() FAILED for usr_indx=%u,udi_ref=%u,pool_elem=%u",
					ap_cntxt->session_cookie.self_indx, l_udi_ref,ap_cntxt->p_data_ref_arr[l_data_ref].elem_indx);

				}
			}
			else if(RT_DIA_SH_IMS_PUBLIC_ID !=	l_data_ref)//Y
			{
				void *  lp_void_ptr  								=	NULL;
				RtBoolT l_empty_data_present_in_pnr = false;


				switch(l_udi_ref)
				{
					//HP_CR(DONE): include other switch cases and assign lp_void_ptr
										
					case	RT_DIA_SH_IMS_USER_STATE://IMSUserState
					{
						if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
							(ap_sh_user_data->sh_ims_data.flag	& RT_SH_IMS_DATA_IMS_USER_STATE_PRESENT))
						{
							lp_void_ptr	= (void*)&ap_sh_user_data->sh_ims_data.ims_user_state;

						}							
					}break;

					case	RT_DIA_SH_SCSCF_NAME://CSCFName
					{
						if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
							(ap_sh_user_data->sh_ims_data.flag	& RT_SH_IMS_DATA_SCSCF_PRESENT))
						{							
							if(strlen(ap_sh_user_data->sh_ims_data.scscf_name.val)	!=	0)
							{

								lp_void_ptr	= (void*)&ap_sh_user_data->sh_ims_data.scscf_name;

							}
							else
							{
								l_empty_data_present_in_pnr = true;
							}
						}						
					}break;	
					
					case	RT_DIA_SH_IFC:	//IFCS
					{
						if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
							(ap_sh_user_data->sh_ims_data.flag	& RT_SH_IMS_DATA_IFCS_PRESENT))
						{
							if(ap_sh_user_data->sh_ims_data.ifcs.num_ifc	!=	0 )
							{
								lp_void_ptr	= (void*)&ap_sh_user_data->sh_ims_data.ifcs;


							}
							else
							{
								l_empty_data_present_in_pnr = true;
							}							
						}
					}break;
					
					case	RT_DIA_SH_LOCATION_INFO://Location Info
					{
						if(ap_sh_user_data->flag & RT_SH_LOC_INF_PRESENT)
						{
							lp_void_ptr	= (void*)&ap_sh_user_data->sh_location_info;

						}							
					}break;					
					
					case	RT_DIA_SH_USER_STATE://ShUser State
					{
						if(ap_sh_user_data->flag & RT_SH_USER_STATE_PRESENT)
						{
							lp_void_ptr	= (void*)&ap_sh_user_data->user_state;

						}							
					}break;										
					
					case	RT_DIA_SH_CHARGING_INFO://Charging info
					{
						if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.flag & RT_SH_IMS_DATA_CHARG_INFO_PRESENT))
						
						{
							lp_void_ptr	= (void*)&ap_sh_user_data->sh_ims_data.p_charg_fun_addr;

						}							
					}break;										
										
					case	RT_DIA_SH_PSI_ACTIVATION://PSI Activation
					{
						if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.ims_data_ext.flag & RT_SH_PSI_ACTIVATION_PRESENT))

						{
							lp_void_ptr	= (void*)&ap_sh_user_data->sh_ims_data.ims_data_ext.psi_activation;

						}							
					}break;															

					case	RT_DIA_SH_SERVICE_LEV_TRACE_INFO://SRvc level trans info
					{
						if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.ims_data_ext.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.flag & RT_SH_SERVICE_LEVEL_TRACE_PRESENT))

						{
							lp_void_ptr	= (void*)&ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.service_level_trace_info;

						}							
					}break;															
										
					case	RT_DIA_SH_SMS_REG_INFO://SMS reg info
					{
						if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.ims_data_ext.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.flag & RT_SH_SMS_REG_INFO_PRESENT))

						{
							lp_void_ptr	= (void*)&ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.sms_registration_info;

						}							
					}break;
					
					case	RT_DIA_SH_UE_REACHALE_FOR_IP://ue_reachale_for_ip
					{
						if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.ims_data_ext.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.flag & RT_SH_UE_REACH_FOR_IP_PRESENT))

						{
							lp_void_ptr	= (void*)&ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.ue_reachability_for_ip;

						}							
					}break;	
					
					case	RT_DIA_SH_TADS_INFO://TADs Info
					{
						if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&
						(ap_sh_user_data->flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_data_ext.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
						(ap_sh_user_data->sh_data_ext.sh_data_ext2.flag & RT_SH_TDS_INF_PRESENT) &&
						(ap_sh_user_data->sh_data_ext.sh_data_ext2.sh_data_ext3.flag & RT_SH_UE_REACH_FOR_IP_PRESENT))

						{//HP_CR: check for correctness of FLAGS and structure variable used
							lp_void_ptr	= (void*)&ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.ue_reachability_for_ip;

						}							
					}break;
																																																																											
					default:
					{
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"rtProcessShUserDataRcvdinPNR() INVALID udi_ref usr_indx=%u,udi_ref=%u",
						ap_cntxt->session_cookie.self_indx, l_udi_ref);
						
						l_processing_successful = false;
					
						l_func_rval 						= RT_SH_UDI_ERR_NO_SUBSCRIPTION_TO_DATA;
					
					}
					break;
				}//end of switch

				if(l_processing_successful)
				{//YY
				 if(l_empty_data_present_in_pnr)
				 {
					 if(ap_cntxt->p_data_ref_arr[l_udi_ref].data_state	&	RT_SH_UDI_DATA_STATE_VALID)
					 {


						 mp_data_pool_mgr->rtReturnPoolElem(l_udi_ref,ap_cntxt->p_data_ref_arr[l_udi_ref].elem_indx);

						 ap_cntxt->p_data_ref_arr[l_udi_ref].data_state  	&=	~RT_SH_UDI_DATA_STATE_VALID;
						 ap_cntxt->p_data_ref_arr[l_udi_ref].p_data_ptr		= NULL;
						 ap_cntxt->p_data_ref_arr[l_udi_ref].elem_indx 		= 0;

						 //HP_CR: INFO logs
					 }
					 else
					 {
						 l_processing_successful = false;

						 l_func_rval = RT_SH_UDI_ERR_NO_SUBSCRIPTION_TO_DATA;
						 //HP_CR: give ALERT LOgs
					 }					
				 }
				 else
				 {//XX
				 	 if(NULL	!=	lp_void_ptr)//SHASHI_03052012 null check added
					 {
						 if(ap_cntxt->p_data_ref_arr[l_udi_ref].data_state	&	RT_SH_UDI_DATA_STATE_VALID)
						 {
							 //updation case
							 l_rval = mp_data_pool_mgr->rtUpdDataRefData(l_udi_ref,lp_void_ptr,ap_cntxt->p_data_ref_arr[l_udi_ref].p_data_ptr);
							 if(RT_SUCCESS !=	l_rval)
							 {
								 l_processing_successful = false;
								 l_func_rval 						= RT_SH_UDI_ERR_IN_DATA_REF_UPD;

								 //HP_CR: ALERT logs
							 }
							 else
							 {
								 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
								 "rtProcessShUserDataRcvdinPNR() mp_data_pool_mgr->rtUpdDataRefData() done for usr_indx=%u,udi_ref=%u",
								 ap_cntxt->session_cookie.self_indx, l_udi_ref);
							 }
						 }
						 else
						 {
							 //addition  case may be possible if empty in UDA
							 l_rval = mp_data_pool_mgr->rtStoreDataRefData(l_udi_ref , lp_void_ptr,ap_cntxt->p_data_ref_arr[l_udi_ref].elem_indx,&ap_cntxt->p_data_ref_arr[l_udi_ref].p_data_ptr);
							 if (RT_SUCCESS == l_rval)
							 {
								 ap_cntxt->p_data_ref_arr[l_udi_ref].data_state |= RT_SH_UDI_DATA_STATE_VALID;

								 //HP_CR: INFO logs

							 }
							 else
							 {
								 ap_cntxt->p_data_ref_arr[l_udi_ref].data_state  	&=	~RT_SH_UDI_DATA_STATE_VALID;
								 ap_cntxt->p_data_ref_arr[l_udi_ref].p_data_ptr		= NULL;
								 ap_cntxt->p_data_ref_arr[l_udi_ref].elem_indx 		= 0;

								 l_func_rval 						= RT_SH_UDI_ERR_IN_DATA_REF_ADD;
								 l_processing_successful = false;

								 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
								 "rtProcessShUserDataRcvdinPNR() mp_data_pool_mgr->rtStoreDataRefData() FAILED for usr_indx=%u,udi_ref=%u",
								 ap_cntxt->session_cookie.self_indx, l_udi_ref);

							 }

						 }
					 }//SHASHI_03052012 	
					 else
					 {
						 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
						 "rtProcessShUserDataRcvdinPNR() data not present in PNR usr_indx=%u,udi_ref=%u",
						 ap_cntxt->session_cookie.self_indx, l_udi_ref);
					 }					
				 }//XX
				}//YY
			}//Y
		}//data reference received

					
		l_data_ref_list.pop_front();
		
		
	}//end of while
		
	if(RT_SUCCESS == l_func_rval)
	{
		l_rval = RT_SUCCESS;
		RtShData l_sh_data;
		bzero(&l_sh_data,sizeof(RtShData));
		
		if(l_update_in_srvc_data)
		{
			l_rval = rtCreateShSrvcData(ap_cntxt,&l_sh_data,true,false);
		}
		else
		{
			if(ap_cntxt->alias_ids_list.empty())
			{
				//as all the Public Id's has been deleted so sh module should delete the cntxt 
				//so sent negetive response to subs module 
				
				strcpy(ap_cntxt->subs_data.impu.val,ap_sh_user_data->public_id.deleted_public_id[0].val);
				strcpy(l_sh_data.subs_data.impu.val,ap_sh_user_data->public_id.deleted_public_id[0].val);
				
				l_rval = RT_SH_UDI_ERR_USER_DATA_DOESNT_EXIST;
			}
			else
			{
				strcpy(ap_cntxt->subs_data.impu.val,ap_sh_user_data->public_id.alias_public_id[0].val);
				l_rval = rtCreateShSrvcData(ap_cntxt,&l_sh_data,false,true);
			}
		}
		
		l_sh_data.result_code = l_rval;
		l_rval = rtInvokeSubsRspCallback(&(ap_cntxt->subs_data),&l_sh_data);
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"Exit RtShUserDataIntf :: rtProcessShUserDataRcvdinPNR() with usr_indx=%u,ret_val=%d",ap_cntxt->session_cookie.self_indx,l_func_rval);
		
	return l_func_rval;
}
					
					
/*******************************************************************************
 *
 * FUNCTION NAME : rtProcessSrvcProvReq()
 *
 * DESCRIPTION   : This function is called by Sh provisioning interface whenever
 *                 any request(Create,Update,Delete,View) for service provisioning is received.
 *                 
 *
 * INPUT         : RtShUDIUserCntxt* ,RtShUserData* 
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtProcessSrvcProvReq(RtS8T* 									ap_user_identity,					/* Input */
																							RtShProvIntfMsgOpcode	  a_opcode, 								/* Input */
																							RtU32T*									a_app_enum,								/* Input */
																							RtShProvToken 					a_token,									/* Input */
																							RtU32T*									ar_data_size,							/* Input/Ouput */
																							void**    								ap_data,             			/* Input/Ouput */
																							RtBoolT&                ar_is_mandatory,
																							RtU32T&                 ar_cntxt_indx_ret,				/* output  */	//dharm_28102012_new added
																							RtTransAddr*      			ap_app_addr,
																							RtSelSrvUpdType					a_sel_upd_type,						/* default arg, valid in case of opcode RT_O_SH_UDI_PROV_INTF_UPDATE_SEL_REQ */			
																							RtBoolT 								a_prov_from_rest,
																							RtUtrequiredData*       ap_ut_req_data,
																							RtU32T 		        			a_prov_servc_counter) 
{ 
  RtS8T	l_app_enum_concat[50];
	memset(&l_app_enum_concat,0,sizeof(l_app_enum_concat));
	for(RtU32T l_temp_variable=0;l_temp_variable<a_prov_servc_counter;l_temp_variable++)
	{
	  snprintf(l_app_enum_concat+strlen(l_app_enum_concat),(sizeof(l_app_enum_concat) - strlen(l_app_enum_concat)- 1),"%d,",a_app_enum[l_temp_variable]);
	}
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"ENTER RtShUserDataIntf :: rtProcessSrvcProvReq() with ap_user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u.a_sel_upd_type=%d,a_prov_from_rest=%d,a_prov_servc_counter=%d",
	ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_sel_upd_type,a_prov_from_rest,a_prov_servc_counter);

	RtRcT 						l_func_ret_val  	= RT_SUCCESS;	
	RtRcT 						l_rval  					= RT_SUCCESS;
	RtU32T						l_cntxt_id				= 0;
	RtShUDIUserCntxt* lp_cntxt					= NULL;
	RtU8T 						l_udi_ref[RT_SH_UDI_MAX_MULTIPLE_PROV_SERVICE_ALLOW]; 		
	RtBoolT 					l_handle_wrkr_case	= false;
	RtTransMsg				l_trans_msg;
	memset(&l_trans_msg,0,sizeof(RtTransMsg));
	char*							lp_encoded_buffer     										= NULL;
	RtU32T 						l_encoded_buffer_size 										= 0;
	RtBoolT 					l_is_memory_allocated_for_encoded_buffer 	= false;
	void*    					lp_final_data[RT_SH_UDI_MAX_MULTIPLE_PROV_SERVICE_ALLOW]			= { NULL };
	RtU32T    				l_final_data_size[RT_SH_UDI_MAX_MULTIPLE_PROV_SERVICE_ALLOW]	= {0};
	RtU32T    				l_srv_cnt_dft_check	= 0;
	
	RtShUDIProcReq*		lp_udi_proc_req 	= (RtShUDIProcReq*)l_trans_msg.msg_buffer;
  
	RtS8T	l_udi_ref_concat[50];
	memset(&l_udi_ref_concat,0,sizeof(l_udi_ref_concat));
	memset(&l_udi_ref,0,sizeof(l_udi_ref));//100_HOTFIX2_klocwork_25082016

	mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqCalled);
	
	if(RT_O_SH_UDI_PROV_INTF_UPDATE_SEL_REQ	==	a_opcode)
	{
		if(RT_SEL_SRV_UPD_OP_INVALID	==	a_sel_upd_type)
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
			"RtShUserDataIntf :: rtProcessSrvcProvReq() no sel_upd_type for UPDATE_SEL_REQ with ap_user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_sel_upd_type=%d,a_prov_from_rest=%d,a_prov_servc_counter=%d",
			ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_sel_upd_type,a_prov_from_rest,a_prov_servc_counter);
			
			return RT_FAILURE;
			//TBD raise count
		}
		else
		{
			lp_udi_proc_req->req_body.prov_msg.sel_upd_type	=	a_sel_upd_type;
		}
	}
	
  for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)
  {
		 l_rval = rtGetUDIRefFromAppEnum(a_app_enum[l_srv_cnt_dft_check],l_udi_ref[l_srv_cnt_dft_check]);

		 if(RT_SUCCESS !=	l_rval && (RT_O_SH_UDI_PROV_INTF_VIEW_SRVC_LIST_REQ != a_opcode))//SHASHI_30062012 incase of view_list no app_enum will come in arg
		 {
			 //HP_CR(DONE):: DEBUG error log

			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
			 "rtProcessSrvcProvReq(): rtGetUDIRefFromAppEnum() :FAILED with ap_user_identity=%s,a_opcode=%d,a_app_enum=%d,a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
			 ap_user_identity, a_opcode, a_app_enum[l_srv_cnt_dft_check], a_token,a_prov_from_rest,a_prov_servc_counter);

			 return RT_SH_UDI_ERR_INVALID_APP_ENUM;
		 }

     snprintf(l_udi_ref_concat+strlen(l_udi_ref_concat),(sizeof(l_udi_ref_concat) - strlen(l_udi_ref_concat)- 1),"%d,",l_udi_ref[l_srv_cnt_dft_check]);
		 //NEW 
		 if((a_opcode	==	RT_O_SH_UDI_PROV_INTF_CREATE_SRVC_REQ)	||
				 (a_opcode	==	RT_O_SH_UDI_PROV_INTF_UPDATE_SRVC_REQ)	||
				 (a_opcode	==	RT_O_SH_UDI_PROV_INTF_UPDATE_SEL_REQ))
		 {
			 l_rval = mp_data_pool_mgr->rtInvokeSrvcDataProvValidationFunc(l_udi_ref[l_srv_cnt_dft_check] ,
																																	 a_opcode,
																																	 ap_data[l_srv_cnt_dft_check]);
			 if (RT_SUCCESS != l_rval)
			 {
				 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
				 "rtProcessSrvcProvReq(): Validation check FAILED with ap_user_identity=%s,a_opcode=%d,a_app_enum=%d,a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
				 ap_user_identity, a_opcode, a_app_enum[l_srv_cnt_dft_check], a_token,a_prov_from_rest,a_prov_servc_counter);

				 return l_rval;

			 }

			 l_rval = mp_data_pool_mgr->rtInvokeSrvcDataProvEncFunc(l_udi_ref[l_srv_cnt_dft_check] ,
																												 		 a_opcode,
																												 		 ap_data[l_srv_cnt_dft_check],
																														 (void **)&lp_encoded_buffer,
																												 		 l_encoded_buffer_size,
																												  	 l_is_memory_allocated_for_encoded_buffer);
			 if (RT_SUCCESS != l_rval)
			 {
				 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
				 "rtProcessSrvcProvReq(): Encoding FAILED with ap_user_identity=%s,a_opcode=%d,a_app_enum=%d,a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
				 ap_user_identity, a_opcode, a_app_enum[l_srv_cnt_dft_check], a_token,a_prov_from_rest,a_prov_servc_counter);

				 return l_rval;

			 }

		 }
		 else
		 {
			 //	validation and encoding is not required in VIEW and DELETE operations
		 }

		 if(	l_is_memory_allocated_for_encoded_buffer)
		 {
			 lp_final_data[l_srv_cnt_dft_check] 		= (void *)lp_encoded_buffer;
			 l_final_data_size[l_srv_cnt_dft_check]	= l_encoded_buffer_size;
		 }
		 else
		 {
			 lp_final_data [l_srv_cnt_dft_check]		= ap_data[l_srv_cnt_dft_check];
			 l_final_data_size[l_srv_cnt_dft_check]	= ar_data_size[l_srv_cnt_dft_check];
		 }
  
	 //Multi_service_Creation shifted upper 
		 lp_udi_proc_req->req_body.prov_msg.msg_body.app_enum[l_srv_cnt_dft_check]					= a_app_enum[l_srv_cnt_dft_check];
		 lp_udi_proc_req->req_body.prov_msg.msg_body.udi_ref[l_srv_cnt_dft_check] 					= l_udi_ref[l_srv_cnt_dft_check];
		 //Used in case of create , when wrkr needs to send reply
		 lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[l_srv_cnt_dft_check] 			= lp_final_data[l_srv_cnt_dft_check];

		 //HP_CR(DONE):: put data size blindly
		 lp_udi_proc_req->req_body.prov_msg.msg_body.data_size[l_srv_cnt_dft_check]    		=  l_final_data_size[l_srv_cnt_dft_check];
	
	}			
	lp_udi_proc_req->opcode 															= RT_O_SH_UDI_PROV_INTF_MSG;
	lp_udi_proc_req->req_body.prov_msg.opcode							= a_opcode;//will change in case of Ut interface....
	lp_udi_proc_req->req_body.prov_msg.msg_body.token 		= a_token;

	//Tag_amit_18032013 - new flag/addr introduced for Rest/Appl request
	lp_udi_proc_req->req_body.prov_msg.msg_body.rest_prov_req 		= a_prov_from_rest;
	lp_udi_proc_req->req_body.prov_msg.msg_body.trans_addr 				= *ap_app_addr;
	
	
	
	
	//Tag_amit_17072013 -- store user identity for PUR
	strcpy(lp_udi_proc_req->req_body.prov_msg.msg_body.user_identity,ap_user_identity);
	
	if(NULL != ap_ut_req_data)
	{
		lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data 				= *ap_ut_req_data;//change_Ut 
	}
	else
	{
		bzero(&lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data,sizeof(lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data));
	}
	
	lp_udi_proc_req->req_body.prov_msg.msg_body.number_of_prov_service 				= a_prov_servc_counter;	//Multi_service_Creation
	//NOTE:cntxt_indx and cntxt_state to be filled later in function
	switch(a_opcode)
	{
		case RT_O_SH_UDI_PROV_INTF_CREATE_SRVC_REQ:
		case RT_O_SH_UDI_PROV_INTF_UPDATE_SRVC_REQ:
		case RT_O_SH_UDI_PROV_INTF_UPDATE_SEL_REQ	:
		{

			l_rval = rtFindCntxtForUserIden(ap_user_identity,l_cntxt_id);
			
			if(RT_SUCCESS !=	l_rval)
	  	{
			
				//- release lock on m_user_iden_vs_cntxt_lock_arr[l_block]
	
				l_rval = mp_user_cntxt_mgr->rtGetNewCntxt(&lp_cntxt);
				
				if( RT_SUCCESS != l_rval )
				{
					l_func_ret_val = RT_SH_UDI_ERR_RSRC_TEMP_UNAVAILABLE;
					
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"rtProcessSrvcProvReq() mp_user_cntxt_mgr->rtGetNewCntxt returned =%d for user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
						l_rval, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);
				}
				else
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
						"rtProcessSrvcProvReq() rtGetNewCntxt success cntxt_id =%u for user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
						lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode,l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);
				
					//lp_cntxt->cntxt_state 						= RT_SH_UDI_CNTXT_NON_IDLE;
					//lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROV_INTF_REQ;
					
					l_cntxt_id	=	lp_cntxt->session_cookie.self_indx;//SHASHI_28042012
					string l_user_id(ap_user_identity);
					lp_cntxt->alias_ids_list.push_back(l_user_id);
					
					l_rval = rtInsertUserIdenVsCntxtMapEntry(ap_user_identity,lp_cntxt->session_cookie.self_indx);
				
					if( RT_SUCCESS != l_rval )
					{
					
						//shall not happen
						l_func_ret_val = RT_SH_UDI_ERR_IN_INTRNL_MAP;
						
						mp_user_cntxt_mgr->rtReturnCntxt(lp_cntxt->session_cookie.self_indx);
						
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
							"rtProcessSrvcProvReq() rtInsertUserIdenVsCntxtMapEntry FAILED for user_cntxt_id =%u,user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
							lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);					
					}
					else
					{
					
						//Important
						//- set cntxt_state RtShUDIProcReq->req_body->RtShProvIntfMsg |= RT_SH_UDI_PROV_INTF_CNTXT_NEWLY_CREATED
						lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_state |= RT_SH_UDI_PROV_INTF_CNTXT_NEWLY_CREATED;
						lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_indx	= lp_cntxt->session_cookie.self_indx;
						
						l_handle_wrkr_case  			= true;
					}
				
				}
			}
			else
			{
				if (RT_SUCCESS == mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt))
				{
					lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_indx	= lp_cntxt->session_cookie.self_indx;
					
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
						"rtProcessSrvcProvReq() rtRetrieveCntxtData returned SUCCESS for user_identity=%s,a_opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
						ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

					if( RT_O_SH_UDI_PROV_INTF_CREATE_SRVC_REQ == a_opcode)
					{
						if(lp_cntxt->cntxt_flag &	RT_SH_UDI_UDA_RECEIVED)
						{
							//Multi_service_Creation
						 RtBoolT l_ret_function_hit=false;
 						 for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)
						 {
							//check whether data already exists ar_data_sizefor service in user context 
							if( (lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID)
													&&
									(! (lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY))

								)
							{
								//means service data already exists
								mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
								
								l_func_ret_val = RT_SH_UDI_ERR_DATA_ALREADY_EXISTS;
								l_ret_function_hit=false;//Multi_service_Creation
								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
									"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - service data exists,create noT allowed for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
									ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
								 break;//Multi_service_Creation
							}
							else if(!(lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID )
																		||
												( (lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID)
												  					&&
													(lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY)
												)
											)	
							{

								//NOTE:- Data state has not been updated at this stage because of following reasons
								//			- scenario in which context state is NON_IDLE (handling application req etc) and provisioning request needs to be pushed 
								//						into pending queue. 
								//			 Therefore when create request will be handled and successfully acknowleged by HSS, only then
								//			 user data repository data state will be set to VALID

								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
									"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - service data absent/mandatory,create allowed for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
									ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

								if(	lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY)
								{
									lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_state |= RT_SH_UDI_PROV_INTF_DATA_VALID_DUE_TO_MANDATORY;
								}
								l_ret_function_hit=true;
// 								if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
// 								{
// 									l_handle_wrkr_case  							= true;
// 									//lp_cntxt->cntxt_state 						= RT_SH_UDI_CNTXT_NON_IDLE;
// 									//lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROV_INTF_REQ;	
// 								}
// 								else
// 								{
// 									if ( (lp_cntxt->cntxt_flag & 	RT_SH_UDI_USR_CNTXT_MARKED_FOR_DELETION) )
// 									{
// 										l_func_ret_val = RT_SH_UDI_ERR_USR_CNTXT_MARKED_FOR_DELETION;
// 
// 										mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
// 											"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - deletion in progress for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d",
// 											ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest);
// 
// 									}
// 									else
// 									{
// 										//TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES - 27102012
// 										//l_func_ret_val = rtCheckForMissedTimeoutFromDiaStack(lp_cntxt,ap_user_identity);
// 										//if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
// 										//{
// 											//- push in lp_cntxt->processing_req.proc_req_list from push_back above processing request
// 
// 											//HP_CR(DONE): use rtStoreDataRepositoryData() instead of rtStoreDataRefData()
// 											//RtShUDIRef a_udi_ref_val,void* ap_buffer,RtU32T a_buffer_size ,RtU32T& ar_index,void ** app_elem_pool,RtBoolT a_is_decoding_of_buffer_reqd
// 											l_rval = mp_data_pool_mgr->rtStoreDataRepositoryData(l_udi_ref ,
// 																																			lp_final_data,
// 																																			l_final_data_size,
// 																																			lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx,
// 																																			&lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr,
// 																																			false);
// 											if (RT_SUCCESS == l_rval)
// 											{
// 
// 												lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);
// 
// 												//SHASHI_18102012 : counters
// 												mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProvReqPushedInPend);
// 
// 												l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
// 												ar_cntxt_indx_ret=l_cntxt_id;//dharm_28102012
// 
// 												mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
// 													"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - adding create req to pending queue user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d",
// 													ap_user_identity, a_opcode, l_udi_ref, l_cntxt_id,a_prov_from_rest);
// 											}
// 											else
// 											{
// 												//HP_CR(DONE)::set l_func_ret_val, give critical logs
// 												mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 													"rtProcessSrvcProvReq():rtStoreDataRepositoryData FAILED  - case when RT_SH_UDI_UDA_RECEIVED - user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d",
// 													ap_user_identity, a_opcode, l_udi_ref, l_cntxt_id,a_prov_from_rest);										
// 
// 												l_func_ret_val = RT_SH_UDI_ERR_IN_DATA_REF_UPD ;
// 											}
// 										//}
// 									}
// 
// 	// 								if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
// // 									{
//  										mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
// //									}
// 
// 								}//end of else of if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
							}//end of else
							else
							{
								l_ret_function_hit=false;//Multi_service_Creation for safety but dead code
								//HP_CR(DONE):: give appropriate return value and logs wording


								//Dead code! will never be hit(SHASHI 13/03/2012)


								//mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);																																			 	
								//																																																														 
								//l_func_ret_val = RT_SH_UDI_UPDATE_NOT_ALLOWED_AS_MANDATORY ;																																 	
								//																																																														 
								//mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__, 																						 	
								//	"rtProcessSrvcProvReq() service data exists,create noT allowed for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u",	 	
								//	ap_user_identity, a_opcode, l_udi_ref, l_cntxt_id); 																																			 	

							}	
						 }//end of Multi_service_Creation for loop
						 if(l_ret_function_hit)  // upper else condition part move Multi_service_Creation
						 {
							 if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
							 {
								 l_handle_wrkr_case  							= true;
								 //lp_cntxt->cntxt_state 						= RT_SH_UDI_CNTXT_NON_IDLE;
								 //lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROV_INTF_REQ;	
							 }
							 else
							 {
								 if ( (lp_cntxt->cntxt_flag & 	RT_SH_UDI_USR_CNTXT_MARKED_FOR_DELETION) )
								 {
									 l_func_ret_val = RT_SH_UDI_ERR_USR_CNTXT_MARKED_FOR_DELETION;

									 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
										 "rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - deletion in progress for user_identity=%s,a_opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
										 ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

								 }
								 else
								 {
									 //TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES - 27102012
									 //l_func_ret_val = rtCheckForMissedTimeoutFromDiaStack(lp_cntxt,ap_user_identity);
									 //if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
									 //{
										 //- push in lp_cntxt->processing_req.proc_req_list from push_back above processing request

										 //HP_CR(DONE): use rtStoreDataRepositoryData() instead of rtStoreDataRefData()
										 //RtShUDIRef a_udi_ref_val,void* ap_buffer,RtU32T a_buffer_size ,RtU32T& ar_index,void ** app_elem_pool,RtBoolT a_is_decoding_of_buffer_reqd
 										 RtBoolT l_return_pool_back=false;;
 										 RtU32T l_number_return_pool_element=0;;
										 for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)
									   {
											 l_rval = mp_data_pool_mgr->rtStoreDataRepositoryData(l_udi_ref[l_srv_cnt_dft_check] ,
																																			 lp_final_data[l_srv_cnt_dft_check],
																																			 l_final_data_size[l_srv_cnt_dft_check],
																																			 lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_dft_check],
																																			 &lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[l_srv_cnt_dft_check],
																																			 false);
												if(RT_SUCCESS == l_rval)
												{
														mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
															"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED -rtStoreDataRepositoryData success for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
															ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
													
												}
												else
												{
													l_return_pool_back=true;
													l_number_return_pool_element=l_srv_cnt_dft_check;
													mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
														"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED -rtStoreDataRepositoryData success for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d l_number_return_pool_element=%d,l_return_pool_back=%d,a_prov_servc_counter=%d",
														ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,l_number_return_pool_element,l_return_pool_back,a_prov_servc_counter);
													break;
												}
										 }
										 if(l_return_pool_back && l_number_return_pool_element)
										 {

 				    						for(RtU32T l_srv_cnt_ret_pool_element=0;l_srv_cnt_ret_pool_element<l_number_return_pool_element;l_srv_cnt_ret_pool_element++)
												 mp_data_pool_mgr->rtReturnPoolElem(l_udi_ref[l_srv_cnt_ret_pool_element], lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_ret_pool_element]);
												//if value is value l_number_return_pool_element=0 then no need of return 
												//return pool element 
										 }
										 
										 if (RT_SUCCESS == l_rval)
										 {

											 lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);

											 //SHASHI_18102012 : counters
											 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProvReqPushedInPend);

											 l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
											 ar_cntxt_indx_ret=l_cntxt_id;//dharm_28102012

											 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
												 "rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - adding create req to pending queue user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
												 ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
										 }
										 else
										 {
											 //HP_CR(DONE)::set l_func_ret_val, give critical logs
											 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
												 "rtProcessSrvcProvReq():rtStoreDataRepositoryData FAILED  - case when RT_SH_UDI_UDA_RECEIVED - user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
												 ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);										

											 l_func_ret_val = RT_SH_UDI_ERR_IN_DATA_REF_UPD ;
										 }
									 //}
								 }

 // 								if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
// 									{
 									 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
//									}

							 }//end of else of if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
						 }
						 
						 	
						}//end of if
						else
						{
							if ( (lp_cntxt->cntxt_flag & 	RT_SH_UDI_USR_CNTXT_MARKED_FOR_DELETION) )
							{
								l_func_ret_val = RT_SH_UDI_ERR_USR_CNTXT_MARKED_FOR_DELETION;

								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
									"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_NOT_RECEIVED - deletion in progress for user_identity=%s,a_opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
									ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

							}
							else
							{
								//TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES - 27102012
						//		l_func_ret_val = rtCheckForMissedTimeoutFromDiaStack(lp_cntxt,ap_user_identity);



// 								if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
// 								{
// 									//- push in lp_cntxt->processing_req.proc_req_list from push_back above processing request

									//HP_CR(DONE): use rtStoreDataRepositoryData() instead of rtStoreDataRefData()
									//RtShUDIRef a_udi_ref_val,void* ap_buffer,RtU32T a_buffer_size ,RtU32T& ar_index,void ** app_elem_pool,RtBoolT a_is_decoding_of_buffer_reqd
 									RtBoolT l_return_pool_back=false;;
 									RtU32T l_number_return_pool_element=0;;
 									for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)
									{
											 l_rval = mp_data_pool_mgr->rtStoreDataRepositoryData(l_udi_ref[l_srv_cnt_dft_check] ,
																																			 lp_final_data[l_srv_cnt_dft_check],
																																			 l_final_data_size[l_srv_cnt_dft_check],
																																			 lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_dft_check],
																																			 &lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[l_srv_cnt_dft_check],
																																			 false);
											if(RT_SUCCESS == l_rval)
											{
													mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
														"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_NOT_RECEIVED -rtStoreDataRepositoryData success for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
														ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

											}
											else
											{
												l_return_pool_back=true;
												l_number_return_pool_element=l_srv_cnt_dft_check;
												mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
													"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_NOT_RECEIVED -rtStoreDataRepositoryData success for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d l_number_return_pool_element=%d,l_return_pool_back=%d,a_prov_servc_counter=%d",
													ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,l_number_return_pool_element,l_return_pool_back,a_prov_servc_counter);
												break;
											}
									}
									if(l_return_pool_back && l_number_return_pool_element)
									{

 				    				 for(RtU32T l_srv_cnt_ret_pool_element=0;l_srv_cnt_ret_pool_element<l_number_return_pool_element;l_srv_cnt_ret_pool_element++)
											mp_data_pool_mgr->rtReturnPoolElem(l_udi_ref[l_srv_cnt_ret_pool_element], lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_ret_pool_element]);
										 //if value is value l_number_return_pool_element=0 then no need of return 
										 //return pool element 
									}
									if (RT_SUCCESS == l_rval)
									{

										lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);

										//SHASHI_18102012 : counters
										mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProvReqPushedInPend);

										l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
										ar_cntxt_indx_ret=l_cntxt_id;//dharm_28102012

										mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
											"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_NOT_RECEIVED - adding create req to pending queue user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
									}
									else
									{
										//HP_CR(DONE)::set l_func_ret_val, give critical logs
										mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
											"rtProcessSrvcProvReq():rtStoreDataRepositoryData FAILED - case when RT_SH_UDI_UDA_NOT_RECEIVED - user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);										

										l_func_ret_val = RT_SH_UDI_ERR_IN_DATA_REF_UPD ;
									}
								//}
							}
							//tag_25012014
							mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

						}
					}
					else//else of if( RT_O_SH_PROV_INTF_CREATE_SRVC_REQ == a_opcode)
					{
						if(lp_cntxt->cntxt_flag &	RT_SH_UDI_UDA_RECEIVED)
						{
							
							
						 RtBoolT l_hit_multi_creation_check=false;
						 for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)
						 {
								//check whether data already exists for service in user context 
								//pass Ut request if service is not state_valid then behave like Create
								//special check for Ut-interface
							if( !(lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data.Is_Ut_request) &&  !(lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID) 
								)
							{
								//means service data doesnt exists
								mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
								l_hit_multi_creation_check=false;
								l_func_ret_val = RT_SH_UDI_ERR_SRVC_DATA_DOESNT_EXIST_FOR_USER;

								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
									"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - Data does not exist/exist due to mandatory for update user_id=%s,opcode=%d,udi_ref=%u,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
									ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);							
								
								break;
							}
							else 
							{
								l_hit_multi_creation_check=true;
// 								if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
// 								{
// 									l_handle_wrkr_case  							= true;
// 
// 								}
// 								else
// 								{
// 									if ( (lp_cntxt->cntxt_flag & 	RT_SH_UDI_USR_CNTXT_MARKED_FOR_DELETION) )
// 									{
// 										l_func_ret_val = RT_SH_UDI_ERR_USR_CNTXT_MARKED_FOR_DELETION;
// 
// 										mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
// 											"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - deletion in progress for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d",
// 											ap_user_identity, a_opcode, l_udi_ref, l_cntxt_id,a_prov_from_rest);
// 									}
// 									else
// 									{
// 
// 										//TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES - 27102012
// 										//l_func_ret_val = rtCheckForMissedTimeoutFromDiaStack(lp_cntxt,ap_user_identity);
// 
// // 										if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
// // 										{
// 
// 
// 
// 											//HP_CR(DONE): use rtStoreDataRepositoryData() instead of rtStoreDataRefData()
// 											l_rval = mp_data_pool_mgr->rtStoreDataRepositoryData(l_udi_ref ,
// 																																			lp_final_data,
// 																																			l_final_data_size,
// 																																			lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx,
// 																																			&lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr,
// 																																			false);
// 											if (RT_SUCCESS == l_rval)
// 											{
// 
// 												lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);
// 
// 												//SHASHI_18102012 : counters
// 												mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProvReqPushedInPend);
// 
// 												l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
// 												ar_cntxt_indx_ret=l_cntxt_id;//dharm_28102012
// 
// 												mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
// 													"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - adding create req to pending queue user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d",
// 													ap_user_identity, a_opcode, l_udi_ref, l_cntxt_id,a_prov_from_rest);
// 											}
// 											else
// 											{
// 												//HP_CR(DONE)::set l_func_ret_val, give critical logs
// 
// 												mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 													"rtProcessSrvcProvReq(): rtStoreDataRepositoryData() FAILED - case when RT_SH_UDI_UDA_RECEIVED - user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d",
// 													ap_user_identity, a_opcode, l_udi_ref, l_cntxt_id,a_prov_from_rest);
// 
// 												l_func_ret_val = RT_SH_UDI_ERR_IN_DATA_REF_UPD; 	
// 											}
// 										//}
// 									}
// 
// // 									if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
// // 									{
// 										//SHASHI moved up
// 										mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
// // 									}
// 								}//end of else of if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
							}
						 }//end of for loop //Multi_service_Creation
						 if(l_hit_multi_creation_check) //start of if Multi_service_Creation
						 {
						 	  if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
								{
									l_handle_wrkr_case  							= true;

								}
								else
								{
									if ( (lp_cntxt->cntxt_flag & 	RT_SH_UDI_USR_CNTXT_MARKED_FOR_DELETION) )
									{
										l_func_ret_val = RT_SH_UDI_ERR_USR_CNTXT_MARKED_FOR_DELETION;

										mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
											"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - deletion in progress for user_identity=%s,a_opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
									}
									else
									{

										//TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES - 27102012
										//l_func_ret_val = rtCheckForMissedTimeoutFromDiaStack(lp_cntxt,ap_user_identity);

// 										if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
// 										{



											//HP_CR(DONE): use rtStoreDataRepositoryData() instead of rtStoreDataRefData()
 											RtBoolT l_return_pool_back=false;;
 											RtU32T l_number_return_pool_element=0;;
 											for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)
											{
													l_rval = mp_data_pool_mgr->rtStoreDataRepositoryData(l_udi_ref[l_srv_cnt_dft_check] ,
																																					lp_final_data[l_srv_cnt_dft_check],
																																					l_final_data_size[l_srv_cnt_dft_check],
																																					lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_dft_check],
																																					&lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[l_srv_cnt_dft_check],
																																					false);
													if(RT_SUCCESS == l_rval)
													{
															mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
																"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED -rtStoreDataRepositoryData success for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
																ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

													}
													else
													{
														l_return_pool_back=true;
														l_number_return_pool_element=l_srv_cnt_dft_check;
														mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
															"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED -rtStoreDataRepositoryData success for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d l_number_return_pool_element=%d,l_return_pool_back=%d,a_prov_servc_counter=%d",
															ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,l_number_return_pool_element,l_return_pool_back,a_prov_servc_counter);
														break;
													}
											}
											if(l_return_pool_back && l_number_return_pool_element)
											{
 				    						for(RtU32T l_srv_cnt_ret_pool_element=0;l_srv_cnt_ret_pool_element<l_number_return_pool_element;l_srv_cnt_ret_pool_element++)
												 mp_data_pool_mgr->rtReturnPoolElem(l_udi_ref[l_srv_cnt_ret_pool_element], lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_ret_pool_element]);

												 //if value is value l_number_return_pool_element=0 then no need of return 
												 //return pool element 
											}

											if (RT_SUCCESS == l_rval)
											{

												lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);

												//SHASHI_18102012 : counters
												mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProvReqPushedInPend);

												l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
												ar_cntxt_indx_ret=l_cntxt_id;//dharm_28102012

												mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
													"rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED - adding create req to pending queue user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
													ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
											}
											else
											{
												//HP_CR(DONE)::set l_func_ret_val, give critical logs

												mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
													"rtProcessSrvcProvReq(): rtStoreDataRepositoryData() FAILED - case when RT_SH_UDI_UDA_RECEIVED - user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
													ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

												l_func_ret_val = RT_SH_UDI_ERR_IN_DATA_REF_UPD; 	
											}
										//}
									}

// 									if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
// 									{
										//SHASHI moved up
										mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
// 									}
								}//end of else of if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )

						  }//end of if Multi_service_Creation
						}
						else
						{
							 if ( (lp_cntxt->cntxt_flag & 	RT_SH_UDI_USR_CNTXT_MARKED_FOR_DELETION) )
							 {
								 l_func_ret_val = RT_SH_UDI_ERR_USR_CNTXT_MARKED_FOR_DELETION;

								 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
									 "rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_NOT_RECEIVED - deletion in progress for user_identity=%s,a_opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
									 ap_user_identity, a_opcode,l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
							 }
							 else
							 {
								 //TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES - 27102012
// 								 l_func_ret_val = rtCheckForMissedTimeoutFromDiaStack(lp_cntxt,ap_user_identity);
// 
// 								 if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
// 								 {
									 //HP_CR(DONE): use rtStoreDataRepositoryData() instead of rtStoreDataRefData()

 									 RtBoolT l_return_pool_back=false;;
 									 RtU32T l_number_return_pool_element=0;;
 									 for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)
									 {
									     l_rval = mp_data_pool_mgr->rtStoreDataRepositoryData(l_udi_ref[l_srv_cnt_dft_check] ,
																																		lp_final_data[l_srv_cnt_dft_check],
																																		l_final_data_size[l_srv_cnt_dft_check],
																																		lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_dft_check],
																																		&lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[l_srv_cnt_dft_check],
																																		false);
											 if(RT_SUCCESS == l_rval)
											 {
													 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
														 "rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED -rtStoreDataRepositoryData success for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
														 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

											 }
											 else
											 {
												 l_return_pool_back=true;
												 l_number_return_pool_element=l_srv_cnt_dft_check;
												 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
													 "rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED -rtStoreDataRepositoryData success for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d l_number_return_pool_element=%d,l_return_pool_back=%d,a_prov_servc_counter=%d",
													 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,l_number_return_pool_element,l_return_pool_back,a_prov_servc_counter);
												 break;
											 }
									 }
									 if(l_return_pool_back && l_number_return_pool_element)
									 {

 				    					for(RtU32T l_srv_cnt_ret_pool_element=0;l_srv_cnt_ret_pool_element<l_number_return_pool_element;l_srv_cnt_ret_pool_element++)
											 mp_data_pool_mgr->rtReturnPoolElem(l_udi_ref[l_srv_cnt_ret_pool_element], lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_ret_pool_element]);
											//if value is value l_number_return_pool_element=0 then no need of return 
											//return pool element 
									 }
									 if (RT_SUCCESS == l_rval)
									 {
										 lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);

										 //SHASHI_18102012 : counters
										 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProvReqPushedInPend);

										 l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
										 ar_cntxt_indx_ret=l_cntxt_id;//dharm_28102012

										 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
											 "rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_NOT_RECEIVED - adding create req to pending queue user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											 ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
									 }
									 else
									 {
										 //HP_CR(DONE)::set l_func_ret_val, give critical logs

										 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
											 "rtProcessSrvcProvReq(): rtStoreDataRepositoryData() - case when RT_SH_UDI_UDA_NOT_RECEIVED - FAILED user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											 ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

										 l_func_ret_val = RT_SH_UDI_ERR_IN_DATA_REF_UPD; 	
									 }
								 //}
							 }
							 
	// 						 if( RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
// 							 {
// 								 //SHASHI moved up
								 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
//							 }
						}
					}
				}//end of if of mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt) )
				else
				{
				
					//shall not happen	give ALERT Logs
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"rtProcessSrvcProvReq() entry in map exists but retrieve failed user_id=%s,opcode=%d,udi_ref=%s,a_prov_from_rest=%d,a_prov_servc_counter=%d",
						ap_user_identity, a_opcode, l_udi_ref_concat,a_prov_from_rest,a_prov_servc_counter);
					
					RtU32T		l_block_no	= 0;
					rtDecideBlock(ap_user_identity, l_block_no);
					rtEraseUserIdenVsCntxtMapEntry(ap_user_identity,l_block_no);
					
					// get new context and set state and processing state 
					l_rval = mp_user_cntxt_mgr->rtGetNewCntxt(&lp_cntxt);
					
					if( RT_SUCCESS != l_rval )
					{
						l_func_ret_val = RT_SH_UDI_ERR_RSRC_TEMP_UNAVAILABLE;

						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
							"rtProcessSrvcProvReq() mp_user_cntxt_mgr->rtGetNewCntxt returned =%d for user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
							l_rval, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);
					}
					else
					{
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
							"rtProcessSrvcProvReq() rtGetNewCntxt success cntxt_id =%u for user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
							lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);

						//lp_cntxt->cntxt_state 						= RT_SH_UDI_CNTXT_NON_IDLE;
						//lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROV_INTF_REQ;
						
						//XYZ_SHASHI
						l_cntxt_id	=	lp_cntxt->session_cookie.self_indx;

						string l_user_id(ap_user_identity);
						lp_cntxt->alias_ids_list.push_back(l_user_id);

						l_rval  = rtInsertUserIdenVsCntxtMapEntry(ap_user_identity,lp_cntxt->session_cookie.self_indx);

						if( RT_SUCCESS != l_rval )
						{

							//shall not happen
							l_func_ret_val = RT_SH_UDI_ERR_IN_INTRNL_MAP;

							mp_user_cntxt_mgr->rtReturnCntxt(lp_cntxt->session_cookie.self_indx);

							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
								"rtProcessSrvcProvReq() rtInsertUserIdenVsCntxtMapEntry FAILED for user_cntxt_id =%u,user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
								lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);					
						}
						else
						{

							//Important
							//- set cntxt_state RtShUDIProcReq->req_body->RtShProvIntfMsg |= RT_SH_UDI_PROV_INTF_CNTXT_NEWLY_CREATED
							lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_state |= RT_SH_UDI_PROV_INTF_CNTXT_NEWLY_CREATED;
							lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_indx	= lp_cntxt->session_cookie.self_indx;
							l_handle_wrkr_case  			= true;
						}

					}
				}
			}//else of entry not found
		
			if(l_handle_wrkr_case)
			{
				RtBoolT l_negative_hndling = false;
				
				//HP_CR(DONE): use rtStoreDataRepositoryData() instead of rtStoreDataRefData()
 				 RtBoolT l_return_pool_back=false;;
 				 RtU32T l_number_return_pool_element=0;;
 				 for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)
				 {
						l_rval = mp_data_pool_mgr->rtStoreDataRepositoryData(l_udi_ref[l_srv_cnt_dft_check] ,
																														lp_final_data[l_srv_cnt_dft_check],
																										    		l_final_data_size[l_srv_cnt_dft_check], 
																														lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_dft_check],
																														&lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[l_srv_cnt_dft_check],
																														false);
						 if(RT_SUCCESS == l_rval)
						 {
								lp_udi_proc_req->req_body.prov_msg.msg_body.data_size[l_srv_cnt_dft_check] = l_final_data_size[l_srv_cnt_dft_check];
								 
								 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
									 "rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED -rtStoreDataRepositoryData success for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
									 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

						 }
						 else
						 {
							 l_return_pool_back=true;
							 l_number_return_pool_element=l_srv_cnt_dft_check;
							 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
								 "rtProcessSrvcProvReq() - case when RT_SH_UDI_UDA_RECEIVED -rtStoreDataRepositoryData success for user_identity=%s,a_opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d l_number_return_pool_element=%d,l_return_pool_back=%d,a_prov_servc_counter=%d",
								 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,l_number_return_pool_element,l_return_pool_back,a_prov_servc_counter);
							 break;
						 }
				 }
				 if(l_return_pool_back && l_number_return_pool_element)
				 {
						 
 				    for(RtU32T l_srv_cnt_ret_pool_element=0;l_srv_cnt_ret_pool_element<l_number_return_pool_element;l_srv_cnt_ret_pool_element++)
						 mp_data_pool_mgr->rtReturnPoolElem(l_udi_ref[l_srv_cnt_ret_pool_element], lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_ret_pool_element]);
						//if value is value l_number_return_pool_element=0 then no need of return 
						//return pool element 
				 }

				if (RT_SUCCESS == l_rval)
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
						"rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtStoreDataRefData success for user_cntxt_id =%u,user_identity=%s,a_opcode=%d,udiref=%u pool_indx= %u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
						lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_dft_check],a_prov_from_rest,a_prov_servc_counter);					
					
					//lp_udi_proc_req->req_body.prov_msg.msg_body.data_size[l_srv_cnt_dft_check] = l_final_data_size[l_srv_cnt_dft_check];
					
				  lp_cntxt->cntxt_state 						= RT_SH_UDI_CNTXT_NON_IDLE;
				  lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROV_INTF_REQ ;



					lp_cntxt->processing_req.proc_req_list.push_front(*lp_udi_proc_req);

					lp_cntxt->processing_req.curr_req_iter = lp_cntxt->processing_req.proc_req_list.begin();

					mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

					//		- send to worker threads MODULO by l_cntxt_id using function
					
					//CR TRANS_MSG NOT FILLED
					l_rval = rtSendToWrkr(&l_trans_msg, l_cntxt_id);
					
					if( RT_FAILURE == l_rval)
					{
						//HP_CR(DONE): give critical logs
						//HP_CR(DONE):: 
						// - free lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx
						// - call retrivecontext lock to take context lock
						
 					  for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)//Multi_service_Creation
					  {
							mp_data_pool_mgr->rtReturnPoolElem(l_udi_ref[l_srv_cnt_dft_check], lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_dft_check]);
					  }
					  if (RT_SUCCESS != mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt))
						{
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
							 "rtProcessSrvcProvReq() rtRetrieveCntxtData() FAILED user_identity=%s,a_opcode=%d,udiref=%s pool_indx= %u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
							 ap_user_identity, a_opcode, l_udi_ref_concat, lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_dft_check],a_prov_from_rest,a_prov_servc_counter);						
							
							//return RT_SH_UDI_ERR_IN_TAKING_LOCK; //SHASHI Failure of failure cases ahould be ignored giving CRITICAL logs
						}
						else
						{
							//	nothing
						}
						
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						 "rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtSendToWrkr FAILED user_cntxt_id =%u,user_identity=%s,a_opcode=%d,udiref=%s pool_indx= %u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
						 lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode, l_udi_ref_concat, lp_udi_proc_req->req_body.prov_msg.msg_body.elem_indx[l_srv_cnt_dft_check],a_prov_from_rest,a_prov_servc_counter);						
						
						 l_negative_hndling = true;
						 
						 //SHASHI_28042012 : bug fix;
						 lp_cntxt->processing_req.proc_req_list.pop_front();
					}
					else
					{
						//XYZ_SHASHI send to worker success, set return code
						l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
						ar_cntxt_indx_ret=l_cntxt_id;//dharm_28102012
						
					}
				}//end of if StoreRepositoryData
				else
				{

					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtStoreDataRefData FAILED for user_cntxt_id =%u,user_identity=%s,a_opcode=%d,udiref=%s,a_prov_from_rest=%d,a_prov_servc_counter=%d",
						lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode, l_udi_ref_concat,a_prov_from_rest,a_prov_servc_counter);					

					l_negative_hndling = true;
					
					
				}
				
				if(l_negative_hndling)
				{
					if(lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_state & RT_SH_UDI_PROV_INTF_CNTXT_NEWLY_CREATED)
					{
						rtEraseUserIdenVsCntxtMapEntry(ap_user_identity);

						mp_user_cntxt_mgr->rtReturnCntxt(l_cntxt_id);
					}
					else
					{
						lp_cntxt->cntxt_state = RT_SH_UDI_CNTXT_IDLE;
						
						//HP_CR(DONE)::lp_cntxt->processing_req.proc_req_list.pop()
						
						//lp_cntxt->processing_req.proc_req_list.pop_front();//SHASHI_28042012 core dumped 
						
						mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
					}
					
					l_func_ret_val = RT_SH_UDI_ERR_RSRC_TEMP_UNAVAILABLE;
				}


			}//end of if(l_handle_wrkr_case)
			
		}break;
		
		case RT_O_SH_UDI_PROV_INTF_VIEW_SRVC_REQ:
		case RT_O_SH_UDI_PROV_INTF_VIEW_SRVC_LIST_REQ:
		case RT_O_SH_UDI_PROV_INTF_DELETE_SRVC_REQ:
		{
		
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
				"rtProcessSrvcProvReq() DEL/VIEW called from prov_intf for user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
				 ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);

			l_rval = rtFindCntxtForUserIden(ap_user_identity,l_cntxt_id);
			
			if(RT_SUCCESS !=	l_rval)
	  	{
				l_rval = mp_user_cntxt_mgr->rtGetNewCntxt(&lp_cntxt);
				//l_cntxt_id	=	lp_cntxt->session_cookie.self_indx;//SHASHI_30042012 Not needed
				
				if( RT_SUCCESS != l_rval )
				{
					l_func_ret_val = RT_SH_UDI_ERR_RSRC_TEMP_UNAVAILABLE;
					
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"rtProcessSrvcProvReq() mp_user_cntxt_mgr->rtGetNewCntxt returned =%d for user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
						l_rval, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);
				}
				else
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
						"rtProcessSrvcProvReq() rtGetNewCntxt success cntxt_id =%u for user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
						lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);
				
					l_cntxt_id	=	lp_cntxt->session_cookie.self_indx;
					string l_user_id(ap_user_identity);
					lp_cntxt->alias_ids_list.push_back(l_user_id);
					
					l_rval = rtInsertUserIdenVsCntxtMapEntry(ap_user_identity,lp_cntxt->session_cookie.self_indx);
				
					if( RT_SUCCESS != l_rval )
					{
					
						//shall not happen
						l_func_ret_val = RT_SH_UDI_ERR_IN_INTRNL_MAP;
						
						mp_user_cntxt_mgr->rtReturnCntxt(lp_cntxt->session_cookie.self_indx);
						
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
							"rtProcessSrvcProvReq() rtInsertUserIdenVsCntxtMapEntry FAILED for user_cntxt_id =%u,user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
							lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);					
					}
					else
					{
					
						//Important
						//- set cntxt_state RtShUDIProcReq->req_body->RtShProvIntfMsg |= RT_SH_UDI_PROV_INTF_CNTXT_NEWLY_CREATED
						lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_state |= RT_SH_UDI_PROV_INTF_CNTXT_NEWLY_CREATED;
						lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_indx	= lp_cntxt->session_cookie.self_indx;
						
						l_handle_wrkr_case  			= true;
					}
				
				}
			}			
			else
			{			
				
				if ( RT_SUCCESS == mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt) )
				{
					lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_indx	= lp_cntxt->session_cookie.self_indx;//Moved up
					
					//TRAFFIX_STACK_TIMEOUT_NOT_RCVD_CHANGES - 27102012
					//l_func_ret_val = rtCheckForMissedTimeoutFromDiaStack(lp_cntxt, ap_user_identity); 
			
					//if(RT_SH_UDI_ERR_STACK_TIMEOUT_NOT_RCVD != l_func_ret_val)
					//{
						//SHASHI_29062012 UDA_RECEIVED_CHECKING
						if(lp_cntxt->cntxt_flag & RT_SH_UDI_UDA_RECEIVED)
						{
							if(RT_O_SH_UDI_PROV_INTF_VIEW_SRVC_LIST_REQ == a_opcode)
							{
								mp_data_pool_mgr->rtPrepareListOfServices(lp_cntxt->p_data_ref_arr, lp_cntxt->num_data_ref, (RtS8T*)ap_data[0]);

								if(RT_SUCCESS != mp_cache_keeper->rtReCacheData(lp_cntxt->cache_id) )
								{
									 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
											 "rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtReCacheData() failed user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,lp_cntxt->cache_id=%lld,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											 ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,lp_cntxt->cache_id,a_prov_from_rest,a_prov_servc_counter);

									 RtCacheKeeperBuffer l_buffer;
									 bzero(&l_buffer,sizeof(l_buffer));
									 l_buffer.index = lp_cntxt->session_cookie.self_indx;

									 RtTransAddr 	l_self_addr;
									 memset(&l_self_addr,0,sizeof(l_self_addr));
									 RtMglIntf::rtGetInstance()->rtGetSelfAddress(l_self_addr);//SHASHI_20102012
									 l_self_addr.ee_id = RT_EE_SH_UDI_WRKR_BASE  + (l_buffer.index % m_num_wrkr);

									 if( RT_SUCCESS != mp_cache_keeper->rtCacheData(&l_buffer,&l_self_addr,lp_cntxt->cache_id) )
									 {
										 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
										 "rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtReCacheData() failed, rtCacheData() also FAILED user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,lp_cntxt->cache_id=%lld,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											 ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,lp_cntxt->cache_id,a_prov_from_rest,a_prov_servc_counter);


									 }
									 else
									 {
										 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
											 "rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtReCacheData() failed, rtCacheData() SUCCESS user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,lp_cntxt->cache_id=%lld,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											 ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,lp_cntxt->cache_id,a_prov_from_rest,a_prov_servc_counter);
									 }
								}
								l_func_ret_val	= RT_SUCCESS;
								mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
							}
							else if(a_opcode == RT_O_SH_UDI_PROV_INTF_VIEW_SRVC_REQ)
							{
									//check whether data already exists for service in user context 
			 					 RtBoolT l_is_srv_data_exist=false;
								 for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)
								 {

									if(   !(lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID)
										) 
									{

										// set false in pool element:: so can check at PROV side invalid then no decode
										strcpy((char *)ap_data[l_srv_cnt_dft_check],"INVALID");
										// END
										mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
											"rtProcessSrvcProvReq() service data state not valid  user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
									}
									else if(( (lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID)
													&&
													(lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY)	
												)
												)
									{
												l_func_ret_val = RT_SUCCESS;

											//HP_CR(DONE):: call rtCopyPoolElem() instead of rtGetDataRefData, use ap_data

											//NEW data needs to be decoded before copying
											void* l_elm_ptr	=	NULL;

											l_rval	= mp_data_pool_mgr->rtPointToPoolElem(l_udi_ref[l_srv_cnt_dft_check], lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].elem_indx, &l_elm_ptr);
											if(RT_SUCCESS !=	l_rval)
											{
												mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
													"rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtPointToPoolElem FAILED user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
													ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

												l_func_ret_val = RT_SH_UDI_ERR_RSRC_TEMP_UNAVAILABLE;
											}
											else
											{
												if(RT_SUCCESS != mp_cache_keeper->rtReCacheData(lp_cntxt->cache_id) )
												{
													 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
															 "rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtReCacheData() failed user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,lp_cntxt->cache_id=%lld,a_prov_from_rest=%d,a_prov_servc_counter=%d",
															 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,lp_cntxt->cache_id,a_prov_from_rest,a_prov_servc_counter);

													 RtCacheKeeperBuffer l_buffer;
													 bzero(&l_buffer,sizeof(l_buffer));
													 l_buffer.index = lp_cntxt->session_cookie.self_indx;

													 RtTransAddr 	l_self_addr;
													 memset(&l_self_addr,0,sizeof(l_self_addr));
													 RtMglIntf::rtGetInstance()->rtGetSelfAddress(l_self_addr);//SHASHI_20102012
													 l_self_addr.ee_id = RT_EE_SH_UDI_WRKR_BASE  + (l_buffer.index % m_num_wrkr);

													 if( RT_SUCCESS != mp_cache_keeper->rtCacheData(&l_buffer,&l_self_addr,lp_cntxt->cache_id) )
													 {
														 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
														 "rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtReCacheData() failed, rtCacheData() also FAILED user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,lp_cntxt->cache_id=%lld,a_prov_from_rest=%d,a_prov_servc_counter=%d",
															 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,lp_cntxt->cache_id,a_prov_from_rest,a_prov_servc_counter);


													 }
													 else
													 {
														 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
															 "rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtReCacheData() failed, rtCacheData() SUCCESS user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,lp_cntxt->cache_id=%lld,a_prov_from_rest=%d,a_prov_servc_counter=%d",
															 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,lp_cntxt->cache_id,a_prov_from_rest,a_prov_servc_counter);
													 }


												}//end of if(recache fails)
												else
												{
													//recache success; enjoy!
												}

												RtBoolT l_is_dec_func_null = true;

												l_rval	= mp_data_pool_mgr->rtInvokeSrvcDataProvDecFunc(l_udi_ref[l_srv_cnt_dft_check],a_opcode,l_elm_ptr,ap_data[l_srv_cnt_dft_check],l_is_dec_func_null);
												if(RT_SUCCESS	!=	l_rval)
												{
													l_rval = RT_SH_UDI_ERR_RSRC_TEMP_UNAVAILABLE;

													mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
													"rtProcessSrvcProvReq()::rtInvokeSrvcDataProvDecFunc FAILED,opcode=%d,udi_ref=%d,cntxt_id=%u,user_id=%s,a_prov_servc_counter=%d",
													a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,ap_user_identity,a_prov_from_rest,a_prov_servc_counter);

												}

												if(l_is_dec_func_null)
												{
													l_is_srv_data_exist=true;
													mp_data_pool_mgr->rtCopyPoolElem(l_udi_ref[l_srv_cnt_dft_check],lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].elem_indx, ap_data[l_srv_cnt_dft_check]);
												}

											}


											//NEW ENDS
											ar_is_mandatory = true;//Himanshu 16/03/12

											//mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
																			
									}
									else if (
														( (lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID)
												  							&&
															!(lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY)
														)
													)	
									{
										l_func_ret_val = RT_SUCCESS;

										void* l_elm_ptr	=	NULL;

										l_rval	= mp_data_pool_mgr->rtPointToPoolElem(l_udi_ref[l_srv_cnt_dft_check], lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].elem_indx, &l_elm_ptr);
										if(RT_SUCCESS !=	l_rval)
										{
											mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
												"rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtPointToPoolElem FAILED user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
												ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

											l_func_ret_val = RT_SH_UDI_ERR_RSRC_TEMP_UNAVAILABLE;
										}
										else
										{
											if(RT_SUCCESS != mp_cache_keeper->rtReCacheData(lp_cntxt->cache_id) )
											{
												 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
														 "rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtReCacheData() failed user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,lp_cntxt->cache_id=%lld,a_prov_from_rest=%d,a_prov_servc_counter=%d",
														 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,lp_cntxt->cache_id,a_prov_from_rest,a_prov_servc_counter);

												 RtCacheKeeperBuffer l_buffer;
												 bzero(&l_buffer,sizeof(l_buffer));
												 l_buffer.index = lp_cntxt->session_cookie.self_indx;

												 RtTransAddr 	l_self_addr;
												 memset(&l_self_addr,0,sizeof(l_self_addr));
												 RtMglIntf::rtGetInstance()->rtGetSelfAddress(l_self_addr);//SHASHI_20102012
												 l_self_addr.ee_id = RT_EE_SH_UDI_WRKR_BASE  + (l_buffer.index % m_num_wrkr);

												 if( RT_SUCCESS != mp_cache_keeper->rtCacheData(&l_buffer,&l_self_addr,lp_cntxt->cache_id) )
												 {
													 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
													 "rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtReCacheData() failed, rtCacheData() also FAILED user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,lp_cntxt->cache_id=%lld,a_prov_from_rest=%d,a_prov_servc_counter=%d",
														 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,lp_cntxt->cache_id,a_prov_from_rest,a_prov_servc_counter);


												 }
												 else
												 {
													 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
														 "rtProcessSrvcProvReq() RtShUDIDataPoolMgr::rtReCacheData() failed, rtCacheData() SUCCESS user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,lp_cntxt->cache_id=%lld,a_prov_from_rest=%d,a_prov_servc_counter=%d",
														 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,lp_cntxt->cache_id,a_prov_from_rest,a_prov_servc_counter);
												 }


											}//end of if(recache fails)

											RtBoolT l_is_dec_func_null = true;

											l_rval	= mp_data_pool_mgr->rtInvokeSrvcDataProvDecFunc(l_udi_ref[l_srv_cnt_dft_check],a_opcode,l_elm_ptr,ap_data[l_srv_cnt_dft_check],l_is_dec_func_null);
											if(RT_SUCCESS	!=	l_rval)
											{
												l_rval = RT_SH_UDI_ERR_RSRC_TEMP_UNAVAILABLE;

												mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
												"rtProcessSrvcProvReq()::rtInvokeSrvcDataProvDecFunc FAILED,opcode=%d,udi_ref=%d,cntxt_id=%u,user_id=%s,a_prov_from_rest=%d,a_prov_servc_counter=%d",
												a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,ap_user_identity,a_prov_from_rest,a_prov_servc_counter);

											}

											if(l_is_dec_func_null)
											{
												l_is_srv_data_exist=true;
												mp_data_pool_mgr->rtCopyPoolElem(l_udi_ref[l_srv_cnt_dft_check],lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].elem_indx, ap_data[l_srv_cnt_dft_check]);
											}
											ar_is_mandatory = false;
										}

										//mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

									}
									else
									{
										//means service data doesnt exists
										mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
											"rtProcessSrvcProvReq() service data state  mandatory user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
									}
								}//for loop }
								
								if(l_is_srv_data_exist)
								{
									mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
											"rtProcessSrvcProvReq() service data successfully get for atleast one service  user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
									l_func_ret_val = RT_SUCCESS;
									mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
									
								}
								else
								{
										//means service data doesnt exists
										mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

										l_func_ret_val = RT_SH_UDI_ERR_SRVC_DATA_DOESNT_EXIST_FOR_USER;

										mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
											"rtProcessSrvcProvReq() service data state not valid  user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											ap_user_identity, a_opcode,l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
									
								}
							}							
							else//means delete
							{
									//check whether data already exists for service in user context 
								 RtBoolT l_hndl_wrkr_case_ut=false;
								 for(l_srv_cnt_dft_check=0;l_srv_cnt_dft_check<a_prov_servc_counter;l_srv_cnt_dft_check++)
								 {

									 if(   !(lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID)
										 ) 
									 {
										 //means service data doesnt exists
										 if((lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data.Is_Ut_request))
										 {
											 if(a_prov_servc_counter!=2)
											 {
												 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

												 l_func_ret_val = RT_SH_UDI_ERR_SRVC_DATA_DOESNT_EXIST_FOR_USER;

												 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
													 "rtProcessSrvcProvReq() service data state not valid  user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
													 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

												 break;
											}
											else
											{
												if(l_srv_cnt_dft_check==0)
												{
														//swapping of OCB DATA at zero position
														lp_udi_proc_req->req_body.prov_msg.msg_body.app_enum[0]						=lp_udi_proc_req->req_body.prov_msg.msg_body.app_enum[1];
														lp_udi_proc_req->req_body.prov_msg.msg_body.udi_ref[0] 						= lp_udi_proc_req->req_body.prov_msg.msg_body.udi_ref[1];
														lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[0] 				=lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[1];
														lp_udi_proc_req->req_body.prov_msg.msg_body.data_size[0]    			=  lp_udi_proc_req->req_body.prov_msg.msg_body.data_size[1];
												    lp_udi_proc_req->req_body.prov_msg.msg_body.number_of_prov_service--;	//Multi_service_Creation
												}
												else
												{
													 lp_udi_proc_req->req_body.prov_msg.msg_body.number_of_prov_service--;
												}
											}
										 }
										 else
										 {
											 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

											 l_func_ret_val = RT_SH_UDI_ERR_SRVC_DATA_DOESNT_EXIST_FOR_USER;

											 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
												 "rtProcessSrvcProvReq() service data state not valid  user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
												 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

											 break;
										 	
										 }
 
									 }
									 else if(( (lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID)
													 &&
													 (lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY)	
												 )
												 )
									 {


										 if(lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data.Is_Ut_request)
										 { //RT_O_SH_UDI_PROV_INTF_DELETE_SRVC_REQ
												if(a_prov_servc_counter!=2)
												{
													 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

													 //HP_CR(DONE):: error indicating that data cant be deleted for MANDATORY
													 l_func_ret_val = RT_SH_UDI_DLETE_NOT_ALLOWED_AS_MANDATORY;
													 break;
												}
												else
												{
													 if(l_srv_cnt_dft_check==0)
													 {
															 //swapping of OCB DATA at zero position
															 lp_udi_proc_req->req_body.prov_msg.msg_body.app_enum[0]						=lp_udi_proc_req->req_body.prov_msg.msg_body.app_enum[1];
															 lp_udi_proc_req->req_body.prov_msg.msg_body.udi_ref[0] 						= lp_udi_proc_req->req_body.prov_msg.msg_body.udi_ref[1];
															 lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[0] 				=lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[1];
															 lp_udi_proc_req->req_body.prov_msg.msg_body.data_size[0]    			=  lp_udi_proc_req->req_body.prov_msg.msg_body.data_size[1];
												    	 lp_udi_proc_req->req_body.prov_msg.msg_body.number_of_prov_service--;	//Multi_service_Creation
													 }
													 else
													 {
															lp_udi_proc_req->req_body.prov_msg.msg_body.number_of_prov_service--;
													 }
												
											  }	 
										 }
										 else
										 {	
									 			mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

											 //HP_CR(DONE):: error indicating that data cant be deleted for MANDATORY
											 l_func_ret_val = RT_SH_UDI_DLETE_NOT_ALLOWED_AS_MANDATORY;
											 break;
										 }	



										 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
											 "rtProcessSrvcProvReq() service data state  mandatory user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
											 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
									 }
									 else if (
														 ( (lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID)
												  							 &&
															 !(lp_cntxt->p_data_ref_arr[l_udi_ref[l_srv_cnt_dft_check]].data_state & RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY)
														 )
													 )	
									 {

										 if( RT_O_SH_UDI_PROV_INTF_DELETE_SRVC_REQ == a_opcode )
										 {
												if(lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data.Is_Ut_request && a_prov_servc_counter==2 )
												{
													l_hndl_wrkr_case_ut=true;
												}
												if((l_srv_cnt_dft_check==(a_prov_servc_counter-1)) && !(lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data.Is_Ut_request && a_prov_servc_counter==2 ))
												{	
													 if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
													 {
														 l_handle_wrkr_case = true;
													 }
													 else
													 {

														 if ( (lp_cntxt->cntxt_flag & 	RT_SH_UDI_USR_CNTXT_MARKED_FOR_DELETION) )
														 {
															 l_func_ret_val = RT_SH_UDI_ERR_USR_CNTXT_MARKED_FOR_DELETION;
														 }
														 else
														 {
															 lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);

															 //SHASHI_18102012 : counters
															 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProvReqPushedInPend);

															 l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
															 ar_cntxt_indx_ret=l_cntxt_id;//dharm_28102012

															 //??SHASHI_02052012 lp_cntxt->cntxt_flag |= 	RT_SH_UDI_USR_CNTXT_MARKED_FOR_DELETION;

															 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
																 "rtProcessSrvcProvReq() adding DELETE req to pending queue user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
																 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
														 }

														 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);


													 }//end of else of if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
											 }
											 else
											 {
															 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
																 "rtProcessSrvcProvReq() normal check going on user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
																 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

											 }

										 }

									 }
									 else
									 {
										 //means service data doesnt exists
										if(lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data.Is_Ut_request)
										{
											if(a_prov_servc_counter!=2)
											{
												 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

												 //HP_CR(DONE):: appropriate request
												 l_func_ret_val = RT_SH_UDI_ERR_USER_DATA_DOESNT_EXIST;

												 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
													 "rtProcessSrvcProvReq() service data state  mandatory user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
													 ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
												 break;
											}
											else
											{
													 if(l_srv_cnt_dft_check==0)
													 {
															 //swapping of OCB DATA at zero position
															 lp_udi_proc_req->req_body.prov_msg.msg_body.app_enum[0]						=lp_udi_proc_req->req_body.prov_msg.msg_body.app_enum[1];
															 lp_udi_proc_req->req_body.prov_msg.msg_body.udi_ref[0] 						= lp_udi_proc_req->req_body.prov_msg.msg_body.udi_ref[1];
															 lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[0] 				=lp_udi_proc_req->req_body.prov_msg.msg_body.p_data_ptr[1];
															 lp_udi_proc_req->req_body.prov_msg.msg_body.data_size[0]    			=  lp_udi_proc_req->req_body.prov_msg.msg_body.data_size[1];
												    	 lp_udi_proc_req->req_body.prov_msg.msg_body.number_of_prov_service--;	//Multi_service_Creation
													 }
													 else
													 {
															lp_udi_proc_req->req_body.prov_msg.msg_body.number_of_prov_service--;
													 }
											
											} 
										}
										else 
										{	
											mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

											//HP_CR(DONE):: appropriate request
											l_func_ret_val = RT_SH_UDI_ERR_USER_DATA_DOESNT_EXIST;

											mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
												"rtProcessSrvcProvReq() service data state  mandatory user_id=%s,opcode=%d,udi_ref=%d,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
												ap_user_identity, a_opcode, l_udi_ref[l_srv_cnt_dft_check], l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
											break;
										}	
									}

								 }//for loop }
								 if(a_prov_servc_counter==2 && lp_udi_proc_req->req_body.prov_msg.msg_body.number_of_prov_service==0 && (lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data.Is_Ut_request))
								 {
											mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

											//HP_CR(DONE):: appropriate request
											l_func_ret_val = RT_SH_UDI_ERR_USER_DATA_DOESNT_EXIST;

											mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
												"rtProcessSrvcProvReq() All service donest have data user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
												ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
								 		
								 }
								 else if(l_hndl_wrkr_case_ut && a_prov_servc_counter==2 && (lp_udi_proc_req->req_body.prov_msg.msg_body.ut_req_data.Is_Ut_request))
								 {	
										 if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
										 {
											 l_handle_wrkr_case = true;
										 }
										 else
										 {

											 if ( (lp_cntxt->cntxt_flag & 	RT_SH_UDI_USR_CNTXT_MARKED_FOR_DELETION) )
											 {
												 l_func_ret_val = RT_SH_UDI_ERR_USR_CNTXT_MARKED_FOR_DELETION;
											 }
											 else
											 {
												 lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);

												 //SHASHI_18102012 : counters
												 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProvReqPushedInPend);

												 l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
												 ar_cntxt_indx_ret=l_cntxt_id;//dharm_28102012

												 //??SHASHI_02052012 lp_cntxt->cntxt_flag |= 	RT_SH_UDI_USR_CNTXT_MARKED_FOR_DELETION;

												 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
													 "rtProcessSrvcProvReq() adding DELETE req to pending queue user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
													 ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);
											 }

											 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);


										 }//end of else of if( RT_SH_UDI_CNTXT_IDLE == lp_cntxt->cntxt_state )
									}
							}
						}	
						else
						{

							 //Push Request in Pending Queue
							 lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_state |= RT_SH_UDI_PROV_DISPATCHED_DUE_TO_UDA_NOT_RECEIVED;

							 lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);

							 //SHASHI_18102012 : counters
							 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProvReqPushedInPend);

							 l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
							 ar_cntxt_indx_ret=l_cntxt_id;//dharm_28102012

							 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
								 "rtProcessSrvcProvReq() adding VIEW/DELETE req to pending queue user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
								 ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

							 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);


						}				
				//	}
				}//end of if of mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt) )
				else
				{
					//shall not happen	give ALERT Logs
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"rtProcessSrvcProvReq() entry in map exists but retrieve failed user_id=%s,opcode=%d,udi_ref=%s,cntxt_id=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
						ap_user_identity, a_opcode, l_udi_ref_concat, l_cntxt_id,a_prov_from_rest,a_prov_servc_counter);

					RtU32T		l_block_no	= 0;
					rtDecideBlock(ap_user_identity, l_block_no);
					rtEraseUserIdenVsCntxtMapEntry(ap_user_identity,l_block_no);

					// get new context and set state and processing state 
					l_rval = mp_user_cntxt_mgr->rtGetNewCntxt(&lp_cntxt);

					if( RT_SUCCESS != l_rval )
					{
						l_func_ret_val = RT_SH_UDI_ERR_RSRC_TEMP_UNAVAILABLE;

						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
							"rtProcessSrvcProvReq() mp_user_cntxt_mgr->rtGetNewCntxt returned =%d for user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
							l_rval, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);
					}
					else
					{
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
							"rtProcessSrvcProvReq() rtGetNewCntxt success cntxt_id =%u for user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
							lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);

						l_cntxt_id	=	lp_cntxt->session_cookie.self_indx;
						string l_user_id(ap_user_identity);
						lp_cntxt->alias_ids_list.push_back(l_user_id);

						l_rval = rtInsertUserIdenVsCntxtMapEntry(ap_user_identity,lp_cntxt->session_cookie.self_indx);

						if( RT_SUCCESS != l_rval )
						{

							//shall not happen
							l_func_ret_val = RT_SH_UDI_ERR_IN_INTRNL_MAP;

							mp_user_cntxt_mgr->rtReturnCntxt(lp_cntxt->session_cookie.self_indx);

							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
								"rtProcessSrvcProvReq() rtInsertUserIdenVsCntxtMapEntry FAILED for user_cntxt_id =%u,user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,a_prov_from_rest=%d,a_prov_servc_counter=%d",
								lp_cntxt->session_cookie.self_indx, ap_user_identity, a_opcode, l_app_enum_concat, a_token,a_prov_from_rest,a_prov_servc_counter);					
						}
						else
						{

							//Important
							//- set cntxt_state RtShUDIProcReq->req_body->RtShProvIntfMsg |= RT_SH_UDI_PROV_INTF_CNTXT_NEWLY_CREATED
							lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_state |= RT_SH_UDI_PROV_INTF_CNTXT_NEWLY_CREATED;
							lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_indx	= lp_cntxt->session_cookie.self_indx;

							l_handle_wrkr_case  			= true;
						}

					}

				}//end of else of shall not happen
			}//else of entry not found
		
			if(l_handle_wrkr_case)
			{
				lp_cntxt->cntxt_state 						= RT_SH_UDI_CNTXT_NON_IDLE;
				
				lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROV_INTF_REQ ;
				
				lp_cntxt->processing_req.proc_req_list.push_front(*lp_udi_proc_req);

				lp_cntxt->processing_req.curr_req_iter = lp_cntxt->processing_req.proc_req_list.begin();

				mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

				//		- send to worker threads MODULO by l_cntxt_id using function

				l_rval = rtSendToWrkr(&l_trans_msg, l_cntxt_id);

				if( RT_FAILURE == l_rval)
				{

					//HP_CR(DONE):: take context lock using retrivecontxtddata lock
					mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt);

					if(lp_udi_proc_req->req_body.prov_msg.msg_body.cntxt_state & RT_SH_UDI_PROV_INTF_CNTXT_NEWLY_CREATED)
					{
						rtEraseUserIdenVsCntxtMapEntry(ap_user_identity);

						mp_user_cntxt_mgr->rtReturnCntxt(l_cntxt_id);
					}
					else
					{
						lp_cntxt->cntxt_state = RT_SH_UDI_CNTXT_IDLE;

						//HP_CR(DONE):: pop from lp_cntxt->processing_req.proc_req_list.pop()
						lp_cntxt->processing_req.proc_req_list.pop_front();

						mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
					}

					l_func_ret_val = RT_SH_UDI_ERR_RSRC_TEMP_UNAVAILABLE;

				}
				else
				{ 
					ar_cntxt_indx_ret =  l_cntxt_id;//dharm_28102012
					l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
				}

			}//end of if(l_handle_wrkr_case)
			
		}break;
		
		default:
		{
			l_rval = RT_SH_UDI_ERR_INVALID_OPCODE;
		}break;
	}
	

	//NEW 	
	if(	l_is_memory_allocated_for_encoded_buffer)
	{
		delete lp_encoded_buffer;
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtProcessSrvcProvReq() deleted lp_encoded_buffer for with ap_user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,ret_val=%d,a_prov_from_rest=%d,a_prov_servc_counter=%d",
		ap_user_identity, a_opcode, l_app_enum_concat, a_token,l_func_ret_val,a_prov_from_rest,a_prov_servc_counter);
		
	}

	if(l_func_ret_val	==	RT_SUCCESS)
	{
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetSucc);
	}
	else if(l_func_ret_val == RT_SH_UDI_WAIT_FOR_RSP)
	{
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqWaitRsp);
	}
	else
	{
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetFail);
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"EXIT RtShUserDataIntf :: rtProcessSrvcProvReq() with ap_user_identity=%s,a_opcode=%d,a_app_enum=%s a_token=%u,ret_val=%d,a_prov_from_rest=%d,a_prov_servc_counter=%d",
	ap_user_identity, a_opcode, l_app_enum_concat, a_token,l_func_ret_val,a_prov_from_rest,a_prov_servc_counter);

 return l_func_ret_val;
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtDecideBlock()
 *
 * DESCRIPTION   : - This function is called to decide the block no to used
 *                   when m_user_iden_vs_cntxt_map is to be accessed
 *                 
 *
 * INPUT         : RtS8T* ap_service_indic, 
 *
 * OUTPUT        : RtU32T& ar_block_no
 *
 * RETURN        : void
 *
 ******************************************************************************/
void RtShUserDataIntf ::rtDecideBlock(RtS8T* ap_user_identity,RtU32T& ar_block_no)
{
	//use some algorithm based on ap_user_identity to decide block no.
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtDecideBlock() with user_identity=%s,length=%d",ap_user_identity,strlen(ap_user_identity));
		
	RtS8T l_first_letter = 0;
	if(strlen(ap_user_identity) < 6)
	{
		ar_block_no = 0;
	}
	else
	{
		l_first_letter	= ap_user_identity[5];
		ar_block_no = l_first_letter % RT_SH_UDI_MAX_NUM_USER_IDEN_BLOCKS;
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtDecideBlock() with user_identity=%s,length=%d,first_letter =%d,ar_block =%u",ap_user_identity,strlen(ap_user_identity),l_first_letter,ar_block_no);
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtGetUDIRefFromSrvcInd()
 *
 * DESCRIPTION   : - This function is called to obtain UDI reference corresponding to
 *                  service indication value.
 *                 - Required in scenarios when DIAMETER msg is received from HSS
 *                 
 *
 * INPUT         : RtS8T* ap_service_indic, 
 *
 * OUTPUT        : RtShUDIRef& ar_udi_ref
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtGetUDIRefFromSrvcInd(RtS8T* ap_service_indic, RtShUDIRef& ar_udi_ref)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtGetUDIRefFromSrvcInd() with ap_service_indic=%s",ap_service_indic);
	
	RtRcT l_ret_val = RT_SUCCESS;
	RtSrvcIndVsSrvcTokenDataMapItr	l_map_itr;
	l_map_itr = m_srvc_ind_vs_srvc_token_map.find(ap_service_indic);
	if(l_map_itr	!=	m_srvc_ind_vs_srvc_token_map.end())
	{
		ar_udi_ref	= l_map_itr->second.srvc_token;
		
	}
	else
	{
		l_ret_val 	= RT_FAILURE;
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtGetUDIRefFromSrvcInd() with ap_service_indic=%s,ar_udi_ref =%u,ret_val = %d",
		ap_service_indic, ar_udi_ref, l_ret_val);
		
	return l_ret_val;	
}
	
	
/*******************************************************************************
 *
 * FUNCTION NAME : rtGetUDIRefFromAppEnum()
 *
 * DESCRIPTION   : - This function is called to obtain UDI reference corresponding to
 *                  application enum value for service
 *                 - Required in scenarios when provisioning interface invokes class API
 *                 
 *
 * INPUT         : RtU32T a_app_enum 
 *
 * OUTPUT        : RtShUDIRef& ar_udi_ref
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtGetUDIRefFromAppEnum(RtU32T a_app_enum, RtShUDIRef& ar_udi_ref)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtGetUDIRefFromAppEnum() with a_app_enum=%d",a_app_enum);
	
	RtRcT l_ret_val = RT_SUCCESS;
	RtApplEnumVsUDIRefMapItr	l_map_itr;
	l_map_itr = m_app_enum_vs_udi_ref_map.find(a_app_enum);
	if(l_map_itr	!=	m_app_enum_vs_udi_ref_map.end())
	{
		ar_udi_ref	= l_map_itr->second;
		
	}
	else
	{
		l_ret_val 	= RT_FAILURE;
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtGetUDIRefFromAppEnum() with a_app_enum=%d,ar_udi_ref =%u,ret_val = %d",
		a_app_enum, ar_udi_ref, l_ret_val);
	return l_ret_val;	
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtCheckNStoreNonTransData()
 *
 * DESCRIPTION   : This function is called to store non-transparent data in pool when 
 * 								UDA/PNR comes from HSS
 *                 
 * INPUT         : RtU8T reg_data_ref, RtShUDIUserCntxt* ap_cntxt,RtShUserData* ap_sh_user_data
 *
 * OUTPUT        : NONE
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/

RtRcT RtShUserDataIntf	::	rtCheckNStoreNonTransData(RtU8T a_reg_data_ref, 
																											RtShUDIUserCntxt* ap_cntxt,
																											RtShUserData* ap_sh_user_data)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtCheckNStoreNonTransData() with reg_data_ref=%u,user_index =%u",
		a_reg_data_ref, ap_cntxt->session_cookie.self_indx);
	

	RtRcT 	l_fun_ret_val 	= RT_SUCCESS;
	void* 	lp_void_ptr 		= NULL;
	
	ap_cntxt->p_data_ref_arr[a_reg_data_ref].data_state  	&=	~RT_SH_UDI_DATA_STATE_VALID;
	ap_cntxt->p_data_ref_arr[a_reg_data_ref].p_data_ptr		= NULL;
	ap_cntxt->p_data_ref_arr[a_reg_data_ref].elem_indx 		= 0;
		
		
	switch(a_reg_data_ref)
	{
		case	RT_DIA_SH_IMS_PUBLIC_ID://IMSPublicIdentity DB_19022013 Added all cases; Dataref 10 not being sent in SNR
		{
			if((ap_sh_user_data->flag	& RT_SH_PUBLIC_ID_PRESENT) ||
					(ap_sh_user_data->flag	& RT_SH_ALIAS_IDS_PRESENT) ||
					(ap_sh_user_data->flag	& RT_SH_ALL_IDS_PRESENT)	||
					(ap_sh_user_data->flag	& RT_SH_IMPLICIT_IDS_PRESENT)	||
					(ap_sh_user_data->flag	& RT_SH_REG_IDS_PRESENT))
			{
				
				lp_void_ptr = (void*) &ap_sh_user_data->public_id;
				
			}	
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() IMSPublicIdentity not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
				
			}
			
		}
		break;
		
		case	RT_DIA_SH_IMS_USER_STATE://IMSUserState
		{
			if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
				(ap_sh_user_data->sh_ims_data.flag	& RT_SH_IMS_DATA_IMS_USER_STATE_PRESENT))
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_ims_data.ims_user_state;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() IMSUserState not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;
		
		case	RT_DIA_SH_SCSCF_NAME://S-CSCFName 
		{
			if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
				(ap_sh_user_data->sh_ims_data.flag	& RT_SH_IMS_DATA_SCSCF_PRESENT))
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_ims_data.scscf_name;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() S-CSCFName not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;
		
		case	RT_DIA_SH_IFC://InitialFilterCriteria
		{
			if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
				(ap_sh_user_data->sh_ims_data.flag	& RT_SH_IMS_DATA_IFCS_PRESENT))
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_ims_data.ifcs;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() InitialFilterCriteria not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;
		
		case	RT_DIA_SH_LOCATION_INFO://LocationInformation
		{
			if(ap_sh_user_data->flag	& RT_SH_LOC_INF_PRESENT)
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_location_info;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() LocationInformation not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;
		
		case	RT_DIA_SH_USER_STATE://UserState
		{ //TBD 
			if(ap_sh_user_data->flag	& RT_SH_USER_STATE_PRESENT)
			{
				//HP_CR(DONE): check that RtShCSUserState and RtShPSUserState need to be merged in userdata
				//HP_CR(DONE): use wrapper structure
				lp_void_ptr = (void*) &ap_sh_user_data->user_state;
			}	
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() RT_SH_UDI_REF_USER_STATE not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;
		
		
		case	RT_DIA_SH_CHARGING_INFO://ChargingInformation
		{
			if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
				(ap_sh_user_data->sh_ims_data.flag	& RT_SH_IMS_DATA_CHARG_INFO_PRESENT))
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_ims_data.p_charg_fun_addr;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() RT_DIA_SH_CHARGING_INFO not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;
		

		//HP_CR(DONE): check for other left out cases 
		//HP_CR(DONE): define enum for all cases in header files
		//HP_CR(DONE): check correct flags and use correct structure name
		case	RT_DIA_SH_PSI_ACTIVATION://PSIActivation
		{
			if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
				(ap_sh_user_data->sh_ims_data.flag	& RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.flag & RT_SH_PSI_ACTIVATION_PRESENT))
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_ims_data.ims_data_ext.psi_activation;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() PSIActivation not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;
		
		case	RT_DIA_SH_SERVICE_LEV_TRACE_INFO:
		{
			if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
				(ap_sh_user_data->sh_ims_data.flag	& RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.flag & RT_SH_SERVICE_LEVEL_TRACE_PRESENT))
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.service_level_trace_info;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() service_level_trace_info not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;

    case	RT_DIA_SH_SRV_PRIORITY_LEVEL:
		{
			if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
				(ap_sh_user_data->sh_ims_data.flag	& RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.flag & RT_SH_SERVICE_PRIORITY_LEV_PRESENT))
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.service_priority_level;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() service_priority_level not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;
		
    case	RT_DIA_SH_SMS_REG_INFO:
		{
			if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
				(ap_sh_user_data->sh_ims_data.flag	& RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.flag & RT_SH_SMS_REG_INFO_PRESENT))
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.sms_registration_info;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() sms_registration_info not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;	
		case	RT_DIA_SH_UE_REACHALE_FOR_IP:
		{
			if((ap_sh_user_data->flag & RT_SH_IMS_DATA_PRESENT) &&	
				(ap_sh_user_data->sh_ims_data.flag	& RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.flag & RT_SH_UE_REACH_FOR_IP_PRESENT))
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_ims_data.ims_data_ext.ims_data_ext2.ims_data_ext3.ue_reachability_for_ip;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() ue_reachability_for_ip not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;	
		
		case	RT_DIA_SH_TADS_INFO:
		{
			if((ap_sh_user_data->flag & RT_SH_DATA_EXTENTION_PRESENT) &&	
				(ap_sh_user_data->sh_data_ext.flag	& RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_data_ext.sh_data_ext2.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
				(ap_sh_user_data->sh_data_ext.sh_data_ext2.sh_data_ext3.flag & RT_SH_TDS_INF_PRESENT))
			{
				lp_void_ptr = (void*) &ap_sh_user_data->sh_data_ext.sh_data_ext2.sh_data_ext3.tads_info;
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
					"rtCheckNStoreNonTransData() tads_info not received in UDA,user_index =%u,udi_ref=%u",
					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			}			
			
		}
		break;	
			
		// case	RT_DIA_SH_TADS_INFO:
// 		{
// 			if((ap_sh_user_data->flag & RT_SH_DATA_EXTENTION_PRESENT) &&	
// 				(ap_sh_user_data->sh_data_ext.flag	& RT_SH_DATA_EXTENTION_PRESENT) &&
// 				(ap_sh_user_data->sh_data_ext.sh_data_ext2.flag & RT_SH_DATA_EXTENTION_PRESENT) &&
// 				(ap_sh_user_data->sh_data_ext.sh_data_ext2.sh_data_ext3.flag & RT_SH_TDS_INF_PRESENT))
// 			{
// 				lp_void_ptr = (void*) &ap_sh_user_data->sh_data_ext.sh_data_ext2.sh_data_ext3.tads_info;
// 			}
// 			else
// 			{
// 				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
// 					"rtCheckNStoreNonTransData() tads_info not received in UDA,user_index =%u,udi_ref=%u",
// 					ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
// 			}			
// 			
// 		}
// 		break;	
		
		default:
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtCheckNStoreNonTransData() INVALID Data reference ,user_index =%u,udi_ref=%u",
				ap_cntxt->session_cookie.self_indx,a_reg_data_ref);
			
			l_fun_ret_val = RT_FAILURE;
			
		}
		break;
	}//end of switch
	
	if((RT_SUCCESS == l_fun_ret_val) && (NULL !=	lp_void_ptr))
	{
		l_fun_ret_val = mp_data_pool_mgr->rtStoreDataRefData(a_reg_data_ref, 
																								lp_void_ptr,
																								ap_cntxt->p_data_ref_arr[a_reg_data_ref].elem_indx,
																								&ap_cntxt->p_data_ref_arr[a_reg_data_ref].p_data_ptr);

		if(RT_SUCCESS ==	l_fun_ret_val)
		{
			ap_cntxt->p_data_ref_arr[a_reg_data_ref].data_state |= RT_SH_UDI_DATA_STATE_VALID;

			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"rtCheckNStoreNonTransData() data successfully stored in pool,user_index =%u,udi_ref=%u,elem_indx=%u",
				ap_cntxt->session_cookie.self_indx,a_reg_data_ref,ap_cntxt->p_data_ref_arr[a_reg_data_ref].elem_indx);

		}
		else
		{
			//??shashi_04042012
		}
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtCheckNStoreNonTransData() with reg_data_ref=%u,user_index =%u,ret_val = %d",
		a_reg_data_ref, ap_cntxt->session_cookie.self_indx, l_fun_ret_val);

	return l_fun_ret_val;
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtInvokeAppRspCallback()
 *
 * DESCRIPTION   : This function is called to invoke callback by app_server to send ordered list
 *                 
 * INPUT         : RtU8T reg_data_ref, RtShUDIUserCntxt* ap_cntxt,RtShUserData* ap_sh_user_data
 *
 * OUTPUT        : NONE
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/

RtRcT			RtShUserDataIntf :: rtInvokeAppRspCallback(RtShUDIApplReq* ap_appl_req, RtRcT a_result,RtShUDIOrderedList a_ordered_list)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUserDataIntf ::rtInvokeAppRspCallback() ::ENTER Contex Index [%u],Result [%d],Appl_token[%u],Session_case[%d], Sip_method[%d]",
	ap_appl_req->cntxt_indx, a_result,ap_appl_req->appl_token, ap_appl_req->sess_case, ap_appl_req->sip_method); 									
	//SHASHI_24_04_2012
	RtShUDIOrderedArr	l_ordered_arr;
	memset(&l_ordered_arr,0,sizeof(l_ordered_arr));
	
	l_ordered_arr.num_ordered_elem	=	a_ordered_list.size();

	list<RtShUDIOrderedListElem>::iterator  l_itr;
	RtU32T	l_count = 0;
	for(l_itr = a_ordered_list.begin(); l_itr !=	a_ordered_list.end(); l_itr++)
	{
		l_ordered_arr.ordered_arr_elem[l_count].udi_ref		= l_itr->udi_ref;
		l_ordered_arr.ordered_arr_elem[l_count].app_enum	= l_itr->data_ref_appl_info.app_enum;
		l_count++;
	}

	//SHASHI_24_04_2012 --ENDS

	RtRcT	l_ret_val	=	mp_app_rsp_callback(ap_appl_req, a_result,&l_ordered_arr);
	
	if(RT_SUCCESS	!= l_ret_val)
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
		"RtShUserDataIntf ::rtInvokeAppRspCallback() ::Callback to send reply FAILED Contex Index [%u],Result [%d],Appl_token[%u],Session_case[%d], Sip_method[%d]",
		ap_appl_req->cntxt_indx, a_result,ap_appl_req->appl_token, ap_appl_req->sess_case, ap_appl_req->sip_method); 									
		
	}
	
	return l_ret_val;
}
/*******************************************************************************
 *
 * FUNCTION NAME : rtInvokeSubsRspCallback()
 *
 * DESCRIPTION   : This function is called to invoke callback by app_server to send ordered list
 *                 
 * INPUT         : RtU8T reg_data_ref, RtShUDIUserCntxt* ap_cntxt,RtShUserData* ap_sh_user_data
 *
 * OUTPUT        : NONE
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/

RtRcT			RtShUserDataIntf :: rtInvokeSubsRspCallback(RtShSubsData* ap_subs_data,RtShData* ap_sh_data)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUserDataIntf ::rtInvokeSubsRspCallback() ::ENTER Result =[%d] Appl_token[%u] [user =%s] [service_update = %u] [alias_update = %u]",
	ap_sh_data->result_code, ap_subs_data->appl_token,ap_sh_data->subs_data.impu.val,ap_sh_data->is_service_upd,ap_sh_data->is_alias_upd); 									

	RtRcT	l_ret_val	=	mp_subs_rsp_callback(ap_subs_data,ap_sh_data);
	
	if(RT_SUCCESS	!= l_ret_val)
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			"RtShUserDataIntf ::rtInvokeSubsRspCallback() ::Callback to send reply FAILED Result =[%d] Appl_token[%u] [user =%s][service_update = %u] [alias_update = %u]",
			ap_sh_data->result_code, ap_subs_data->appl_token,ap_sh_data->subs_data.impu.val,ap_sh_data->is_service_upd,ap_sh_data->is_alias_upd); 									
		
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUserDataIntf ::rtInvokeSubsRspCallback() ::LEAVE Result =[%d] Appl_token[%u] [user =%s] [service_update = %u] [alias_update = %u]",
	ap_sh_data->result_code, ap_subs_data->appl_token,ap_sh_data->subs_data.impu.val,ap_sh_data->is_service_upd,ap_sh_data->is_alias_upd); 									

	return l_ret_val;
}
/*******************************************************************************
 *
 * FUNCTION NAME : rtIsSrvcDataAvailable()
 *
 * DESCRIPTION   : This function is called by application server to get udi_ref and flag stating 
 *									if service data is available.
 *									
 *
 * INPUT         : ap_user_identity, a_udi_ref
 *
 * OUTPUT        : ap_udi_ref_data 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtIsSrvcDataAvailable(  RtS8T*        						ap_user_identity,/* Input */
																								 RtS8T*                   ap_entprise_id, 
																								 RtS8T*                   ap_cug_id,   
																								 RtU32T										a_app_enum,	/* Input */
																								 RtShUDIRef&							ar_udi_ref,				/* Output */
																								 RtBoolT& 								ar_if_available) /* Ouput*/
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"ENTER RtShUserDataIntf :: rtIsSrvcDataAvailable() with ap_user_identity=%s, ap_entprise_id = %s, ap_cug_id = %s, a_app_enum=%u",
	ap_user_identity,ap_entprise_id,ap_cug_id, a_app_enum);
	
	RtRcT 						l_ret_val 			= RT_FAILURE;
	RtU32T						l_cntxt_id			= 0;
	RtShUDIUserCntxt*	lp_cntxt;						
										ar_if_available	=	false;	
										ar_udi_ref			=	0;
	
//DB_15012013 failure username="NULL Validation check"
	if(	(ap_user_identity	!= NULL)  && (ap_entprise_id != NULL) && (ap_cug_id	!= NULL))
	{
		//OK let us proceed
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf :: rtIsSrvcDataAvailable() Parameter ValidationFAILED with ap_user_identity=%s, ap_entprise_id = %s, ap_cug_id = %s, a_app_enum=%u",
		ap_user_identity,ap_entprise_id,ap_cug_id, a_app_enum);
			
		return RT_FAILURE;	
		
	}

//DB_15012013 failure username="NULL Validation check" 	

  RtS8T l_usr_ent_cug_id[3*RT_MAX_STRING_LEN] = {'\0'};
	snprintf(l_usr_ent_cug_id,sizeof(l_usr_ent_cug_id),"%s_%s_%s",ap_user_identity,ap_entprise_id,ap_cug_id);

	l_ret_val	= rtFindCntxtForUserIden(l_usr_ent_cug_id, l_cntxt_id);
	if(RT_SUCCESS ==	l_ret_val)
	{
		l_ret_val = mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id,	&lp_cntxt);
		if(RT_SUCCESS ==	l_ret_val)
		{
				l_ret_val	=	rtGetUDIRefFromAppEnum(a_app_enum, ar_udi_ref);
				if(RT_SUCCESS ==	l_ret_val)
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
						"rtIsSrvcDataAvailable() mp_user_cntxt_mgr->rtRetrieveCntxtData() success with ap_user_identity=%s, ap_entprise_id = %s, ap_cug_id = %s, a_udi_ref=%u,l_cntxt_id =%u",
						ap_user_identity,ap_entprise_id,ap_cug_id, ar_udi_ref,l_cntxt_id);	
						
					if(RT_SH_UDI_DATA_STATE_VALID ==	lp_cntxt->p_data_ref_arr[ar_udi_ref].data_state)
					{
						ar_if_available	=	true;
						l_ret_val				=	RT_SUCCESS;
					}								
				}
				else
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
						"rtIsSrvcDataAvailable() mp_user_cntxt_mgr->rtRetrieveCntxtData() FAILED with ap_user_identity=%s, ap_entprise_id = %s, ap_cug_id = %s,l_cntxt_id =%u",
						ap_user_identity,ap_entprise_id,ap_cug_id,l_cntxt_id);				
					
				}
		}
		else
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtIsSrvcDataAvailable() mp_user_cntxt_mgr->rtRetrieveCntxtData() returned FAILURE with ap_user_identity=%s, ap_entprise_id = %s, ap_cug_id = %s",
				ap_user_identity,ap_entprise_id,ap_cug_id);				
		}

		RtRcT l_ret_val_tmp	= mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
		if(RT_SUCCESS ==	l_ret_val_tmp)
		{

			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"rtIsSrvcDataAvailable() mp_user_cntxt_mgr->rtReleaseCntxtDataLock returned SUCCESS with ap_user_identity=%s, ap_entprise_id = %s, ap_cug_id = %s,l_ret_val_tmp=%d",
				ap_user_identity,ap_entprise_id,ap_cug_id,l_ret_val_tmp);		
		}
		else
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtIsSrvcDataAvailable() mp_user_cntxt_mgr->rtReleaseCntxtDataLock returned FAILURE with ap_user_identity=%s, ap_entprise_id = %s, ap_cug_id = %s,l_ret_val_tmp=%d",
				ap_user_identity,ap_entprise_id,ap_cug_id,l_ret_val_tmp);				
		}

	}
	else	
	{

		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
			"rtIsSrvcDataAvailable() user id not found in map ap_user_identity=%s, ap_entprise_id = %s, ap_cug_id = %s",
				ap_user_identity,ap_entprise_id,ap_cug_id);	

		l_ret_val	=	RT_SH_UDI_ERR_USER_UNKNOWN;

	}

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
	"EXIT RtShUserDataIntf :: rtIsSrvcDataAvailable() with ap_user_identity=%s, ap_entprise_id = %s, ap_cug_id = %s,ret_val =%d",
	ap_user_identity,ap_entprise_id,ap_cug_id,l_ret_val);

	return l_ret_val;
}


/*******************************************************************************
 *
 * FUNCTION NAME : rtGetCntxtData().
 *
 * DESCRIPTION   : Function will be called by RtUDIDumpCntxtData class to get user context status
 *
 * INPUT         :  none
 *
 * OUTPUT        : RtCntxtDataStatArr& 
 *
 * RETURN        : void 
 *
 ******************************************************************************/

void		RtShUserDataIntf :: rtGetCntxtData(	RtCntxtDataStatArr&		ar_cntxt_stat)		
{
// 	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
// 	 "RtShUserDataIntf::rtGetCntxtData() ENTER");
	
	if(mp_user_cntxt_mgr == NULL)
	  return;
		
	mp_user_cntxt_mgr->rtGetCntxtData(ar_cntxt_stat);
	mp_data_pool_mgr->rtGetCntxtData(ar_cntxt_stat);
	mp_cache_keeper->rtGetCntxtData(ar_cntxt_stat);
	
	sprintf(ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].resource_name, "UDI-CntxtMapSize");
	ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].allocated = mp_sys_agent->rtGetRtU32T(RT_SH_UDI_MAX_NUM_USR_CNTXT);//zero index is not being used
	ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].max 			= 0;
	ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].current 	= 0;
	for(RtU32T	l_cnt = 0;l_cnt < RT_SH_UDI_MAX_NUM_USER_IDEN_BLOCKS;++l_cnt)
	ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].current 	+= m_user_iden_vs_cntxt_map[l_cnt].size();
	if(ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].current > m_cur_max_usr_cntxt_size)
	{
		m_cur_max_usr_cntxt_size                                  = ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].current;
	}
	ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].max    =    m_cur_max_usr_cntxt_size;
	++ar_cntxt_stat.num_of_cntxts;
	
	for(RtU32T l_cnt=0; l_cnt < m_num_wrkr; l_cnt++)
	{ 
		sprintf(ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].resource_name, "UDI-Wrkr[%u]QSize",l_cnt);
		ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].allocated =  mp_sys_agent->rtGetRtU32T(RT_SH_UDI_WRKR_THRD_Q_SIZE);// not derived from PAM
		
		RtU32T	l_max_q_size	=	0;
		
		RtU32T	l_q_size	=	mp_wrkr_arr[l_cnt]->rtGetQueueSize(l_max_q_size);
		
		ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].current 	= l_q_size;
		
		if(mp_wrkr_arr[l_cnt]->m_cur_max_queue_size < ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].current)
		{
			mp_wrkr_arr[l_cnt]->m_cur_max_queue_size = ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].current;
		}
		ar_cntxt_stat.cntxt_data[ar_cntxt_stat.num_of_cntxts].max 			= mp_wrkr_arr[l_cnt]->m_cur_max_queue_size;
		
		++ar_cntxt_stat.num_of_cntxts;
	}
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtDumpUserCntxt().
 *
 * DESCRIPTION   : Function will be invoked by CLI to dump user context in a file.
 *
 * INPUT         :  RtS8T* ap_user_identity
 *
 * OUTPUT        :  
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUserDataIntf :: rtDumpUserCntxt(RtS8T* ap_user_identity, RtS8T* ap_output_path)		
{
	RtRcT 						l_ret_val 	= RT_FAILURE;
	RtU32T						l_cntxt_id	= 0;
	RtShUDIUserCntxt*	lp_cntxt;
	
	l_ret_val	= rtFindCntxtForUserIden(ap_user_identity, l_cntxt_id);
	if(RT_SUCCESS ==	l_ret_val)
	{
		l_ret_val = mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id,	&lp_cntxt);
		
		if(RT_SUCCESS ==	l_ret_val)
		{
			RtS8T 		l_sys_time[RT_MAX_STRING_LEN] = {'\0'};
			struct tm *lp_tm=NULL;
			time_t clock;
			time(&clock);
			struct tm l_local_tm;
			lp_tm=localtime_r(&clock,&l_local_tm);
			RtS8T 		l_filename[RT_MAX_FILE_NAME_LEN] = {'\0'};
			FILE* 		lp_file;
			
			strftime(l_sys_time, sizeof(l_sys_time),(RtS8T *)("%d%m%Y_%T"),lp_tm);
			//KLOCWORK_FIX RM 03092013
 			snprintf(l_filename,sizeof(l_filename)-1,"%s/cntxt_dump/%s_%s.dump",getenv("COMMON_OUTPUT_PATH"), ap_user_identity, l_sys_time);
			
			strcpy(ap_output_path,l_filename);
			
			if((lp_file=fopen(l_filename,"w")))
  		{
    		printf("\nRtShUserDataIntf :: rtDumpUserCntxt-> New dump file successfully created as %s",l_filename);
				fflush(NULL);
  		}
  		else
  		{
    		printf("\nRtShUserDataIntf :: rtDumpUserCntxt-> Unable to create dump file %s!! (fopen() failed <errno=%d>",l_filename ,errno);
				fflush(NULL);
				
				mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
				return  RT_FAILURE;
  		}	


			/*--everything OK till here, now we can dump context--*/
			
			fprintf(lp_file,"========================================================\n");
			fprintf(lp_file," --- Context snapshot of user = %s ---\n",ap_user_identity);
			fprintf(lp_file,"========================================================\n\n");
			
			fprintf(lp_file,"context flag 			= %d\n",lp_cntxt->cntxt_flag);
			fprintf(lp_file,"context state  		= %d\n",lp_cntxt->cntxt_state);
			fprintf(lp_file,"processing_state  	= %d\n",lp_cntxt->cntxt_processing_state);
			
			fprintf(lp_file,"context index  		= %d\n",lp_cntxt->session_cookie.self_indx);
			fprintf(lp_file,"num_data_ref  			= %d\n",lp_cntxt->num_data_ref);
			fprintf(lp_file,"\n ----------Details of data references------------\n");
			for(RtU32T	l_cnt = 0;l_cnt < lp_cntxt->num_data_ref; ++l_cnt)
			{
				if(lp_cntxt->p_data_ref_arr[l_cnt].data_state & RT_SH_UDI_DATA_STATE_VALID)
				{
					fprintf(lp_file,"\np_data_ref[%u].data_state 		= %d\n",l_cnt,lp_cntxt->p_data_ref_arr[l_cnt].data_state);
					fprintf(lp_file,"p_data_ref[%u].elem_indx 		= %d\n",l_cnt,lp_cntxt->p_data_ref_arr[l_cnt].elem_indx);
					fprintf(lp_file,"p_data_ref[%u].seq_no 				= %d\n",l_cnt,lp_cntxt->p_data_ref_arr[l_cnt].seq_no);
					fprintf(lp_file,"p_data_ref[%u].old_seq_no 				= %d\n",l_cnt,lp_cntxt->p_data_ref_arr[l_cnt].old_seq_no);
					fprintf(lp_file,"p_data_ref[%u].p_data_ptr 		= %p\n",l_cnt,lp_cntxt->p_data_ref_arr[l_cnt].p_data_ptr);
				}
				else
				{
					fprintf(lp_file,"\np_data_ref[%d].data_state 		= INVALID\n",l_cnt);
				}
					
			}
			
			fprintf(lp_file,"\n ----------Details of ordered list------------\n");
			
			list<RtShUDIOrderedListElem>::iterator l_itr;
			for(RtU32T	l_cntr1	= 0;	l_cntr1	< RT_SH_MAX_SESS_CASE;++l_cntr1)
			{
				for(RtU32T	l_cntr2	= 0;	l_cntr2	< RT_SIP_METHOD_FOR_CTF;++l_cntr2)
				{
					l_itr	= lp_cntxt->case_ord_list[l_cntr1][l_cntr2].begin();
					while(l_itr !=	lp_cntxt->case_ord_list[l_cntr1][l_cntr2].end())
					{
						fprintf(lp_file,"\ncase_ord_list[%u][%u].udi_ref = %d, service_ind = %s, app_enum = %d \n",l_cntr1,l_cntr2,l_itr->udi_ref,l_itr->data_ref_appl_info.app_srvc_indic,l_itr->data_ref_appl_info.app_enum);
						l_itr++;
					}//end of while				
				}//end of inner for
			}//end of outer for	

			fprintf(lp_file,"\n ----------Details of processing_req_list------------\n");
			list<RtShUDIProcReq>::iterator l_itr1	= lp_cntxt->processing_req.proc_req_list.begin();
			while(l_itr1  != lp_cntxt->processing_req.proc_req_list.end())
			{
				fprintf(lp_file,"processing_req.proc_req_list.opcode  		= %d\n",l_itr1->opcode);
				l_itr1++;
			}	
			
			fprintf(lp_file,"subscription_timer_id  		= %lld\n",lp_cntxt->subscription_timer_id);
			fprintf(lp_file,"cache_id  									= %lld\n",lp_cntxt->cache_id);
			
			fprintf(lp_file,"\n ----------Details of alias_list------------\n");
			list<string>::iterator l_itr2	= lp_cntxt->alias_ids_list.begin();
			while(l_itr2  != lp_cntxt->alias_ids_list.end())
			{
				fprintf(lp_file,"alias_ids  = %s\n",l_itr2->c_str());
				l_itr2++;
			}	
			
			//Add snapshot of service data also after decoding into xml format
			fprintf(lp_file,"\n ==============Details of Service Data===================\n");
			for(RtU32T	l_cnt = m_max_data_ref +1;l_cnt <= m_max_data_ref + m_max_data_repository + 1; ++l_cnt)
			{
				if(lp_cntxt->p_data_ref_arr[l_cnt].data_state & RT_SH_UDI_DATA_STATE_VALID)
				{
					RtS8T l_srv_data[RT_REST_MAX_MSG_BODY_LEN];
					memset(l_srv_data,0,RT_REST_MAX_MSG_BODY_LEN);

					RtS8T l_tmp_srv_data[RT_REST_MAX_MSG_BODY_LEN];
					memset(l_tmp_srv_data,0,RT_REST_MAX_MSG_BODY_LEN);
					
					RtS8T 			l_srvc_indic[RT_DIA_SH_MAX_SRV_INDICATION_LEN];
					memset(l_srvc_indic,0,RT_DIA_SH_MAX_SRV_INDICATION_LEN);
					RtU32T	l_app_enum = 0;
					
					mp_data_pool_mgr->rtGetSrvIndAppEnumFromUdiRef(l_cnt, l_srvc_indic, l_app_enum);
					
					fprintf(lp_file,"\n ----------Details of %s service------------\n",l_srvc_indic);
					
					if(lp_cntxt->p_data_ref_arr[l_cnt].data_state & RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY)
					fprintf(lp_file,"\n ----------Service is applicable as mandatory------------\n");

					l_ret_val = mp_data_pool_mgr->rtCopyPoolElem(l_cnt, lp_cntxt->p_data_ref_arr[l_cnt].elem_indx, l_tmp_srv_data);
					if(RT_SUCCESS ==	l_ret_val)
					{
						//if(RT_SUCCESS ==	mp_prov_rest_mgr->mp_decode_servc_data_cb(l_tmp_srv_data, l_app_enum, l_srv_data))
						if(RT_SUCCESS ==	mp_prov_rest_mgr->mp_decode_servc_data_cb(l_tmp_srv_data, l_app_enum, l_srv_data,false))
						{
							fprintf(lp_file,"\n%s\n",l_srv_data);
						}
						else
						{
    					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
							"RtShUserDataIntf :: rtDumpUserCntxt-> Unable to decode data for service=%s",l_srvc_indic);

						}
					}
					else
					{
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"RtShUserDataIntf :: rtDumpUserCntxt()mp_data_pool_mgr->rtCopyPoolElem failed for user_id=%s,cntxt_index=%u, udi_ref=%d,pool_indx=%d",
						ap_user_identity, l_cntxt_id, l_cnt,	lp_cntxt->p_data_ref_arr[l_cnt].elem_indx);
										
					}
				}
			}	
			
			fclose(lp_file);
			
			l_ret_val = RT_SUCCESS;

			mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
		}
		else
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtDumpUserCntxt() mp_user_cntxt_mgr->rtRetrieveCntxtData() returned FAILURE or UDA not received yet with ap_user_identity=%s, cntxt_id=%u",
				ap_user_identity,l_cntxt_id);

			l_ret_val	=	RT_FAILURE;							
		
		}	
		
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
			"rtDumpUserCntxt() user id not found in map ap_user_identity=%s",
				ap_user_identity);	

		l_ret_val	=	RT_SH_UDI_ERR_USER_UNKNOWN;

	}	
	
	
	
	return l_ret_val;
}
/*******************************************************************************
 *
 * FUNCTION NAME : rtProcessMandProvReq().
 *
 * DESCRIPTION   : Function processes provisioning of mandatory services(UPDATE/VIEW/ENABLE/DISABLE)
 *
 * INPUT         :  RtShProvIntfMsgOpcode, RtU32T	a_servc_index,
 *
 * OUTPUT        :  
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUserDataIntf :: rtProcessMandProvReq(RtShProvIntfMsgOpcode a_opcode,RtU32T	a_servc_index, RtU32T& ar_servc_data_len, void* ap_serv_data,RtS8T* ap_xml_body  )
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER rtProcessMandProvReq() a_opcode = %u,a_servc_index =%d",a_opcode,a_servc_index);
	
	RtRcT 						l_ret_val 				= RT_FAILURE;
	RtShUDIDataRefVal	l_udi_ref 				= 0;
	RtU32T						l_mand_pool_elem  = 0;

	rtGetUDIRefFromAppEnum(a_servc_index,l_udi_ref);
	
	if((RT_O_SH_UDI_PROV_INTF_ENABLE_MAND_REQ <= a_opcode) && (RT_O_SH_UDI_PROV_INTF_DISABLE_MAND_TERM >= a_opcode))
	{
		
		//DB_13022013 : Generate xml file if not present already.
		if(	(!(mp_data_pool_mgr->rtIsSrvMandatory(l_udi_ref , l_mand_pool_elem))) && 
				((RT_O_SH_UDI_PROV_INTF_ENABLE_MAND_REQ == a_opcode) ||
				(RT_O_SH_UDI_PROV_INTF_ENABLE_MAND_ORIG == a_opcode) ||
				(RT_O_SH_UDI_PROV_INTF_ENABLE_MAND_TERM == a_opcode)))
		{
				
				l_ret_val = mp_data_pool_mgr->rtCreateMandatorySrvData(l_udi_ref, ap_serv_data, ar_servc_data_len, ap_xml_body);
				
				if(RT_SUCCESS == l_ret_val)
				{
						mp_data_pool_mgr->rtUpdateCkptForDftSrvData(l_udi_ref,ap_xml_body);
				}
				else
				{
								//CRITICAL logs
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
							"rtProcessMandProvReq(): rtCreateMandatorySrvData Return Failure for  a_opcode = %u,a_servc_index =%d",a_opcode,a_servc_index);
						return RT_FAILURE;
					
				}
		   
		}
		
		switch(a_opcode)
		{	
			case 		RT_O_SH_UDI_PROV_INTF_ENABLE_MAND_REQ:
			{
				l_ret_val = mp_data_pool_mgr->rtEnableDftService(l_udi_ref, true ,true); //note: 3 argument related checkpointing and 4 related session
			}break;
			
			case RT_O_SH_UDI_PROV_INTF_ENABLE_MAND_ORIG:
			{
				l_ret_val = mp_data_pool_mgr->rtEnableDftService(l_udi_ref, true,true,RT_SH_ORIG_CASE);			
			}break;
			
			case RT_O_SH_UDI_PROV_INTF_ENABLE_MAND_TERM:
			{
				l_ret_val = mp_data_pool_mgr->rtEnableDftService(l_udi_ref, true,true,RT_SH_TERM_CASE);				
			}break;
			
			case 		RT_O_SH_UDI_PROV_INTF_DISABLE_MAND_REQ:
			{
				l_ret_val = mp_data_pool_mgr->rtEnableDftService(l_udi_ref, false,true);
			}break;
			
			case RT_O_SH_UDI_PROV_INTF_DISABLE_MAND_ORIG:
			{
				l_ret_val = mp_data_pool_mgr->rtEnableDftService(l_udi_ref, false,true,RT_SH_ORIG_CASE);			
			}break;
			
			case RT_O_SH_UDI_PROV_INTF_DISABLE_MAND_TERM:
			{
				l_ret_val = mp_data_pool_mgr->rtEnableDftService(l_udi_ref, false,true,RT_SH_TERM_CASE);					
			}break;
			
			default:
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					"rtProcessMandProvReq() INVALID OPCODE a_opcode = %u,a_servc_index =%d",a_opcode,a_servc_index);
				return l_ret_val;
			}
		}//end of switch(opcode)
			
		if(RT_SUCCESS ==	l_ret_val)
		{
			if(m_immediate_regen)
			{
				mp_user_cntxt_mgr->rtRegenOrderedListForCntxt();
			}
			else
			{
				mp_user_cntxt_mgr->rtMarkCntxtsForOrdListUpd();

			}
			mp_data_pool_mgr->rtUpdateCkptDataRefConfigData(l_udi_ref, false);
		}
		else
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtProcessMandProvReq() mp_data_pool_mgr->rtEnableDftService() returned[%d] a_opcode = %u,a_servc_index =%d",l_ret_val,a_opcode,a_servc_index);					
		}

	}
	else if(mp_data_pool_mgr->rtIsSrvMandatory(l_udi_ref , l_mand_pool_elem))
	{
		switch(a_opcode)
		{
			case 	RT_O_SH_UDI_PROV_INTF_UPDATE_MAND_REQ:
			{
				l_ret_val = mp_data_pool_mgr->rtUpdateMandatorySrvData(l_udi_ref, ap_serv_data, ar_servc_data_len, ap_xml_body,true);
				
				if(RT_SUCCESS == l_ret_val)
				{
					mp_data_pool_mgr->rtUpdateCkptForDftSrvData(l_udi_ref,ap_xml_body);
				}
				
			}break;
			case 		RT_O_SH_UDI_PROV_INTF_VIEW_MAND_REQ:
			case    RT_O_SH_UDI_PROV_INTF_VIEW_MAND_REQ_STATUS:
			{
				l_ret_val = mp_data_pool_mgr->rtCopyPoolElem(l_udi_ref, l_mand_pool_elem, ap_serv_data);
			}break;
						
			default :
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					"rtProcessMandProvReq() INVALID OPCODE a_opcode = %u,a_servc_index =%d",a_opcode,a_servc_index);
			}
		}
	}
	else
	{
		l_ret_val = RT_SH_UDI_ERR_SERVICE_NOT_MANDATORY;
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
			"rtProcessMandProvReq() operation for non-mandatory is prohibited; a_opcode = %u,a_servc_index =%d, ret_val =%d",a_opcode,a_servc_index,l_ret_val);
		
	}	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT rtProcessMandProvReq() a_opcode = %u,a_servc_index =%d, ret_val =%d",a_opcode,a_servc_index,l_ret_val);
		
	return l_ret_val;
}
/* DB add a new function for check data path file exist or not */

/*******************************************************************************
 *
 * FUNCTION NAME : rtIsDftDataReqd().
 *
 * DESCRIPTION   : Function processes no data file service
 *
 * INPUT         : RtShProvIntfMsgOpcode, RtU32T	a_servc_index,
 *
 * OUTPUT        :  
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtBoolT		RtShUserDataIntf :: rtIsDftDataReqd(RtS8T* ap_service_indic)
{
	
	RtShUDIDataRefVal	l_udi_ref 				= 0;
	rtGetUDIRefFromSrvcInd(ap_service_indic,l_udi_ref);
	return (mp_data_pool_mgr->rtIsSrvDefaultDataReqd(l_udi_ref));
	
}




/*******************************************************************************
 *
 * FUNCTION NAME : rtProcessHostedSrvListProvReq().
 *
 * DESCRIPTION   : Function processes service list request hosted by AS
 *
 * INPUT         :  RtShProvIntfMsgOpcode a_opcode, RtS8T* ap_xml_body
 *
 * OUTPUT        :  
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUserDataIntf :: rtProcessHostedSrvListProvReq(RtShProvIntfMsgOpcode a_opcode, RtS8T* ap_xml_body)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER rtProcessHostedSrvListProvReq() a_opcode = %u",a_opcode);
	RtRcT l_ret_val = RT_SUCCESS;
	
	l_ret_val = mp_data_pool_mgr->rtPrepareHostedSrvList(a_opcode, ap_xml_body);

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT rtProcessHostedSrvListProvReq() a_opcode = %u, ret_val =%d",a_opcode,l_ret_val);
	
	return l_ret_val;
}
/*******************************************************************************
 *
 * FUNCTION NAME : rtProcessReturnXmlUtReq().
 *
 * DESCRIPTION   : Function processes service list request hosted by AS
 *
 * INPUT         :  RtShProvIntfMsgOpcode a_opcode, RtS8T* ap_xml_body
 *
 * OUTPUT        :  
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUserDataIntf :: rtProcessReturnXmlUtReq(RtRcT a_rest_ret_code, RtS8T*** ap_xml_body)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER rtProcessReturnXmlUtReq() a_rest_ret_code = %d",a_rest_ret_code);
	RtRcT l_ret_val = RT_SUCCESS;
	xcap_error::xcap_error l_trans_data;//XSD autogenerated class
	switch(a_rest_ret_code)
	{
		case	RT_UDI_PROV_HTTP_ENCODING_FAILURE:
		{

	 		xcap_error::schema_validation_error l_variable;
			l_variable.phrase ("XML ENCODING FAILURE");
			l_trans_data.error_element(l_variable);
		
		}break;   
		case	RT_UDI_PROV_HTTP_NOT_XML_FRAG_FORMAT:  
		{
	 		xcap_error::not_xml_frag l_variable;
			l_variable.phrase ("XML FRAG FROMAT NOT PROPER");
			l_trans_data.error_element(l_variable);
		
		}break;   
		case	RT_UDI_PROV_HTTP_INVALID_SERVICE_NAME:
		{
	 		xcap_error::no_parent l_variable;
			l_variable.phrase ("Specified Service Name not exist");
			l_trans_data.error_element(l_variable);
		
		}break;   
		case	RT_UDI_PROV_HTTP_CANNOT_INSERT:  
		{
	 		xcap_error::cannot_insert l_variable;
			l_variable.phrase ("hello");
			l_trans_data.error_element(l_variable);
		
		}break;   
		case	RT_UDI_PROV_HTTP_NOT_XML_ATT_VAL:
		{
	 		xcap_error::not_xml_att_value l_variable;
			l_variable.phrase ("hello");
			l_trans_data.error_element(l_variable);
		
		}break;   
		case	RT_UDI_PROV_HTTP_UNIQUENESS_FAIL:  
		{
		 		xcap_error::uniqueness_failure l_variable;
				l_variable.phrase ("HTTP UNIQUENESS FAIL");
			 xcap_error::exists l_exist_variable;
			 char *field_value="hello";
			 char *att_value="hello";
			 l_exist_variable.field(std::string((char*)field_value));
			 l_exist_variable.alt_value().push_back(std::string((char*)att_value));
			 l_variable.exists().push_back(l_exist_variable);
			 l_trans_data.error_element(l_variable);
		
		}break;   
		case	RT_UDI_PROV_HTTP_NOT_WELL_FORMED:
		{
	 		xcap_error::not_well_formed l_variable;
			l_variable.phrase ("HTTP NOT WELL FORMED");
			l_trans_data.error_element(l_variable);
		
		}break;   
		case	RT_UDI_PROV_HTTP_CONSTRAINT_FAILURE:  
		{
	 		xcap_error::constraint_failure l_variable;
			l_variable.phrase ("HTTP CONSTRAINT FAILURE");
			l_trans_data.error_element(l_variable);
		
		}break;   
		case	RT_UDI_PROV_HTTP_CANNOT_DELETE_UT:  
		{
	 		xcap_error::cannot_delete l_variable;
			l_variable.phrase ("HTTP CANNOT DELETE UT");
			l_trans_data.error_element(l_variable);
		
		}break;   
		case	RT_UDI_PROV_HTTP_NOT_UTF_FORMAT:
		{
	 		xcap_error::not_utf_8 l_variable;
			l_variable.phrase ("HTTP NOT UTF FORMAT");
		 l_trans_data.error_element(l_variable);
		
		}break;   
		default:
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"EXIT : rtProcessReturnXmlUtReq() INVALID a_rest_ret_code a_rest_ret_code = %d, ret_val =%d",a_rest_ret_code,l_ret_val);

			return RT_FAILURE;;

		}   

	}//End of switch
	
	xml_schema::namespace_infomap map;
	map[""].schema = "return.xsd";
	std::ostringstream str_out_stream;
	::xcap_error::xcap_error_(str_out_stream,l_trans_data,map);
	
	strcpy(**ap_xml_body,str_out_stream.str().c_str());
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT rtPrepareHostedSrvList() a_rest_ret_code = %u, ret_val =%d",a_rest_ret_code,l_ret_val);
	
	return l_ret_val;


}
//UDI_PHASE2
/*******************************************************************************

  FUNCTION NAME : rtEraseMaps()
 
  DESCRIPTION   : This function is called on Quiescing callback from avsv_mgr
									This function clears usr_id - cntxt_id maps.
								 
 
  ARGUMENTS     : None
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT   RtShUserDataIntf::rtClearMaps()
{
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
  "RtShUserDataIntf::rtClearMaps -ENTER");

	for(RtU32T	l_cnt=0; l_cnt < RT_SH_UDI_MAX_NUM_USER_IDEN_BLOCKS;++l_cnt)  //Klock_work_fix@25-08-2016
	{
		pthread_rwlock_wrlock(&m_user_iden_vs_cntxt_lock_arr[l_cnt]);
		m_user_iden_vs_cntxt_map[l_cnt].clear();
		pthread_rwlock_unlock(&m_user_iden_vs_cntxt_lock_arr[l_cnt]);
	}	
		
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
  "RtShUserDataIntf::rtClearMaps -EXIT");
  
	return RT_SUCCESS;
}
/*******************************************************************************

  FUNCTION NAME : rtEraseCachdata()
 
  DESCRIPTION   : This function is called By ClI to erase cached data for a particular User
								
								 
 
  ARGUMENTS     : userid
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT   RtShUserDataIntf::rtEraseCachedata(RtS8T* ap_user_identity)
{
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
  "RtShUserDataIntf::rtEraseCachedata -ENTER");
	RtU32T						l_cntxt_id				= 0;
	RtShUDIUserCntxt* lp_cntxt					= NULL;
	RtRcT 						l_ret_val 	= RT_FAILURE;
 	if(strcmp(ap_user_identity, "")==0)
 	{
	 	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_ALERT,__FILE__,  __LINE__,
	 		"RtShUserDataIntf::rtEraseCachedata :INVALID	Username-NULL");
	 return	RT_FAILURE;

 	}	

 	RtTransMsg l_trans_msg;
 	memset(&l_trans_msg,0,sizeof(RtTransMsg));
 	RtShUDIProcReq* lp_proc_req 							= (RtShUDIProcReq*)l_trans_msg.msg_buffer;

 	l_ret_val = rtFindCntxtForUserIden(ap_user_identity,l_cntxt_id);
 	if(RT_SUCCESS ==	l_ret_val)
 	{
	 	l_ret_val = mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt);
	 	if (RT_SUCCESS == l_ret_val)
	 	{
			lp_proc_req->req_body.cache_msg.index			=	lp_cntxt->session_cookie.self_indx;
			lp_proc_req->req_body.cache_msg.cache_id	=	lp_cntxt->cache_id;
			mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

			l_trans_msg.msg_type  										= RT_M_SH_UDI_APPL_MSG;

  		RtMglIntf::rtGetInstance()->rtGetSelfAddress(l_trans_msg.msg_hdr.src_addr);//SHASHI_20102012
  		RtMglIntf::rtGetInstance()->rtGetSelfAddress(l_trans_msg.msg_hdr.dest_addr);//SHASHI_20102012

  		l_trans_msg.msg_hdr.dest_addr.ee_id 			= RT_EE_SH_UDI_WRKR_BASE;
			lp_proc_req->opcode 											= RT_O_TIMEOUT_CACHE;
			
			if(RT_SUCCESS !=	RtMglIntf::rtGetInstance()->rtSendSockMsg(sizeof(RtTransMsg), &l_trans_msg, true))
			{
			 	printf("\rtEraseCachedata::ERROR in rtSendSockMsg");
				fflush(NULL);
				
		   	l_ret_val	= RT_FAILURE;
		 	}
	 	}
	 	else
	 	{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			  "rtEraseCachedata() mp_user_cntxt_mgr->rtRetrieveCntxtData() returned FAILURE or UDA not received yet with ap_user_identity=%s, cntxt_id=%u",
			  ap_user_identity,l_cntxt_id);

		 	l_ret_val	=	RT_FAILURE;							
	 	}
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
			"RtShUserDataIntf::rtEraseCachedata user id not found in map ap_user_identity=%s",
				ap_user_identity);	

		l_ret_val	=	RT_SH_UDI_ERR_USER_UNKNOWN;
	}
	
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
  "RtShUserDataIntf::rtEraseCachedata -Leave");
	return l_ret_val;
}

/*******************************************************************************
 
  FUNCTION NAME : rtGenerateUDICDR
 
  DESCRIPTION   : 
 
  INPUT         : void*
 
  OUTPUT        : None 
 
  RETURN        : void*
 
 ******************************************************************************/


RtRcT	RtShUserDataIntf:: rtGenerateUDICDR(RtTransMsg* ap_trans_msg, RtS8T* ap_buff)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf :: rtGenerateUDICDR - ENTER");

	RtRcT		l_rval							= RT_SUCCESS;
	RtBoolT	l_write_xml_to_cdr 	= mp_sys_agent->rtGetRtBoolT(RT_UDI_WRITE_XML_TO_CDR);

 	RtUDICdrMsg* lp_cdr_msg = (RtUDICdrMsg*)ap_trans_msg->msg_buffer;
	
	if(!(mp_sys_agent->rtGetRtBoolT(RT_UDI_IS_CDR_GEN_ENBLD)))
 	{
 		
		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtUDICdrMgr :: rtGenerateUDICDR - LEAVE - returns %d",l_rval);
		return RT_SUCCESS;
 	}
	else
	{
		if(l_write_xml_to_cdr)
		{
			//RtUDICdrMsg* lp_cdr_msg = (RtUDICdrMsg*)ap_trans_msg->msg_buffer;
			//Tag_amit_21062013
			//if( (strlen(ap_buff) > 0) && (strlen(ap_buff) < 4000))
			if( (strlen(ap_buff) > 0) && (strlen(ap_buff) < gp_sys_agent->rtGetRtU32T(RT_UDI_CDR_MAX_XML_SZ)))
			{
				RtUDICdrCntxtData* lp_cntxt_data;
			 	RtS8T*	lp_xml_data	=	ap_buff;
			 	RtS8T*	lp_xml_ptr	=	ap_buff;

// 			 while(*lp_xml_data	!=	'\0')
// 			 {
// 				 if(*lp_xml_data=='\n'	||	*lp_xml_data=='\t')
// 				 {
// 				 }
// 				 else
// 				 {
// 					 if(lp_xml_data	>	lp_xml_ptr)
// 					 {
// 						 *lp_xml_ptr	=	*lp_xml_data;
// 						 *lp_xml_data	=	'\0';
// 					 }
// 					 lp_xml_ptr++;
// 				 }
// 				 lp_xml_data++;
// 			 }

				l_rval = mp_udi_cdr_cntxt_mgr->rtGetNewCntxt(lp_cdr_msg->cdr_token,&lp_cntxt_data);
				if(RT_SUCCESS != l_rval)
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,  __LINE__,
					"RtShUserDataIntf :: rtGenerateUDICDR : failed in mp_udi_cdr_cntxt_mgr->rtGetNewCntxt [RtShProvIntfMsgOpcode = %d,RtSelSrvUpdType = %d,srv_name = %s,RtModeType = %d,result_code = %d, user_identity = %s,l_rval = %d]",
					lp_cdr_msg->msg_opcode,
					lp_cdr_msg->msg_sel_upd_type,
					lp_cdr_msg->srv_name,
					lp_cdr_msg->mode_type,
					lp_cdr_msg->result_code,
					lp_cdr_msg->user_identity,
					l_rval);
					
					lp_cdr_msg->cdr_token = 0;
				}
				else
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,  __LINE__,
					"RtShUserDataIntf :: rtGenerateUDICDR : [ap_buff = NULL][RtShProvIntfMsgOpcode = %d,RtSelSrvUpdType = %d,srv_name = %s,RtModeType = %d,result_code = %d, user_identity = %s,cdr_token = %d]",
					lp_cdr_msg->msg_opcode,
					lp_cdr_msg->msg_sel_upd_type,
					lp_cdr_msg->srv_name,
					lp_cdr_msg->mode_type,
					lp_cdr_msg->result_code,
					lp_cdr_msg->user_identity,
					lp_cdr_msg->cdr_token);

					strcpy(lp_cntxt_data->cdr_xml_data,ap_buff);
					mp_udi_cdr_cntxt_mgr->rtReleaseCntxtDataLock(lp_cdr_msg->cdr_token);
				}
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,  __LINE__,
				"RtShUserDataIntf :: rtGenerateUDICDR : [ap_buff = NULL][RtShProvIntfMsgOpcode = %d,RtSelSrvUpdType = %d,srv_name = %s,RtModeType = %d,result_code = %d, user_identity = %s]",
				lp_cdr_msg->msg_opcode,
				lp_cdr_msg->msg_sel_upd_type,
				lp_cdr_msg->srv_name,
				lp_cdr_msg->mode_type,
				lp_cdr_msg->result_code,
				lp_cdr_msg->user_identity);
					
					
				lp_cdr_msg->cdr_token = 0;
			}
		}
		else
		{
			lp_cdr_msg->cdr_token = 0;
		}
		
		l_rval = mp_udi_cdr_mgr->rtPutMsg(ap_trans_msg);
		if (RT_SUCCESS != l_rval)
		{
				mp_sys_agent->rtRaiseCount(prvrestCntrs, prvrestCntrsDefIntfId, prvrestCdrGenFail);

			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
				"RtShUserDataIntf :: rtGenerateUDICDR()-LEAVE- rtPutMsg failed [l_rval=%d]",l_rval );
		}
		else
		{
			//mp_sys_agent->rtRaiseCount(onlinechargingCntrs,onlinechargingCntrsDefIntfId,(IECccrCDRgenerationSucc-RT_CC_IEC)+a_cc_chrg_type);
		}
 	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
				"RtUDICdrMgr :: rtGenerateUDICDR - LEAVE - returns %d",l_rval);
	
	return l_rval;
}													

/*****************************************************************************
 *FUNCTION NAME		: rtSetRetryFlag()
 *
 *DESCRIPTION			: This function is used to set retry flag
 *			
 *INPUT						: RtBoolT
 *
 *OUTPUT 					: 
 *
******************************************************************************/
void  RtShUserDataIntf::rtResetRetryFlag()
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
				"RtShUserDataIntf :: rtResetRetryFlag - Enter - ");
	
	for(RtU32T l_cnt=0; l_cnt < m_num_wrkr; l_cnt++)
	{ 
		 mp_wrkr_arr[l_cnt]->rtSetRetryFlag(true); 
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
				"RtShUserDataIntf :: rtResetRetryFlag - LEAVE");
}	

/*****************************************************************************
 *FUNCTION NAME		: rtGetAndReInsertUserIdenVsCntxtMapEntry()
 *
 *DESCRIPTION			: This function is used to erase old alias entry and re-insert new entries
 *			
 *INPUT						: RtBoolT
 *
 *OUTPUT 					: 
 *
******************************************************************************/
RtRcT RtShUserDataIntf ::	rtGetAndReInsertUserIdenVsCntxtMapEntry(RtS8T* ap_user_identity,RtU32T a_cntxt_indx,RtU32T& ar_duplicate_index,RtBoolT a_flag)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
		"RtShUserDataIntf :: rtGetAndReInsertUserIdenVsCntxtMapEntry() with user_identity=%s,cntxt_index=%u",
		ap_user_identity, a_cntxt_indx);
		
	RtRcT 	l_ret_val = RT_FAILURE;
	RtU32T  l_block_no;
	
	rtDecideBlock(ap_user_identity, l_block_no);
	
	pthread_rwlock_wrlock(&m_user_iden_vs_cntxt_lock_arr[l_block_no]);

	if(!a_flag)
	{
		RtShUDIUserIdenVsCntxtMapItr	map_itr;

		map_itr = m_user_iden_vs_cntxt_map[l_block_no].find(ap_user_identity);
		if(map_itr	!=	m_user_iden_vs_cntxt_map[l_block_no].end())
		{
			ar_duplicate_index = map_itr->second;
			
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
				"rtGetAndReInsertUserIdenVsCntxtMapEntry() ENTRY EXIST FOR user_identity=%s,block no =%u,cntxt_index =%u,a_duplicate_index=%u",
				ap_user_identity, l_block_no, a_cntxt_indx,ar_duplicate_index);
		}
		else
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
				"rtGetAndReInsertUserIdenVsCntxtMapEntry() ENTRY DOES NOT EXIST user_identity=%s,block no =%u",
				ap_user_identity, l_block_no);		
		}	
	}
	
	if(!( m_user_iden_vs_cntxt_map[l_block_no].erase(ap_user_identity)))
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"rtGetAndReInsertUserIdenVsCntxtMapEntry() : ERROR Failed erasing user_id =%s user_iden_vs_cntxt_map block =%u",
			ap_user_identity,l_block_no);		
		pthread_rwlock_unlock(&m_user_iden_vs_cntxt_lock_arr[l_block_no]);

	}
	else
	{
		//HP_CR(DONE): DEBUG logs
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"rtGetAndReInsertUserIdenVsCntxtMapEntry() : SUCCESS User_id [%s] user_iden_vs_cntxt_map block [%u]",
		ap_user_identity,l_block_no);		
		
		pthread_rwlock_unlock(&m_user_iden_vs_cntxt_lock_arr[l_block_no]);

		l_ret_val = rtInsertUserIdenVsCntxtMapEntry(ap_user_identity, a_cntxt_indx, l_block_no);
	
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
			"EXIT RtShUserDataIntf :: rtInsertUserIdenVsCntxtMapEntry() with user_identity=%s,cntxt_index=%u,block_no=%u,ret_val=%d",
			ap_user_identity, a_cntxt_indx, l_block_no, l_ret_val);
	}
	
	
	return l_ret_val;
	
}
/*******************************************************************************
 *
 * FUNCTION NAME : rtGetUserServiceData()
 *
 * DESCRIPTION   : This function is called by Subscription module to get service data of user
 *
 * INPUT         : RtShData, RtTransAddr  
 *
 * OUTPUT        : RtShUDIOrderedList 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT RtShUserDataIntf ::rtGetUserServiceData(RtSubsData*						ap_subs_data,
																							RtEEId*      					ap_ee_id,
																							RtShData*        			ap_sh_data)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtGetUserServiceData() with user_identity=%s,subs_token=%lu,ap_ee_id=%u",
		ap_subs_data->impu.val,ap_subs_data->subs_token,*ap_ee_id);
		
	RtRcT 						l_func_ret_val  = RT_SUCCESS;
	RtRcT 						l_rval 					= RT_SUCCESS;
	RtU32T						l_block_no;
	RtU32T						l_cntxt_id	= 0;
	RtTransMsg				l_trans_msg;
	RtShUDIUserCntxt* lp_cntxt;
	memset(&l_trans_msg,0,sizeof(RtTransMsg));
		
	//:	Bidhu
	//mp_sys_agent	=	NULL;
	//mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetOrderedListCalled);
	
	//SHASHI_26042012: Validation check
	
	if(	(0 != strlen(ap_subs_data->impu.val)) 
								&&
			(0 != ap_subs_data->subs_token)
								&&
			(NULL != ap_ee_id)
		)
	{
		//OK let us proceed
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"rtGetUserServiceData() Parameter Validation FAILED with ap_subs_data->impu=%s,ap_subs_data->subs_token=%u",
			ap_subs_data->impu.val, ap_subs_data->subs_token);
			
		return RT_SH_UDI_ERR_BAD_PARAMETER;	
		
	}
	
	RtShUDIProcReq* lp_udi_proc_req 					= (RtShUDIProcReq*)l_trans_msg.msg_buffer;
	lp_udi_proc_req->opcode 									= RT_O_SH_UDI_SUBS_MSG;
	
	lp_udi_proc_req->req_body.subs_msg.sh_subs_data.appl_token	= ap_subs_data->subs_token;
	lp_udi_proc_req->req_body.subs_msg.sh_subs_data.subs_ee_id	= *ap_ee_id;
	strcpy(lp_udi_proc_req->req_body.subs_msg.sh_subs_data.impu.val,ap_subs_data->impu.val);
	
	
	l_rval	= rtFindCntxtForUserIden(ap_subs_data->impu.val, l_cntxt_id);	
	
	if(RT_SUCCESS ==	l_rval)
	{
		if ( RT_SUCCESS != mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt) )
		{
			//shall not happen
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtGetUserServiceData() mp_user_cntxt_mgr->rtRetrieveCntxtData() FAILED with user_identity=%s,subs_token=%u",
					ap_subs_data->impu.val, ap_subs_data->subs_token);

			// get new context and set state and processing state 
			l_rval = mp_user_cntxt_mgr->rtGetNewCntxt(&lp_cntxt);

			if( RT_SUCCESS != l_rval )
			{
				rtEraseUserIdenVsCntxtMapEntry(ap_subs_data->impu.val);

				l_func_ret_val = RT_SH_UDI_ERR_USER_CNTXT_FULL;
				
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					"rtGetUserServiceData() user context FULL with user_identity=%s,subs_token=%u",
					ap_subs_data->impu.val, ap_subs_data->subs_token);
			}
			else
			{
				l_cntxt_id												=	lp_cntxt->session_cookie.self_indx;
				lp_cntxt->cntxt_state 						= RT_SH_UDI_CNTXT_NON_IDLE;
				lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROC_SRVC_APPL_REQ;
																						
				//response will be sent as RT_SH_UDI_SUBS_REQ_RECEIVED flag is set here
				lp_cntxt->cntxt_flag 						|= 	RT_SH_UDI_SUBS_REQ_RECEIVED;
				lp_cntxt->subs_data.appl_token	=		ap_subs_data->subs_token;
				lp_cntxt->subs_data.subs_ee_id	=		*ap_ee_id;
				strcpy(lp_cntxt->subs_data.impu.val,ap_subs_data->impu.val);

					
				lp_udi_proc_req->req_body.subs_msg.cntxt_indx	= lp_cntxt->session_cookie.self_indx;
				
				
				lp_cntxt->processing_req.proc_req_list.push_front(*lp_udi_proc_req);
				lp_cntxt->processing_req.curr_req_iter	= lp_cntxt->processing_req.proc_req_list.begin();
				
				l_rval = rtSendToWrkr(&l_trans_msg, l_cntxt_id);	
				
				if(RT_SUCCESS != l_rval )
				{
					l_func_ret_val = RT_SH_UDI_ERR_IN_SNDING_WRKR;
					
					//HP_CR(DONE):: call rtEraseUserIdenVsCntxtMapEntry , mp_user_cntxt_mgr->rtReturnCntxt(l_cntxt_id);
					
					rtEraseUserIdenVsCntxtMapEntry(ap_subs_data->impu.val);
					mp_user_cntxt_mgr->rtReturnCntxt(l_cntxt_id);					
					
					//HP_CR(DONE):: give critical logs
					
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"RtShUserDataIntf ::rtGetUserServiceData(): rtSendToWrkr() FAILED  user_identity=%s,subs_token=%u,Cntxt Id = %u",
						ap_subs_data->impu.val, ap_subs_data->subs_token,l_cntxt_id);					
					
				}
				else
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
						"rtGetUserServiceData() appl msg dispatched to wrkr user_identity=%s,subs_token=%u,Cntxt Id = %u",
						ap_subs_data->impu.val, ap_subs_data->subs_token,l_cntxt_id);

					mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
					
					l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
				}
				
			}

		}
		else
		{//user data retrieved
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"rtGetUserServiceData() mp_user_cntxt_mgr->rtRetrieveCntxtData() SUCCESS with user_identity=%s,subs_token=%u,Cntxt Id = %u",
				ap_subs_data->impu.val, ap_subs_data->subs_token, l_cntxt_id);

			if(lp_cntxt->cntxt_flag & RT_SH_UDI_UDA_RECEIVED)//SHASHI_04062012 addition of UDA flag
			{
				//here we are not setting RT_O_SH_UDI_SUBS_MSG so on any faliure for this cntxt
				//we can't sent response to subs module on the basis of  RT_O_SH_UDI_SUBS_MSG, 
				//due to this purpose inroducing new cntxt flag RT_SH_UDI_SUBS_REQ_RECEIVED

				strcpy(lp_cntxt->subs_data.impu.val,ap_subs_data->impu.val);

				if(RT_SUCCESS != rtCreateShSrvcData(lp_cntxt,ap_sh_data,true,true))
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"rtGetUserServiceData() - rtCreateShSrvcData() return failure with user_identity=%s,subs_token=%u,Cntxt Id = %u",
						ap_subs_data->impu.val, ap_subs_data->subs_token, l_cntxt_id);

					l_func_ret_val = RT_FAILURE;
				}
				else
				{
					//response will be sent as RT_SH_UDI_SUBS_REQ_RECEIVED flag is set here
					lp_cntxt->cntxt_flag 						|= 	RT_SH_UDI_SUBS_REQ_RECEIVED;
					lp_cntxt->subs_data.appl_token	=		ap_subs_data->subs_token;
					lp_cntxt->subs_data.subs_ee_id	=		*ap_ee_id;
					
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
						"rtGetUserServiceData() - rtCreateShSrvcData() return success with user_identity=%s,subs_token=%u,Cntxt Id = %u",
						ap_subs_data->impu.val, ap_subs_data->subs_token, l_cntxt_id);
				}
			}
			else
			{
				//set the flag RT_SH_UDI_SUBS_REQ_RECEIVED when pick the request from pending Queue				
				
				lp_udi_proc_req->req_body.subs_msg.cntxt_indx	= 	l_cntxt_id;
				lp_cntxt->processing_req.proc_req_list.push_back(*lp_udi_proc_req);

				mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiApplReqPushedInPend);

				l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
			}
			mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);

		}//user data retrieved
	}//rtFindCntxtForUserIden
	else
	{ //not found
		l_rval = mp_user_cntxt_mgr->rtGetNewCntxt(&lp_cntxt);

		if( RT_SUCCESS != l_rval )
		{
			l_func_ret_val = RT_SH_UDI_ERR_USER_CNTXT_FULL;

			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtGetUserServiceData() mp_user_cntxt_mgr->rtGetNewCntxt returned =%d for user_identity=%s,subs_token=%u",
				l_func_ret_val,ap_subs_data->impu.val, ap_subs_data->subs_token);
		}
		else
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"rtGetUserOrderedSrvcList() rtGetNewCntxt success user_identity=%s,subs_token=%u,Cntxt Id=%u",
				ap_subs_data->impu.val, ap_subs_data->subs_token, lp_cntxt->session_cookie.self_indx);

			lp_cntxt->cntxt_state 						= RT_SH_UDI_CNTXT_NON_IDLE;
			lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROC_SRVC_APPL_REQ;

			string l_user_id(ap_subs_data->impu.val); 
			lp_cntxt->alias_ids_list.push_back(l_user_id);

			l_rval = rtInsertUserIdenVsCntxtMapEntry(ap_subs_data->impu.val,lp_cntxt->session_cookie.self_indx);

			if( RT_SUCCESS != l_rval )
			{

				//shall not happen
				l_func_ret_val = RT_SH_UDI_ERR_IN_INTRNL_MAP;

				mp_user_cntxt_mgr->rtReturnCntxt(lp_cntxt->session_cookie.self_indx);

				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					"rtGetUserServiceData() rtInsertUserIdenVsCntxtMapEntry FAILED for user_identity=%s,subs_token=%u,Cntxt Id=%u",
					ap_subs_data->impu.val, ap_subs_data->subs_token, lp_cntxt->session_cookie.self_indx);					
			}
			else
			{
				lp_udi_proc_req->req_body.subs_msg.cntxt_indx	= lp_cntxt->session_cookie.self_indx;
				
				lp_cntxt->processing_req.proc_req_list.push_front(*lp_udi_proc_req);
				lp_cntxt->processing_req.curr_req_iter	= lp_cntxt->processing_req.proc_req_list.begin();
				
				//response will be sent as RT_SH_UDI_SUBS_REQ_RECEIVED flag is set here
				lp_cntxt->cntxt_flag 						|= 	RT_SH_UDI_SUBS_REQ_RECEIVED;
				lp_cntxt->subs_data.appl_token	=		ap_subs_data->subs_token;
				lp_cntxt->subs_data.subs_ee_id	=		*ap_ee_id;
				strcpy(lp_cntxt->subs_data.impu.val,ap_subs_data->impu.val);
				
				l_rval = rtSendToWrkr(&l_trans_msg, lp_udi_proc_req->req_body.subs_msg.cntxt_indx);	

				
				if(RT_SUCCESS != l_rval )
				{
					l_func_ret_val = RT_SH_UDI_ERR_IN_SNDING_WRKR;
					
					//HP_CR(DONE):: call rtEraseUserIdenVsCntxtMapEntry , mp_user_cntxt_mgr->rtReturnCntxt(l_cntxt_id);
					
					rtEraseUserIdenVsCntxtMapEntry(ap_subs_data->impu.val);
					mp_user_cntxt_mgr->rtReturnCntxt(lp_udi_proc_req->req_body.subs_msg.cntxt_indx);		
					
					//HP_CR(DONE):: give critical logs
					
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"RtShUserDataIntf ::rtGetUserServiceData(): rtSendToWrkr() FAILED  user_identity=%s,subs_token=%u,Cntxt Id=%u",
						ap_subs_data->impu.val, ap_subs_data->subs_token, lp_udi_proc_req->req_body.subs_msg.cntxt_indx);										
				}
				else
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
						"rtGetUserServiceData() appl msg dispatched to wrkr user_identity=%s,subs_token=%u,Cntxt Id=%u",
						ap_subs_data->impu.val, ap_subs_data->subs_token, lp_udi_proc_req->req_body.subs_msg.cntxt_indx);

					mp_user_cntxt_mgr->rtReleaseCntxtDataLock(lp_udi_proc_req->req_body.subs_msg.cntxt_indx);

					l_func_ret_val = RT_SH_UDI_WAIT_FOR_RSP;
				}
			
			}
		}
	}
	
	if(l_func_ret_val	==	RT_SUCCESS)
	{
		//mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetOrderedListRetSucc);
	}
	else if(l_func_ret_val == RT_SH_UDI_WAIT_FOR_RSP)
	{
		//mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetOrderedListWaitRsp);
	}
	else
	{
		//mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiGetOrderedListRetFail);
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT RtShUserDataIntf :: rtGetUserServiceData() with user_identity=%s,subs_token=%u,ap_ee_id=%u,ret_val =%d",
		ap_subs_data->impu.val, ap_subs_data->subs_token,*ap_ee_id,l_func_ret_val);
	
	return l_func_ret_val;
}

/*******************************************************************************************
 *
 * FUNCTION NAME : rtTerminateUserSubs()
 *
 * DESCRIPTION   : This function is called by Subscription module to unsubscribe the user
 *
 * INPUT         : RtShData, RtTransAddr  
 *
 * OUTPUT        :  
 *
 * RETURN        : RtRcT 
 *
 ********************************************************************************************/
RtRcT RtShUserDataIntf ::rtTerminateUserSubs(RtSubsData* ap_subs_data)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER RtShUserDataIntf :: rtTerminateUserSubs() with user_identity=%s,subs_token=%lu",
		ap_subs_data->impu.val,ap_subs_data->subs_token);
		
	RtRcT 						l_rval 			= RT_SUCCESS;
	RtU32T						l_cntxt_id	= 0;
	RtShUDIUserCntxt* lp_cntxt;
		
	if(0 != strlen(ap_subs_data->impu.val) ) 
	{
		//OK let us proceed
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"LEAVE RtShUserDataIntf :: rtTerminateUserSubs() Parameter Validation FAILED with ap_subs_data->impu=%s",
			ap_subs_data->impu.val);
			
		return RT_SH_UDI_ERR_BAD_PARAMETER;	
		
	}
	
	l_rval	= rtFindCntxtForUserIden(ap_subs_data->impu.val, l_cntxt_id);	
	
	if(RT_SUCCESS == l_rval)
	{
		if ( RT_SUCCESS != mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt) )
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"rtTerminateUserSubs() mp_user_cntxt_mgr->rtRetrieveCntxtData() FAILED with user_identity=%s",
					ap_subs_data->impu.val);
			
			l_rval = RT_SH_UDI_ERR_USER_DATA_DOESNT_EXIST;
		}
		else
		{
			lp_cntxt->cntxt_flag 	&= 	~RT_SH_UDI_SUBS_REQ_RECEIVED;
			mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
		}
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"rtTerminateUserSubs() rtFindCntxtForUserIden() FAILED with user_identity=%s",
				ap_subs_data->impu.val);
		
		l_rval = RT_SH_UDI_ERR_USER_DATA_DOESNT_EXIST;
	}
		
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"LEAVE RtShUserDataIntf :: rtTerminateUserSubs() with user_identity=%s",
		ap_subs_data->impu.val);
	
	return l_rval;
}

/*****************************************************************************
 *FUNCTION NAME		: rtCreateShSrvcData()
 *
 *DESCRIPTION			: This function is used to create service data
 *			
 *INPUT						: 
 *
 *OUTPUT 					: 
 *
******************************************************************************/
RtRcT  RtShUserDataIntf::rtCreateShSrvcData(RtShUDIUserCntxt* ap_cntxt,RtShData* ap_sh_data,RtBoolT a_update_in_service_data, RtBoolT a_update_in_alias_data)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"Enter RtShUserDataIntf :: rtCreateShSrvcData() - [Subs Token = %u] [Impu = %s]",
			ap_cntxt->subs_data.appl_token,ap_cntxt->subs_data.impu.val);
		
	RtRcT 	l_rval 	= RT_SUCCESS;
	RtU32T	l_cnt		=	0;
	
	//fill alias_impu data
	if(a_update_in_alias_data)
	{
		list<string>::iterator itr;
		for(itr = ap_cntxt->alias_ids_list.begin(); itr != ap_cntxt->alias_ids_list.end(); itr++)
		{
			ap_sh_data->alias_data.num_alias_public_id += 1;
			strncpy(ap_sh_data->alias_data.alias_public_id[l_cnt].val,(RtS8T*)itr->c_str(),sizeof(RtShIMSPublicId));

			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"RtShUserDataIntf :: rtCreateShSrvcData() -  Alias-List[%u]=%s",
					l_cnt,ap_sh_data->alias_data.alias_public_id[l_cnt].val);
					
			l_cnt++;		
		}

		if(0 == ap_sh_data->alias_data.num_alias_public_id)
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"RtShUserDataIntf :: rtCreateShSrvcData() - No Impu present in cntxt");

			ap_sh_data->is_alias_upd = false;
		}
		else
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
			"RtShUserDataIntf :: rtCreateShSrvcData() - Num Of Impu present in cntxt = %u",ap_sh_data->alias_data.num_alias_public_id);
			
			ap_sh_data->is_alias_upd = true;
		}
	}
	
	//fill service data

	if(a_update_in_service_data)
	{
		RtS8T 	l_srvc_indic[RT_DIA_SH_MAX_SRV_INDICATION_LEN];
		RtU32T	l_app_enum = 0;
		memset(l_srvc_indic,0,sizeof(l_srvc_indic));

		for(RtU8T l_num_data_ref = m_max_data_ref + 1; l_num_data_ref < m_max_data_ref + m_max_data_repository + 1; l_num_data_ref++)
		{
			if(ap_cntxt->p_data_ref_arr[l_num_data_ref].data_state & RT_SH_UDI_DATA_STATE_VALID)
			{
				mp_data_pool_mgr->rtGetSrvIndAppEnumFromUdiRef((RtShUDIRef)l_num_data_ref,l_srvc_indic, l_app_enum);

				ap_sh_data->doc_info[l_app_enum].is_valid 			= true;
				ap_sh_data->doc_info[l_app_enum].service_index 	= l_app_enum;
				ap_sh_data->doc_info[l_app_enum].prev_etag			=	ap_cntxt->p_data_ref_arr[l_num_data_ref].old_seq_no;
				ap_sh_data->doc_info[l_app_enum].new_etag				= ap_cntxt->p_data_ref_arr[l_num_data_ref].seq_no;

				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"RtShUDIWrkr :: rtCreateShSrvcData() - service detail [udi ref = %u, app_enum = %u, prev_tag = %u, new_tag = %u] ",
					l_num_data_ref,ap_sh_data->doc_info[l_app_enum].service_index,
					ap_sh_data->doc_info[l_app_enum].prev_etag,
					ap_sh_data->doc_info[l_app_enum].new_etag);
			}
			else
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"RtShUDIWrkr :: rtCreateShSrvcData() - service[%u] [m_max_data_ref=%u]is invalid",
					l_num_data_ref,m_max_data_ref);
			}
		}
		
		ap_sh_data->is_service_upd = true;
	}
	//fill subscription data
	
	ap_sh_data->subs_data.subs_token = ap_cntxt->subs_data.appl_token;
	strcpy(ap_sh_data->subs_data.impu.val,ap_cntxt->subs_data.impu.val);

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"Leave RtShUserDataIntf :: rtCreateShSrvcData() - [Subs Token = %u] [Impu = %s]",
			ap_cntxt->subs_data.appl_token,ap_cntxt->subs_data.impu.val);
		
	return l_rval;	
}
/********************************************************************************************************************************************
*
* FUNCTION NAME : rtHandleCugSrvcData()
*
* DESCRIPTION   : This API will first check cug context created or not
*						      if not It will create contxt and inititate UDR to load CUG data,and USR-UDR req is inserted in its processing list
*                 else It will mark contxt as not idle and returns it's contxt id To proceed with usr req
*								 
* INPUT         : ap_cug_id ,ap_entprise_id
*
* OUTPUT        : cug_contxt_id updated in RtShUDIProcReq...contxt_id
*
* RETURN        : RT_SH_UDI_WAIT_FOR_RSP/RT_SUCCESS or ERROR_CODE
*
*******************************************************************************************************************************************/		
RtRcT RtShUserDataIntf::rtHandleCugSrvcData(RtS8T* ap_entprise_id,RtS8T* ap_cug_id,RtShUDIProcReq* ap_proc_req)
{
	 
	 if(ap_entprise_id == NULL)
	 {
		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf::rtHandleCugSrvcData(), ENTER/LEAVE with ap_entprise_id as NULL,usr=%s appl_token=%ld ,So leaving with SUCCESS",
			 ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);
 
	   return RT_SUCCESS;
	 
	 }
	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf::rtHandleCugSrvcData(), ENTER ap_entprise_id=%s,ap_cug_id=%s usr=%s appl_token=%ld",
			 ap_entprise_id,ap_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);
	
	 
	 
	 /*-----------------------------------------------------------------------------------------------------------------------------------------
	 - 
	 - 																								 +++   API LOGIC   +++
	 -  1. Check whether CUG contxt is present
	 -     if not create it ,make entry for cug_name->contxt_id in map 
	 -  2. update cntxt_id in proc_req to be used by the caller **
	 -  3. if  contxt is newly created,Make entry in its processing list and initiate UDR to DbWrkr return UDA_RSP_WAIT
	 -  4. else 
	 -          - check contxt state if UDA received then return SUCCESS
	 -          - else Make entry in its processing list and return UDA_RSP_WAIT 
	 -   
	 -  OUTPUT :: cug_cntxt_id and Making cug_contxt has NON_IDLE  ( UDR may be initiated )(Entry in processing list)(Recache done)
	 -----------------------------------------------------------------------------------------------------------------------------------------*/
   
	 RtShUDIUserCntxt*  lp_cntxt = NULL;
	 
	 RtBoolT l_is_create_cntx_req = false;
	 
	 /*-----------------------------------------------------------------------------------------------------------------------------------------
	 -     MULTI-CUG changes by RAJENDER
	 - 1.Context is created for enterprise_id and cug_id combination, We are using '_' as a delimiter while key genration
	 -----------------------------------------------------------------------------------------------------------------------------------------*/
   RtS8T l_ent_cug_id[2*RT_MAX_STRING_LEN] = {'\0'};
	 snprintf(l_ent_cug_id,sizeof(l_ent_cug_id),"%s_%s",ap_entprise_id,ap_cug_id);
	 
	 RtRcT l_rval	= rtFindCntxtForUserIden(l_ent_cug_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);	
	 
	 if(RT_SUCCESS !=	l_rval)
	 {
			  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_ALERT,__FILE__,__LINE__,
			    "RtShUserDataIntf::rtHandleCugSrvcData(),ALERT FindContxt failed for l_ent_cug_id=%s for usr=%s l_rval=%d,Setting l_is_create_cntx_req TRUE",
					  l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id,l_rval);
       
			 l_is_create_cntx_req = true;
	 
	 }
	 else
	 {
			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"RtShUserDataIntf::rtHandleCugSrvcData(),rtFindCntxtForUserIden() returnd Cntxt_id=%u for l_ent_cug_id =%s,app_usr_id=%s,a_appl_token=%lu",
						ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);

	     if ( RT_SUCCESS != mp_user_cntxt_mgr->rtRetrieveCntxtData(ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx, &lp_cntxt) )
			 {
					 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						 "RtShUserDataIntf::rtHandleCugSrvcData(),ERROR RACE-CONDITION rtRetrieveCntxtData() FAILED for l_cntxt_id=%d l_ent_cug_id=%s,app_usr_id=%s,a_appl_token[%ld] setting l_is_create_cntx_req TRUE",
						  ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);										
           
					 rtEraseUserIdenVsCntxtMapEntry(l_ent_cug_id);
					 
					 l_is_create_cntx_req = true;
			 }
			 else
			 {
			   	 RtBoolT l_is_cntxt_in_valid = true;
					 list<string>::iterator l_alias_iter = lp_cntxt->alias_ids_list.begin();
					 while(l_alias_iter	!= lp_cntxt->alias_ids_list.end())
					 {
		 					if(strcmp((RtS8T*)l_alias_iter->c_str(),l_ent_cug_id) == 0)
							{
							   l_is_cntxt_in_valid = false;
							   break;
        			}
							else
							   ++l_alias_iter;
    			 }
					 if(l_is_cntxt_in_valid)
					 {
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
								"RtShUserDataIntf::rtHandleCugSrvcData(),ERROR l_cntxt_id=%d l_is_cntxt_in_valid for l_ent_cug_id=%s,app_usr_id=%s,a_appl_token[%ld] setting l_is_create_cntx_req TRUE",
						  	 ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);										

							rtEraseUserIdenVsCntxtMapEntry(l_ent_cug_id);
              mp_user_cntxt_mgr->rtReleaseCntxtDataLock(lp_cntxt->session_cookie.self_indx);
							l_is_create_cntx_req = true;
							lp_cntxt = NULL;
					 }
			 }
	 }
	 /*---------------------------------------------------------------------------------------------------------------------------------------------------
	 -         l_is_create_cntx_req Can be TRUE in two cases
	 - 1. rtFindCntxtForUserIden() returned failure i.e contxt_id is not mapped for user
	 - 2. rtFindCntxtForUserIden() returned Success i.e contxt_id is mapped for user BUt rtRetrieveCntxtData(contxt_id) Failed
	 -
	 - ## NEW :: There may be other race condition like This contxt_id is allocated to other User also
	 -----------------------------------------------------------------------------------------------------------------------------------------------------*/

	 if(l_is_create_cntx_req)
	 {
			  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_ALERT,__FILE__,__LINE__,
			    "RtShUserDataIntf::rtHandleCugSrvcData(),ALERT l_is_create_cntx_req is TRUE for l_ent_cug_id=%s for usr=%s l_rval=%d",
					  l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id,l_rval);
						
				l_rval = mp_user_cntxt_mgr->rtGetNewCntxt(&lp_cntxt);

				if( RT_SUCCESS != l_rval )
				{
					 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						 "RtShUserDataIntf::rtHandleCugSrvcData() mp_user_cntxt_mgr->rtGetNewCntxt returned =%d for l_ent_cug_id=%s,app_usr_id=%s",
						 l_rval, l_ent_cug_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id);
					 l_rval = RT_SH_UDI_ERR_USER_CNTXT_FULL;
				}
				else
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
						"RtShUserDataIntf::rtHandleCugSrvcData() rtGetNewCntxt() success cntxt_id =%u for l_ent_cug_id=%s user_identity=%s",
						lp_cntxt->session_cookie.self_indx, l_ent_cug_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id);

					lp_cntxt->cntxt_state 						= RT_SH_UDI_CNTXT_NON_IDLE;
					
					lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROC_SRVC_APPL_REQ;
          
					lp_cntxt->cntxt_flag |=  RT_UDI_IS_CUG_CNTXT;

					l_rval = rtInsertUserIdenVsCntxtMapEntry(l_ent_cug_id,lp_cntxt->session_cookie.self_indx);

					if( RT_SUCCESS != l_rval )
					{
						 mp_user_cntxt_mgr->rtReturnCntxt(lp_cntxt->session_cookie.self_indx);

						 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
							 "RtShUserDataIntf::rtHandleCugSrvcData(),ERROR rtInsertUserIdenVsCntxtMapEntry FAILED for user_cntxt_id =%u,l_ent_cug_id=%s,a_token=%lu",
							 lp_cntxt->session_cookie.self_indx, l_ent_cug_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);
						
						 l_rval = RT_SH_UDI_ERR_IN_INTRNL_MAP;
					
					}
					else
					{
				     	/*------------------------------------------------------------------------------------------------
	            - Updating cug_cntx_id in proc_req..cntxt_indx,It will be replaced by caller with their cntxt_id
							- we r updating there just to return cug_cntxt_id and it also used while processing usr udr req
							- This cug_cntx_id has to be shifted to its place in usr_cntxt later
	            --------------------------------------------------------------------------------------------------*/

						  ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx	= lp_cntxt->session_cookie.self_indx;
							
							/*------------------------------------------------------------------------------------
	                - RT_O_UDI_INITIATE_UDR_MSG to Initiate UDR for user_srvc data req
	                --------------------------------------------------------------------------------------*/
	  

							ap_proc_req->opcode                                  =   RT_O_UDI_INITIATE_UDR_MSG;
							
							string l_eid_cid(l_ent_cug_id); 
			        lp_cntxt->alias_ids_list.push_back(l_eid_cid);

							lp_cntxt->processing_req.proc_req_list.push_front(*ap_proc_req);
							lp_cntxt->processing_req.curr_req_iter	= lp_cntxt->processing_req.proc_req_list.begin();
							
							mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiApplReqPushedInPend);
						 
						 /*------------------------------------------------------------------------------------
	            	- RT_O_SH_UDI_APPL_MSG to Initiate UDR for cug_srvc data req
	            	--------------------------------------------------------------------------------------*/

				      
							ap_proc_req->opcode                                  =   RT_O_SH_UDI_APPL_MSG;
							
							RtTransMsg				l_trans_msg;
							memset(&l_trans_msg,0,sizeof(RtTransMsg));
      				RtShUDIProcReq*		lp_proc_req 	= (RtShUDIProcReq*)l_trans_msg.msg_buffer;

							*lp_proc_req = *ap_proc_req;
              
							lp_proc_req->req_body.appl_msg.msg_body.appl_req.is_cug_req = true;
							//No need to replace user_id with eid,As we are maintaining user,eid and cid all in proc_req
							//strcpy(lp_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id,ap_cug_name);
							 
							l_rval = rtSendToDbWrkr(&l_trans_msg, ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);

							if(RT_SUCCESS != l_rval )
							{
								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
									"RtShUserDataIntf::rtHandleCugSrvcData(): rtSendToDbWrkr() FAILED  l_ent_cug_id [%s],a_appl_token[%ld],l_cntxt_id[%u]",
									l_ent_cug_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token, ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);										
							  
								l_rval = RT_SH_UDI_ERR_IN_SNDING_WRKR;
							}
							else
							{
								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
									"RtShUserDataIntf::rtHandleCugSrvcData() appl msg dispatched to Dbwrkr l_ent_cug_id =%s,app_usr_id=%s,a_appl_token=%lu,l_cntxt_id=%u",
									l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token, ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);
								
								l_rval = RT_SH_UDI_WAIT_FOR_RSP;
							}
							mp_user_cntxt_mgr->rtReleaseCntxtDataLock(lp_cntxt->session_cookie.self_indx);
				   }
	       }/* end of create new contxt susccess */
		}//end of l_is_create_cntx_req TRUE CASE
		else
		{
			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
				"RtShUserDataIntf::rtHandleCugSrvcData(),Cntxt_id=%u present and Retrievd SUCCESSFully Valid for l_ent_cug_id =%s,app_usr_id=%s,a_appl_token=%lu",
						ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);

			if(lp_cntxt->cntxt_flag & RT_SH_UDI_UDA_RECEIVED)
			{
				 lp_cntxt->cntxt_state 						 = RT_SH_UDI_CNTXT_NON_IDLE;
				 lp_cntxt->cntxt_processing_state  = RT_SH_UDI_PROC_SRVC_APPL_REQ;
				 RtBoolT l_cache_req = true;
				 
				 /*------------------------------------------------------------------------------------------------------------------------
	       -   							                     ++++ NOTE ++++
				 -  Cug conxt caching is DISABLED in this Release , As it required additional handling in cacheTimeOut->flushing
				 -  We can enable cache by maintaining contxt referal count to decide flush or not
	       --------------------------------------------------------------------------------------------------------------------------*/
				 
				 
// 				 if(lp_cntxt->cntxt_flag & RT_SH_UDI_USR_CNTXT_CACHE_ENABLED)
// 				 {
// 						if( RT_SUCCESS != mp_cache_keeper->rtReCacheData(lp_cntxt->cache_id) )
// 						{
// 							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
// 							"RtShUserDataIntf::rtHandleCugSrvcData(), rtReCacheData failed for cache_id=%lld of cntxt_indx=%lld ap_cug_name =%s,app_usr_id=%s,a_appl_token=%lu",
// 						      	 lp_cntxt->cache_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,ap_cug_name,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);
// 						} 
// 						else
// 						{
// 							 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
// 									"RtShUserDataIntf::rtHandleCugSrvcData(), rtReCacheData SUCCESS for cache_id=%d of cntxt_indx=%d ap_cug_name =%s,app_usr_id=%s,a_appl_token=%lu",
// 						      	 lp_cntxt->cache_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,ap_cug_name,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);
// 						   
// 							 l_cache_req = false;
// 						}
//          }
// 				 if(l_cache_req)
// 				 {
// 				   	 RtCacheKeeperBuffer l_buffer;
// 						 bzero(&l_buffer,sizeof(l_buffer));
// 						 l_buffer.index = lp_cntxt->session_cookie.self_indx;
// 
// 						 RtTransAddr 	l_self_addr;
// 						 memset(&l_self_addr,0,sizeof(l_self_addr));
// 						 RtMglIntf::rtGetInstance()->rtGetSelfAddress(l_self_addr);
// 						 l_self_addr.ee_id = RT_EE_SH_UDI_WRKR_BASE  + (l_buffer.index % m_num_wrkr);
// 
// 						 if( RT_SUCCESS != mp_cache_keeper->rtCacheData(&l_buffer,&l_self_addr,lp_cntxt->cache_id) )
// 						 {
// 							 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 								 "RtShUserDataIntf::rtHandleCugSrvcData(),ERROR rtCacheData failed with cache_id=%lld for cntxt_indx=%lld ap_cug_name =%s,app_usr_id=%s,a_appl_token=%lu",
// 						      	lp_cntxt->cache_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,ap_cug_name,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);
// 						 }
// 						 else
// 						 {
// 							 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
// 								 "RtShUserDataIntf::rtHandleCugSrvcData(),rtCacheData success with cache_id=%lld for cntxt_indx=%lld ap_cug_name =%s,app_usr_id=%s,a_appl_token=%lu",
// 						      	lp_cntxt->cache_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,ap_cug_name,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id, ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);
// 
// 							 lp_cntxt->cntxt_flag |= RT_SH_UDI_USR_CNTXT_CACHE_ENABLED;
// 						 }
// 				 }
				 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					 "RtShUserDataIntf::rtHandleCugSrvcData() l_cntxt_id=%u is present with data returing SUCCESS for l_ent_cug_id=%s,app_usr_id=%s appl_token=%ld",
					 ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);

         l_rval =  RT_SUCCESS;
			}
			else if(lp_cntxt->cntxt_processing_state == RT_SH_UDI_WAIT_FOR_UDA           // Waiting for UDA
					    || RT_SH_UDI_PROC_SRVC_APPL_REQ == lp_cntxt->cntxt_processing_state) // Waiting for UDR handling
			{
				 lp_cntxt->cntxt_state 						 = RT_SH_UDI_CNTXT_NON_IDLE;

				 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					 "RtShUserDataIntf::rtHandleCugSrvcData() l_cntxt_id=%u is present with cntxt_processing_state=%u UDA_WAIT so returing RT_SH_UDI_WAIT_FOR_RSP for l_ent_cug_id=%s,app_usr_id=%s appl_token=%ld",
						ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,lp_cntxt->cntxt_processing_state,l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);

				 ap_proc_req->opcode                =   RT_O_UDI_INITIATE_UDR_MSG;

	       lp_cntxt->processing_req.proc_req_list.push_back(*ap_proc_req);
				 
				 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiApplReqPushedInPend);
				 
				 if(lp_cntxt->processing_req.proc_req_list.size() == 1)
						lp_cntxt->processing_req.curr_req_iter	= lp_cntxt->processing_req.proc_req_list.begin();

          ap_proc_req->opcode               =   RT_O_SH_UDI_APPL_MSG;

					l_rval =  RT_SH_UDI_WAIT_FOR_RSP;
			}
			else
			{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					 "RtShUserDataIntf::rtHandleCugSrvcData() ERROR l_cntxt_id=%u present with UNKNOWN state cntxt_flag=%u cntxt_processing_state=%d for l_ent_cug_id=%s,app_usr_id=%s appl_token=%ld",
					 ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx,lp_cntxt->cntxt_flag,lp_cntxt->cntxt_processing_state,l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.appl_token);

					l_rval = RT_FAILURE; 
			}
			mp_user_cntxt_mgr->rtReleaseCntxtDataLock(ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);

			
		}/* end of retrieve contxt success or end of l_is_create_cntx_req FALSE CASE */
	  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			  "RtShUserDataIntf::rtHandleCugSrvcData(), Leave with l_rval=%d l_ent_cug_id=%s,usr=%s with cug contxt_id=%u",
				l_rval,l_ent_cug_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.app_usr_id,ap_proc_req->req_body.appl_msg.msg_body.appl_req.cntxt_indx);

	 return l_rval;
}
/*****************************************************************************************
 *
 * FUNCTION NAME : rtHandleProvMsg()
 *
 * DESCRIPTION   : This API will Sync its service data with db on receiving prov msg
 *						     
 *								 
 * INPUT         : 
 *
 * OUTPUT        : 
 *
 * RETURN        : RtRcT
 *
 ****************************************************************************************/		
/* RtRcT RtShUserDataIntf::rtHandleProvMsg(RtMMTelServiceAmfEventData& ar_srvc_amf_evnt_data)
{
	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf::rtHandleProvMsg() request_for=%d service_prov_action=%d",ar_srvc_amf_evnt_data.request_for,ar_srvc_amf_evnt_data.service_prov_action);
	
	
	RtRcT 		l_ret  			     = RT_FAILURE;

	switch(ar_srvc_amf_evnt_data.request_for)
	{
	   case RT_REQUEST_FOR_USER:
		 {
	       mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			     "RtShUserDataIntf::rtHandleProvMsg(),Going to Handle User_srvc_prov update as request_for=%d and service_prov_action=%d",ar_srvc_amf_evnt_data.request_for,ar_srvc_amf_evnt_data.service_prov_action);
	        
				  
				 l_ret = rtHandleCntxtSrvcProvReq(ar_srvc_amf_evnt_data.mmtel_service_data.mmtel_user_service_data.user_identity,
				                                   ar_srvc_amf_evnt_data.service_name,
																					 ar_srvc_amf_evnt_data.service_prov_action,
																					 false,true); 
		 }
		 break;
	   case RT_REQUEST_FOR_SYSTEM:
		 {
	       mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			     "RtShUserDataIntf::rtHandleProvMsg(),Going to Handle system_srvc_prov update as request_for=%d and service_prov_action=%d",ar_srvc_amf_evnt_data.request_for,ar_srvc_amf_evnt_data.service_prov_action);
	       
				 l_ret = rtHandleSystmSrvcDataProvMsg(ar_srvc_amf_evnt_data.service_name,ar_srvc_amf_evnt_data.service_prov_action); //update only possible in this case.
		 }
		 break;
		 case RT_REQUEST_FOR_SYSTEM_CONFIG:
		 {
	       mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			     "RtShUserDataIntf::rtHandleProvMsg(),Going to Handle system_srvc_prov update as request_for=%d and service_prov_action=%d",ar_srvc_amf_evnt_data.request_for,ar_srvc_amf_evnt_data.service_prov_action);
	       
				 l_ret = rtHandleSystmSrvcCfgProvMsg(ar_srvc_amf_evnt_data.service_name,ar_srvc_amf_evnt_data.service_prov_action); 
		 }
		 break;
	   case RT_REQUEST_FOR_CUG: 
		 {
	       mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			     "RtShUserDataIntf::rtHandleProvMsg(),Going to Handle cug_srvc_prov update as request_for=%d and service_prov_action=%d",ar_srvc_amf_evnt_data.request_for,ar_srvc_amf_evnt_data.service_prov_action);
	       
				l_ret = rtHandleCntxtSrvcProvReq(ar_srvc_amf_evnt_data.mmtel_service_data.mmtel_cug_service_data.cug_name,
				                                   ar_srvc_amf_evnt_data.service_name,
																					 ar_srvc_amf_evnt_data.service_prov_action,
																					 true,true); 

		 }
		 break;
	   default:
		 {
	       mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			     "RtShUserDataIntf::rtHandleProvMsg(),ERROR Invalid request_for=%d with service_prov_action=%d",ar_srvc_amf_evnt_data.request_for,ar_srvc_amf_evnt_data.service_prov_action);
	       
		 }
		 break;
	
	}

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf :: rtHandleProvMsg()[EEId=%d]- ENTER - [Opcode = %u]",m_self_addr.ee_id,ap_sh_msg->opcode);

  return l_ret;
} */



/*****************************************************************************************
*
* FUNCTION NAME : rtHandleCntxtSrvcProvReq()
*
* DESCRIPTION   : This API will Sync its service data with db on receiving prov msg
*						     
*								 
* INPUT         : 
*
* OUTPUT        : 
*
* RETURN        : RtRcT
*
****************************************************************************************/		
RtRcT RtShUserDataIntf::rtHandleCntxtSrvcProvReq(RtS8T* ap_usr_name,
 																								RtS8T* ap_eid,     			/*Nw->input for multi-enterprise*/
																								RtS8T* ap_cid,					/*Nw->input for multi-enterprise*/
                                                RtS8T* ap_srvc_string,
																								RtSrvcProvActionTypeT ar_prov_action,
																								RtBoolT a_is_cug_prov_req,
																								RtBoolT a_regen_req)
{		
	
	 RtRcT 		l_ret  			          = RT_FAILURE;
	 RtShUDIUserCntxt* lp_cntxt  		= NULL;
	 
	 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqCalled);
	 
	 if(ap_srvc_string == NULL || (strlen(ap_srvc_string) < 1)
	    || ap_eid == NULL || ap_cid == NULL
			|| (ap_usr_name == NULL &&(!a_is_cug_prov_req))
			)
	 {
	   mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			  "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ERROR ENTER/LEAVE Invalid input ap_srvc_string=%s or ap_usr_name=%s ap_eid=%s ap_cid=%s a_is_cug_prov_req=%d",ap_srvc_string,ap_usr_name,ap_eid,ap_cid,a_is_cug_prov_req);
	   
		 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetFail);
	   return RT_FAILURE;
	 }

	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf::rtHandleCntxtSrvcProvReq(), ENTER ap_usr_name=%s ap_eid=%s ap_cid=%s ap_srvc_string=%s ar_prov_action=%d and a_is_cug_prov_req=%d a_regen_req=%d",ap_usr_name,ap_eid,ap_cid,ap_srvc_string,ar_prov_action,a_is_cug_prov_req,a_regen_req);


	/*-------------------------------------------------------------------------------------------
	- +++  User services will be flushed upon prov_msg irrespective of service_prov_action +++
	- Check whether context is created for User or not
	- Mark context for DELETION
	- Check context can be flushed right away or not
	- Flush context or Return back ( Let UdiWorker flush upon UDA )
	- TBD :: Any other conditions ???
	-------------------------------------------------------------------------------------------*/



	 /*----------------------------------------------------------------------
	 - BTAS TBD:: RAJENDER  Proper error code set has to be defined
	 ------------------------------------------------------------------------*/


	 /*-----------------------------------------------------------------------------------------------------------------------------------------
	 -     MULTI-CUG changes by RAJENDER
	 - 1.Context is created for user_identity,enterprise_id and cug_id combination, We are using '_' as a delimiter while key genration
	 -----------------------------------------------------------------------------------------------------------------------------------------*/
   RtS8T l_usr_ent_cug_id[3*RT_MAX_STRING_LEN] = {'\0'};
	 if(a_is_cug_prov_req)
	     snprintf(l_usr_ent_cug_id,sizeof(l_usr_ent_cug_id),"%s_%s",ap_eid,ap_cid);
	 else
	     snprintf(l_usr_ent_cug_id,sizeof(l_usr_ent_cug_id),"%s_%s_%s",ap_usr_name,ap_eid,ap_cid);



	 RtU32T 	l_cntxt_id  = 0;
	 RtBoolT  l_regen_req = false;
	 
	 l_ret = rtFindCntxtForUserIden(l_usr_ent_cug_id,l_cntxt_id);
	 if(l_ret != RT_SUCCESS && l_cntxt_id == 0 )
	 {
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
				 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq()-NO ContextForUser Present l_ret=%d l_usr_ent_cug_id=%s ap_srvc_string=%s a_is_cug_prov_req=%d ar_prov_action=%d",l_ret,l_usr_ent_cug_id,ap_srvc_string,a_is_cug_prov_req,ar_prov_action);
		 
		  /*******************************************************************
			  - User Contxt is not Found,It might be flushed/or not loaded
				- Returning SUCCESS in this case as nothing need to be done
			 *******************************************************************/
			
			l_ret  			     = RT_SUCCESS;
	 }
	 else
	 {
		 RtS8T* lp_orig_srvc_str = new RtS8T[strlen(ap_srvc_string)+1];
	   strcpy(lp_orig_srvc_str,ap_srvc_string);

		 if ( RT_SUCCESS != mp_user_cntxt_mgr->rtRetrieveCntxtData(l_cntxt_id, &lp_cntxt) )
		 {
			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ERROR rtRetrieveCntxtData FAILED  for l_cntxt_id=%u with l_usr_ent_cug_id=%s",
					l_cntxt_id,l_usr_ent_cug_id);

			/***********************************************************
			  - User Contxt retreival failure means,It might be flushed
				-  Returning SUCCESS in this case as nothing need to be done
			 ***********************************************************/

			 
			 l_ret = RT_SUCCESS;
		 }
		 else
		 {
			 	RtBoolT l_is_cntxt_valid = false;
				list<string>::iterator l_alias_iter = lp_cntxt->alias_ids_list.begin();
				while(l_alias_iter	!= lp_cntxt->alias_ids_list.end())
				{
		 			 if(strcmp((RtS8T*)l_alias_iter->c_str(),l_usr_ent_cug_id) == 0)
					 {
							l_is_cntxt_valid = true;
							break;
        	 }
					 else
							++l_alias_iter;
    		}
				if(!l_is_cntxt_valid)
				{
					 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ERROR l_cntxt_id=%d is INVALID for l_usr_ent_cug_id=%s,So Erasing Uname from	CntxtMap and Return Failure",
						  l_cntxt_id,l_usr_ent_cug_id);
           
					 rtEraseUserIdenVsCntxtMapEntry(l_usr_ent_cug_id);
					 
           mp_user_cntxt_mgr->rtReleaseCntxtDataLock(lp_cntxt->session_cookie.self_indx);
					 lp_cntxt = NULL;
					 /***********************************************************
			  		  - User Contxt is not valid,It might be flushed
						  - Returning SUCCESS in this case as nothing need to be done
						 ***********************************************************/

					 l_ret  			     = RT_SUCCESS;;
				}
			  else
			  {
				 if(lp_cntxt->cntxt_flag &	RT_SH_UDI_UDA_RECEIVED)
				 {
						 /*-------------------------------------------------------------------------------------------------------
								- +++ User specific service data based on  service_prov_action +++
								-  RT_MMTEL_CREATE_SERVICE_DATA is not valid for sys_def_srvc
								-  RT_MMTEL_RETREIVE_SERVICE_DATA is not valid at this level
								-  RT_MMTEL_UPDATE_SERVICE_DATA
								-   -For each service_name received in amf_data(use strtok service_name) on  get its RtShSrvcIndTokenData value
								-   -RtShSrvcIndTokenData.srvc_token is the data_ref index
								-   -RtShSrvcIndTokenData.app_info.app_enum is the service_id for the db query
								-------------------------------------------------------------------------------------------------------*/
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
				    	 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(), l_cntxt_id=%u is present with UDA_RCVD fr l_usr_ent_cug_id=%s a_is_cug_prov_req=%d ar_prov_action=%d",l_cntxt_id,l_usr_ent_cug_id,a_is_cug_prov_req,ar_prov_action);



						RtRcT 		l_ret_val  			     = RT_FAILURE;
						RtBoolT   l_srvc_loop_break	   = false;
						RtShSrvcIndTokenData  							l_srvc_token_data;
						RtSrvcIndVsSrvcTokenDataMapItr 			l_itr;
						RtU32T															l_mand_pool_elem  = 0;
  					RtShUDIDataRefVal									  l_udi_ref 		    = 0;
  					RtS8T 					l_str[2] = ",";
  					RtS8T 					*l_srvc_token;
          	RtS8T*           lp_strtok_ptr;
						l_srvc_token = strtok_r(lp_orig_srvc_str, l_str,&lp_strtok_ptr);
						while(l_srvc_token != NULL)
						{
							 l_itr = m_srvc_ind_vs_srvc_token_map.find(l_srvc_token);
							 if( l_itr != m_srvc_ind_vs_srvc_token_map.end() )
							 {
								 l_srvc_token_data = l_itr->second;
								 l_udi_ref = l_srvc_token_data.srvc_token;
								 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			  					 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),Going for l_srvc_token=%s with l_udi_ref=%d for l_usr_ent_cug_id=%s ar_prov_action=%d",l_srvc_token,l_udi_ref,l_usr_ent_cug_id,ar_prov_action);

							 }
							 else
							 {
	    				  	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			  					 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ERROR m_srvc_ind_vs_srvc_token_map.find()-FAILED for service_name=%s for l_usr_ent_cug_id=%s prov req",l_srvc_token,l_usr_ent_cug_id);
                  l_ret_val = RT_FAILURE;	
							  	break;
							 }
							 switch(ar_prov_action)
							 {
									case RT_UDI_PROV_ACTN_UPDATE:
									{
											if( !(lp_cntxt->p_data_ref_arr[l_udi_ref].data_state & RT_SH_UDI_DATA_STATE_VALID ))
											{
												 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
													"RtShUserDataIntf::rtHandleUsrSrvcProvMsg(),ERROR RT_MMTEL_UPDATE_SERVICE_DATA - service=%s l_udi_ref=%d data is NOT Valid in contxt=%u, Update noT allowed for l_usr_ent_cug_id=%s",
													l_srvc_token,l_udi_ref, l_cntxt_id,l_usr_ent_cug_id);
												
												 l_srvc_loop_break = true;		
												 break;	
											}
											else if(RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY & lp_cntxt->p_data_ref_arr[l_udi_ref].data_state)
											{
												mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
													"RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ERROR RT_MMTEL_UPDATE_SERVICE_DATA -UDI_STATE _VALID_DUE_TO_MANDATORY  with udi ref=%d of l_srvc_token=%s with elem_index=%u at l_cntxt_id=%u Update noT allowed",
													 l_udi_ref,l_srvc_token,lp_cntxt->p_data_ref_arr[l_udi_ref].elem_indx,l_cntxt_id);
											  l_srvc_loop_break = true;		
												break;	
											}	
											else
											{
												  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			    						  	 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),RT_UDI_PROV_ACTN_UPDATE Going To Fetch data fr service_name=%s l_udi_ref=%d l_usr_ent_cug_id=%s to update at l_cntxt_id=%d",l_srvc_token,l_udi_ref,l_usr_ent_cug_id,l_cntxt_id);
     
												 RtUdiDbReqInputT l_db_in;
	  										 l_db_in.rg_data_ptr_list.clear();

												 RtUdiDbReqOutputT l_db_out;

												 l_db_out.data.def_srvc_data.data_len = 0;

												 if(a_is_cug_prov_req)
										  		 l_db_in.srvc_type = RT_UDI_DB_CUG_SRVC_DATA_REQ;
												 else
												 {
										  		 l_db_in.srvc_type = RT_UDI_DB_USR_SRVC_DATA_REQ;

												   strcpy(l_db_in.data.usr_info.usr_identity , ap_usr_name);
            						 }
												 strcpy(l_db_in.data.usr_info.service_name,l_srvc_token);
												 
												 strncpy(l_db_in.data.usr_info.enterprise_id,ap_eid,sizeof(l_db_in.data.usr_info.enterprise_id));
												 strncpy(l_db_in.data.usr_info.cug_id,ap_cid,sizeof(l_db_in.data.usr_info.cug_id));


												 l_ret_val = mp_udi_db_hdlr->rtFetchServiceData(l_db_in,l_db_out);
												 if(l_ret_val != RT_SUCCESS)
												 {
														mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
															 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ERROR rtFetchServiceData() Failed - with srvc_type=%d l_usr_ent_cug_id=%s,service_name=%s So Leaving ",l_db_in.srvc_type,l_usr_ent_cug_id,l_db_in.data.usr_info.service_name);
														l_srvc_loop_break = true;		
														break;	
												 }
												 if(l_db_out.db_ext_result != RT_UDI_DB_REQ_RESULT_SUCCESS)
												 {
														mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			  											 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(), ERROR rtFetchServiceData() data_not_Found OR processing_failed as db_ext_result=%d for l_usr_ent_cug_id=%s,service_name=%s",l_db_out.db_ext_result,l_usr_ent_cug_id,l_db_in.data.usr_info.service_name);

														 l_srvc_loop_break = true;
                						 break;
												 }

												 l_ret_val = mp_data_pool_mgr->rtUpdDataRefData(l_udi_ref,l_db_out.data.def_srvc_data.vp_srvc_data,lp_cntxt->p_data_ref_arr[l_udi_ref].p_data_ptr);
  								  		 if(l_ret_val != RT_SUCCESS)
												 {

														mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
															 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(), rtUpdDataRefData() Failed - l_ret_val=%d for l_udi_ref=%d of srvc_type=%s l_usr_ent_cug_id=%s",l_ret_val,l_udi_ref,l_srvc_token,l_usr_ent_cug_id);
										  			l_srvc_loop_break = true;	
												 }
												 else
												 {
										    		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			    						  			"RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),RT_UDI_PROV_ACTN_UPDATE SUCCESS for service_name=%s l_udi_ref=%d l_usr_ent_cug_id=%s at l_cntxt_id=%d",l_srvc_token,l_udi_ref,l_usr_ent_cug_id,l_cntxt_id);
                        		 l_regen_req = true;	
								    		 }
											}//End of DATA_REF STATE VALIDATION SUCCESS case
									}
									break;
									case RT_UDI_PROV_ACTN_CREATE :
									{
										 if( (lp_cntxt->p_data_ref_arr[l_udi_ref].data_state & RT_SH_UDI_DATA_STATE_VALID)
															 &&
										    	(! (lp_cntxt->p_data_ref_arr[l_udi_ref].data_state & RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY))

									      	)
											{

												 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
													 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ALERT RT_MMTEL_CREATE_SERVICE_DATA - RT_SH_UDI_DATA_STATE_VALID for service=%s l_udi_ref=%d in l_cntxt_id=%u,for l_usr_ent_cug_id=%s,Doing Nothing",
													   l_srvc_token,l_udi_ref, l_cntxt_id,l_usr_ent_cug_id);
												 //l_srvc_loop_break = true; //Treating it as success,as nothing to do
												 break;
											}
											else
											{
												 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			    						  			"RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),RT_UDI_PROV_ACTN_CREATE Going To Fetch data fr service_name=%s l_udi_ref=%d l_usr_ent_cug_id=%s to update at l_cntxt_id=%d",l_srvc_token,l_udi_ref,l_usr_ent_cug_id,l_cntxt_id);

												 RtUdiDbReqInputT l_db_in;
	  										 l_db_in.rg_data_ptr_list.clear();

												 RtUdiDbReqOutputT l_db_out;


												 if(a_is_cug_prov_req)
										  		 l_db_in.srvc_type = RT_UDI_DB_CUG_SRVC_DATA_REQ;
												 else
												 {
										  		 l_db_in.srvc_type = RT_UDI_DB_USR_SRVC_DATA_REQ;

												   strcpy(l_db_in.data.usr_info.usr_identity ,ap_usr_name);
            						 }
												 strcpy(l_db_in.data.usr_info.service_name,l_srvc_token);
												 
												 strncpy(l_db_in.data.usr_info.enterprise_id,ap_eid,sizeof(l_db_in.data.usr_info.enterprise_id));
												 strncpy(l_db_in.data.usr_info.cug_id,ap_cid,sizeof(l_db_in.data.usr_info.cug_id));

												 l_ret_val = mp_udi_db_hdlr->rtFetchServiceData(l_db_in,l_db_out);
												 if(l_ret_val != RT_SUCCESS)
												 {
														mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
															 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(), rtFetchServiceData() Failed - with srvc_type=%d l_usr_ent_cug_id=%s,service_name=%s So Leaving ",l_db_in.srvc_type,l_usr_ent_cug_id,l_db_in.data.usr_info.service_name);

														l_srvc_loop_break = true;		
														break;			
												 }
												 if(l_db_out.db_ext_result != RT_UDI_DB_REQ_RESULT_SUCCESS)
												 {
														mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			  											 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(), ERROR rtFetchServiceData() data_not_Found OR processing_failed as db_ext_result=%d for usr_identity=%s,service_name=%s",l_db_out.db_ext_result,l_usr_ent_cug_id,l_db_in.data.usr_info.service_name);

														 l_srvc_loop_break = true;	
                						 break;
												 }
												 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
														 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),Goint To storeDataRefData for service=%s l_udi_ref=%d at elem_indx=%d l_cntxt_id=%d for usr_ecid=%s",
														 l_srvc_token,l_udi_ref,lp_cntxt->p_data_ref_arr[l_udi_ref].elem_indx,l_cntxt_id,l_usr_ent_cug_id);	

												 l_ret_val = mp_data_pool_mgr->rtStoreDataRepositoryData(l_udi_ref, 
																																							 (void*) l_db_out.data.def_srvc_data.vp_srvc_data,
																																							 l_db_out.data.def_srvc_data.data_len,
																																							 lp_cntxt->p_data_ref_arr[l_udi_ref].elem_indx,
																																							 &lp_cntxt->p_data_ref_arr[l_udi_ref].p_data_ptr);
												 if (RT_SUCCESS == l_ret_val)
												 {
													 lp_cntxt->p_data_ref_arr[l_udi_ref].data_state  	|= 	RT_SH_UDI_DATA_STATE_VALID;
                        	 lp_cntxt->p_data_ref_arr[l_udi_ref].data_state    &= ~RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY;
													 //Seq no is not handled

													 //lp_cntxt->p_data_ref_arr[l_udi_ref].old_seq_no				= 	ap_cntxt->p_data_ref_arr[l_udi_ref].seq_no;
													 //lp_cntxt->p_data_ref_arr[l_udi_ref].seq_no						= 	ap_sh_user_data->p_reporitory_data_arr[l_cnt].seq_no;


													 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
														 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),RT_UDI_PROV_ACTN_CREATE SUCCESS for service=%s l_udi_ref=%d with updated elem_indx=%d at l_cntxt_id=%d l_usr_ent_cug_id=%s",
														 l_srvc_token,l_udi_ref,lp_cntxt->p_data_ref_arr[l_udi_ref].elem_indx,l_cntxt_id,l_usr_ent_cug_id);	

													 l_regen_req = true;						
												 }
												 else
												 {
	      						  			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			    						  		 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ERROR RT_UDI_PROV_ACTN_CREATE FAILED for service_name=%s l_udi_ref=%d at l_cntxt_id=%d l_usr_ent_cug_id=%s",
											  			l_srvc_token, l_udi_ref,l_cntxt_id,l_usr_ent_cug_id);				
														l_srvc_loop_break = true;	
														break;						
												 }
											}//end of data_ref validation in contxt					
									}break;
									case RT_UDI_PROV_ACTN_DELETE:
									{
											if(RT_SH_UDI_DATA_STATE_VALID & lp_cntxt->p_data_ref_arr[l_udi_ref].data_state)
											{
												if(RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY & lp_cntxt->p_data_ref_arr[l_udi_ref].data_state)
												{
													 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
													   "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ERROR RT_UDI_PROV_ACTN_DELETE -UDI_STATE _VALID_DUE_TO_MANDATORY  with udi ref=%d of l_srvc_token=%s with elem_index=%u at l_cntxt_id=%u DOing NOTHING",
													    l_udi_ref,l_srvc_token,lp_cntxt->p_data_ref_arr[l_udi_ref].elem_indx,l_cntxt_id);
                           
													 l_srvc_loop_break = true;	
												   break;		
												}	
												else
												{
													 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_INFO, __FILE__,  __LINE__,
														 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(), RT_UDI_PROV_ACTN_DELETE deleting udi ref =%d of l_srvc_token=%s with elem_index=%u at l_cntxt_id=%u of l_usr_ent_cug_id=%s",
														 l_udi_ref,l_srvc_token,lp_cntxt->p_data_ref_arr[l_udi_ref].elem_indx,l_cntxt_id,l_usr_ent_cug_id);							

													 mp_data_pool_mgr->rtReturnPoolElem(l_udi_ref, lp_cntxt->p_data_ref_arr[l_udi_ref].elem_indx);

													 lp_cntxt->p_data_ref_arr[l_udi_ref].data_state  	= 0;
													 lp_cntxt->p_data_ref_arr[l_udi_ref].p_data_ptr		= NULL;
													 lp_cntxt->p_data_ref_arr[l_udi_ref].elem_indx 		= 0;
													 l_regen_req = true;		
												}
											}
											else
											{
												 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
													"RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ALERT RT_UDI_PROV_ACTN_DELETE -UDI_STATE already invalid with udi ref=%d of l_srvc_token=%s with elem_index=%u at l_cntxt_id=%u DOing NOTHING",
													  l_udi_ref,l_srvc_token,lp_cntxt->p_data_ref_arr[l_udi_ref].elem_indx,l_cntxt_id);
												 
												 //l_srvc_loop_break = true;//Treating it as success ,as nothing to do
											}
									}break;
									default:
									{
	      							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			    							"RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ERROR Invalid ar_prov_action=%d for l_srvc_token=%s l_usr_ent_cug_id=%s a_is_cug_prov_req=%d at l_cntxt_id=%d",ar_prov_action,l_srvc_token,l_usr_ent_cug_id,a_is_cug_prov_req,l_cntxt_id);
                    	l_srvc_loop_break = true;
									}
									break;	  							

						 }//end of swtich with mmtelserviceprovaction

						 if(l_srvc_loop_break)
						 {
								l_ret_val = RT_FAILURE;	
								break;
						 }
						 else
					    	l_srvc_token = strtok_r(NULL, l_str,&lp_strtok_ptr);

						}//end of while of token.

						//delete[] lp_orig_srvc_str; //Klock_work_fix@25-08-2016
				 }/* ---- End of UDA received ----- */
				 else /* ---- UDA is not received Yet ----- */
				 {
			    	/*--------------------------------------------------------------------------------------------------------------
							 -     TBD ::  
							 - 1. We can Do nothing in this case , As UDA will come with latest data
							 - 2. Or we can mark it for deletion , In UDA processing reinitiate UDR
							 - 3. Or we can put this prov req in processing list , After UDR processing that has to handle prov req
							 -    
							 -----------------------------------------------------------------------------------------------------------------*/
			    	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
				  	 "RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),ALERT  UDA_NOT_RECEIVED in l_cntxt_id=%u for l_usr_ent_cug_id=%s ap_srvc_string=%s ar_prov_action=%d",
					  	l_cntxt_id,l_usr_ent_cug_id,ap_srvc_string,ar_prov_action);

				  	l_ret = RT_SUCCESS;
				 }
			 
				 
				 if(l_regen_req && a_regen_req)
				 {
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
							"RtShUserDataIntf::rtHandleCntxtSrvcProvReq(),Going to RegenOrderListFrCntxt=%d as SRVC PROV SUCCS and l_regen_req TRUE for l_usr_ent_cug_id=%s ap_srvc_string=%s ar_prov_action=%d,a_is_cug_prov_req=%d",
										l_cntxt_id,l_usr_ent_cug_id,ap_srvc_string,ar_prov_action,a_is_cug_prov_req);	

				    /*----------------------------------------------------------------------------------------------------------------------------
							 -          rtReleaseCntxtDataLock()
							 - 1. In cug_prov_req case we have to regenerate OrderList for each context,So unlock context first and invoke API
							 - 2. In usr_prov_req case we have to regenerate OrderList of that user context only , So regenrate then Unlock COntxt
							 -------------------------------------------------------------------------------------------------------------------------------*/

						if(a_is_cug_prov_req)
						{
							 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
							 if(m_immediate_regen)
							 {
								 mp_user_cntxt_mgr->rtRegenOrderedListForCugCntxt(l_cntxt_id);
							 }
							 else
							 {
								 mp_user_cntxt_mgr->rtMarkCugCntxtsForOrdListUpd(l_cntxt_id);
							 }
						}
						else
						{
							 if(m_immediate_regen)
							 {
								 mp_data_pool_mgr->rtCreateCaseOrderedListForUser(lp_cntxt);
							 }
							 else
							 {
                 lp_cntxt->regen_ord_list = true;
							 }
							 mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
						}
				 }//end of order_list_regen req
				 else
				 {
				    mp_user_cntxt_mgr->rtReleaseCntxtDataLock(l_cntxt_id);
				 }
			 } //End of cntxt valid
		 }/* ------ end of usr_cntxt Retrival SUCCESS -----*/
	   delete[] lp_orig_srvc_str; //Klock_work_fix@25-08-2016
	 }/* ------ end of usr_cntxt found SUCCESS -----*/
	 	
		

	 if(l_ret == RT_SUCCESS)
	    mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetSucc);
	 else
	    mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetFail);
		
	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf::rtHandleCntxtSrvcProvReq(), LEave with l_ret=%d l_cntxt_id=%u for l_usr_ent_cug_id=%s ap_srvc_string=%s ar_prov_action=%d",
			l_ret,l_cntxt_id,l_usr_ent_cug_id,ap_srvc_string,ar_prov_action);
	
  return l_ret;
}
/*****************************************************************************************
*
* FUNCTION NAME : rtHandleSystmSrvcCfgProvMsg()
*
* DESCRIPTION   : This API will Sync its service cfg data with db on receiving prov msg
*						     
*								 
* INPUT         : service_names seperated by ','   and Prov_action
*
* OUTPUT        : service configuration data updation using db
*
* RETURN        : success/failure
*
****************************************************************************************/		
RtRcT RtShUserDataIntf::rtHandleSystmSrvcCfgProvMsg(RtS8T* ap_srvc_string,RtSrvcProvActionTypeT ar_prov_action)
{
	 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqCalled);
	 
	 if(ap_srvc_string == NULL || strlen(ap_srvc_string) < 1)
	 {
	   mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			  "RtShUserDataIntf::rtHandleSystmSrvcCfgProvMsg(),ENTER/LEAVE Invalid input ap_srvc_string=%s with ar_prov_action=%d",ap_srvc_string==NULL?"NULL":ap_srvc_string,ar_prov_action);
	   
		 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetFail);
	   return RT_FAILURE;
	 }
	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf::rtHandleSystmSrvcCfgProvMsg(), ENTER ap_srvc_string=%s and ar_prov_action=%d",ap_srvc_string,ar_prov_action);
	
	
	 RtRcT 		l_ret_val  			     = RT_FAILURE;	
	 RtBoolT l_regen_req       = false;
	 RtBoolT l_srvc_loop_break = false;
	 

	 RtS8T* lp_orig_srvc_str = new RtS8T[strlen(ap_srvc_string)+1];
	 strcpy(lp_orig_srvc_str,ap_srvc_string);
	 
		 
	 /*-------------------------------------------------------------------------------------------------------
	 - +++ Update System service config data based on  service_prov_action +++
	 -  RT_MMTEL_CREATE_SERVICE_DATA is not valid for sys_def_srvc
	 -  RT_MMTEL_RETREIVE_SERVICE_DATA is not valid at this level
	 -  RT_MMTEL_UPDATE_SERVICE_DATA
	 -   -For each service_name received in amf_data(use strtok service_name) on  get its RtShSrvcIndTokenData value
	 -   -RtShSrvcIndTokenData.srvc_token is the data_ref index
	 -   -RtShSrvcIndTokenData.app_info.app_enum is the service_id for the db query
	 -------------------------------------------------------------------------------------------------------*/
 /*----------------------------------------------------------------------
	- BTAS TBD:: RAJENDER  Proper error code set has to be defined
	------------------------------------------------------------------------*/

	RtShSrvcIndTokenData  							l_srvc_token_data;
	RtSrvcIndVsSrvcTokenDataMapItr 			l_itr;
	RtU32T															l_mand_pool_elem  = 0;
  RtShUDIDataRefVal									  l_udi_ref 		    = 0;
  RtS8T 					 l_str[2] = ",";
	RtS8T*           lp_strtok_ptr;
	RtS8T* l_srvc_token = strtok_r(lp_orig_srvc_str, l_str,&lp_strtok_ptr);
	while(l_srvc_token != NULL)
	{
		 l_itr = m_srvc_ind_vs_srvc_token_map.find(l_srvc_token);
		 if( l_itr!= m_srvc_ind_vs_srvc_token_map.end() )
		 {
			 l_srvc_token_data = l_itr->second;
			 l_udi_ref = l_srvc_token_data.srvc_token;
			 
			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			   "RtShUserDataIntf::rtHandleSystmSrvcCfgProvMsg(),Going for l_srvc_tken=%s with l_udi_ref=%d for ar_prov_action=%d",l_srvc_token,l_udi_ref,ar_prov_action);
	
		 }
		 else
		 {
	     mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			   "RtShUserDataIntf::rtHandleSystmSrvcCfgProvMsg(),ERROR m_srvc_ind_vs_srvc_token_map.find()-FAILED for service_name=%s ar_prov_action=%d",l_srvc_token,ar_prov_action);
			 l_ret_val = RT_FAILURE;	
			 break;
		 }
		 switch(ar_prov_action)
		 {
	  		case RT_UDI_PROV_ACTN_UPDATE:
				{
						
						 /*----------------------------------------------------------------------
								 - BTAS TBD:: RAJENDER  Proper error code set has to be defined
								 ------------------------------------------------------------------------*/

						
						RtUdiDbReqInputT l_db_in;
	  				l_db_in.rg_data_ptr_list.clear();

						RtUdiDbReqOutputT l_db_out;
						//memset(&l_db_out,0,sizeof(l_db_out));

						l_db_in.srvc_type = RT_UDI_DB_DEF_SRVC_CFG_INFO_REQ;
            strcpy(l_db_in.data.srvc_cnf_info.service_name,l_srvc_token);
						
						l_ret_val = mp_udi_db_hdlr->rtFetchServiceData(l_db_in,l_db_out);
						if(l_ret_val != RT_SUCCESS)
						{
							 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
									"RtShUserDataIntf ::rtHandleSystmSrvcCfgProvMsg(),ERROR rtFetchServiceData() Failed - with srvc_type=%d for l_srvc_token=%s So Leaving ",l_db_in.srvc_type,l_srvc_token);
							 /*------------------------------------------------------------------------------
										-
										-  TBD :: Verify all possible return values before considering it as a failure
										-  
										------------------------------------------------------------------------------*/

							 l_srvc_loop_break = true;
							 break;			

						}
						if(l_db_out.db_ext_result != RT_UDI_DB_REQ_RESULT_SUCCESS)
						{
							  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			  					"RtShUserDataIntf::rtHandleSystmSrvcCfgProvMsg(), ERROR rtFetchServiceData() data_not_Found OR processing_failed  as db_ext_result=%d for service_name=%s ",l_db_out.db_ext_result,l_db_in.data.srvc_cnf_info.service_name);
								l_srvc_loop_break = true;	
                break;
						}

						l_ret_val = mp_data_pool_mgr->rtUpdateSysDftServiceCnfData(l_udi_ref,&(l_db_out.data.srvc_cnf_data));
						
						if(RT_SUCCESS ==	l_ret_val)
						{
							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
										"RtShUserDataIntf::rtHandleSystmSrvcCfgProvMsg(),RT_UDI_PROV_ACTN_UPDATE SUCCESS for service=%s l_udi_ref=%d,l_ret_val=%d",
										l_srvc_token, l_udi_ref,l_ret_val);	
							l_regen_req = true;		
						}					
						else
						{
								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
									"RtShUserDataIntf ::rtHandleSystmSrvcCfgProvMsg(),ERROR rtUpdateSysDftServiceCnfData() Failed for service=%s l_udi_ref=%d,l_ret_val=%d",
										l_srvc_token, l_udi_ref,l_ret_val);	

							 l_srvc_loop_break = true;
							 break;			
						}
				}
				break;
	  		default:
				{
	      		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			    		"RtShUserDataIntf::rtHandleSystmSrvcCfgProvMsg(),ERROR Invalid ar_prov_action=%d for ap_srvc_string=%s",ar_prov_action,ap_srvc_string);
            l_srvc_loop_break = true;
				}
				break;
	 }//end of switch	 
	 
	 if(l_srvc_loop_break)
	 {
	    l_ret_val = RT_FAILURE;
		  break;
	 }
	 else
	    l_srvc_token = strtok_r(NULL, l_str,&lp_strtok_ptr);
			
	}//end of while of token.
	delete[] lp_orig_srvc_str;  //Klock_work_fix@25-08-2016 
  if(l_regen_req)
	{
		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
			 "RtShUserDataIntf::rtHandleSystmSrvcCfgProvMsg(),Going to RegenOrderListFrCntx as SYS SRVC PROV SUCCS for ar_prov_action=%d,ap_srvc_string=%s",
						 ar_prov_action,ap_srvc_string);	

		if(m_immediate_regen)
		{
			mp_user_cntxt_mgr->rtRegenOrderedListForCntxt();
		}
		else
		{
			mp_user_cntxt_mgr->rtMarkCntxtsForOrdListUpd();

		}
		//TBD is it req to call ckptDataRefConfigData
		//mp_data_pool_mgr->rtUpdateCkptDataRefConfigData(l_udi_ref, false);
	}
	if(l_ret_val == RT_SUCCESS)
	   mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetSucc);
	else
	   mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetFail);

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf::rtHandleSystmSrvcCfgProvMsg(), Leave with l_ret_val=%d ap_srvc_string=%s and ar_prov_action=%d",l_ret_val,ap_srvc_string,ar_prov_action);
	
	return l_ret_val;  
	 
}
/*****************************************************************************************
*
* FUNCTION NAME : rtHandleSystmSrvcCfgProvMsg()
*
* DESCRIPTION   : This API will Sync its service cfg data with db on receiving prov msg
*						     
*								 
* INPUT         : service_names seperated by ','   and Prov_action
*
* OUTPUT        : service configuration data updation using db
*
* RETURN        : success/failure
*
****************************************************************************************/		
RtRcT RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(RtS8T* ap_srvc_string,RtSrvcProvActionTypeT a_prov_action)
{
	 
	 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqCalled);
	 if(ap_srvc_string == NULL || strlen(ap_srvc_string) < 1)
	 {
	   mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			  "RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(),ENTER/LEAVE Invalid input ap_srvc_string=%s with ar_prov_action=%d",ap_srvc_string==NULL?"NULL":ap_srvc_string,a_prov_action);
	   
		 mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetFail);
	   return RT_FAILURE;
	 }
	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(), ENTER ap_srvc_string=%s and a_prov_action=%d",ap_srvc_string,a_prov_action);
	
	 RtBoolT l_regen_req       = false;
	 RtBoolT l_srvc_loop_break = false;
	
	 RtRcT 		l_ret_val  			     = RT_FAILURE;	

	 RtS8T* lp_orig_srvc_str = new RtS8T[strlen(ap_srvc_string)+1];
	 strcpy(lp_orig_srvc_str,ap_srvc_string);
	 
	 /*-------------------------------------------------------------------------------------------------------
	 - +++ Update System service data based on  service_prov_action +++
	 -  RT_MMTEL_CREATE_SERVICE_DATA is not valid for sys_def_srvc
	 -  RT_MMTEL_RETREIVE_SERVICE_DATA is not valid at this level
	 -  RT_MMTEL_UPDATE_SERVICE_DATA
	 -   -For each service_name received in amf_data(use strtok service_name) on  get its RtShSrvcIndTokenData value
	 -   -RtShSrvcIndTokenData.srvc_token is the data_ref index
	 -   -RtShSrvcIndTokenData.app_info.app_enum is the service_id for the db query
	 -------------------------------------------------------------------------------------------------------*/
	RtShSrvcIndTokenData  							l_srvc_token_data;
	RtSrvcIndVsSrvcTokenDataMapItr 			l_itr;
	RtU32T															l_mand_pool_elem  = 0;
  RtShUDIDataRefVal									  l_udi_ref 		    = 0;
  RtS8T 					l_str[2] = ",";
  
	RtS8T*          lp_strtok_ptr;
	RtS8T* l_srvc_token = strtok_r(lp_orig_srvc_str, l_str,&lp_strtok_ptr);
	while(l_srvc_token != NULL)
	{
		 l_itr = m_srvc_ind_vs_srvc_token_map.find(l_srvc_token);
		 if( l_itr!= m_srvc_ind_vs_srvc_token_map.end() )
		 {
			 l_srvc_token_data = l_itr->second;
			 l_udi_ref = l_srvc_token_data.srvc_token;
			 
			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			   "RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(),Going for l_srvc_tken=%s with l_udi_ref=%d for a_prov_action=%d",l_srvc_token,l_udi_ref,a_prov_action);	

		 }
		 else
		 {
	     mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			   "RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(),ERROR m_srvc_ind_vs_srvc_token_map.find()-FAILED for service_name=%s a_prov_action=%d",l_srvc_token,a_prov_action);
			 l_ret_val = RT_FAILURE;	
			 break;
		 }
		 switch(a_prov_action)
		 {
	  		case RT_UDI_PROV_ACTN_UPDATE:
				{
						RtUdiDbReqInputT l_db_in;
	  				l_db_in.rg_data_ptr_list.clear();

						RtUdiDbReqOutputT l_db_out;
						//memset(&l_db_out,0,sizeof(l_db_out));

						l_db_in.srvc_type = RT_UDI_DB_DEF_SRVC_DATA_REQ;
            strcpy(l_db_in.data.srvc_cnf_info.service_name,l_srvc_token);
						
						//RtShUDIRegData l_reg_data;
						//memset(&l_reg_data,0,sizeof(l_reg_data));
						
						//l_db_out.data.mp_reg_data = &l_reg_data;
						l_db_out.data.def_srvc_data.data_len = 0;
						
						l_ret_val = mp_udi_db_hdlr->rtFetchServiceData(l_db_in,l_db_out);
						if(l_ret_val != RT_SUCCESS)
						{
							 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
									"RtShUserDataIntf ::rtHandleSystmSrvcDataProvMsg() :: rtFetchServiceData() Failed - with srvc_type=%d for l_srvc_token=%s So Leaving ",l_db_in.srvc_type,l_srvc_token);
							 /*------------------------------------------------------------------------------
							 -
							 -  TBD :: Verify all possible return values before considering it as a failure
							 -  
							 ------------------------------------------------------------------------------*/
							 l_srvc_loop_break = true;
							 break;			
						}
						if(l_db_out.db_ext_result != RT_UDI_DB_REQ_RESULT_SUCCESS)
						{
							 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			  					"RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(), ERROR rtFetchServiceData() data_not_Found OR processing_failed  as db_ext_result=%d for service_name=%s ",l_db_out.db_ext_result,l_db_in.data.srvc_cnf_info.service_name);

							    /*-------------------------------
											 - 
											 ---------------------------------*/
								l_srvc_loop_break = true;
                break;
						}
						 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			    		  	"RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(),db get returnd data_len=%d service_name=%s l_udi_ref=%d",l_db_out.data.def_srvc_data.data_len,l_srvc_token,l_udi_ref);

						 l_ret_val = mp_data_pool_mgr->rtUpdateSrvcData(l_udi_ref,l_db_out.data.def_srvc_data.vp_srvc_data,l_db_out.data.def_srvc_data.data_len);
						 if(RT_SUCCESS == l_ret_val)
						 {
						  	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			    		  	"RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(),RT_UDI_PROV_ACTN_UPDATE SUCCESS for service_name=%s l_udi_ref=%d data_len=%d",l_srvc_token,l_udi_ref,l_db_out.data.def_srvc_data.data_len);

								l_regen_req = true;
						 }
						 else
						 {
								 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
									 "RtShUserDataIntf ::rtHandleSystmSrvcDataProvMsg(),ERROR rtUpdateSrvcData() Failed for service=%s l_udi_ref=%d,l_ret_val=%d",
										 l_srvc_token, l_udi_ref,l_ret_val);	

								l_srvc_loop_break = true;
								break;			
						 }
				}
				break;
	  		default:
				{
	      		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			    		"RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(),ERROR Invalid a_prov_action=%d for ap_srvc_string=%s",a_prov_action,ap_srvc_string);
            l_srvc_loop_break = true;
				}
				break;
	 }//end of switch
	
	 if(l_srvc_loop_break)
	 {
	    l_ret_val = RT_FAILURE;
		  break;
	 }
	 else
		 l_srvc_token = strtok_r(NULL, l_str,&lp_strtok_ptr);
		 
	}//end of while of token. 
	delete[] lp_orig_srvc_str;  //Klock_work_fix@25-08-2016
  if(l_regen_req)
	{
		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
			 "RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(),Going to RegenOrderListFrCntx as SYS SRVC PROV SUCCS for ar_prov_action=%d,ap_srvc_string=%s",
						 a_prov_action,ap_srvc_string);	

		if(m_immediate_regen)
		{
			mp_user_cntxt_mgr->rtRegenOrderedListForCntxt();
		}
		else
		{
			mp_user_cntxt_mgr->rtMarkCntxtsForOrdListUpd();

		}
		//mp_data_pool_mgr->rtUpdateCkptDataRefConfigData(l_udi_ref, false);
	}
	if(l_ret_val == RT_SUCCESS)
	   mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetSucc);
	else
	   mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udiProcessPrvReqRetFail);
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
			"RtShUserDataIntf::rtHandleSystmSrvcDataProvMsg(), Leave with l_ret_val=%d for ap_srvc_string=%s and a_prov_action=%d",l_ret_val,ap_srvc_string,a_prov_action);
	
	return l_ret_val;
	 
}

#endif


