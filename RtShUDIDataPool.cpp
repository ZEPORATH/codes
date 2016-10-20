#ifdef __RT_DIA_ENABLE_SH__

/******************************************************
Copyright (c) 2007
Rancore Technologies (P) Ltd.
All Rights Reserved
*******************************************************/

/*************************************************************************************************************************************
* FILE NAME 	:    	RtShUDIDataPoolMgr.cpp
*
* DESCRIPTION : 		Member Functions for RtShUDIDataPoolMgr Class are defined here.
*
* DOCUMENTS 	:			A reference to the applicable design documents and coding guidelines document used.
*
* AUTHOR 			: 		
*
* DATE 				: 		
*
***************************************************************************************************************************************/


#include "RtShUDIDataPoolMgrInclude.hpp"
//UDI_PHASE2 c style function defined in RtEncDecMgr class
extern RtRcT rtEncodeServiceData(RtS8T*		ap_servc_xml_data,	//input
													RtS8T*		ap_servc_name,			//input
													RtBoolT 	a_to_encode,				//input
													RtU32T& 	ar_servc_index,			//output
													RtU32T&  	ar_data_len, 				//output
													void**  	app_servc_data); 		//output
//c-style function to be defined by application to support selective update on service data.
extern RtRcT rtAppSrvcExtnCap(RtU32T ar_app_enum,	void*	ap_input_data,RtU32T ar_buffer_old_size,	void** app_updated_data,RtU32T& ar_buffer_new_size,RtBoolT& ar_buffer_exchange);

RtShUDIDataPoolMgr*  	RtShUDIDataPoolMgr ::  mp_self_ref = NULL;

/*******************************************************************************
 *
 * FUNCTION NAME : rtEdsvUpdateCkptCb()
 *
 * DESCRIPTION   : Callback :  Handles CheckPoint Configurations Update events in standby 
 *
 * INPUT         : None. 
 *
 * OUTPUT        : None 
 *
 * RETURN        : void	
 ******************************************************************************/
void rtEdsvUpdateCkptCb(RtS8T*					ap_pub_name,
												RtEvtAttrData*	ap_evt_attr_data,
												RtS32T					a_evt_data_len,
												RtU8T*					ap_evt_data)
{		
	RtUdiCkptUpdData* lp_evt_data = (RtUdiCkptUpdData*)ap_evt_data;
	
	
	(RtSysAgent::rtGetInstance())->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"rtEdsvUpdateCkptCb(), Entered with ap_pub_name=%s, AppEvtId=%d, Section Id=%d", ap_pub_name, lp_evt_data->app_evt_id,lp_evt_data->section_id);
	
	
	RtProcCurrentState l_proc_cur_state;
	memset(&l_proc_cur_state,0,sizeof(RtProcCurrentState));
	l_proc_cur_state	= gp_mgl_intf->rtGetProcCurrentState();
	
	if( RT_HA_ACTIVE == l_proc_cur_state.procHAState)
	{
		(RtSysAgent::rtGetInstance())->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
		"rtEdsvUpdateCkptCb()-LEAVE-Section Id=%d-RT_HA_ACTIVE",lp_evt_data->section_id);
	
	}
	else
	{
		RtRcT l_ret_val;	

		if(0	==	lp_evt_data->section_id)
		{
			//means mandatory service enablement-disablement event 
			l_ret_val = ((RtShUDIDataPoolMgr::rtGetInstance())->rtEnableDftService(lp_evt_data->udi_ref, lp_evt_data->to_enable,false));
		}
		else
		{
			l_ret_val = ((RtShUDIDataPoolMgr::rtGetInstance())->rtUpdateDftSrvDataFromCkpt(lp_evt_data->section_id, lp_evt_data->udi_ref,false));
		}
	}
	
	(RtSysAgent::rtGetInstance())->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"rtEdsvUpdateCkptCb()-LEAVE-Section Id=%d",lp_evt_data->section_id);
			
}

/*******************************************************************************
 *
 * FUNCTION NAME : RtShUDIDataPoolMgr().
 *
 * DESCRIPTION   : This is the constructor of the class. .rtInitialize() function is called inside
 * 								the constructor to initialize all the data that is needed during system startup
 *
 * INPUT         : none. 
 *
 * OUTPUT        : none 
 *
 * RETURN        : none. 
 *
 ******************************************************************************/
RtShUDIDataPoolMgr :: RtShUDIDataPoolMgr(RtU8T a_max_data_ref,RtU8T a_max_data_repository,RtS8T* a_comp_name)
{

	printf("\nRtShUDIDataPoolMgr::RtShUDIDataPoolMgr() ENTER a_max_data_ref=%d,a_max_data_repository=%d",a_max_data_ref,a_max_data_repository);
	fflush(NULL);
	
	m_max_data_ref  				= a_max_data_ref;
	m_max_data_repository 	= a_max_data_repository;
	
	memset(m_comp_name,0,sizeof(m_comp_name));
	strcpy(m_comp_name,a_comp_name);
	
	if(RT_SUCCESS != rtInitialize() )
  {
    printf("\nERROR::RtShUDIDataPoolMgr::RtShUDIDataPoolMgr()-rtInitilaize returned FAILURE (EXCEPTION THROW)" );
 		fflush(NULL);
		
    throw RT_SH_UDI_EXCEP_INITIALIZE_FAILED;
  } 
	
	printf("\nSUCCESS::RtShUDIDataPoolMgr Successfully created");                                                                      
  fflush(NULL);

}

/*******************************************************************************
 *
 * FUNCTION NAME : ~RtShUDIDataPoolMgr().
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
RtShUDIDataPoolMgr :: ~RtShUDIDataPoolMgr()
{
}
 
/*******************************************************************************
 *
 * FUNCTION NAME : rtCreate
 *
 * DESCRIPTION   : This function creates the singleton instance of this class
 *
 * INPUT         : none. 
 *
 * OUTPUT        : none 
 *
 * RETURN        : static *RtShUDIDataPoolMgr. 
 *
 ******************************************************************************/
RtShUDIDataPoolMgr* RtShUDIDataPoolMgr :: rtCreate(RtU8T a_max_data_ref,RtU8T a_max_data_repository,RtS8T* a_comp_name)
{
	try
  {
    if(mp_self_ref == NULL)
    {
      return(mp_self_ref = new RtShUDIDataPoolMgr(a_max_data_ref,a_max_data_repository,a_comp_name));

    }
    else
    {
      return NULL;
    }
  }
  catch(...)
  {
    printf("\nERROR::RtShUDIDataPoolMgr::rtCreate - RtShUDIDataPoolMgr could not be created due to Memory problem or not able to read from files %s,%d\n",__FILE__,__LINE__);                                                                      
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
 * RETURN        : static *RtShUDIDataPoolMgr. 
 *
 ******************************************************************************/
RtShUDIDataPoolMgr* RtShUDIDataPoolMgr :: rtGetInstance()
{
	return mp_self_ref;
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtInitialze()
 *
 * DESCRIPTION   : 
 *								 
 *
 * INPUT         : none. 
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT. 
 *
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr :: rtInitialize()
{
  printf("\nRtShUDIDataPoolMgr::rtInitialize() ENTER - ");                                                                      
  fflush(NULL);

	mp_sys_agent	= RtSysAgent::rtGetInstance();
	if(NULL ==	mp_sys_agent)
	{
		printf("RtShUDIDataPoolMgr :: rtInitialize()  RtSysAgent::rtGetInstance() returned NULL");
		fflush(NULL);
		
		return RT_FAILURE;
	}

	m_enable_disable_in_progress	= false;

	RtRcT l_ret_val = RT_SUCCESS;
	 	
// 	RtCkptLogicalNameT	l_ckpt_logical_name = RT_CKPT_LN_SH_UDI_INFO;
// 	
// 	RtRcT l_ret_val = gp_mgl_intf->getCkptIdForName(l_ckpt_logical_name,m_ckpt_id);
// 	if(l_ret_val != RT_SUCCESS)
// 	{
// 	  printf("\n RtShUDIDataPoolMgr::rtInitialize(), gp_mgl_intf->getCkptIdForName() RT_CKPT_LN_SH_UDI_INFO=%d returned(File=%s,Line=%d,l_ret_val=%d)",RT_CKPT_LN_SH_UDI_INFO,__FILE__,__LINE__,l_ret_val);
// 		fflush(NULL);
// 		return RT_FAILURE;
// 	}
// 	
// 	printf("\nm_ckpt_id for RT_CKPT_LN_SH_UDI_INFO =%d",m_ckpt_id);fflush(NULL);
	
	l_ret_val = gp_mgl_intf->rtRegSubTypeCallBack( APP_SUB_TYPE_UDI_CKPT_UPD_EVENTS, 
																								 rtEdsvUpdateCkptCb);
	if(l_ret_val != RT_SUCCESS)
	{
		printf("\n RtShUDIDataPoolMgr::rtInitialize() , mp_mgl_intf->rtRegSubTypeCallBack()!=RT_SUCCESS for APP_SUB_TYPE_UDI_CKPT_UPD_EVENTS Exiting !!!\n");
		fflush(NULL);
		return RT_FAILURE;
	}

	
	
	//PLus 1 required
	m_max_data_indx 					= m_max_data_ref + m_max_data_repository + 1;

	mp_reg_data     					= new (nothrow)RtShUDIRegData[m_max_data_indx];

	if(NULL !=	mp_reg_data)
	{
		mp_valid_reg_data 			= new  (nothrow)RtBoolT[m_max_data_indx];
	}
	else
	{
		printf("RtShUDIDataPoolMgr :: rtInitialize()  MEMORY not allocated to mp_reg_data for m_max_data_indx=%d",m_max_data_indx);
		fflush(NULL);		
		return RT_FAILURE;
	}
	
	if(NULL !=	mp_valid_reg_data)
	{
		mp_data_pool_container		= new  (nothrow)RtShUDIDataPoolContainer*[m_max_data_indx];
		
		if(NULL ==	mp_data_pool_container)
		{
			printf("RtShUDIDataPoolMgr :: rtInitialize()  MEMORY not allocated to mp_data_pool_container for m_max_data_indx=%d",m_max_data_indx);
			fflush(NULL);		
			return RT_FAILURE;
			
		}
		
	}
	else
	{
		printf("RtShUDIDataPoolMgr :: rtInitialize()  MEMORY not allocated to mp_valid_reg_data for m_max_data_indx=%d",m_max_data_indx);
		fflush(NULL);		
		return RT_FAILURE;
		
	}
	
	/*- run loop for m_max_data_indx
		- bzero each mp_reg_data element
		- bzero each mp_valid_reg_data element
		- mp_data_pool_container element = NULL*/

	for(RtU32T l_cnt =0; l_cnt < m_max_data_indx; l_cnt++)
	{			
		mp_data_pool_container[l_cnt] = NULL;	
		memset(&mp_reg_data[l_cnt], 0 ,sizeof(RtShUDIRegData));//SHASHI_VALGRIND FIX
		mp_valid_reg_data[l_cnt]	= false;		
	} 	

	//- clear the m_data_ref_list

	m_reg_data_ref_list.clear();
	m_reg_srvc_indic_list.clear(); 

	//HP_CR(DONE): clear() m_case_ord_list[RT_SH_MAX_SESS_CASE][RT_SIP_METHOD_FOR_CTF];
	
	for(RtU32T l_cnt1 = 0; l_cnt1 < RT_SH_MAX_SESS_CASE ; l_cnt1++)
	{
		for(RtU32T l_cnt2 = 0; l_cnt2 < RT_SIP_METHOD_FOR_CTF ; l_cnt2++)
		{
			m_case_ord_list[l_cnt1][l_cnt2].clear();
		}
	}

//	- insert in m_data_ref_list value ZERO (of data repository)
//SHASHI_19042012: we cannot pushback 0 dataref blindly.(should be pushed only if at least one service is registered)
  //m_reg_data_ref_list.push_back(0);
	 
	
	mp_user_data_intf = RtShUserDataIntf::rtGetInstance();
	if(NULL ==	mp_user_data_intf)
	{
		printf("RtShUDIDataPoolMgr :: rtInitialize()  RtShUserDataIntf::rtGetInstance() returned NULL");
		fflush(NULL);
		
		//HP_CR(DONE):: give critical logs
	  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtInitialise():RtShUserDataIntf::rtGetInstance() FAILED");  		
		
		return RT_FAILURE;
	}
	//HP_CR(DONE):: give debug logs , prints
	
	m_sip_method_arr[RT_SIP_METHOD_INVITE]  		= "RT_SIP_METHOD_INVITE";
	m_sip_method_arr[RT_SIP_METHOD_REINVITE]  	= "RT_SIP_METHOD_REINVITE";
	m_sip_method_arr[RT_SIP_METHOD_UPDATE]  		= "RT_SIP_METHOD_UPDATE";
	m_sip_method_arr[RT_SIP_METHOD_REFER] 			= "RT_SIP_METHOD_REFER";
	m_sip_method_arr[RT_SIP_METHOD_MESSAGE] 		= "RT_SIP_METHOD_MESSAGE";
	m_sip_method_arr[RT_SIP_METHOD_PUBLISH] 		= "RT_SIP_METHOD_PUBLISH";
	m_sip_method_arr[RT_SIP_METHOD_SUBSCRIBE] 	= "RT_SIP_METHOD_SUBSCRIBE";
	m_sip_method_arr[RT_SIP_METHOD_NOTIFY]  		= "RT_SIP_METHOD_NOTIFY";
	m_sip_method_arr[RT_SIP_METHOD_REGISTER]  	= "RT_SIP_METHOD_REGISTER";
	m_sip_method_arr[RT_SIP_METHOD_OPTIONS] 		= "RT_SIP_METHOD_OPTIONS";
	m_sip_method_arr[RT_SIP_METHOD_INFO]  			= "RT_SIP_METHOD_INFO";
	m_sip_method_arr[RT_SIP_METHOD_CANCEL]  		= "RT_SIP_METHOD_CANCEL";
	m_sip_method_arr[RT_SIP_METHOD_BYE] 				= "RT_SIP_METHOD_BYE";
	
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtInitialise():SUCCESS with m_max_data_indx=%d",m_max_data_indx);  

	return RT_SUCCESS;

}


/*******************************************************************************
 *
 * FUNCTION NAME : rtSetRegData()
 *
 * DESCRIPTION   : This function is called by RtShUserDataIntf class to set registration
 *                 data for data reference
 *								 
 *
 * INPUT         : RtShUDIDataPoolIndx ,RtShUDIRegData* 
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT. 
 *
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr :: rtSetRegData(RtShUDIDataPoolIndx a_pool_indx,RtShUDIRegData* ap_reg_data)
{
	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtSetRegData(), ENTER a_pool_indx=%u for app_srvc_indic =%s", a_pool_indx,ap_reg_data->data_ref_appl_info.app_srvc_indic);  

	 RtRcT l_ret_val = RT_SUCCESS;

	 if(a_pool_indx < m_max_data_indx)
	 {

		 if( (a_pool_indx <= m_max_data_ref) && (mp_valid_reg_data[a_pool_indx] != true) )
		 {//add only data reference (index being data reference)
			 // - insert in m_data_ref_list
			 m_reg_data_ref_list.push_back(a_pool_indx);

			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_INFO,__FILE__,__LINE__,
	  	 "RtShUDIDataPoolMgr::rtSetRegData()  Push Reg data in reg_data_ref_list for Data Ref [%u]" , a_pool_indx);

		 }
		 else if(mp_valid_reg_data[a_pool_indx] != true)
		 {
			 //- insert service indication in m_reg_srvc_indic_list
			 m_reg_srvc_indic_list.push_back(ap_reg_data->data_ref_appl_info.app_srvc_indic);

			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_INFO,__FILE__,__LINE__,
	  	 "RtShUDIDataPoolMgr::rtSetRegData()  Push Reg data in reg_srvc_indic_list for Srvc Indication [%u,%s]" , a_pool_indx,ap_reg_data->data_ref_appl_info.app_srvc_indic);

		 }

		 //repository data
		 mp_reg_data[a_pool_indx] = *ap_reg_data;

		 mp_valid_reg_data[a_pool_indx] = true;

		 mp_reg_data[a_pool_indx].udi_ref = a_pool_indx; 
		 
		 mp_data_pool_container[a_pool_indx]	= new RtShUDIDataPoolContainer(ap_reg_data->max_num_data,ap_reg_data->max_data_size,a_pool_indx);
		 
		 if( NULL == mp_data_pool_container[a_pool_indx])
		 {
			 l_ret_val = RT_SH_UDI_ERR_CREATING_DATA_POOL_CONTAINER;

			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  	 "RtShUDIDataPoolMgr::rtSetRegData():ERROR  Pool Container not Initialized for index =[%u]" , a_pool_indx);
		 }
		 else
		 {
		 
		  //UDI_PHASE2 : Reading default data for mandatory services
			//if service is mandatory craete pool element as it is common to all users
			//HP_CR(DONE): use %c or ?? for l_udi_ref(for numeric values in RtU8T we have to use %u or %d)
			if((mp_reg_data[a_pool_indx].flag & RT_SH_UDI_REG_DATA_SRVC_MANDATORY) && !(mp_reg_data[a_pool_indx].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT)) 
			{
				//Shashi UDI_Phase2
				l_ret_val = rtStoreDataRepositoryData(mp_reg_data[a_pool_indx].udi_ref, 
																							mp_reg_data[a_pool_indx].dflt_srvc_data,
																							mp_reg_data[a_pool_indx].max_data_size,
																							mp_reg_data[a_pool_indx].elem_indx,
																							&mp_reg_data[a_pool_indx].p_data_ptr,
																							false);
				if(RT_SUCCESS != l_ret_val)
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
					 "rtSetRegData() ERROR ::rtStoreDataRepositoryData() for Mandatory service FAILED, udi_ref = %d ",
					 mp_reg_data[a_pool_indx].udi_ref);
					
				}
				else
				{

					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_INFO,__FILE__,__LINE__,
					 "rtSetRegData()SUCCESSFULLY stored for Mandatory service data, udi_ref = %d,elem_indx=%u, p_data_ptr=%p",
					 mp_reg_data[a_pool_indx].udi_ref,mp_reg_data[a_pool_indx].elem_indx,mp_reg_data[a_pool_indx].p_data_ptr);

				}

 			}
		 }

	 }
	 else
	 {
		 l_ret_val = RT_SH_UDI_ERR_INVALID_DATA_REF;

		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
		 "RtShUDIDataPoolMgr::rtSetRegData()  Called with invalid Pool Index [%u]", a_pool_indx);

	 }

		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
		"EXIT : RtShUDIDataPoolMgr::rtSetRegData()-Set Reg Data at index [%u] [RETVAL=%d]",
		a_pool_indx,l_ret_val);

		return l_ret_val;
	
}


/*******************************************************************************
 *
 * FUNCTION NAME : rtEndOfSetRegData()
 *
 * DESCRIPTION   : This is called by RtShUserDataIntf class to indicate end of
 *                 data reference registration
 *								 
 *
 * INPUT         :None
 *
 * OUTPUT        : none 
 *
 * RETURN        : RtRcT. 
 *
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr :: rtEndOfSetRegData()
{
	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtEndOfSetRegData() ENTER");  	


	//SHASHI_19042012: PUSH dataref 0 if m_reg_srvc_indic_list is not empty
	 if(!m_reg_srvc_indic_list.empty())
	 {
	 		m_reg_data_ref_list.push_back(0);
	 }

	 RtRcT l_rval = rtCreateCaseOrderedListUsingConfig();

	 if(RT_SUCCESS != l_rval)
	 {
		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
		 "RtShUDIDataPoolMgr::rtEndOfSetRegData():: rtCreateCaseOrderedListUsingConfig  FAILED");  	
	 }
	 else
	 {
		RtProcCurrentState l_proc_cur_state;
		memset(&l_proc_cur_state,0,sizeof(RtProcCurrentState));
		l_proc_cur_state	= gp_mgl_intf->rtGetProcCurrentState();

		if(l_proc_cur_state.procHAState == RT_HA_ACTIVE)
		{
	  	/*if(
					(  (l_proc_cur_state.procStartUpType == RT_PROC_FIRST_START) 
								&& 
						 (l_proc_cur_state.procCsiAssignment == RT_PROC_ACTIVE_NEW_ASSIGN)
					) 
												||
					(
						(
					  	(l_proc_cur_state.procStartUpType == RT_PROC_AUTO_RESTART)
																	||
							(l_proc_cur_state.procStartUpType == RT_PROC_ADMIN_START)
						) 
								&& 
				 		l_proc_cur_state.is_node_reboot 
								&& 
						(l_proc_cur_state.procCsiAssignment == RT_PROC_ACTIVE_NEW_ASSIGN)
					)
				)
				{*/
				//12-04-2013: below code has been comnneted as there is no need for active UDI to copy file towards standby
				//					 This also results in delay when active AS process come up
				// 	l_rval = rtCreateSections();
// 					if(l_rval != RT_SUCCESS)
// 					{
// 	  				printf("\n RtShUDIDataPoolMgr::rtEndOfSetRegData(), rtCreateSections() returned(File=%s,Line=%d,l_ret_val=%d)",__FILE__,__LINE__,l_rval);
// 						fflush(NULL);
// 
// 						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
// 		  			"RtShUDIDataPoolMgr::rtEndOfSetRegData(), rtCreateSections() returned %d",l_rval); 
// 
// 						l_rval = RT_FAILURE;
// 					}
				//}



			}
			else if(l_proc_cur_state.procHAState == RT_HA_STANDBY)
			{
		  	//sync
				if(RT_SUCCESS != rtSyncStandbyUsingCkpt())
				{
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
		  			"RtShUDIDataPoolMgr::rtEndOfSetRegData(), rtSyncStandbyUsingCkpt() returned failure"); 

						l_rval = RT_FAILURE;				
				}
			}

		}
	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtEndOfSetRegData() EXIT with rval	= %d",l_rval);  	

	 return l_rval;

}

/*******************************************************************************
*
* FUNCTION NAME : rtCreateSections()
*
* DESCRIPTION   : This function is used to create sections for SH config info
* 							  Called only in ACTIVE AS
*                 
*
* ARGUMENTS     : None
*                 
*
* RETURN        : RtRcT
*                     
*******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtCreateSections()
{
	
	RtRcT l_ret_val = RT_SUCCESS;
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
	"rtCreateSections():- ENTER ");

  if ( NULL == getenv("COMMON_CONFIG_PATH") )//KLOCWORK_FIX RM 03092013
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
		"rtCreateSections() COMMON_CONFIG_PATH not set.");							
		return RT_FAILURE;
  }
	
//	RtS8T   			l_key[RT_CKPT_SECT_KEY_LEN] ;
	std::ifstream l_ifstream;

//	RtU32T  			l_file_length;
	string l_dft_data_file_path = getenv("COMMON_CONFIG_PATH");
	l_dft_data_file_path 	+= "/sh/";
	

// 	RtCkptShUDIData 	l_ckpt_data;
// 	l_ckpt_data.is_ckpt_data_valid		= true;
	
	RtU32T	l_num_mand_srv = 0;		
	for (RtU32T l_cnt=(m_max_data_ref+1);l_cnt	< m_max_data_indx;l_cnt++)
	{
	
		if( (mp_reg_data[l_cnt].flag &	RT_SH_UDI_REG_DATA_SRVC_MANDATORY)	&& !(mp_reg_data[l_cnt].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT))
		{
			++l_num_mand_srv;
			//////////////////
			//Construct Key//
			//////////////////
// 			bzero(l_key,sizeof(l_key)); 
// 			sprintf(l_key,"%d",mp_reg_data[l_cnt].data_ref_appl_info.app_enum);

			//////////////////
			//Construct Data//
			//////////////////
			//l_file_length = 0;
	
			l_dft_data_file_path = getenv("COMMON_CONFIG_PATH");
			l_dft_data_file_path 	+= "/sh/";
			l_dft_data_file_path 	+= mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic;
			l_dft_data_file_path 	+= ".xml";
			
			l_ifstream.open(l_dft_data_file_path.c_str());      // open input file
			
			if(l_ifstream.fail())
			{
				 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
				 "rtCreateSections() ERROR opening file = %s  ",l_dft_data_file_path.c_str());							
				 
				 l_ret_val = RT_FAILURE;
				 
				 break;
				
			}
//Tag-Suman-01Apr2013,Opensaf Ckpt Srvice Removal from App
			
// 			l_ifstream.seekg(0, std::ios::end);      //go to the end
// 			l_file_length = l_ifstream.tellg();      // report location (this is the length)
// 			l_ifstream.seekg(0, std::ios::beg);    					 // go back to the beginning
// 			if(l_file_length	<	RT_SH_UDI_MAX_CKPT_DATA_LEN)
// 			{
// 				bzero(&l_ckpt_data.data,sizeof(l_ckpt_data.data));
// 				
// 				l_ifstream.read(l_ckpt_data.data, l_file_length);      
// 			}
// 			else
// 			{
// 				 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
// 				 "rtCreateSections() service data file size too large.File=%s,l_file_length=%u, max_size =%u",l_dft_data_file_path.c_str(),l_file_length, RT_SH_UDI_MAX_CKPT_DATA_LEN);							
// 				 
// 				 l_ifstream.close();
// 				 
// 				 l_ret_val = RT_FAILURE;
// 				 
// 				 break;
// 				 			
// 			}
			
			
		///////////////////////
		//Close the opened file
		//////////////////////
			l_ifstream.close();  
			
		//////////////////////
		//Scp to Remote Server
		//////////////////////
			
			l_ret_val 	= 	rtScpToRemoteController((RtS8T*)l_dft_data_file_path.c_str());
			if(l_ret_val != RT_SUCCESS)
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
				"rtCreateSections():- rtScpToRemoteController Failed for Configuration[Path=%s] [RETVAL=%d]",l_dft_data_file_path.c_str(),l_ret_val);

 				l_ret_val = RT_FAILURE;
			 
			  break;
			}
			else
			{
			
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
				"rtCreateSections():- rtScpToRemoteController SUCCESS for Configuration[Path=%s] [RETVAL=%d]",l_dft_data_file_path.c_str(),l_ret_val);

			}
			
//Tag-Suman-01Apr2013,Opensaf Ckpt Srvice Removal from App_END			
// 
// 			l_ret_val = gp_mgl_intf->rtInsertCkptData(	m_ckpt_id, 
// 																									(void*)&l_ckpt_data,
// 																									sizeof(l_ckpt_data),
// 																									l_key);
// 
// 			if(l_ret_val == RT_RET_CPSV_EXISTS)
// 			{
// 				
// 
// 				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
// 				"rtCreateSections():- gp_mgl_intf->rtInsertCkptData() returned RT_RET_CPSV_EXISTS for CKPT_ID=%d,SEC_KEY=%s",m_ckpt_id,l_key);
// 				
// 				l_ret_val = RT_SUCCESS;
// 			}
// 			else if(l_ret_val != RT_SUCCESS)
// 			{
// 				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
// 				"rtCreateSections():- mp_mgl_intf->rtInsertCkptData() Failed for CKPT_ID=%d,SEC_KEY=%s [RETVAL=%d]",m_ckpt_id,l_key,l_ret_val);
// 
// 				l_ret_val = RT_FAILURE;
// 				 
// 			  break;
// 			}
// 			else
// 			{
// 				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
// 				"rtCreateSections():- mp_mgl_intf->rtInsertCkptData() SUCCESS for File=%s,l_file_length=%u, max_size =%u,CKPT_ID=%d,SEC_KEY=%s [RETVAL=%d]",l_dft_data_file_path.c_str(),l_file_length, RT_SH_UDI_MAX_CKPT_DATA_LEN,m_ckpt_id,l_key,l_ret_val);
// 			}
			
		}

	}
	
	// Following is Insertion of mp_reg_data for RtShDatarefConfig.xml at index zero
	
	// defensive check on size of l_ckpt_data.data against no of mandatory services *sizeof(RtShUDIRegData)

	//Need to be discussed,as per my understanding ,here we can just simply copy RtShDatarefConfig.xml to standby.
	
	if((RT_SUCCESS ==	l_ret_val) &&(RT_SH_UDI_MAX_CKPT_DATA_LEN > l_num_mand_srv*sizeof(RtShUDIRegData)))
	{

//Tag-Suman-01Apr2013,Opensaf Ckpt Srvice Removal from App

		string l_config_path 	= getenv("COMMON_CONFIG_PATH");
		l_config_path 	+= "sh/RtShDataRefConfig.xml"; 

		l_ifstream.open(l_config_path.c_str());      // open input file
			 
		if(l_ifstream.fail())
		{
			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			 "rtCreateSections() ERROR opening file = %s.Exit RT_FAILURE  ",l_config_path.c_str());							

			 return RT_FAILURE;

			 

		}
		///////////////////////
		//Close the opened file
		//////////////////////
		l_ifstream.close();  
			
		//////////////////////
		//Scp to Remote Server
		//////////////////////
			
		l_ret_val 	= 	rtScpToRemoteController((RtS8T*)l_config_path.c_str());
		if(l_ret_val != RT_SUCCESS)
		{
			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
			 "rtCreateSections():- rtScpToRemoteController Failed for Configuration[Path=%s] [RETVAL=%d]",l_config_path.c_str(),l_ret_val);

 			 l_ret_val = RT_FAILURE;

			 
		 }
		 else
		 {

			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
			 "rtCreateSections():- rtScpToRemoteController SUCCESS for Configuration[Path=%s] [RETVAL=%d]",l_config_path.c_str(),l_ret_val);

		 }

//Tag-Suman-01Apr2013,Opensaf Ckpt Srvice Removal from App_END
	
// 		//////////////////
// 		//Construct Key//
// 		//////////////////
// 		bzero(l_key,sizeof(l_key)); 
// 		sprintf(l_key,"%d",0);
// 
// 		//////////////////
// 		//Construct Data//
// 		//////////////////
// 		memset(&l_ckpt_data,0,sizeof(l_ckpt_data));
// 		l_ckpt_data.is_ckpt_data_valid		= true;
// 		memcpy(l_ckpt_data.data,&mp_reg_data,sizeof(mp_reg_data));//make contiguous memcpy for all reg_data
// 		
// 		RtU32T l_base_ctr = 0;
// 		for(RtU32T l_cnt=(m_max_data_ref+1);l_cnt <	m_max_data_indx;l_cnt++)
// 		{
// 				if( (mp_reg_data[l_cnt].flag &	RT_SH_UDI_REG_DATA_SRVC_MANDATORY))
// 				{
// 					memcpy((l_ckpt_data.data + l_base_ctr*sizeof(RtShUDIRegData)),&mp_reg_data[l_cnt],sizeof(RtShUDIRegData));
// 					l_base_ctr++;
// 				}	
// 		}
// 
// 		l_ret_val = gp_mgl_intf->rtInsertCkptData(	m_ckpt_id, 
// 																								(void*)&l_ckpt_data,
// 																								sizeof(l_ckpt_data),
// 																								l_key);
// 
// 		if(l_ret_val == RT_RET_CPSV_EXISTS)
// 		{
// 
// 
// 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
// 			"rtCreateSections():- gp_mgl_intf->rtInsertCkptData() returned RT_RET_CPSV_EXISTS for CKPT_ID=%d,SEC_KEY=%s",m_ckpt_id,l_key);
// 			
// 			l_ret_val = RT_SUCCESS;
// 		}
// 		else if(l_ret_val != RT_SUCCESS)
// 		{
// 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
// 			"rtCreateSections():- mp_mgl_intf->rtInsertCkptData() Failed for CKPT_ID=%d,SEC_KEY=%s [RETVAL=%d]",m_ckpt_id,l_key,l_ret_val);
// 
// 			l_ret_val = RT_FAILURE;
// 
// 		
// 		}
// 		else
// 		{
// 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
// 			"rtCreateSections():- mp_mgl_intf->rtInsertCkptData() SUCCESS for CKPT_ID=%d,SEC_KEY=%s [RETVAL=%d]",m_ckpt_id,l_key,l_ret_val);
// 		}
				
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
		"rtCreateSections():- size of RT_SH_UDI_MAX_CKPT_DATA_LEN[%d] is less than required for mandatory services[%d],ret_val =%d",RT_SH_UDI_MAX_CKPT_DATA_LEN,l_num_mand_srv*sizeof(RtShUDIRegData),l_ret_val);

		l_ret_val = RT_FAILURE;
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
		"rtCreateSection():- LEAVE  [RETVAL=%d]",l_ret_val);
	
	return	l_ret_val;					
	
}



/*******************************************************************************
*
* FUNCTION NAME : rtSyncStandbyUsingCkpt()
*
* DESCRIPTION   : This function is used to sync RtShDatarefConfig.xml and <Service>.xml 
* 								from active during satartup using ckpt.
*                 
*
* ARGUMENTS     : NONE
*                 
*
* RETURN        : RtRcT
*                     
*******************************************************************************/

RtRcT RtShUDIDataPoolMgr::rtSyncStandbyUsingCkpt()
{
	RtRcT l_ret_val = RT_SUCCESS;
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
	"rtSyncStandbyUsingCkpt():- ENTER ");

	printf("\nrtSyncStandbyUsingCkpt():- ENTER ");

  if ( NULL == getenv("COMMON_CONFIG_PATH") )//KLOCWORK_FIX RM 03092013
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
		"rtSyncStandbyUsingCkpt() COMMON_CONFIG_PATH not set.");							
		return RT_FAILURE;
  }

// 	RtCkptKeyData l_ckpt_key_data;
// 	memset(&l_ckpt_key_data,0, sizeof(l_ckpt_key_data))	;
// 	
// 
// 	
// 	l_ckpt_key_data.key=(char*)malloc(sizeof(char)*RT_CKPT_SECT_KEY_LEN);
// 	l_ckpt_key_data.data_buf=malloc(sizeof(RtCkptShUDIData));
// 	l_ckpt_key_data.data_size=sizeof(RtCkptShUDIData);
// 	bzero(l_ckpt_key_data.key,sizeof(char)*RT_CKPT_SECT_KEY_LEN);		
// 	sprintf(l_ckpt_key_data.key,"%d",0);		
	
//JFC-02042013

//first we read for key zero (RtShDataRefConfig.xml)
	
	 ////////////////////////
	 //Copy XML From Remote//
	 ///////////////////////
	 
	 string l_config_path 	= getenv("COMMON_CONFIG_PATH");
	 l_config_path 	+= "sh/RtShDataRefConfig.xml"; 

	 l_ret_val = rtScpFromRemoteController( (RtS8T*)	l_config_path.c_str() );
	 if(l_ret_val != RT_SUCCESS)
	 {
		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
		 "rtSyncStandbyUsingCkpt():-rtScpFromRemoteController Failed for Configuration[Path=%s] .Exit [RETVAL=%d]",l_config_path.c_str(),l_ret_val);

		printf("\nrtSyncStandbyUsingCkpt():-rtScpFromRemoteController Failed for Configuration[Path=%s] .Exit [RETVAL=%d]",l_config_path.c_str(),l_ret_val);
		fflush(NULL);
		
		 return RT_FAILURE;
	 }
	 
// 	l_ret_val = gp_mgl_intf->rtReadCkptData(m_ckpt_id,1,&l_ckpt_key_data);
// 	
// 	if(l_ret_val!=RT_SUCCESS)
// 	{
// 		
// 		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
// 		"rtSyncStandbyUsingCkpt()-mp_mgl_intf->rtReadCkptData() Returned FAILURE for section with key=%s",l_ckpt_key_data.key);
// 				
// 	}
	
	
	
	 RtShDataRefConfigReader* lp_config_reader = new RtShDataRefConfigReader();
	 RtRcT l_xml_ret_val	= lp_config_reader->rtInitialize();
	 if(RT_SUCCESS !=	l_xml_ret_val)
	 {
	   mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
		 "rtSyncStandbyUsingCkpt():-ERROR:RtShDataRefConfigReader::rtInitialize() returns %d",l_xml_ret_val);
		 
		 printf("\nrtSyncStandbyUsingCkpt():-ERROR:RtShDataRefConfigReader::rtInitialize() returns %d",l_xml_ret_val);
		 fflush(NULL);
		 //Klocwork: Priti: 18082012:
		 delete lp_config_reader;

		 return RT_FAILURE;		
	 }
		
		
		
		
	l_config_path 	= getenv("COMMON_CONFIG_PATH");
	l_config_path 	+= "sh/RtShDataRefConfig.xml"; 

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"rtSyncStandbyUsingCkpt():--COMMON_CONFIG_PATH=%s",l_config_path.c_str());

	RtDataRefConfigInfo l_data_ref_config;
	memset(&l_data_ref_config,0,sizeof(RtDataRefConfigInfo));

	l_xml_ret_val	= lp_config_reader->rtGetShDataRefConfigData(l_config_path,l_data_ref_config);

	if(RT_XML_SUCCESS !=	l_xml_ret_val)
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
		  "rtSyncStandbyUsingCkpt():-ERROR:RtShDataRefConfigReader::rtGetShDataRefConfigData() returns %d",l_xml_ret_val);

		printf("\nrtSyncStandbyUsingCkpt():-ERROR:RtShDataRefConfigReader::rtGetShDataRefConfigData() returns %d",l_xml_ret_val);
		fflush(NULL);
		//Klocwork: Priti: 18082012:
		delete lp_config_reader;
		return	RT_FAILURE;
	}

	lp_config_reader->rtDisplayShDataRefConfigData(l_data_ref_config);

	//Klocwork: Priti: 18082012:
	delete lp_config_reader;
	RtShUDIRef l_udi_ref;
	RtRcT l_local_ret_val;
 	for(RtU32T	l_cnt = 0; l_cnt	< l_data_ref_config.trans_data.num_srv_data; ++l_cnt)
 	{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
					"rtSyncStandbyUsingCkpt() - [l_cnt=%d]Checking for app_enum=%u,srvc_ind=%s ",
						l_cnt,l_data_ref_config.trans_data.srv_data[l_cnt].app_enum,l_data_ref_config.trans_data.srv_data[l_cnt].srv_ind);					

			l_udi_ref =0;
			
			l_local_ret_val = rtGetUdiRefFromAppEnum(l_data_ref_config.trans_data.srv_data[l_cnt].app_enum,&l_udi_ref );

			if(RT_SUCCESS == l_local_ret_val)
			{
 			 if( ( l_data_ref_config.trans_data.srv_data[l_cnt].orig_case_data.is_srv_mandatory 
												 || 
							 l_data_ref_config.trans_data.srv_data[l_cnt].term_case_data.is_srv_mandatory
					 )
												 &&
					 (l_data_ref_config.trans_data.srv_data[l_cnt].is_activated )			
				 )
				{
					if(mp_reg_data[l_cnt].orig_case.is_service_data_mandatory != l_data_ref_config.trans_data.srv_data[l_cnt].orig_case_data.srv_validity)
					{
						rtEnableDftService(l_udi_ref,l_data_ref_config.trans_data.srv_data[l_cnt].orig_case_data.srv_validity,false,RT_SH_ORIG_CASE);
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_ALERT, __FILE__, __LINE__, 
								"rtSyncStandbyUsingCkpt() - [l_cnt=%d]Checking for app_enum=%u,srvc_ind=%s in orig",
						l_cnt,l_data_ref_config.trans_data.srv_data[l_cnt].app_enum,l_data_ref_config.trans_data.srv_data[l_cnt].srv_ind);					

					}
					else
					{
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
						"rtSyncStandbyUsingCkpt() same in orig case of both structure");					
					}
					
				 if(mp_reg_data[l_cnt].term_case.is_service_data_mandatory != l_data_ref_config.trans_data.srv_data[l_cnt].term_case_data.srv_validity)
					{
						rtEnableDftService(l_udi_ref,l_data_ref_config.trans_data.srv_data[l_cnt].term_case_data.srv_validity,false,RT_SH_TERM_CASE);

						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_ALERT, __FILE__, __LINE__, 
								"rtSyncStandbyUsingCkpt() - [l_cnt=%d]Checking for app_enum=%u,srvc_ind=%s. in Term case",
						l_cnt,l_data_ref_config.trans_data.srv_data[l_cnt].app_enum,l_data_ref_config.trans_data.srv_data[l_cnt].srv_ind);					


					}
					else
					{
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
						"rtSyncStandbyUsingCkpt() same in Term case of both structure");					
					}
				}
				else
				{
					//do nothing as service is not configured as mandatory
				}		
	
	
			}
			else
			{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_ALERT, __FILE__, __LINE__, 
					"rtSyncStandbyUsingCkpt() - [l_cnt=%d]Checking for app_enum=%u,srvc_ind=%s,rtGetUdiRefFromAppEnum() returned FAILURE ",
						l_cnt,l_data_ref_config.trans_data.srv_data[l_cnt].app_enum,l_data_ref_config.trans_data.srv_data[l_cnt].srv_ind);					

		   }
	}		
	//Now we sync every mandatory service data(<service>.xml)
	for (RtU32T l_cnt=(m_max_data_ref+1);l_cnt	< m_max_data_indx;l_cnt++)	
	{
		if(( mp_reg_data[l_cnt].flag &	RT_SH_UDI_REG_DATA_SRVC_MANDATORY	) && !(mp_reg_data[l_cnt].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT))
		{
			l_ret_val = rtUpdateDftSrvDataFromCkpt(mp_reg_data[l_cnt].data_ref_appl_info.app_enum, mp_reg_data[l_cnt].udi_ref,true);

			if(RT_SUCCESS != l_ret_val)
			{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
		  		"rtSyncStandbyUsingCkpt():-ERROR:RrtUpdateDftSrvDataFromCkpt returns %d fpr app_enum=%d,udi_ref=%u",l_ret_val,mp_reg_data[l_cnt].data_ref_appl_info.app_enum, mp_reg_data[l_cnt].udi_ref);


				break;
			}
		}
	}			
	
	
	//free (l_ckpt_key_data.key);
	//free (l_ckpt_key_data.data_buf);
	

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
	"rtSyncStandbyUsingCkpt():- LEAVE  l_ret_val=%d",l_ret_val);

	printf("\nrtSyncStandbyUsingCkpt():- LEAVE  l_ret_val=%d",l_ret_val);fflush(NULL);
	
	return l_ret_val;
}
/*******************************************************************************
*
* FUNCTION NAME : rtGetPoolElem()
*
* DESCRIPTION   : This function is used to get NEW pool element from container class
*                 
*
* ARGUMENTS     : RtShUDIDataPoolIndx a_pool_indx,void** app_elem_ptr,RtU32T& ar_index
*                 
*
* RETURN        : RtRcT
*                     
*******************************************************************************/

RtRcT RtShUDIDataPoolMgr::rtGetPoolElem(RtShUDIDataPoolIndx a_pool_indx,void** app_elem_ptr,RtU32T& ar_index)
{
	/* - use mp_valid_reg_data to do validation
	   - use mp_data_pool_container[] to get element from data pool container
	   - raise or clear alarm appropriately
	   - return appropriate value from this function  */
		 
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtGetPoolElem() Get Pool Element of Pool Container[%d]-ENTER", a_pool_indx);  	 
	
	RtRcT l_ret_val = RT_SUCCESS;
	if(mp_valid_reg_data[a_pool_indx])
	{
		l_ret_val = mp_data_pool_container[a_pool_indx]->rtGetPoolElem(&app_elem_ptr,ar_index);
    if(RT_SUCCESS == l_ret_val)
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	    "RtShUDIDataPoolMgr::rtGetPoolElem() Get Pool Element Execution SUCCESS");	
		}
		else
		{
			l_ret_val = RT_SH_INVALID_UDI_REFERENCE;  
			
			mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udigetpoolelemfailed);
			
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
      "RtShUDIDataPoolMgr::rtGetPoolElem() Get Pool Element Called with invalid Pool Index [%d]", a_pool_indx);
		}
	}
	else
	{
		l_ret_val = RT_SH_INVALID_UDI_REFERENCE;  
		
		mp_sys_agent->rtRaiseCount(udiCntrs, udiCntrsDefIntfId, udigetpoolelemfailed);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtGetPoolElem() Get Pool Element Called with invalid Pool Index [%d]", a_pool_indx);
	}
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtGetPoolElem() Get Pool Element of Pool Container[%d] At Index [%d]  [RETVAL=%d] -EXIT",
	a_pool_indx, ar_index, l_ret_val);
	
  return l_ret_val;
}



/*******************************************************************************

  FUNCTION NAME : rtReturnPoolElem().
 
  DESCRIPTION   : This function is used to return pool element to container class
                  
 
  ARGUMENTS     : RtShUDIDataPoolIndx a_pool_indx,RtU32T a_index
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtReturnPoolElem(RtShUDIDataPoolIndx a_pool_indx,RtU32T a_index)
{
	/* - use mp_valid_reg_data to do validation
	   - use mp_data_pool_container[] to return element to data pool container
	   - return appropriate value from this function   */
		 	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtReturnPoolElem() Return Pool Element of Pool Container[%d] At Index [%d] -ENTER", a_pool_indx, a_index);  	 
	
	RtRcT l_ret_val = RT_SUCCESS;
	if(mp_valid_reg_data[a_pool_indx])
	{
		l_ret_val = mp_data_pool_container[a_pool_indx]->rtReturnPoolElem(a_index);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtReturnPoolElem() Return Pool Element Executed with Return value", l_ret_val);
	
	}
	else
	{
		l_ret_val = RT_SH_INVALID_UDI_REFERENCE;  /* TBD set proper return code */
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtReturnPoolElem() Return Pool Element Called with invalid Pool Index [%d]", a_pool_indx);
	}
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtReturnPoolElem() Return Pool Element of Pool Container[%d] At Index [%d]  [RETVAL=%d] -EXIT",
	a_pool_indx, a_index, l_ret_val);
	
  return l_ret_val;
}		
	
/*******************************************************************************

  FUNCTION NAME : rtCopyPoolElem().
 
  DESCRIPTION   : This function is used to retrive pool element from container class
                  
 
  ARGUMENTS     : RtShUDIDataPoolIndx a_pool_indx,RtU32T a_index, void* ap_elem_ptr
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr :: rtCopyPoolElem(RtShUDIDataPoolIndx a_pool_indx, RtU32T a_index, void* ap_elem_ptr)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtCopyPoolElem() Retrive Pool Element of Pool Container[%d] At Index [%d] -ENTER", a_pool_indx, a_index); 
 
  RtRcT l_ret_val = RT_SUCCESS;
	if(mp_valid_reg_data[a_pool_indx])
	{
		l_ret_val = mp_data_pool_container[a_pool_indx]->rtCopyPoolElem(ap_elem_ptr, a_index);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtCopyPoolElem() Retrive Pool Element Executed with Return value", l_ret_val);
	
	}
	else
	{
		l_ret_val = RT_SH_INVALID_UDI_REFERENCE; 
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtCopyPoolElem() Retrive Pool Element Called with invalid Pool Index [%d]", a_pool_indx);
	}
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtCopyPoolElem() Return Pool Element of Pool Container[%d] At Index [%d]  [RETVAL=%d] -EXIT",
	a_pool_indx, a_index, l_ret_val);
	
  return l_ret_val;

}

/*******************************************************************************

  FUNCTION NAME : rtPointToPoolElem().
 
  DESCRIPTION   : This function is used to point to pool element from container class
                  
 
  ARGUMENTS     : RtShUDIDataPoolIndx a_pool_indx,RtU32T a_index, void** app_elem_ptr
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr :: rtPointToPoolElem(RtShUDIDataPoolIndx a_pool_indx, RtU32T a_index, void** app_elem_ptr)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtPointToPoolElem() ENTER Pool Container[%d] At Index [%d] -ENTER", a_pool_indx, a_index); 
 
  RtRcT l_ret_val = RT_SUCCESS;
	if(mp_valid_reg_data[a_pool_indx])
	{
		l_ret_val = mp_data_pool_container[a_pool_indx]->rtPointToPoolElem(&app_elem_ptr, a_index);
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtPointToPoolElem() Retrive Pool Element Executed with Return value", l_ret_val);
	
	}
	else
	{
		l_ret_val = RT_SH_INVALID_UDI_REFERENCE; 
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtPointToPoolElem() Retrive Pool Element Called with invalid Pool Index [%d]", a_pool_indx);
	}
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtPointToPoolElem() EXIT Pool Container[%d] At Index [%d]  [RETVAL=%d] -EXIT",
	a_pool_indx, a_index, l_ret_val);
	
  return l_ret_val;

}
	
/*******************************************************************************

  FUNCTION NAME : rtGetRegDataRefList()
 
  DESCRIPTION   : This function returns the registered data reference list
                  
 
  ARGUMENTS     : RtShUDIDataRefList&
                  
 
  RETURN        : void
                      
 ******************************************************************************/
void RtShUDIDataPoolMgr::rtGetRegDataRefList(RtShUDIDataRefList&  ar_data_ref_list)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtGetRegDataRefList() ENTER");
		
	ar_data_ref_list = m_reg_data_ref_list;
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtGetRegDataRefList() EXIT");

}

/*******************************************************************************

  FUNCTION NAME : rtIsDataRefRegistered()
 
  DESCRIPTION   : This function is called to enquire whether data reference is
	                registered or not. 
									Used while filling AVP in Sh Intf
                  
 
  ARGUMENTS     : RtShUDIDataPoolIndx a_pool_indx,RtBoolT& ar_is_data_ref_registered
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtIsDataRefRegistered(RtShUDIDataPoolIndx a_pool_indx,RtBoolT& ar_is_data_ref_registered)
{
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtIsDataRefRegistered() Called for Pool Container[%d] -ENTER", a_pool_indx);
	
	RtRcT l_ret_val = RT_SUCCESS;
	
	if(a_pool_indx < m_max_data_indx)
	{
		ar_is_data_ref_registered = mp_valid_reg_data[a_pool_indx];
	}
	else
	{
	
		l_ret_val = RT_SH_INVALID_UDI_REFERENCE;
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtIsDataRefRegistered() Called for Invalid Data Reference[%d] ", a_pool_indx);
	}
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtIsDataRefRegistered() [RETVAL = %d] EXIT", l_ret_val);
	
	return l_ret_val;
}


/*******************************************************************************

  FUNCTION NAME : rtIfDataRefCanBeSentInSNR()
 
  DESCRIPTION   : This function is called to enquire whether data reference  
									can be sent in SNR
                  
 
  ARGUMENTS     : RtShUDIDataPoolIndx a_pool_indx,RtBoolT& ar_data_ref_can_be_sent_in_snr
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtIfDataRefCanBeSentInSNR(RtShUDIDataPoolIndx a_pool_indx,RtBoolT& ar_data_ref_can_be_sent_in_snr)
{
  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtIfDataRefCanBeSentInSNR() Called for Pool Index [%d] -ENTER", a_pool_indx);
	
	RtRcT l_ret_val = RT_SUCCESS;
	ar_data_ref_can_be_sent_in_snr = false;
	
	if(a_pool_indx <= m_max_data_ref)
	{
		if(mp_reg_data[a_pool_indx].flag & RT_SH_UDI_REG_DATA_INCLUDE_IN_SNR)
			ar_data_ref_can_be_sent_in_snr = true;
	}
	else
	{
	
		l_ret_val = RT_SH_INVALID_UDI_REFERENCE;
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtIfDataRefCanBeSentInSNR() Called for Invalid Pool Index [%d] ", a_pool_indx);
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtIfDataRefCanBeSentInSNR() Data send in SNR is [%d] [RETVAL = %d] EXIT",
	ar_data_ref_can_be_sent_in_snr, l_ret_val);
		
	return l_ret_val;
}



/*******************************************************************************

  FUNCTION NAME : rtValidateSrvcMaxDataSize()
 
  DESCRIPTION   : This function is called to to validate whether the passed 
									max data size is correct or not for particular data reference
                  
 
  ARGUMENTS     : RtShUDIDataPoolIndx a_pool_indx,RtU32T a_max_data_size,RtBoolT& ar_is_data_ref_registered
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtValidateSrvcMaxDataSize(RtShUDIDataPoolIndx a_pool_indx,RtU32T a_max_data_size,RtBoolT& ar_validation_result)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtValidateSrvcMaxDataSize() Called for Pool Index [%d] For data Size -ENTER", a_pool_indx, a_max_data_size);
	
	RtRcT l_ret_val = RT_SUCCESS;
	
	if(a_pool_indx < m_max_data_indx)
	{
		
		if(a_max_data_size <= mp_reg_data[a_pool_indx].max_data_size)
		{
			ar_validation_result = true;
		}
		else
		{
			ar_validation_result = false;
		}						
	}
	else
	{
	
		l_ret_val = RT_FAILURE;
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtValidateSrvcMaxDataSize() Called for Invalid Pool Index [%d] ", a_pool_indx);
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtValidateSrvcMaxDataSize() Validate max data size [%d] [RETVAL = %d] EXIT",
	ar_validation_result, l_ret_val);
	
	return l_ret_val;
}

	

/*******************************************************************************

  FUNCTION NAME : rtGetRegSrvcIndicList()
 
  DESCRIPTION   : This functions returns the registered service indication list 
                  
 
  ARGUMENTS     : RtShUDISrvcIndicList&
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
void RtShUDIDataPoolMgr::rtGetRegSrvcIndicList(RtShUDISrvcIndicList&  ar_srvc_indic_list)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtGetRegSrvcIndicList() ENTER");
	
	ar_srvc_indic_list = m_reg_srvc_indic_list;
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtGetRegSrvcIndicList() EXIT");
}



/*******************************************************************************

  FUNCTION NAME : rtStoreDataRefData()
 
  DESCRIPTION   : This functions is used to store the NEW data reference information received from HSS
									NOTE:- This function is not called for data reference = 0	
                  
 
  ARGUMENTS     : RtShUDIDataRefVal a_data_ref_val,void* ap_data,RtU32T& ar_index,void ** app_elem_pool
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtStoreDataRefData(RtShUDIDataRefVal a_data_ref_val,void* ap_data,RtU32T& ar_index,void ** app_elem_pool)
		
{

	//////////////////////////////////////////////////////
	//This function is not called for data reference = 0//
	//////////////////////////////////////////////////////

  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtStoreDataRefData() UDI reference value [%d] -ENTER", a_data_ref_val);
	
	 RtRcT l_rval = RT_SUCCESS;
	
	if(a_data_ref_val < m_max_data_indx)
	{
		 void* lp_elem_ptr;

		 l_rval = rtGetPoolElem(a_data_ref_val, &lp_elem_ptr,ar_index);

		 if (RT_SUCCESS == l_rval)
		 {
			 //- memcpy at location lp_elem_ptr, from location ap_data, size = mp_reg_data[a_data_ref_val]->max_data_size
				//?Himanshu Size validation checking required
			 memcpy(lp_elem_ptr, ap_data, mp_reg_data[a_data_ref_val].max_data_size);

			 *app_elem_pool = lp_elem_ptr;
		 }
		 else
		 {
	  	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  	 "RtShUDIDataPoolMgr::rtStoreDataRefData()::rtGetPoolElem() FAILED Ret_val[%d]", l_rval);

			 l_rval =	RT_SH_UDI_ERR_CREATING_DATA_POOL_CONTAINER;
		 }
	 }
	 else
	 {
	  //HP_CR(DONE) : critical log + negative return value
		l_rval = RT_SH_UDI_ERR_INVALID_DATA_REF;
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtStoreDataRefData() Called for Invalid Data Ref [%u] ", a_data_ref_val);		
	 }

  mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtStoreDataRefData() [RETVAL = %d] EXIT", l_rval);
	
	return l_rval;
}

/*******************************************************************************

  FUNCTION NAME : rtUpdDataRefData()
 
  DESCRIPTION   : This functions is used to UPDATE data reference information received from HSS
									NOTE:- This function is not called for data reference = 0	
                  
 
  ARGUMENTS     : RtShUDIDataRefVal a_data_ref_val,void* ap_data,void * ap_elem_pool
                  
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtUpdDataRefData(RtShUDIDataRefVal a_data_ref_val,void* ap_data,void* ap_elem_pool)
{

	//////////////////////////////////////////////////////
	//This function is not called for data reference = 0//
	//////////////////////////////////////////////////////

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtUpdDataRefData() UDI Data Reference Value[%d] ENTER", a_data_ref_val);
	
	 RtRcT l_rval = RT_SUCCESS;
	
	if(a_data_ref_val < m_max_data_indx)
	{
	//?Himanshu Size validation checking required
	 //- memcpy at location ap_elem_ptr, from location ap_data, size = mp_reg_data[a_data_ref_val]->max_data_size
	
	 memcpy(ap_elem_pool, ap_data, mp_reg_data[a_data_ref_val].max_data_size);
	}
	else
	{
	  //HP_CR(DONE): critical log + negative return value
		l_rval = RT_SH_UDI_ERR_INVALID_DATA_REF;
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtUpdDataRefData() Called for Invalid Data Ref [%u] ", a_data_ref_val);				
	}
	
	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtUpdDataRefData() [RETVAL = %d] EXIT", l_rval);

	 return l_rval;
}

//HP_CR(DONE): not required as per SHASHI
// /*******************************************************************************
// 
//   FUNCTION NAME : rtGetDataRefData()
//  
//   DESCRIPTION   : HP_CR(DONE): not required
//                   
//  
//   ARGUMENTS     : RtShUDIDataRefVal a_data_ref_val,void* ap_data,void * ap_elem_pool
//                   
//  
//   RETURN        : RtRcT
//                       
//  ******************************************************************************/
// 
// RtRcT RtShUDIDataPoolMgr :: rtGetDataRefData(RtShUDIRef a_udi_ref_val,void** app_buffer,RtU32T& ar_buffer_size )
// {
// 	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
// 	"RtShUDIDataPoolMgr::rtGetDataRefData() UDI Data Reference Value[%d] ENTER", a_udi_ref_val);
// 	
// 	RtRcT l_rval = RT_SUCCESS;
// 
// 	*app_buffer = &mp_reg_data[a_udi_ref_val];
// 	
// 	ar_buffer_size = mp_reg_data[a_udi_ref_val].max_data_size;
// 
// 	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
//  "RtShUDIDataPoolMgr::rtUpdDataRefData() [RETVAL = %d] EXIT", l_rval);
// 
// 	return l_rval;
// }

/*******************************************************************************

  FUNCTION NAME : rtStoreDataRepositoryData()
 
  DESCRIPTION   : This functions is used to store the NEW service indication information received from HSS
									NOTE:- This function is ONLY called for data reference = 0	
                  
 
  ARGUMENTS     : RtShUDIDataRefVal a_data_ref_val,void* ap_data,RtU32T data_size,RtU32T& ar_index,void ** app_elem_pool
                  ,RtBoolT a_is_decoding_of_buffer_reqd(by default it is true)
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtStoreDataRepositoryData(RtShUDIRef a_udi_ref_val,void* ap_buffer,RtU32T a_buffer_size ,RtU32T& ar_index,void ** app_elem_pool,RtBoolT a_is_decoding_of_buffer_reqd)
		
{

	///////////////////////////////////////////////////////
	//This function is ONLY called for data reference = 0//
	///////////////////////////////////////////////////////

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtStoreDataRepositoryData() UDI Data Reference Value[%d]  -ENTER", a_udi_ref_val);
	
	 RtRcT l_rval = RT_SUCCESS;
	
	if(a_udi_ref_val < m_max_data_indx)
	{
		 void* lp_elem_ptr;

		 l_rval = rtGetPoolElem(a_udi_ref_val, &lp_elem_ptr,ar_index);

		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_INFO,__FILE__,__LINE__,
		"RtShUDIDataPoolMgr::rtStoreDataRepositoryData()::rtGetPoolElem() Executed with Ret_val[%d] ,ar_index=%d", l_rval,ar_index);

		 if (RT_SUCCESS == l_rval)
		 {

			 if( (a_is_decoding_of_buffer_reqd) && (mp_reg_data[a_udi_ref_val].dec_func_ptr != NULL) )
			 {
			  	l_rval = mp_reg_data[a_udi_ref_val].dec_func_ptr(ap_buffer,a_buffer_size,lp_elem_ptr,mp_reg_data[a_udi_ref_val].max_data_size);
			  	if (RT_SUCCESS == l_rval)
					{
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	        	"RtShUDIDataPoolMgr::rtStoreDataRepositoryData() Decoding Function executed successfully");

						*app_elem_pool = lp_elem_ptr;
					}
					else
					{
				  	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	        	"RtShUDIDataPoolMgr::rtStoreDataRepositoryData() Decoding Function is not executed successfully");

						 mp_data_pool_container[a_udi_ref_val]->rtReturnPoolElem(ar_index);
				  	 
						 l_rval = RT_FAILURE;
					}
			 }
			 else
			 {
				 //- validate a_buffer_size against mp_reg_data[a_data_ref_val]->max_data_size
					//tag_db Add callback for service extensionability
  			 void* lp_updated_data = NULL;
				 RtU32T l_buffer_size=0;
				 RtBoolT l_buffer_exchange=false;
				 RtRcT l_rval_tmp=RT_FAILURE;


					RtS8T 			l_srvc_indic[RT_DIA_SH_MAX_SRV_INDICATION_LEN];
					memset(l_srvc_indic,0,RT_DIA_SH_MAX_SRV_INDICATION_LEN);
					RtU32T	l_app_enum = 0;
					
				  rtGetSrvIndAppEnumFromUdiRef(a_udi_ref_val, l_srvc_indic, l_app_enum);
				 
				 l_rval_tmp	=	rtAppSrvcExtnCap(l_app_enum,ap_buffer,a_buffer_size,(void**)&lp_updated_data,l_buffer_size,l_buffer_exchange);
				 if((RT_SUCCESS ==	l_rval_tmp)&&(l_buffer_exchange))
				 {
					 
					 memcpy(lp_elem_ptr, lp_updated_data, l_buffer_size);
			  	 *app_elem_pool = lp_elem_ptr; 
					 delete lp_updated_data;

 				 }
				 else
				 {

					 if(a_buffer_size == mp_reg_data[a_udi_ref_val].max_data_size)
					 {

			  		 //memcpy at location lp_elem_ptr, from location ap_buffer, size = mp_reg_data[a_data_ref_val]->max_data_size

						 memcpy(lp_elem_ptr, ap_buffer, a_buffer_size);

			  		 *app_elem_pool = lp_elem_ptr; 
						 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_INFO,__FILE__,__LINE__,
	        	          "RtShUDIDataPoolMgr::rtStoreDataRepositoryData(),INFO Data a_buffer_size=%d copied at ar_index=%d in a_udi_ref_val=%d",a_buffer_size,ar_index,a_udi_ref_val);
						 
						 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_INFO,__FILE__,__LINE__,
	        	          "RtShUDIDataPoolMgr::rtStoreDataRepositoryData(),JFT calling rtPrintSrvcData() for l_app_enum=%d and pool ar_index=%d a_udi_ref_val=%d",l_app_enum,ar_index,a_udi_ref_val);

						 rtPrintSrvcData(l_app_enum,*app_elem_pool);


					 }
					 else
					 {
				    	mp_data_pool_container[a_udi_ref_val]->rtReturnPoolElem(ar_index);

			  			l_rval = RT_FAILURE;   	

							mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	        		"RtShUDIDataPoolMgr::rtStoreDataRepositoryData() Buffer size Exceed max_data_size [BUFFER Size %d] [MAX_DATA_SIZE %d]",
							a_buffer_size, mp_reg_data[a_udi_ref_val].max_data_size);
					 }

				 }
					 
	// 					
// 				 if(a_buffer_size <= mp_reg_data[a_udi_ref_val].max_data_size)
// 				 {
// 
// 			  	 //memcpy at location lp_elem_ptr, from location ap_buffer, size = mp_reg_data[a_data_ref_val]->max_data_size
// 
// 					 memcpy(lp_elem_ptr, ap_buffer, a_buffer_size);
// 
// 			  	 *app_elem_pool = lp_elem_ptr; 
// 				 }
// 				 else
// 				 {
// 				    mp_data_pool_container[a_udi_ref_val]->rtReturnPoolElem(ar_index);
// 						 
// 			  		l_rval = RT_FAILURE;   	
// 
// 						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
// 	        	"RtShUDIDataPoolMgr::rtStoreDataRepositoryData() Buffer size Exceed max_data_size [BUFFER Size %d] [MAX_DATA_SIZE %d]",
// 						a_buffer_size, mp_reg_data[a_udi_ref_val].max_data_size);
// 				 }
			 }
		 }
		 else
		 {
			 l_rval = RT_FAILURE;

			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  	 "RtShUDIDataPoolMgr::rtStoreDataRepositoryData()::rtGetPoolElem Pool Element Not Created Successfully");
		 }
	 }
	 else
	 {
	  //HP_CR(DONE): critical logs + negative return value
		l_rval = RT_SH_UDI_ERR_INVALID_DATA_REF;
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtStoreDataRepositoryData() Called for Invalid Data Ref [%u] ", a_udi_ref_val);	
	 }

   mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtStoreDataRepositoryData() [RETVAL = %d] -EXIT", l_rval);
	
	 return l_rval;
}

/*******************************************************************************

  FUNCTION NAME : rtUpdDataRepositoryData()
 
  DESCRIPTION   : This functions is used to UPDATE the service indication information received from HSS
									NOTE:- This function is ONLY called for data reference = 0	
                  
 
  ARGUMENTS     : RtShUDIDataRefVal a_data_ref_val,void* ap_data,void * ap_elem_pool
                  ,RtBoolT a_is_decoding_of_buffer_reqd(by default it is true)
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtUpdDataRepositoryData(RtShUDIRef a_udi_ref_val,void* ap_buffer,RtU32T a_buffer_size ,void * ap_elem_ptr)		
{

	///////////////////////////////////////////////////////
	//This function is ONLY called for data reference = 0//
	///////////////////////////////////////////////////////

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtUpdDataRepositoryData() UDI Reference [%d] Buffer Size[%d]",a_udi_ref_val, a_buffer_size);
	
	RtRcT l_rval = RT_SUCCESS;
  if(a_udi_ref_val < m_max_data_indx)
	{
		if(mp_reg_data[a_udi_ref_val].dec_func_ptr != NULL) 
		{
			l_rval = mp_reg_data[a_udi_ref_val].dec_func_ptr(ap_buffer,a_buffer_size,ap_elem_ptr,mp_reg_data[a_udi_ref_val].max_data_size);
			if (RT_SUCCESS == l_rval)
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	  		"RtShUDIDataPoolMgr::rtUpdDataRepositoryData() Decoding Function executed successfully");

			}
			else
			{
				//HP_CR(DONE): critical logs
				l_rval = RT_SH_UDI_ERR_INVALID_DATA_REF;

				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  		"RtShUDIDataPoolMgr::rtUpdDataRepositoryData() Decoding Failed for Data Ref [%u] ", a_udi_ref_val);					
			}
		}
		else
		{
			if(a_buffer_size <= mp_reg_data[a_udi_ref_val].max_data_size)
			{				 
				memcpy(ap_elem_ptr, ap_buffer, a_buffer_size);
			}
			else
			{
				//HP_CR(DONE): negative return value + critical logs
				l_rval = RT_SH_UDI_ERR_TOO_MUCH_DATA;

				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  		"RtShUDIDataPoolMgr::rtUpdDataRepositoryData() Data Size Exceed for Data Ref [%u] Max Data Size[%d] , a_data_size[%d]", 
				a_udi_ref_val, mp_reg_data[a_udi_ref_val].max_data_size, a_buffer_size);					
			}
		}
	}
	else
	{
		//HP_CR(DONE): critical logs + negative return value
		l_rval = RT_SH_UDI_ERR_INVALID_DATA_REF;
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
	  "RtShUDIDataPoolMgr::rtUpdDataRepositoryData() Called for Invalid Data Ref [%u] ", a_udi_ref_val);	
	}
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtUpdDataRepositoryData() [RETVAL = %d] -EXIT", l_rval);
	
	return l_rval;
}

/*******************************************************************************

  FUNCTION NAME : rtGetSrvIndFromUdiRef()
 
  DESCRIPTION   : This functions is used to UPDATE the service indication information received from HSS
									NOTE:- This function is ONLY called for data reference = 0	
                  
 
  ARGUMENTS     : RtShUDIDataRefVal a_data_ref_val,void* ap_data,void * ap_elem_pool
                  ,RtBoolT a_is_decoding_of_buffer_reqd(by default it is true)
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr:: rtGetSrvIndFromUdiRef(RtShUDIRef a_udi_ref,RtS8T* ap_srvc_indic)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtGetSrvIndFromUdiRef()  ENTER  udi_ref=[%u]",
	 a_udi_ref);
	
	RtRcT l_rval = RT_SUCCESS;
	
	strcpy(ap_srvc_indic,mp_reg_data[a_udi_ref].data_ref_appl_info.app_srvc_indic);
	 
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtGetSrvIndFromUdiRef()  EXIT  udi_ref=[%u], Srv_indic [%s]",
	 a_udi_ref, ap_srvc_indic);
	
	return l_rval; 

}


/*******************************************************************************

  FUNCTION NAME : rtCreateCaseOrderedListForUser()
 
  DESCRIPTION   : This functions is used to create case (orig, term) ordered 
									list as per configuration and user service data 
									received from HSS. This function can also be used to modify caselist.
 
  ARGUMENTS     : RtShUDIUserCntxt* ap_cntxt
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtCreateCaseOrderedListForUser(RtShUDIUserCntxt* ap_cntxt)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtCreateCaseOrderedListForUser()  ENTER  cntxt_id=[%u] and cug_cntxt_id=%u",
	 ap_cntxt->session_cookie.self_indx,ap_cntxt->cug_cntxt_id);
		
	RtShUDIDataReference* ap_cug_ref_info = NULL;	
	if(ap_cntxt->cug_cntxt_id !=  0)
	{
		RtShUDIUserCntxtMgr* lp_cntxt_mgr =  RtShUDIUserCntxtMgr::rtGetInstance();

		ap_cug_ref_info = new (nothrow)RtShUDIDataReference[m_max_data_repository+1];
    
		memset(ap_cug_ref_info,0,sizeof(RtShUDIDataReference)*(m_max_data_repository+1));
		
		RtRcT  l_ret =	lp_cntxt_mgr->rtGetCntxtDataRefInfo( ap_cntxt->cug_cntxt_id,
		                                                    	ap_cug_ref_info,
																											  	m_max_data_repository+1 );

  	if(l_ret != RT_SUCCESS)
		{
	  	 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	    	 "rtCreateCaseOrderedListForUser(),ERROR rtGetCntxtDataRefInfo(%d) Failed l_ret=%d to getCugDataRefInfo for cntxt_id=[%u] request",
	      	ap_cntxt->cug_cntxt_id,l_ret,ap_cntxt->session_cookie.self_indx);
	  	 delete[] ap_cug_ref_info; //Klock_work_fix@25-08-2016
			 return l_ret; 
		}
	}
	//create ordered list for user
  list<RtShUDIOrderedListElem>::iterator l_itr;
	for(RtU32T	l_cntr1	= 0;	l_cntr1	< RT_SH_MAX_SESS_CASE;++l_cntr1)
	{
		for(RtU32T	l_cntr2	= 0;	l_cntr2	< RT_SIP_METHOD_FOR_CTF;++l_cntr2)
		{
			ap_cntxt->case_ord_list[l_cntr1][l_cntr2].clear();
			
			l_itr	= m_case_ord_list[l_cntr1][l_cntr2].begin();
			while(l_itr !=	m_case_ord_list[l_cntr1][l_cntr2].end())
			{
				if((ap_cntxt->p_data_ref_arr[l_itr->udi_ref].data_state	&	RT_SH_UDI_DATA_STATE_VALID)	&&
						!(ap_cntxt->p_data_ref_arr[l_itr->udi_ref].data_state &	RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY))
				{
					ap_cntxt->case_ord_list[l_cntr1][l_cntr2].push_back(*l_itr);
					
					//HP_CR(DONE): give logs of list with info - l_cntr1,l_cntr2,data in *l_itr
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
					"rtCreateCaseOrderedListForUser(),usr_srvc Considered user cntxt_id=[%d],Session Case [=%d],Sip Method[=%d] Values-- ,AppEmm[%u],SrvIndic[=%s]",
				  ap_cntxt->session_cookie.self_indx,l_cntr1, l_cntr2, (*l_itr).data_ref_appl_info.app_enum, (*l_itr).data_ref_appl_info.app_srvc_indic);					
					
				}
			  else if(ap_cug_ref_info != NULL && ((ap_cug_ref_info[l_itr->udi_ref].data_state	&	RT_SH_UDI_DATA_STATE_VALID)	&&
						!(ap_cug_ref_info[l_itr->udi_ref].data_state &	RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY)))
				{
					ap_cntxt->case_ord_list[l_cntr1][l_cntr2].push_back(*l_itr);
					
				
			     /*--------------------------------------------------------------------------------------
							- 	TBD :: code review of Cug data handling
							-  RAJENDER::
							- 1. Assuming CUG_DATA_REF's are present with it's default data always
							- 2. WE are not validating its data_ref inside system_def data for default_data_flag
							--------------------------------------------------------------------------------------*/

					//if(!(mp_reg_data[l_itr->udi_ref].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT))
					{
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].data_state |=	RT_SH_UDI_DATA_STATE_VALID;
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].data_state |=	RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY;
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].p_data_ptr =  ap_cug_ref_info[l_itr->udi_ref].p_data_ptr;
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].elem_indx  =  ap_cug_ref_info[l_itr->udi_ref].elem_indx;
					}
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
					"rtCreateCaseOrderedListForUser() Cug data_ref=%d Considered  user cntxt_id=[%d],Session Case [=%d],Sip Method[=%d] Values-- ,AppEmm[%u],SrvIndic[=%s]",
				     l_itr->udi_ref,ap_cntxt->session_cookie.self_indx,l_cntr1, l_cntr2, (*l_itr).data_ref_appl_info.app_enum, (*l_itr).data_ref_appl_info.app_srvc_indic);					
					
				}

				else if((mp_reg_data[l_itr->udi_ref].orig_case.is_service_data_mandatory && (RT_SH_ORIG_CASE ==	l_cntr1))
																			||
							(mp_reg_data[l_itr->udi_ref].term_case.is_service_data_mandatory && (RT_SH_TERM_CASE ==	l_cntr1)) )
				{
					ap_cntxt->case_ord_list[l_cntr1][l_cntr2].push_back(*l_itr);

					if(!(mp_reg_data[l_itr->udi_ref].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT))
					{
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].data_state |=	RT_SH_UDI_DATA_STATE_VALID;
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].data_state |=	RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY;
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].p_data_ptr = mp_reg_data[l_itr->udi_ref].p_data_ptr;
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].elem_indx = mp_reg_data[l_itr->udi_ref].elem_indx;
					}

					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
					"rtCreateCaseOrderedListForUser(),sys_def srvc considrd  user cntxt_id=[%d],Session Case [=%d],Sip Method[=%d] Values-- ,AppEmm[%u],SrvIndic[=%s]",
				  ap_cntxt->session_cookie.self_indx,l_cntr1, l_cntr2, (*l_itr).data_ref_appl_info.app_enum, (*l_itr).data_ref_appl_info.app_srvc_indic);		
				}				
				else if( !mp_reg_data[l_itr->udi_ref].orig_case.is_service_data_mandatory 
																			&&
								 !mp_reg_data[l_itr->udi_ref].term_case.is_service_data_mandatory  
								)
				{
						//reset 
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].data_state &=	~RT_SH_UDI_DATA_STATE_VALID;
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].data_state &=	~RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY;
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].p_data_ptr = 	 NULL;
						ap_cntxt->p_data_ref_arr[l_itr->udi_ref].elem_indx 	=	 	0;
						
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
					"rtCreateCaseOrderedListForUser() neither state is valid nor a mandatory service user cntxt_id=[%d],Session Case [=%d],Sip Method[=%d] Values-- ,AppEmm[%u],SrvIndic[=%s]",
				  ap_cntxt->session_cookie.self_indx,l_cntr1, l_cntr2, (*l_itr).data_ref_appl_info.app_enum, (*l_itr).data_ref_appl_info.app_srvc_indic);		
					
				}
				else
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
					"rtCreateCaseOrderedListForUser() Already Handled In elseIf Condition service user cntxt_id=[%d],Session Case [=%d],Sip Method[=%d] Values-- ,AppEmm[%u],SrvIndic[=%s]",
				  ap_cntxt->session_cookie.self_indx,l_cntr1, l_cntr2, (*l_itr).data_ref_appl_info.app_enum, (*l_itr).data_ref_appl_info.app_srvc_indic);		
				
				}
				l_itr++;
			}//end of while				
		}//end of inner for
	}//end of outer for	
	
	if(ap_cug_ref_info)
	{
		 //if(m_max_data_repository+1 > 1) //Klock_work_fix@25-08-2016
    		delete[] ap_cug_ref_info;
		// else
	  		//delete  ap_cug_ref_info ;
	}
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtCreateCaseOrderedListForUser()  EXIT  cntxt_id=[%u]",
	 ap_cntxt->session_cookie.self_indx);

	return RT_SUCCESS;
}	


/*******************************************************************************

  FUNCTION NAME : rtCreateCaseOrderedListUsingConfig()
 
  DESCRIPTION   : This functions is used to create case (orig, term) ordered 
									list as per configuration i.e using mp_reg_data
 
  ARGUMENTS     : 
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtCreateCaseOrderedListUsingConfig()
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	"RtShUDIDataPoolMgr::rtCreateCaseOrderedListUsingConfig() ENTER ");
  
	RtRcT l_ret_val 				= RT_SUCCESS;
	RtPrecSrvIndicMultiMap	l_case_ord_multimap[RT_SH_MAX_SESS_CASE][RT_SIP_METHOD_FOR_CTF];
	 
	RtShUDIOrderedListElem l_sh_udi_ord_list_elem_orig;	
	RtShUDIOrderedListElem l_sh_udi_ord_list_elem_term;

	
	for (RtU32T l_cnt= (m_max_data_ref+1);l_cnt<m_max_data_indx;l_cnt++)
	{
		if(mp_reg_data[l_cnt].orig_case.is_data_valid)
		{
			for(RtS8T l_sip_method_cnt=0; l_sip_method_cnt < RT_SIP_METHOD_FOR_CTF ; l_sip_method_cnt++)
			{
				if(mp_reg_data[l_cnt].orig_case.sip_method_arr[l_sip_method_cnt])
				{
					l_sh_udi_ord_list_elem_orig.udi_ref = mp_reg_data[l_cnt].udi_ref;
					l_sh_udi_ord_list_elem_orig.data_ref_appl_info = mp_reg_data[l_cnt].data_ref_appl_info;
									
					l_case_ord_multimap[RT_SH_ORIG_CASE][l_sip_method_cnt].insert(make_pair(mp_reg_data[l_cnt].orig_case.precedence, l_sh_udi_ord_list_elem_orig));
				}
			}			
		
		
		}
		
		//terminating
		
		if(mp_reg_data[l_cnt].term_case.is_data_valid)
		{
			for(RtS8T l_sip_method_cnt=0; l_sip_method_cnt < RT_SIP_METHOD_FOR_CTF ; l_sip_method_cnt++)
			{
				if(mp_reg_data[l_cnt].term_case.sip_method_arr[l_sip_method_cnt])
				{
          l_sh_udi_ord_list_elem_term.udi_ref = mp_reg_data[l_cnt].udi_ref;
					l_sh_udi_ord_list_elem_term.data_ref_appl_info = mp_reg_data[l_cnt].data_ref_appl_info;					
					
					l_case_ord_multimap[RT_SH_TERM_CASE][l_sip_method_cnt].insert(make_pair(mp_reg_data[l_cnt].term_case.precedence, l_sh_udi_ord_list_elem_term));
				}
			}			
		}
		
	}//end of outermost for loop


//creation of  m_case_ord_list
//	originating

	//RtShUDIOrderedListElem	l_list_elem;
	//list<RtShUDIOrderedListElem>	l_list;
	for(RtS8T l_sip_method_cnt=0; l_sip_method_cnt < RT_SIP_METHOD_FOR_CTF ; l_sip_method_cnt++)
	{
		//Originating
		for(RtPrecSrvIndicMultiMapItr l_itr = l_case_ord_multimap[RT_SH_ORIG_CASE][l_sip_method_cnt].begin();l_itr != l_case_ord_multimap[RT_SH_ORIG_CASE][l_sip_method_cnt].end();l_itr++)
		{
			m_case_ord_list[RT_SH_ORIG_CASE][l_sip_method_cnt].push_back(l_itr->second);
			
			//HP_CR(DONE):: alo put RtShUDIDataRefApplInfo in logs
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
 			"RtShUDIDataPoolMgr::rtCreateCaseOrderedListUsingConfig() added udi_ref = %u to m_case_ord_list[RT_SH_ORIG_CASE][%d],AppEmm[%u],SrvIndic[=%s]",
 		  l_itr->second.udi_ref,l_sip_method_cnt,(*l_itr).second.data_ref_appl_info.app_enum, (*l_itr).second.data_ref_appl_info.app_srvc_indic);
			     
		}
		
		//	terminating 
		for(RtPrecSrvIndicMultiMapItr l_itr = l_case_ord_multimap[RT_SH_TERM_CASE][l_sip_method_cnt].begin();l_itr != l_case_ord_multimap[RT_SH_TERM_CASE][l_sip_method_cnt].end();l_itr++)
		{
			m_case_ord_list[RT_SH_TERM_CASE][l_sip_method_cnt].push_back(l_itr->second); 

			//HP_CR(DONE):: alo put RtShUDIDataRefApplInfo in logs
 						
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
 			"RtShUDIDataPoolMgr::rtCreateCaseOrderedListUsingConfig() added udi_ref = %u to m_case_ord_list[RT_SH_TERM_CASE][%d],AppEmm[%u],SrvIndic[=%s]",
 		  l_itr->second.udi_ref,l_sip_method_cnt,(*l_itr).second.data_ref_appl_info.app_enum, (*l_itr).second.data_ref_appl_info.app_srvc_indic);
			
		}
		
	}
			
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtCreateCaseOrderedListUsingConfig() Exit ret_val = %d",l_ret_val);
	
	return l_ret_val;
}

/*******************************************************************************

  FUNCTION NAME : rtInvokeSrvcDataEncFunc()
 
  DESCRIPTION   : 
									
 
  ARGUMENTS     : 
 
  RETURN        : RtRcT
                      
 ******************************************************************************/

RtRcT 			RtShUDIDataPoolMgr::rtInvokeSrvcDataEncFunc(RtShUDIRef a_udi_ref,
																												void* ap_input_buffer,
																												RtU32T a_input_buffer_size,
																												void* ap_encoded_buffer,
																												RtU32T& ar_encoded_buffer_size,
																												RtBoolT& ar_encoded_buffer_same_as_input)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtInvokeSrvcDataEncFunc() ENTER for udi_ref=%d",a_udi_ref);
	
	RtRcT l_ret_val = RT_SUCCESS;
	ar_encoded_buffer_same_as_input	=	true;
	
	if(NULL	!=	mp_reg_data[a_udi_ref].enc_func_ptr)
	{
		l_ret_val = mp_reg_data[a_udi_ref].enc_func_ptr(ap_input_buffer,
																										a_input_buffer_size,
																										&ap_encoded_buffer,
																										ar_encoded_buffer_size,
																										ar_encoded_buffer_same_as_input);
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtInvokeSrvcDataEncFunc() Exit ret_val = %d",l_ret_val);
	
	return l_ret_val;
	
}																												



//NEW
/*******************************************************************************

  FUNCTION NAME : rtInvokeSrvcDataProvValidationFunc()
 
  DESCRIPTION   : This function is called whenever any operation from provisioning intf is invoked
								  This functions internally invokes the validation call back function registered
									by application during registration time
									
 
  ARGUMENTS     : RtU8T a_udi_ref,RtShProvIntfMsgOpcode a_opcode,void* ap_input_buffer
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT RtShUDIDataPoolMgr::rtInvokeSrvcDataProvValidationFunc(RtU8T a_udi_ref,RtShProvIntfMsgOpcode a_opcode,void* ap_input_buffer)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtInvokeSrvcDataProvValidationFunc() ENTER for a_udi_ref=%d,a_opcode=%d",a_udi_ref,a_opcode);
	
	RtRcT l_ret_val = RT_SUCCESS;

	if(mp_reg_data[a_udi_ref].prov_valid_func_ptr != NULL)
	{
		l_ret_val = mp_reg_data[a_udi_ref].prov_valid_func_ptr(a_opcode,ap_input_buffer);
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 	"RtShUDIDataPoolMgr::rtInvokeSrvcDataProvValidationFunc() Validation function NULL a_udi_ref=%d,a_opcode=%d prov_valid_func_ptr is NULL",a_udi_ref,a_opcode);
	
	}
	
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtInvokeSrvcDataProvValidationFunc() Exit a_udi_ref=%d,a_opcode=%d ret_val = %d",a_udi_ref,a_opcode,l_ret_val);
	
	return l_ret_val;
	
}



//NEW
/*******************************************************************************

  FUNCTION NAME : rtInvokeSrvcDataProvEncFunc()
 
  DESCRIPTION   : This function is called whenever any operation from provisioning intf is invoked
								  This functions internally invokes the enc call back function registered
									by application during registration time
									
 
  ARGUMENTS     : RtU8T a_udi_ref,RtShProvIntfMsgOpcode a_opcode,void* ap_input_buffer,void** app_encoded_buffer,RtU32T& ar_encoded_buffer_size
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT 			RtShUDIDataPoolMgr::rtInvokeSrvcDataProvEncFunc(RtU8T a_udi_ref,RtShProvIntfMsgOpcode a_opcode,void* ap_input_buffer,void** app_encoded_buffer,RtU32T& ar_encoded_buffer_size,RtBoolT& ar_is_memory_allocated_for_encoded_buffer)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtInvokeSrvcDataProvEncFunc() ENTER for a_udi_ref=%d,a_opcode=%d",a_udi_ref,a_opcode);
	
	RtRcT l_ret_val = RT_SUCCESS;
	ar_is_memory_allocated_for_encoded_buffer = false;

	if(mp_reg_data[a_udi_ref].prov_enc_func_ptr != NULL)
	{
		l_ret_val = mp_reg_data[a_udi_ref].prov_enc_func_ptr(a_opcode,ap_input_buffer,app_encoded_buffer,ar_encoded_buffer_size,ar_is_memory_allocated_for_encoded_buffer);
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 	"RtShUDIDataPoolMgr::rtInvokeSrvcDataProvValidationFunc() Exit a_udi_ref=%d,a_opcode=%d enc_valid_func_ptr is NULL",a_udi_ref,a_opcode);
	
		
	}
	
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtInvokeSrvcDataProvEncFunc() Exit a_udi_ref=%d,a_opcode=%d ret_val = %d",a_udi_ref,a_opcode,l_ret_val);
	
	return l_ret_val;
	
}



//NEW
/*******************************************************************************

  FUNCTION NAME : rtInvokeSrvcDataProvDecFunc()
 
  DESCRIPTION   : This function is called whenever any operation from provisioning intf is invoked
								  This functions internally invokes the enc call back function registered
									by application during registration time
									
 
  ARGUMENTS     : RtU8T a_udi_ref,RtShProvIntfMsgOpcode a_opcode,void* ap_input_buffer,void** app_encoded_buffer,RtU32T& ar_encoded_buffer_size,RtBoolT& ar_is_dec_func_null
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtRcT 			RtShUDIDataPoolMgr::rtInvokeSrvcDataProvDecFunc(RtU8T a_udi_ref,RtShProvIntfMsgOpcode a_opcode,void* ap_input_buffer,void* ap_decoded_buffer,RtBoolT& ar_is_dec_func_null)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtInvokeSrvcDataProvDecFunc() ENTER for a_udi_ref=%d,a_opcode=%d",a_udi_ref,a_opcode);
	
	RtRcT l_ret_val = RT_SUCCESS;

	if(mp_reg_data[a_udi_ref].prov_dec_func_ptr != NULL)
	{
		l_ret_val = mp_reg_data[a_udi_ref].prov_dec_func_ptr(a_opcode,ap_input_buffer,ap_decoded_buffer);
	}
	else
	{
		ar_is_dec_func_null = true;
		
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 	"RtShUDIDataPoolMgr::rtInvokeSrvcDataProvDecFunc() a_udi_ref=%d,a_opcode=%d enc_valid_func_ptr is NULL",a_udi_ref,a_opcode);
		
	}
	
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtInvokeSrvcDataProvDecFunc() Exit a_udi_ref=%d,a_opcode=%d ret_val = %d",a_udi_ref,a_opcode,l_ret_val);
	
	return l_ret_val;
	
}


/*******************************************************************************

  FUNCTION NAME : rtIsBase64EncodingReqd()
 
  DESCRIPTION   : 
								  
									
									
 
  ARGUMENTS     : RtU8T a_udi_ref,RtShProvIntfMsgOpcode a_opcode,void* ap_input_buffer,void** app_encoded_buffer,RtU32T& ar_encoded_buffer_size
 
  RETURN        : RtRcT
                      
 ******************************************************************************/
RtBoolT RtShUDIDataPoolMgr::rtIsBase64EncodingReqd(RtShUDIRef a_udi_ref)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "RtShUDIDataPoolMgr::rtInvokeSrvcDataProvDecFunc() ENTER a_udi_ref=%d",a_udi_ref);
	
	if(mp_reg_data[a_udi_ref].flag	&	RT_SH_UDI_REG_DATA_BASE64_ENC_REQD)
	{
		return	true;
	}
	else
	{
		return false;
	}
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtGetCntxtData().
 *
 * DESCRIPTION   : Function will be called by RtUDIDumpCntxtData class to get status of pool containers
 *
 * INPUT         :  none
 *
 * OUTPUT        : RtCntxtDataStatArr& 
 *
 * RETURN        : void 
 *
 ******************************************************************************/

void		RtShUDIDataPoolMgr :: rtGetCntxtData(	RtCntxtDataStatArr&		ar_cntxt_stat)		
{
// 	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
// 	 "RtShUDIDataPoolMgr::rtGetCntxtData() ENTER");

	for(RtU32T l_cnt =0; l_cnt < m_max_data_indx; l_cnt++)
	{			
		if(mp_valid_reg_data[l_cnt])
		{
			mp_data_pool_container[l_cnt]->rtGetCntxtData(ar_cntxt_stat);
		}	
	}
}


//UDI_PHASE2
/*******************************************************************************
 *
 * FUNCTION NAME : rtPrepareListOfServices().
 *
 * DESCRIPTION   : Function provides list of services provisioned for a user.
 *
 * INPUT         :  RtShUDIDataReference* ap_data_ref_arr, RtU32T a_num_data_ref
 *
 * OUTPUT        :  RtS8T* ap_srv_list
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtPrepareListOfServices(RtShUDIDataReference* ap_data_ref_arr, RtU32T a_num_data_ref, RtS8T* ap_srv_list)		
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER : rtPrepareListOfServices() num_data_ref=%u",a_num_data_ref);	


	service_list l_service_list;
	
	service_list::service_data_sequence& l_seq = l_service_list.service_data();
	
	for (RtU32T l_cnt= (m_max_data_ref+1);l_cnt<m_max_data_indx;l_cnt++)
	{
		//dharmendra
			 std:string l_srv_name(mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic);
		if(mp_reg_data[l_cnt].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT)
		{
			 RtBoolT 	 l_is_mandatory = true;			

				 l_is_mandatory = true;
			 service_data l_srv_data(l_srv_name, l_is_mandatory);
			 l_seq.push_back(l_srv_data);
			
		}
		//dharmendra
		if(ap_data_ref_arr[l_cnt].data_state	& RT_SH_UDI_DATA_STATE_VALID)
		{
			//std:string l_srv_name(mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic);
			RtBoolT 	 l_is_mandatory = false;			
			
			if(ap_data_ref_arr[l_cnt].data_state	& RT_SH_UDI_DATA_STATE_VALID_DUE_TO_MANDATORY)
			{
				l_is_mandatory = true;
				
			}
			
			service_data l_srv_data(l_srv_name, l_is_mandatory);
			l_seq.push_back(l_srv_data);
		}

	}

	std::ostringstream oss;
	service_list_ (oss, l_service_list);
	
	strcpy(ap_srv_list,oss.str().c_str());

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT : rtPrepareListOfServices() num_data_ref=%u",a_num_data_ref);	
	
	return RT_SUCCESS;
}


//UDI_PHASE2
/*******************************************************************************
 *
 * FUNCTION NAME : rtUpdateMandatorySrvData().
 *
 * DESCRIPTION   : Function updates Mandatory pool element by swapping pointers.
 *									It also updates xml file for service.
 *
 * INPUT         :  RtShUDIDataReference* ap_data_ref_arr, RtU32T a_num_data_ref
 *
 * OUTPUT        :  RtS8T* ap_srv_list
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtUpdateMandatorySrvData(RtShUDIRef a_udi_ref_val, void* ap_mand_data,RtU32T a_buffer_size, RtS8T* ap_xml_body,RtBoolT a_perform_xml_oper)		
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER: rtUpdateMandatorySrvData() a_udi_ref_val=%d,a_perform_xml_oper=%d",a_udi_ref_val,a_perform_xml_oper);	
	
	RtRcT 	l_rval	= RT_SUCCESS;
	
	if(a_perform_xml_oper)
	{			
				//CR:: maintain history COMMON_CONFIG_PATH/history/sh
				string 		l_filename 										= getenv("COMMON_CONFIG_PATH");
									l_filename										=	l_filename + "/sh/"+ mp_reg_data[a_udi_ref_val].data_ref_appl_info.app_srvc_indic +".xml";

				RtRcT 		l_rval												= RT_SUCCESS;
				RtS8T 		l_sys_time[RT_MAX_STRING_LEN] = {'\0'};

				struct tm *lp_tm=NULL;
				time_t clock;
				time(&clock);
				struct tm l_local_tm;
				lp_tm=localtime_r(&clock,&l_local_tm);
				RtS8T 		l_bkp_filename[RT_MAX_FILE_NAME_LEN] = {'\0'};
				FILE* 		lp_file;

				strftime(l_sys_time, sizeof(l_sys_time),(RtS8T *)("%d%m%Y_%T"),lp_tm);
				//KLOCWORK_FIX RM 03092013
 				snprintf(l_bkp_filename,sizeof(l_bkp_filename)-1,"%s/history/sh/%s_%s.xml",getenv("COMMON_CONFIG_PATH"), mp_reg_data[a_udi_ref_val].data_ref_appl_info.app_srvc_indic, l_sys_time);

				string	l_shell_command = "\\mv	";
	  						l_shell_command = l_shell_command + l_filename + " " + l_bkp_filename;

				if( 0 > system(l_shell_command.c_str()) )
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__,
					"ERROR::rtUpdateMandatorySrvData():- [ERROR-LEAVE] system() FAILED to execute '%s'", l_shell_command.c_str());
				}

				if(NULL == ap_xml_body)
				{
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
						"ERROR::rtUpdateMandatorySrvData() empty ap_xml_body a_udi_ref_val=%d",a_udi_ref_val);	
					l_rval	= RT_FAILURE;
				}
				else
				{
					//updating default service data file first
					FILE* 		lp_file;

					if((lp_file=fopen(l_filename.c_str(),"w")))
  				{
						gp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,  __LINE__,
						"rtUpdateMandatorySrvData():success opening  mandatory xml file = %s",l_filename.c_str());

  				}
  				else
  				{
						mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,  __LINE__,
						"rtUpdateMandatorySrvData():ERROR unable to open mandatory xml file");

						return  RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
  				}
					//Printing dafault service data
					fprintf(lp_file,"%s",ap_xml_body);
					fclose(lp_file);

					l_rval	= mp_data_pool_container[a_udi_ref_val]->rtUpdateMandatoryPoolElem(mp_reg_data[a_udi_ref_val].elem_indx, ap_mand_data, a_buffer_size);
					rtPointToPoolElem(a_udi_ref_val,mp_reg_data[a_udi_ref_val].elem_indx,&(mp_reg_data[a_udi_ref_val].p_data_ptr));//FAILURE case here is unlikely
				}
		}
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT : rtUpdateMandatorySrvData() a_udi_ref_val=%d,ret_val= %d",a_udi_ref_val,l_rval);	
	
	return l_rval;
}
//UDI_PHASE2
/*******************************************************************************
 *
 * FUNCTION NAME : rtUpdateMandatorySrvData().
 *
 * DESCRIPTION   : Function updates Mandatory pool element by swapping pointers.
 *									It also updates xml file for service.
 *
 * INPUT         :  RtShUDIDataReference* ap_data_ref_arr, RtU32T a_num_data_ref
 *
 * OUTPUT        :  RtS8T* ap_srv_list
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtCreateMandatorySrvData(RtShUDIRef a_udi_ref_val, void* ap_mand_data,RtU32T a_buffer_size, RtS8T* ap_xml_body)		
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER: rtCreateMandatorySrvData() a_udi_ref_val=%d",a_udi_ref_val);	

//DB_22102013 Default data absant so no need to create *.xml file
	if(mp_reg_data[a_udi_ref_val].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT)
				
	{
		return RT_SUCCESS;		
	}
	
	
	//CR:: maintain history COMMON_CONFIG_PATH/history/sh
	string 		l_filename 										= getenv("COMMON_CONFIG_PATH");
						l_filename										=	l_filename + "/sh/"+ mp_reg_data[a_udi_ref_val].data_ref_appl_info.app_srvc_indic +".xml";

	RtRcT 		l_rval												= RT_SUCCESS;
	RtS8T 		l_sys_time[RT_MAX_STRING_LEN] = {'\0'};

	struct tm *lp_tm=NULL;
	time_t clock;
	time(&clock);
	struct tm l_local_tm;
	lp_tm=localtime_r(&clock,&l_local_tm);
	RtS8T 		l_bkp_filename[RT_MAX_FILE_NAME_LEN] = {'\0'};
	FILE* 		lp_file;

	strftime(l_sys_time, sizeof(l_sys_time),(RtS8T *)("%d%m%Y_%T"),lp_tm);
	//KLOCWORK_FIX RM 03092013
 	snprintf(l_bkp_filename,sizeof(l_bkp_filename),"%s/history/sh/%s_%s.xml",getenv("COMMON_CONFIG_PATH"), mp_reg_data[a_udi_ref_val].data_ref_appl_info.app_srvc_indic, l_sys_time);

	string	l_shell_command = "\\mv	";
	  			l_shell_command = l_shell_command + l_filename + " " + l_bkp_filename;
	
	if( 0 > system(l_shell_command.c_str()) )
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__,
		"ERROR::rtCreateMandatorySrvData():- [ERROR-LEAVE] system() FAILED to execute '%s'", l_shell_command.c_str());
	}
	
	if(strlen(ap_xml_body)<=0)
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
			"ERROR::rtCreateMandatorySrvData() empty ap_xml_body a_udi_ref_val=%d",a_udi_ref_val);	
		l_rval	= RT_FAILURE;
	}
	else
	{
		//updating default service data file first
		FILE* 		lp_file;

		if((lp_file=fopen(l_filename.c_str(),"w")))
  	{
			gp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,  __LINE__,
			"rtCreateMandatorySrvData():success opening  mandatory xml file = %s",l_filename.c_str());

  	}
  	else
  	{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,  __LINE__,
			"rtCreateMandatorySrvData():ERROR unable to open mandatory xml file");

			return  RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
  	}
		//Printing dafault service data
		fprintf(lp_file,"%s",ap_xml_body);
		fclose(lp_file);
		l_rval = rtStoreDataRepositoryData(mp_reg_data[a_udi_ref_val].udi_ref,ap_mand_data,
																						mp_reg_data[a_udi_ref_val].max_data_size,
																						mp_reg_data[a_udi_ref_val].elem_indx,
																						&mp_reg_data[a_udi_ref_val].p_data_ptr,
																						false);
		if(RT_SUCCESS != l_rval)
		{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
				 "rtCreateMandatorySrvData() ERROR ::rtStoreDataRepositoryData() for Mandatory service FAILED, udi_ref = %d ",
				 mp_reg_data[a_udi_ref_val].udi_ref);

		}
		else
		{

				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_INFO,__FILE__,__LINE__,
				 "rtCreateMandatorySrvData()SUCCESSFULLY stored for Mandatory service data, udi_ref = %d,elem_indx=%u, p_data_ptr=%p",
				 mp_reg_data[a_udi_ref_val].udi_ref,mp_reg_data[a_udi_ref_val].elem_indx,mp_reg_data[a_udi_ref_val].p_data_ptr);

		}
	
	//	l_rval	= mp_data_pool_container[a_udi_ref_val]->rtUpdateMandatoryPoolElem(mp_reg_data[a_udi_ref_val].elem_indx, ap_mand_data, a_buffer_size);
	//	rtPointToPoolElem(a_udi_ref_val,mp_reg_data[a_udi_ref_val].elem_indx,&(mp_reg_data[a_udi_ref_val].p_data_ptr));//FAILURE case here is unlikely
	
	
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT : rtCreateMandatorySrvData() a_udi_ref_val=%d,ret_val= %d",a_udi_ref_val,l_rval);	
	
	return l_rval;
}
//UDI_PHASE2

/*******************************************************************************
 *
 * FUNCTION NAME : rtIsSrvMandatory().
 *
 * DESCRIPTION   : Function updates Mandatory pool element by swapping pointers.
 *
 * INPUT         :  RtShUDIDataReference* ap_data_ref_arr, RtU32T a_num_data_ref
 *
 * OUTPUT        :  RtS8T* ap_srv_list
 *
 * RETURN        : RtBoolT 
 *
 ******************************************************************************/
RtBoolT		RtShUDIDataPoolMgr :: rtIsSrvMandatory(RtShUDIRef a_udi_ref_val , RtU32T& ar_mand_pool_elem)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"Enter : rtIsSrvMandatory() a_udi_ref_val=%u",a_udi_ref_val);	

	ar_mand_pool_elem = mp_reg_data[a_udi_ref_val].elem_indx;
	return (mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory || mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory);
}

//UDI_PHASE2
/*******************************************************************************
 *
 * FUNCTION NAME : rtIsSrvDefaultDataReqd().
 *
 * DESCRIPTION   : Function to check if service has default data
 *
 * INPUT         : RtShUDIRef a_udi_ref_val  
 *
 * OUTPUT        :  true if exist otherwise false
 *
 * RETURN        : RtBoolT 
 *
 ******************************************************************************/

RtBoolT	RtShUDIDataPoolMgr :: rtIsSrvDefaultDataReqd(RtShUDIRef a_udi_ref_val)

{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"Enter : rtIsSrvDefaultDataReqd() a_udi_ref_val=%u",a_udi_ref_val);

	return ((mp_reg_data[a_udi_ref_val].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT) ? false:true);

	
	
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtEnableDftService().
 *
 * DESCRIPTION   : Function Enable/Disables default service.
 *
 * INPUT         :  RtShUDIRef, RtBoolT a_to_enable (true means enable, false means disable)
 *
 * OUTPUT        :  
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtEnableDftService(RtShUDIRef a_udi_ref_val, RtBoolT a_to_enable,RtBoolT a_perform_xml_oper,RtShUDISessCase a_session)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"Enter : rtEnableDftService() a_udi_ref_val=%u, a_to_enable =%d,a_perform_xml_oper=%d,sess_case =%d",a_udi_ref_val,a_to_enable,a_perform_xml_oper,a_session);
	
	if((a_session / RT_SH_MAX_SESS_CASE) == 0)
	{
		//OK Proceed
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
			"Enter : rtEnableDftService()ERROR::INVALID SESSION CASE a_udi_ref_val=%u, a_to_enable =%d,sess_case =%d",a_udi_ref_val,a_to_enable,a_session);
		return RT_FAILURE;
	}

	RtRcT 		l_ret_val 				= RT_SUCCESS;
	RtBoolT		l_is_flag_updated	=	false;
	RtBoolT		l_orig_flag	=	mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory;
	RtBoolT		l_term_flag	=	mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory;
	
	if(!m_enable_disable_in_progress)
	{		
		 m_enable_disable_in_progress	= true;
		
		
		if((RT_SH_ORIG_CASE == a_session || RT_SH_ALL_CASE == a_session) && mp_reg_data[a_udi_ref_val].orig_case.is_data_valid)
		{
			if(mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory && a_to_enable)
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"Enter : rtEnableDftService()ERROR::RT_SH_UDI_ERR_DEFAULT_SRV_ALREADY_ENABLED a_udi_ref_val=%u, a_to_enable =%d,sess_case =%d",a_udi_ref_val,a_to_enable,a_session);
				
				l_ret_val = RT_SH_UDI_ERR_DEFAULT_SRV_ALREADY_ENABLED;
			}
			else if(!mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory && !a_to_enable)
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"Enter : rtEnableDftService()ERROR::RT_SH_UDI_ERR_DEFAULT_SRV_ALREADY_DISABLED a_udi_ref_val=%u, a_to_enable =%d,sess_case =%d",a_udi_ref_val,a_to_enable,a_session);
				
				l_ret_val = RT_SH_UDI_ERR_DEFAULT_SRV_ALREADY_DISABLED;
			}
			else
			{
				mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=a_to_enable;
				l_is_flag_updated	=	true;
			}
		}
		
		if((RT_SH_TERM_CASE == a_session || RT_SH_ALL_CASE == a_session) && mp_reg_data[a_udi_ref_val].term_case.is_data_valid)
		{
			if(mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory && a_to_enable)
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"Enter : rtEnableDftService()ERROR::RT_SH_UDI_ERR_DEFAULT_SRV_ALREADY_ENABLED a_udi_ref_val=%u, a_to_enable =%d,sess_case =%d",a_udi_ref_val,a_to_enable,a_session);
				
				l_ret_val = RT_SH_UDI_ERR_DEFAULT_SRV_ALREADY_ENABLED;
			}
			else if(!mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory && !a_to_enable)
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
					"Enter : rtEnableDftService()ERROR::RT_SH_UDI_ERR_DEFAULT_SRV_ALREADY_DISABLED a_udi_ref_val=%u, a_to_enable =%d,sess_case =%d",a_udi_ref_val,a_to_enable,a_session);
				
				l_ret_val = RT_SH_UDI_ERR_DEFAULT_SRV_ALREADY_DISABLED;
			}
			else
			{
				mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=a_to_enable;
				l_is_flag_updated	=	true;
			}
			
		}
			
		if(l_is_flag_updated)
		{
			if(mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory || mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory)
			{
				mp_reg_data[a_udi_ref_val].flag |=	RT_SH_UDI_REG_DATA_SRVC_MANDATORY;
			}
			else
			{
				mp_reg_data[a_udi_ref_val].flag &=	~RT_SH_UDI_REG_DATA_SRVC_MANDATORY;
				mp_data_pool_container[a_udi_ref_val]->rtReturnPoolElem(mp_reg_data[a_udi_ref_val].elem_indx);
				mp_reg_data[a_udi_ref_val].elem_indx=	0;
				mp_reg_data[a_udi_ref_val].p_data_ptr=NULL;
				//TBD SHASHI:CR delete pool element
			}
			if(a_perform_xml_oper)
			{

				if(RT_SUCCESS != rtUpdateDataRefXml())
				{
					//Revert back 
					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
					"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d,sess_case =%d",a_udi_ref_val,a_to_enable,a_session);

					mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory	=	l_orig_flag;
					mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory	=	l_term_flag;				

					l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
				}

			}
			l_ret_val	=	RT_SUCCESS;
		}
		m_enable_disable_in_progress	= false;	
	}		
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
			"Enter : ENABLE/DISABLE already in progress for a_udi_ref_val=%u, a_to_enable =%d,sess_case =%d",a_udi_ref_val,a_to_enable,a_session);
		
		l_ret_val = RT_SH_UDI_ERR_ENABLE_DISABLE_ALREADY_IN_PROGRESS;
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT : rtEnableDftService() a_udi_ref_val=%u, a_to_enable =%d,l_ret_val=%d,sess_case =%d",a_udi_ref_val,a_to_enable,l_ret_val,a_session);
	return l_ret_val;
}
// 			
// 			if(mp_reg_data[a_udi_ref_val].orig_case.is_data_valid)
// 			{
// 					if((mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory)&&(mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory))
// 					{
// 						if(mp_reg_data[a_udi_ref_val].is_dft_srv_enbld)
// 						{
// 								 //single mandatory field
// 								 RtBoolT l_default_a_session=mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory;
// 							 	mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=a_to_enable;
// 		
// 								if(RT_SUCCESS != rtUpdateDataRefXml())
// 								{
// 								 //Revert back is_dft_srv_enbld
// 						 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 						 			"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 						 			mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=l_default_a_session;
// 					 				l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 								}
// 		
// 								m_enable_disable_in_progress	= false;		
// 						}
// 						else
// 						{
// 						//both flag
// 								RtBoolT l_default_a_session=mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory;
// 								mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=a_to_enable;
// 							
// 								RtBoolT l_dft_enabled = mp_reg_data[a_udi_ref_val].is_dft_srv_enbld;
// 								mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = a_to_enable;
// 
// 								if(RT_SUCCESS != rtUpdateDataRefXml())
// 								{
// 								//Revert back is_dft_srv_enbld
// 								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 								"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 									mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = l_dft_enabled;
// 									mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=l_default_a_session;
// 								l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 								}
// 		
// 							m_enable_disable_in_progress	= false;		
// 
// 						
// 						}
// 					}
// 					else if((mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory))
// 					{			
// 								//both flag
// 								RtBoolT l_default_a_session=mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory;
// 								mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=a_to_enable;
// 							
// 								RtBoolT l_dft_enabled = mp_reg_data[a_udi_ref_val].is_dft_srv_enbld;
// 								mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = a_to_enable;
// 
// 								if(RT_SUCCESS != rtUpdateDataRefXml())
// 								{
// 								//Revert back is_dft_srv_enbld
// 								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 								"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 									mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = l_dft_enabled;
// 									mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=l_default_a_session;
// 								l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 								}
// 		
// 							m_enable_disable_in_progress	= false;		
// 					
// 					}
// 					else if((mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory))
// 					{
// 					
// 						if(!mp_reg_data[a_udi_ref_val].is_dft_srv_enbld)
// 						{
// 						//both flag
// 								RtBoolT l_default_a_session=mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory;
// 								mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=a_to_enable;
// 							
// 								RtBoolT l_dft_enabled = mp_reg_data[a_udi_ref_val].is_dft_srv_enbld;
// 								mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = a_to_enable;
// 
// 								if(RT_SUCCESS != rtUpdateDataRefXml())
// 								{
// 								//Revert back is_dft_srv_enbld
// 								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 								"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 									mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = l_dft_enabled;
// 									mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=l_default_a_session;
// 								l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 								}
// 		
// 							m_enable_disable_in_progress	= false;		
// 
// 						}
// 						else
// 						{
// 						//mandatory flag
// 						//single mandatory field
// 								 RtBoolT l_default_a_session=mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory;
// 							 	mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=a_to_enable;
// 		
// 								if(RT_SUCCESS != rtUpdateDataRefXml())
// 								{
// 								 //Revert back is_dft_srv_enbld
// 						 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 						 			"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 						 			mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory=l_default_a_session;
// 					 				l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 								}
// 		
// 								m_enable_disable_in_progress	= false;		
// 				
// 						}					
// 					}
// 					else
// 					{
// 					//if not mandatory anyone handling latter
// 					}  
// 			}
// 			else
// 			{
// 			//return error
// 			}
// 			
// 		}	
// 		
// 		if(a_session == 1)
// 		{
// 		//a_session start
// 		
// 			if(mp_reg_data[a_udi_ref_val].term_case.is_data_valid)
// 			{
// 					if((mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory)&&(mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory))
// 					{
// 						if(mp_reg_data[a_udi_ref_val].is_dft_srv_enbld)
// 						{
// 								 //single mandatory field
// 								 RtBoolT l_default_a_session=mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory;
// 							 	mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=a_to_enable;
// 		
// 								if(RT_SUCCESS != rtUpdateDataRefXml())
// 								{
// 								 //Revert back is_dft_srv_enbld
// 						 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 						 			"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 						 			mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=l_default_a_session;
// 					 				l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 								}
// 		
// 								m_enable_disable_in_progress	= false;		
// 						}
// 						else
// 						{
// 						//both flag
// 								RtBoolT l_default_a_session=mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory;
// 								mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=a_to_enable;
// 							
// 								RtBoolT l_dft_enabled = mp_reg_data[a_udi_ref_val].is_dft_srv_enbld;
// 								mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = a_to_enable;
// 
// 								if(RT_SUCCESS != rtUpdateDataRefXml())
// 								{
// 								//Revert back is_dft_srv_enbld
// 								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 								"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 									mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = l_dft_enabled;
// 									mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=l_default_a_session;
// 								l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 								}
// 		
// 							m_enable_disable_in_progress	= false;		
// 
// 						
// 						}
// 					}
// 					else if((mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory))
// 					{			
// 								//both flag
// 								RtBoolT l_default_a_session=mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory;
// 								mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=a_to_enable;
// 							
// 								RtBoolT l_dft_enabled = mp_reg_data[a_udi_ref_val].is_dft_srv_enbld;
// 								mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = a_to_enable;
// 
// 								if(RT_SUCCESS != rtUpdateDataRefXml())
// 								{
// 								//Revert back is_dft_srv_enbld
// 								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 								"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 									mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = l_dft_enabled;
// 									mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=l_default_a_session;
// 								l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 								}
// 		
// 							m_enable_disable_in_progress	= false;		
// 					
// 					}
// 					else if((mp_reg_data[a_udi_ref_val].orig_case.is_service_data_mandatory))
// 					{
// 					
// 						if(!mp_reg_data[a_udi_ref_val].is_dft_srv_enbld)
// 						{
// 						//both flag
// 								RtBoolT l_default_a_session=mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory;
// 								mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=a_to_enable;
// 							
// 								RtBoolT l_dft_enabled = mp_reg_data[a_udi_ref_val].is_dft_srv_enbld;
// 								mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = a_to_enable;
// 
// 								if(RT_SUCCESS != rtUpdateDataRefXml())
// 								{
// 								//Revert back is_dft_srv_enbld
// 								mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 								"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 									mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = l_dft_enabled;
// 									mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=l_default_a_session;
// 								l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 								}
// 		
// 							m_enable_disable_in_progress	= false;		
// 
// 						}
// 						else
// 						{
// 						//mandatory flag
// 						//single mandatory field
// 								 RtBoolT l_default_a_session=mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory;
// 							 	mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=a_to_enable;
// 		
// 								if(RT_SUCCESS != rtUpdateDataRefXml())
// 								{
// 								 //Revert back is_dft_srv_enbld
// 						 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 						 			"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 						 			mp_reg_data[a_udi_ref_val].term_case.is_service_data_mandatory=l_default_a_session;
// 					 				l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 								}
// 		
// 								m_enable_disable_in_progress	= false;		
// 				
// 						}					
// 					}
// 					else
// 					{
// 					//if not mandatory anyone handling latter
// 					}  
// 			}
// 			else
// 			{
// 			//return error
// 			}
// 	 
// 		//a_session end	
// 		}
// 
// 		if(a_session==-1)
// 		{				
// 			if(a_to_enable && mp_reg_data[a_udi_ref_val].is_dft_srv_enbld)
// 			{
// 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 				"Enter : rtEnableDftService() Default already enabled a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 			l_ret_val = RT_SH_UDI_ERR_DEFAULT_SRV_ALREADY_ENABLED;
// 			}
// 			else if(!a_to_enable && !mp_reg_data[a_udi_ref_val].is_dft_srv_enbld)
// 			{
// 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 				"Enter : rtEnableDftService() Default already disabled a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 			l_ret_val = RT_SH_UDI_ERR_DEFAULT_SRV_ALREADY_DISABLED;
// 
// 			}
// 			else
// 			{
// 			RtBoolT l_dft_enabled = mp_reg_data[a_udi_ref_val].is_dft_srv_enbld;
// 			mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = a_to_enable;
// 
// 				if(RT_SUCCESS != rtUpdateDataRefXml())
// 				{
// 				//Revert back is_dft_srv_enbld
// 				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
// 					"Enter : rtEnableDftService() FAILURE updating config file a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 
// 				mp_reg_data[a_udi_ref_val].is_dft_srv_enbld = l_dft_enabled;
// 
// 				l_ret_val = RT_SH_UDI_ERR_CONFIG_FILE_UPDATION_FAILED;
// 				}
// 			}
// 	
// 		m_enable_disable_in_progress	= false;		
// 		}
// 	}
// 	else
// 	{
// 		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_ALERT, __FILE__,  __LINE__,
// 			"Enter : ENABLE/DISABLE already in progress for a_udi_ref_val=%u, a_to_enable =%d",a_udi_ref_val,a_to_enable);
// 		
// 		l_ret_val = RT_SH_UDI_ERR_ENABLE_DISABLE_ALREADY_IN_PROGRESS;
// 	}
// 
// 	
// 	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
// 		"EXIT : rtEnableDftService() a_udi_ref_val=%u, a_to_enable =%d,l_ret_val=%d",a_udi_ref_val,a_to_enable,l_ret_val);
// 	return l_ret_val;
// }
// 

//UDI_PHASE2
/*******************************************************************************
 *
 * FUNCTION NAME : rtUpdateDataRefXml()
 *
 * DESCRIPTION   : Function to update RtShDataRefConfig.xml on enablement/disablement of mandatory service.
 *
 * INPUT         : RtShUDIRef, RtBoolT a_to_enable (true means enable, false means disable)
 *
 * OUTPUT        :  
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtUpdateDataRefXml()
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER :  rtUpdateDataRefXml()");
	RtRcT l_ret_val = RT_SUCCESS;

	//Write xml file to <Service_name>.xml
	string 		l_filename  = getenv("COMMON_CONFIG_PATH");
						l_filename	+=	"/sh/RtShDataRefConfig.xml";

	RtS8T 		l_sys_time[RT_MAX_STRING_LEN] = {'\0'};
	struct tm *lp_tm=NULL;
	time_t clock;
	time(&clock);
	struct tm l_local_tm;
	lp_tm=localtime_r(&clock,&l_local_tm);
	RtS8T 		l_bkp_filename[RT_MAX_FILE_NAME_LEN] = {'\0'};
	FILE* 		lp_file;

	//Read file size before moving to history
	std::ifstream l_if_stream;
	RtU32T l_file_length = 0;
	l_if_stream.open(l_filename.c_str());      // open input file

	if(l_if_stream.fail())
	{
		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
		 "rtUpdateDataRefXml()-ERROR opening file = %s",l_filename.c_str());							
		 return RT_FAILURE;	

	}

	l_if_stream.seekg(0, std::ios::end);    // go to the end
	l_file_length = l_if_stream.tellg();      // report location (this is the length)
	l_if_stream.close();

	
	strftime(l_sys_time, sizeof(l_sys_time),(RtS8T *)("%d%m%Y_%T"),lp_tm);
	//KLOCWORK_FIX RM 03092013
 	snprintf(l_bkp_filename,sizeof(l_bkp_filename),"%s/history/sh/RtShDataRefConfig_%s.xml",getenv("COMMON_CONFIG_PATH"), l_sys_time);

	string	l_shell_command = "\\cp	";
	  			l_shell_command += l_filename;
					l_shell_command += " ";
					l_shell_command += l_bkp_filename;
	
	if( 0 > system(l_shell_command.c_str()) )
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__,
		"ERROR::rtUpdateDataRefXml():- [ERROR-LEAVE] system() FAILED to execute '%s'", l_shell_command.c_str());
	}

	
	RtS8T* l_file_content = new RtS8T[l_file_length + 100];//taking extra space


	sprintf(l_file_content,"<ROOT>\n\n <DATA_REFERENCES_INFO>\n\n");

	//Print non-transparent data first
	
	sprintf(l_file_content + strlen(l_file_content), "\t\t<NON_TRANSPARENT_DATA>\n\n");
	
	RtShUDIDataRefListItr l_itr;
	for(l_itr = m_reg_data_ref_list.begin(); l_itr != m_reg_data_ref_list.end() && *l_itr != 0; l_itr++)//0 is for transparent data
	{
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t<DATA_REFERENCE>\n");
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<DATA_REF_ID>%d</DATA_REF_ID>\n",mp_reg_data[*l_itr].ref_data_val);
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<NUM_DATA_REF_POOL_ELEM>%u</NUM_DATA_REF_POOL_ELEM>\n",mp_reg_data[*l_itr].max_num_data);
		
		if(mp_reg_data[*l_itr].flag & RT_SH_UDI_REG_DATA_INCLUDE_IN_SNR)
		{
			sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<IS_INCLUDED_IN_SNR>TRUE</IS_INCLUDED_IN_SNR>\n");
		}
		else
		{
			sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<IS_INCLUDED_IN_SNR>FALSE</IS_INCLUDED_IN_SNR>\n");
		}
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t</DATA_REFERENCE>\n\n");
	}
	
	sprintf(l_file_content + strlen(l_file_content), "\t\t</NON_TRANSPARENT_DATA>\n\n");
	
	//Print transparent data now
	sprintf(l_file_content + strlen(l_file_content), "\t\t<TRANSPARENT_DATA>\n\n\t\t\t<DATA_REF_ID>0</DATA_REF_ID>\n\t\t\t<IS_INCLUDED_IN_SNR>TRUE</IS_INCLUDED_IN_SNR>\n\n\t\t\t<SERVICE_DATA_GROUP>\n\n");
	RtU32T l_num = 0 ;
	for (RtU32T l_cnt= (m_max_data_ref+1);(l_cnt<m_max_data_indx) && (l_num < m_reg_srvc_indic_list.size());l_cnt++, l_num++)
	{
		
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<SERVICE_DATA>\n");
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<SERVICE_INDICATION>%s</SERVICE_INDICATION>\n",mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic);		
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<APPL_ENUM>%u</APPL_ENUM>\n",mp_reg_data[l_cnt].data_ref_appl_info.app_enum);
		
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<IS_SERVICE_ACTIVATED>TRUE</IS_SERVICE_ACTIVATED>\n");
		
		if(mp_reg_data[l_cnt].flag & RT_SH_UDI_REG_DATA_BASE64_ENC_REQD)
		{
			sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t<IS_BASE64_ENC_REQD>TRUE</IS_BASE64_ENC_REQD>\n");
		}
		else
		{
			sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t<IS_BASE64_ENC_REQD>FALSE</IS_BASE64_ENC_REQD>\n");		
		}
		
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t<NUM_DATA_REF_POOL_ELEM>%u</NUM_DATA_REF_POOL_ELEM>\n\n", mp_reg_data[l_cnt].max_num_data);

//orig case
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t<SESSION_ORIG_CASE>\n");

		if(mp_reg_data[l_cnt].orig_case.is_data_valid)
		{
			sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t\t<SERVICE_VALIDITY>TRUE</SERVICE_VALIDITY>\n");
			if(mp_reg_data[l_cnt].orig_case.is_service_data_mandatory)
			{
				sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t\t<IS_SERVICE_MANDATORY>TRUE</IS_SERVICE_MANDATORY>\n");
			}
			else
			{
				sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t\t<IS_SERVICE_MANDATORY>FALSE</IS_SERVICE_MANDATORY>\n");
			}	

			sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t<PRECEDENCE>%d</PRECEDENCE>\n", mp_reg_data[l_cnt].orig_case.precedence);

			for(RtU32T	l_cnt1 = 0;l_cnt1 < RT_SIP_METHOD_FOR_CTF;l_cnt1++)
			{
				if(mp_reg_data[l_cnt].orig_case.sip_method_arr[l_cnt1])
				sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t<METHOD>%s</METHOD>\n", (m_sip_method_arr[l_cnt1]).c_str());
			}
		}
		else
		{
			sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t\t<SERVICE_VALIDITY>FALSE</SERVICE_VALIDITY>\n");
		}	
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t</SESSION_ORIG_CASE>\n\n");

//Term case
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t<SESSION_TERM_CASE>\n");

		if(mp_reg_data[l_cnt].term_case.is_data_valid)
		{
			sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t\t<SERVICE_VALIDITY>TRUE</SERVICE_VALIDITY>\n");
			if(mp_reg_data[l_cnt].term_case.is_service_data_mandatory)
			{
				sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t\t<IS_SERVICE_MANDATORY>TRUE</IS_SERVICE_MANDATORY>\n");
			}
			else
			{
				sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t\t<IS_SERVICE_MANDATORY>FALSE</IS_SERVICE_MANDATORY>\n");
			}	

			sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t<PRECEDENCE>%d</PRECEDENCE>\n", mp_reg_data[l_cnt].term_case.precedence);

			for(RtU32T	l_cnt1 = 0;l_cnt1 < RT_SIP_METHOD_FOR_CTF;l_cnt1++)
			{
				if(mp_reg_data[l_cnt].term_case.sip_method_arr[l_cnt1])
				sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t<METHOD>%s</METHOD>\n", (m_sip_method_arr[l_cnt1]).c_str());
			}
		}
		else
		{
			sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t\t\t<SERVICE_VALIDITY>FALSE</SERVICE_VALIDITY>\n");
		}
			
		sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t</SESSION_TERM_CASE>\n\n");
		/* db for update tag of IS_SERVICE_DATA_FILE */
		
		//DB_21022013
		if((mp_reg_data[l_cnt].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT))
		 {
				sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<IS_DFT_DATA_REQD>FALSE</IS_DFT_DATA_REQD>\n");
		 }
		else
		{
					sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<IS_DFT_DATA_REQD>TRUE</IS_DFT_DATA_REQD>\n");
		}

		
		//Till here
		
		if(mp_reg_data[l_cnt].flag & RT_SH_UDI_REG_DATA_SRVC_MANDATORY)
		{
			//if((mp_reg_data[l_cnt].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT))
				//{
			//	sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<IS_DFT_DATA_REQD>FALSE</IS_DFT_DATA_REQD>\n");
			//	}
				if(!(mp_reg_data[l_cnt].flag & RT_SH_UDI_REG_DATA_DEFAULT_DATA_ABSENT))
				{
					//sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<IS_DFT_DATA_REQD>TRUE</IS_DFT_DATA_REQD>\n");
					if(strstr(mp_reg_data[l_cnt].dft_srv_data_file_path,".xml")==NULL)
					{
							strcpy(mp_reg_data[l_cnt].dft_srv_data_file_path,	mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic);
							sprintf(mp_reg_data[l_cnt].dft_srv_data_file_path + strlen(mp_reg_data[l_cnt].dft_srv_data_file_path),".xml");
					}		
					sprintf(l_file_content + strlen(l_file_content), "\t\t\t\t<DEFAULT_SERVICE_DATA_PATH>%s</DEFAULT_SERVICE_DATA_PATH>\n",mp_reg_data[l_cnt].dft_srv_data_file_path);

				}
		
		 }
			sprintf(l_file_content + strlen(l_file_content), "\t\t\t</SERVICE_DATA>\n\n");
	}
	
	sprintf(l_file_content + strlen(l_file_content), "\t\t</SERVICE_DATA_GROUP>\n\t</TRANSPARENT_DATA>\n</DATA_REFERENCES_INFO>\n</ROOT>");

	lp_file = fopen(l_filename.c_str(),"w");
	if(!lp_file )
  {
		gp_sys_agent->rtRaiseLog(RT_SH_PRV_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,  __LINE__,
		"rtProvRestCallBack():ERROR unable to open xml file = %s error=%s",l_filename.c_str(),strerror(errno));

		l_ret_val = RT_FAILURE;

  }
  else
  {
		gp_sys_agent->rtRaiseLog(RT_SH_PRV_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,  __LINE__,
		"rtProvRestCallBack():success opening xml file = %s",l_filename.c_str());
		//Printing dafault service data
		fprintf(lp_file,"%s",l_file_content);
		fclose(lp_file);
  }
	
	
	delete[] l_file_content;
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT :  rtUpdateDataRefXml()");
	return l_ret_val;
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtGetSrvIndAppEnumFromUdiRef()
 *
 * DESCRIPTION   : utility function.
 *
 * INPUT         : RtShUDIRef a_udi_ref
 *
 * OUTPUT        : RtS8T* ap_srvc_indic, RtU32T& ar_app_enum 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtGetSrvIndAppEnumFromUdiRef(RtShUDIRef a_udi_ref,RtS8T* ap_srvc_indic, RtU32T& ar_app_enum)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtGetSrvIndAppEnumFromUdiRef()  ENTER  udi_ref=[%u]",
	 a_udi_ref);
	
	
	strcpy(ap_srvc_indic,mp_reg_data[a_udi_ref].data_ref_appl_info.app_srvc_indic);
	ar_app_enum = mp_reg_data[a_udi_ref].data_ref_appl_info.app_enum;
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtGetSrvIndAppEnumFromUdiRef()  EXIT  udi_ref=[%u], Srv_indic [%s]",
	 a_udi_ref, ap_srvc_indic);
	
	return RT_SUCCESS; 
	
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtUpdateCkptForDftSrvData()
 *
 * DESCRIPTION   : utility function to update checkpointing for default service data.
 *								 called only on active AS
 *
 * INPUT         : RtShUDIRef a_udi_ref,RtS8T* ap_xml_body
 *
 * OUTPUT        : None 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtUpdateCkptForDftSrvData(RtShUDIRef a_udi_ref,RtS8T* ap_xml_body)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtUpdateCkptForDftSrvData()  ENTER  udi_ref=[%u]",a_udi_ref);
	
	RtRcT l_ret_val = RT_SUCCESS;

	std::ifstream l_ifstream;

//Tag-Suman-01Apr2013,Opensaf Ckpt Srvice Removal from App

//1.Get XML Path for corresponding Application
//2.Copy concerned XML to Remote Server
//3.Publish Corresponding Data

	string l_dft_data_file_path = getenv("COMMON_CONFIG_PATH");
	l_dft_data_file_path 	+= "/sh/";
	l_dft_data_file_path 	+= mp_reg_data[a_udi_ref].data_ref_appl_info.app_srvc_indic;
	l_dft_data_file_path 	+= ".xml";


	l_ifstream.open(l_dft_data_file_path.c_str());      // open input file

	if(l_ifstream.fail())
	{
		 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
		 "rtUpdateCkptForDftSrvData() ERROR opening file = %s  ",l_dft_data_file_path.c_str());							

		 l_ret_val = RT_FAILURE;

//		 break;

	}

///////////////////////
//Close the opened file
//////////////////////
	l_ifstream.close();  

//////////////////////
//Scp to Remote Server
//////////////////////

	l_ret_val 	= 	rtScpToRemoteController((RtS8T*)l_dft_data_file_path.c_str());
// 	RtCkptKeyData l_ckpt_key_data;
// 	memset(&l_ckpt_key_data,0, sizeof(l_ckpt_key_data))	;
// 	
// 
// 	
// 	l_ckpt_key_data.key=(char*)malloc(sizeof(char)*RT_CKPT_SECT_KEY_LEN);
// 	l_ckpt_key_data.data_buf=malloc(sizeof(RtCkptShUDIData));
// 	l_ckpt_key_data.data_size=sizeof(RtCkptShUDIData);
// 	bzero(l_ckpt_key_data.key,sizeof(char)*RT_CKPT_SECT_KEY_LEN);		
// 	sprintf(l_ckpt_key_data.key,"%d",mp_reg_data[a_udi_ref].data_ref_appl_info.app_enum);		
// 	
// 	
// 	RtCkptShUDIData* lp_ckpt_udi_data=(RtCkptShUDIData*)l_ckpt_key_data.data_buf;
// 	lp_ckpt_udi_data->is_ckpt_data_valid		= true;
// 	strcpy(lp_ckpt_udi_data->data,ap_xml_body);
	
	
//	l_ret_val = gp_mgl_intf->rtUpdateCkptData(m_ckpt_id,1,&l_ckpt_key_data);

 if(l_ret_val != RT_SUCCESS)
 {
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
		"rtUpdateCkptForDftSrvData():- rtScpToRemoteController Failed for Configuration[Path=%s] [RETVAL=%d]",l_dft_data_file_path.c_str(),l_ret_val);

 		l_ret_val = RT_FAILURE;

//		break;
	}
	else
	{

		RtUdiCkptUpdData l_ckpt_udi_data;
		memset(&l_ckpt_udi_data,0,sizeof(l_ckpt_udi_data));
		
		l_ckpt_udi_data.app_evt_id	= AMF_EVENT_UDI_CKPT_UPD;
		l_ckpt_udi_data.section_id	= mp_reg_data[a_udi_ref].data_ref_appl_info.app_enum;
		l_ckpt_udi_data.udi_ref 		= a_udi_ref;
		l_ckpt_udi_data.to_enable 	= false;
		
		// Publish the event
		l_ret_val = gp_mgl_intf->rtPublishEvent(AMF_EVENT_UDI_CKPT_UPD,
																						m_comp_name,
																						sizeof(RtUdiCkptUpdData),
																						(RtU8T*)&l_ckpt_udi_data);
		if(l_ret_val != RT_SUCCESS)
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,"rtUpdateCkptForDftSrvData() mp_mgl_intf->rtPublishEvent() Failed.Returnval=%d", l_ret_val);
			return l_ret_val;
		}		

		
	}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtUpdateCkptForDftSrvData()  EXIT  udi_ref=[%u],ret_val=%d",a_udi_ref,l_ret_val);

//	free (l_ckpt_key_data.key);
//	free (l_ckpt_key_data.data_buf);

	
	return l_ret_val;
}

/*******************************************************************************
 *
 * FUNCTION NAME : rtUpdateCkptDataRefConfigData()
 *
 * DESCRIPTION   : utility function to update checkpointing for RtShDataRefConfig.xml.
 *								 called only on active AS when there is operation from REST to
 *                 enable/disable mandatory service
 *
 * INPUT         : RtShUDIRef a_udi_ref_val, RtBoolT a_to_enable
 *
 * OUTPUT        : None 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtUpdateCkptDataRefConfigData(RtShUDIRef a_udi_ref_val, RtBoolT a_to_enable)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtUpdateCkptDataRefConfigData()  ENTER");

	RtRcT l_ret_val = RT_SUCCESS;
	
// 	RtCkptKeyData l_ckpt_key_data;
// 	memset(&l_ckpt_key_data,0, sizeof(l_ckpt_key_data))	;
// 	
// 	
// 	l_ckpt_key_data.key=(char*)malloc(sizeof(char)*RT_CKPT_SECT_KEY_LEN);
// 	l_ckpt_key_data.data_buf=malloc(sizeof(RtCkptShUDIData));
// 	l_ckpt_key_data.data_size=sizeof(RtCkptShUDIData);
// 	bzero(l_ckpt_key_data.key,sizeof(char)*RT_CKPT_SECT_KEY_LEN);		
// 	sprintf(l_ckpt_key_data.key,"%d",0);		
// 	
// 	
// 	RtCkptShUDIData* lp_ckpt_udi_data=(RtCkptShUDIData*)l_ckpt_key_data.data_buf;
// 	lp_ckpt_udi_data->is_ckpt_data_valid		= true;
// 
// 	RtU32T l_base_ctr = 0;
// 	
// 	for(RtU32T l_cnt=(m_max_data_ref+1);l_cnt <	m_max_data_indx;l_cnt++)
// 	{
// 		if((mp_reg_data[l_cnt].flag &	RT_SH_UDI_REG_DATA_SRVC_MANDATORY))
// 		{
// 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
// 			"rtUpdateCkptDataRefConfigData(): writting to ckpt udi_ref=%d,is_dft_srv_enbld=%d",
// 				mp_reg_data[l_cnt].udi_ref,mp_reg_data[l_cnt].is_dft_srv_enbld);
// 
// 			memcpy((lp_ckpt_udi_data->data + l_base_ctr*sizeof(RtShUDIRegData)),&mp_reg_data[l_cnt],sizeof(RtShUDIRegData));
// 			l_base_ctr++;
// 		}	
// 	}

//Tag-Suman-01Apr2013,Opensaf Ckpt Srvice Removal from App

		string l_config_path 	= getenv("COMMON_CONFIG_PATH");
		l_config_path 	+= "sh/RtShDataRefConfig.xml"; 

		std::ifstream l_ifstream;
		
		l_ifstream.open(l_config_path.c_str());      // open input file
			 
		if(l_ifstream.fail())
		{
			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			 "rtUpdateCkptDataRefConfigData() ERROR opening file = %s .Exit RT_FAILURE ",l_config_path.c_str());							

			 return RT_FAILURE;

			 

		}
		///////////////////////
		//Close the opened file
		//////////////////////
		l_ifstream.close();  
			
		//////////////////////
		//Scp to Remote Server
		//////////////////////
			
		l_ret_val 	= 	rtScpToRemoteController((RtS8T*)l_config_path.c_str());
		if(l_ret_val != RT_SUCCESS)
		{
			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
			 "rtUpdateCkptDataRefConfigData():- rtScpToRemoteController Failed for Configuration[Path=%s] Exit [RETVAL=%d]",l_config_path.c_str(),l_ret_val);

 			 l_ret_val = RT_FAILURE;


		 }
// 		 else
// 		 {
// 
// 			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
// 			 "rtCreateSections():- rtScpToRemoteController SUCCESS for Configuration[Path=%s] [RETVAL=%d]",l_config_path.c_str(),l_ret_val);
// 
// 		 }

//Tag-Suman-01Apr2013,Opensaf Ckpt Srvice Removal from App_END
	
	
// 	l_ret_val = gp_mgl_intf->rtUpdateCkptData(m_ckpt_id,1,&l_ckpt_key_data);
// 
// 	if(l_ret_val != RT_SUCCESS)
// 	{
// 		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
// 		"rtUpdateCkptDataRefConfigData():- mp_mgl_intf->rtUpdateCkptData() returned %d key=%s ",l_ret_val,l_ckpt_key_data.key);
// 		
// 	}
		else
		{

			RtUdiCkptUpdData l_ckpt_udi_data;
			memset(&l_ckpt_udi_data,0,sizeof(l_ckpt_udi_data));

			l_ckpt_udi_data.app_evt_id	= AMF_EVENT_UDI_CKPT_UPD;
			l_ckpt_udi_data.section_id	= 0;
			l_ckpt_udi_data.udi_ref 		= a_udi_ref_val;
			l_ckpt_udi_data.to_enable 	= a_to_enable;

			// Publish the event
			l_ret_val = gp_mgl_intf->rtPublishEvent(AMF_EVENT_UDI_CKPT_UPD,
																							m_comp_name,
																							sizeof(RtUdiCkptUpdData),
																							(RtU8T*)&l_ckpt_udi_data);
			if(l_ret_val != RT_SUCCESS)
			{
				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,"rtUpdateCkptDataRefConfigData() mp_mgl_intf->rtPublishEvent() Failed.Returnval=%d", l_ret_val);
			}		

		}
	
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtUpdateCkptDataRefConfigData()  EXIT ret_val=%d",l_ret_val);
	
	
//	free (l_ckpt_key_data.key);
//	free (l_ckpt_key_data.data_buf);

	return l_ret_val;
}


/*******************************************************************************
 *
 * FUNCTION NAME : rtUpdateDftSrvDataFromCkpt()
 *
 * DESCRIPTION   : utility function to update checkpointing for RtShDataRefConfig.xml.
 *								 - called on STANDBY AS when there is runtime update
 *                 - called on STANDBY AS at startup while doing sync
 *
 * INPUT         : RtS32T a_section_id, RtU8T a_udi_ref
 *
 * OUTPUT        : None 
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtUpdateDftSrvDataFromCkpt(RtS32T a_section_id, RtU8T a_udi_ref,RtBoolT a_perform_scp_oper)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
	"rtUpdateDftSrvDataFromCkpt() ENTER a_section_id=%d,a_udi_ref=%d,a_perform_scp_oper=%d",a_section_id,a_udi_ref,a_perform_scp_oper);

	 RtRcT l_ret_val = RT_SUCCESS;
	 
// 	RtCkptKeyData l_ckpt_key_data;
// 	memset(&l_ckpt_key_data,0, sizeof(l_ckpt_key_data))	;
// 	
// 	l_ckpt_key_data.key=(char*)malloc(sizeof(char)*RT_CKPT_SECT_KEY_LEN);
// 	l_ckpt_key_data.data_buf=malloc(sizeof(RtCkptShUDIData));
// 	l_ckpt_key_data.data_size=sizeof(RtCkptShUDIData);
// 	bzero(l_ckpt_key_data.key,sizeof(char)*RT_CKPT_SECT_KEY_LEN);		
// 	sprintf(l_ckpt_key_data.key,"%d",a_section_id);		
// 		
// 
// 	l_ret_val = gp_mgl_intf->rtReadCkptData(m_ckpt_id,1,&l_ckpt_key_data);
// 
// 	if(l_ret_val != RT_SUCCESS)
// 	{
// 
// 		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
// 		"rtUpdateDftSrvDataFromCkpt()-mp_mgl_intf->rtReadCkptData() Returned FAILURE for section with key=%s",l_ckpt_key_data.key);
// 
// 	}
// 	else
// 	{
// 		RtCkptShUDIData* lp_ckpt_udi_data=(RtCkptShUDIData*)l_ckpt_key_data.data_buf;
// 		
// 		RtS8T *lp_mand_data;
// 		RtU32T	l_servc_index  		= 0;
// 		RtU32T	l_servc_data_len  = 0;
// 		
// 		if(lp_ckpt_udi_data->is_ckpt_data_valid)
// 		{
// 			//encode xml file into structure
// 			l_ret_val =	rtEncodeServiceData(lp_ckpt_udi_data->data,
// 																			mp_reg_data[a_udi_ref].data_ref_appl_info.app_srvc_indic,
// 																			true,
// 																			l_servc_index,
// 																			l_servc_data_len,
// 																			(void **)&lp_mand_data);
// 
// 			if(RT_SUCCESS != l_ret_val)
// 			{
// 				mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
// 				"rtUpdateDftSrvDataFromCkpt()-encode failed for service=%s section with key=%s",mp_reg_data[a_udi_ref].data_ref_appl_info.app_srvc_indic,l_ckpt_key_data.key);
// 
// 			}
// 			else
// 			{
// 				l_ret_val =	rtUpdateMandatorySrvData(mp_reg_data[a_udi_ref].udi_ref, lp_mand_data,l_servc_data_len,lp_ckpt_udi_data->data);
// 			
// 				//CR check whethe to delete or free
// 				delete lp_mand_data;
// 			}
// 		}
// 		else
// 		{
// 			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
// 			"rtUpdateDftSrvDataFromCkpt() checkpoint data not valid of CKPT_ID=%d,key=%s",m_ckpt_id,l_ckpt_key_data.key);
// 
// 		}
// 	}

	if(a_perform_scp_oper)
	{
	
		string l_dft_data_file_path = getenv("COMMON_CONFIG_PATH");
		l_dft_data_file_path 	+= "/sh/";
		l_dft_data_file_path 	+= mp_reg_data[a_udi_ref].data_ref_appl_info.app_srvc_indic;
		l_dft_data_file_path 	+= ".xml";


		//////////////////////
		//Scp From Remote Server
		//////////////////////

		l_ret_val 	= 	rtScpFromRemoteController((RtS8T*)l_dft_data_file_path.c_str());
		if(l_ret_val != RT_SUCCESS)
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
			"rtUpdateDftSrvDataFromCkpt() ENTER a_section_id=%d,a_udi_ref=%d,a_perform_scp_oper=%d,Failed for Configuration[Path=%s] .Exit [RETVAL=%d]",a_section_id,a_udi_ref,a_perform_scp_oper ,l_dft_data_file_path.c_str(),l_ret_val);

 			return RT_FAILURE;

		
		}
		

		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
			"rtUpdateDftSrvDataFromCkpt() ENTER a_section_id=%d,a_udi_ref=%d,a_perform_scp_oper=%d,SUCCESS for Configuration[Path=%s] ",a_section_id,a_udi_ref,a_perform_scp_oper ,l_dft_data_file_path.c_str());

		
			


	}
	
	
	
	std::ifstream l_ifstream;

	//Tag-Suman-01Apr2013,Opensaf Ckpt Srvice Removal from App

	//1.Get XML Path for corresponding Application
	//2.Copy concerned XML to Remote Server
	//3.Publish Corresponding Data

		string l_dft_data_file_path = getenv("COMMON_CONFIG_PATH");
		l_dft_data_file_path 	+= "/sh/";
		l_dft_data_file_path 	+= mp_reg_data[a_udi_ref].data_ref_appl_info.app_srvc_indic;
		l_dft_data_file_path 	+= ".xml";


		l_ifstream.open(l_dft_data_file_path.c_str());      // open input file

		if(l_ifstream.fail())
		{
			 mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
			 "rtUpdateDftSrvDataFromCkpt() a_section_id=%d,a_udi_ref=%d, ERROR opening file = %s,,a_perform_scp_oper=%dExit RT_FAILURE  ",a_section_id,a_udi_ref,l_dft_data_file_path.c_str(),a_perform_scp_oper);							

			 return RT_FAILURE;

	//		 break;

		}


		RtS8T 	l_data[RT_SH_UDI_MAX_CKPT_DATA_LEN];
		bzero(&l_data,RT_SH_UDI_MAX_CKPT_DATA_LEN);


		l_ifstream.seekg(0, std::ios::end);      //go to the end
		RtU32T l_file_length = l_ifstream.tellg();      // report location (this is the length)
		l_ifstream.seekg(0, std::ios::beg);    					 // go back to the beginning
		if(l_file_length	>=	RT_SH_UDI_MAX_CKPT_DATA_LEN)
		{
			//bzero(&l_ckpt_data.data,sizeof(l_ckpt_data.data));

			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL,__FILE__,__LINE__,
					"rtUpdateDftSrvDataFromCkpt() a_section_id=%d,a_udi_ref=%d,service data file size too large.File=%s,l_file_length=%u, max_size =%u,a_perform_scp_oper=%dExiting RT_FAILUR",a_section_id,a_udi_ref,l_dft_data_file_path.c_str(),l_file_length, RT_SH_UDI_MAX_CKPT_DATA_LEN,a_perform_scp_oper);							

			l_ifstream.close();

			return RT_FAILURE;


		}


		l_ifstream.read(l_data, l_file_length); 
		l_ifstream.close(); 

		RtS8T *lp_mand_data;
		RtU32T	l_servc_index  		= 0;
		RtU32T	l_servc_data_len  = 0;


		//encode xml file into structure
		l_ret_val =	rtEncodeServiceData(l_data,
																		mp_reg_data[a_udi_ref].data_ref_appl_info.app_srvc_indic,
																		true,
																		l_servc_index,
																		l_servc_data_len,
																		(void **)&lp_mand_data);

		if(RT_SUCCESS != l_ret_val)
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, 
			"rtUpdateDftSrvDataFromCkpt()-a_section_id=%d,a_udi_ref=%d,encode failed for service=%s ,a_perform_scp_oper=%d Exiting RT_FAILURE",a_section_id,a_udi_ref,mp_reg_data[a_udi_ref].data_ref_appl_info.app_srvc_indic,a_perform_scp_oper);

			return RT_FAILURE;

		}

			l_ret_val =	rtUpdateMandatorySrvData(mp_reg_data[a_udi_ref].udi_ref, lp_mand_data,l_servc_data_len,l_data,false);

			//CR check whethe to delete or free
			delete lp_mand_data;

	
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, 
			"rtUpdateDftSrvDataFromCkpt() EXITl_ret_val=%d,a_perform_scp_oper=%d",l_ret_val,a_perform_scp_oper);
		
// free (l_ckpt_key_data.key);
// 	free (l_ckpt_key_data.data_buf);
	
	return l_ret_val;
}			


/*******************************************************************************
 *
 * FUNCTION NAME : rtPrepareHostedSrvList().
 *
 * DESCRIPTION   : Function prepares service list request hosted by AS
 *
 * INPUT         :  RtShProvIntfMsgOpcode a_opcode, 
 *
 * OUTPUT        :  RtS8T* ap_xml_body
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr ::  rtPrepareHostedSrvList(RtShProvIntfMsgOpcode a_opcode, RtS8T* ap_xml_body)
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER rtPrepareHostedSrvList() a_opcode = %u",a_opcode);
	
	RtRcT l_ret_val = RT_SUCCESS;
	RtU32T l_num =0;
	service_list l_service_list;//XSD autogenerated class
	
	service_list::service_data_sequence& l_seq = l_service_list.service_data ();
	
	switch(a_opcode)
	{
		case	RT_O_SH_UDI_PROV_INTF_VIEW_ENABLED_REQ:
		{
			//for (RtU32T l_cnt= (m_max_data_ref+1);l_cnt<m_max_data_indx;l_cnt++)
			//tag_Shashi: changed as list was printing invalid entries
			l_num = 0 ;
			for (RtU32T l_cnt= (m_max_data_ref+1);(l_cnt<m_max_data_indx) && (l_num < m_reg_srvc_indic_list.size());l_cnt++, l_num++)
			{
				if(mp_reg_data[l_cnt].term_case.is_service_data_mandatory || mp_reg_data[l_cnt].orig_case.is_service_data_mandatory)
				{
					string l_srv_name(mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic);
					RtBoolT 	 l_is_mandatory = true;			

					service_data l_srv_data(l_srv_name, l_is_mandatory);
					l_seq.push_back(l_srv_data);

					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
						"rtPrepareHostedSrvList() adding service[%s] to ENABLED list a_opcode = %u,",mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic,a_opcode);

				}
			}
		
		}break;   
		case	RT_O_SH_UDI_PROV_INTF_VIEW_DISABLED_REQ:  
		{
			//for (RtU32T l_cnt= (m_max_data_ref+1);l_cnt<m_max_data_indx;l_cnt++)
			//tag_Shashi: changed as list was printing invalid entries
			l_num = 0 ;
			for (RtU32T l_cnt= (m_max_data_ref+1);(l_cnt<m_max_data_indx) && (l_num < m_reg_srvc_indic_list.size());l_cnt++, l_num++)
			{
				if(!mp_reg_data[l_cnt].term_case.is_service_data_mandatory && !mp_reg_data[l_cnt].orig_case.is_service_data_mandatory)
				{
					string l_srv_name(mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic);
					RtBoolT 	 l_is_mandatory = false;			

					service_data l_srv_data(l_srv_name, l_is_mandatory);
					l_seq.push_back(l_srv_data);

					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
						"rtPrepareHostedSrvList() adding service[%s] to DISABLED list a_opcode = %u,",mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic,a_opcode);

				}

			}
		
		}break;   
// 		case	RT_O_SH_UDI_PROV_INTF_VIEW_INVALID_REQ:  
// 		{
// 			//for (RtU32T l_cnt= (m_max_data_ref+1);l_cnt<m_max_data_indx;l_cnt++)
// 			//tag_Shashi: changed as list was printing invalid entries
// 			//TBD_DB_13022013 this case shall be removed as RT_O_SH_UDI_PROV_INTF_VIEW_DISABLED_REQ serves same purpose
// 			//EDIT provisioning Interface document also
// 			l_num = 0 ;
// 			for (RtU32T l_cnt= (m_max_data_ref+1);(l_cnt<m_max_data_indx) && (l_num < m_reg_srvc_indic_list.size());l_cnt++, l_num++)
// 			{
// 				if(!(mp_reg_data[l_cnt].flag & RT_SH_UDI_REG_DATA_SRVC_MANDATORY))
// 				{
// 					string l_srv_name(mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic);
// 					RtBoolT 	 l_is_mandatory = false;			
// 
// 					service_data l_srv_data(l_srv_name, l_is_mandatory);
// 					l_seq.push_back(l_srv_data);
// 
// 					mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
// 						"rtPrepareHostedSrvList() adding service[%s] to INVALID list a_opcode = %u,",mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic,a_opcode);
// 
// 				}
// 
// 
// 			}
// 		
// 		}break;   
		case	RT_O_SH_UDI_PROV_INTF_VIEW_ALL_REQ: 	   
		{
			//for (RtU32T l_cnt= (m_max_data_ref+1);l_cnt<m_max_data_indx;l_cnt++)
			//tag_Shashi: changed as list was printing invalid entries
			l_num = 0 ;
			for (RtU32T l_cnt= (m_max_data_ref+1);(l_cnt<m_max_data_indx) && (l_num < m_reg_srvc_indic_list.size());l_cnt++, l_num++)
			{
				string l_srv_name(mp_reg_data[l_cnt].data_ref_appl_info.app_srvc_indic);
				RtBoolT 	 l_is_mandatory = false;			

				if(mp_reg_data[l_cnt].flag & RT_SH_UDI_REG_DATA_SRVC_MANDATORY)
				{
					l_is_mandatory = true;

				}

				service_data l_srv_data(l_srv_name, l_is_mandatory);
				l_seq.push_back(l_srv_data);


			}
		
		}break;
		default:
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_CRITICAL, __FILE__,  __LINE__,
				"EXIT : rtPrepareHostedSrvList() INVALID OPCODE a_opcode = %u, ret_val =%d",a_opcode,l_ret_val);

			return RT_FAILURE;

		}   

	}//End of switch
	
	std::ostringstream l_oss;
	service_list_ (l_oss, l_service_list);
	
	strcpy(ap_xml_body,l_oss.str().c_str());

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT rtPrepareHostedSrvList() a_opcode = %u, ret_val =%d",a_opcode,l_ret_val);
	
	return l_ret_val;
}
/*******************************************************************************
 *
 * FUNCTION NAME : rtGetUdiRefFromAppEnum()
 *
 * DESCRIPTION   : utility function.
 *
 * INPUT         : RtU32T a_app_enum
 *
 * OUTPUT        : RtShUDIRef* ap_udi_ref
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtGetUdiRefFromAppEnum(RtU32T a_app_enum,RtShUDIRef* ap_udi_ref )
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	 "rtGetUdiRefFromAppEnum()  ENTER  a_app_enum=[%u]",	 a_app_enum);
	
	RtBoolT l_found=false;
	for (RtU32T l_cnt=(m_max_data_ref+1);l_cnt	< m_max_data_indx;l_cnt++)
	{
	
		if( a_app_enum == mp_reg_data[l_cnt].data_ref_appl_info.app_enum )
		{
			*ap_udi_ref = mp_reg_data[l_cnt].udi_ref;
			l_found=true;

			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_INFO,__FILE__,__LINE__,
	  		"rtGetUdiRefFromAppEnum()  UDI Ref Matched in loop---> a_app_enum=[%u],udi_ref[%u],l_udi_ref[%u]", a_app_enum,mp_reg_data[l_cnt].udi_ref,*ap_udi_ref);

			break;
		}
		else
		{
			mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_INFO,__FILE__,__LINE__,
	  		"rtGetUdiRefFromAppEnum()  UDI Ref in loop---> a_app_enum=[%u],udi_ref[%u]", a_app_enum,mp_reg_data[l_cnt].udi_ref);

		}
	
	}
	
	if(!l_found)
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_ALERT,__FILE__,__LINE__,
	  "rtGetUdiRefFromAppEnum()  EXIT NO UDI re found for a_app_enum=[%u]", a_app_enum);
		
		return RT_FAILURE; 
	}
	else
	{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	  "rtGetUdiRefFromAppEnum()  EXIT  a_app_enum=[%u], udi_ref [%u]",
	  a_app_enum, *ap_udi_ref);
		
		return RT_SUCCESS; 
	
	}

}
/*******************************************************************************
 *
 * FUNCTION NAME : rtUpdateSysDftServiceCnfData()
 *
 * DESCRIPTION   : utility function.
 *
 * INPUT         : RtShUDIRef* ap_udi_ref
 *
 * OUTPUT        :
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtUpdateSysDftServiceCnfData(RtShUDIRef a_udi_ref, RtServiceData* ap_srvc_data )
{
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	  "RtShUserDataIntf :: rtUpdateSysDftServiceCnfData()-ENTER with a_udi_ref=[%u] for Update Service Conf Default Data",a_udi_ref);
 		
		RtShUDIRegData *lp_reg_data = &mp_reg_data[a_udi_ref];
  	
		lp_reg_data->orig_case.is_data_valid = ap_srvc_data->orig_case_data.srv_validity;
		lp_reg_data->term_case.is_data_valid = ap_srvc_data->term_case_data.srv_validity;

		//strcpy((l_reg_data.data_ref_appl_info.app_srvc_indic),	(l_data_ref_config.trans_data.srv_data[l_cnt].srv_ind));



		if(lp_reg_data->orig_case.is_data_valid)
		{
			lp_reg_data->orig_case.is_service_data_mandatory = ap_srvc_data->orig_case_data.is_srv_mandatory;
			lp_reg_data->orig_case.precedence  							 = ap_srvc_data->orig_case_data.precedence;
			//l_reg_data.orig_case.is_service_data_mandatory = l_data_ref_config.trans_data.srv_data[l_cnt].orig_case_data.is_srv_mandatory;
			//l_reg_data.orig_case.precedence 							 = l_data_ref_config.trans_data.srv_data[l_cnt].orig_case_data.precedence;

			for(RtU32T	l_ctr = 0;l_ctr < ap_srvc_data->orig_case_data.num_sip_method; l_ctr++)
			{
				lp_reg_data->orig_case.sip_method_arr[ap_srvc_data->orig_case_data.sip_method[l_ctr]]	= true;
			}
		}
		//term_case

		if(lp_reg_data->term_case.is_data_valid)
		{
			lp_reg_data->term_case.is_service_data_mandatory = ap_srvc_data->term_case_data.is_srv_mandatory;
			lp_reg_data->term_case.precedence  							 = ap_srvc_data->term_case_data.precedence;
			//l_reg_data.term_case.is_service_data_mandatory	= l_data_ref_config.trans_data.srv_data[l_cnt].term_case_data.is_srv_mandatory;
			//l_reg_data.term_case.precedence = l_data_ref_config.trans_data.srv_data[l_cnt].term_case_data.precedence;

			for(RtU32T	l_ctr = 0;l_ctr < ap_srvc_data->term_case_data.num_sip_method; l_ctr++)
			{
				lp_reg_data->term_case.sip_method_arr[ap_srvc_data->term_case_data.sip_method[l_ctr]]	= true;
			}
		}
		mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD,RT_LOG_LEVEL_DEBUG,__FILE__,__LINE__,
	  "RtShUserDataIntf :: rtUpdateSysDftServiceCnfData()-LEAVE with a_udi_ref=[%u] for Update Service Conf Default Data",a_udi_ref);
	
    return RT_SUCCESS;
}
/*******************************************************************************
 *
 * FUNCTION NAME : rtUpdateSrvData().
 *
 * DESCRIPTION   : Function updates Mandatory pool element by swapping pointers.
 *									.
 *
 * INPUT         :  RtShUDIDataReference* ap_data_ref_arr, RtU32T a_num_data_ref
 *
 * OUTPUT        :  RtS8T* ap_srv_list
 *
 * RETURN        : RtRcT 
 *
 ******************************************************************************/
RtRcT		RtShUDIDataPoolMgr :: rtUpdateSrvcData(RtShUDIRef a_udi_ref_val, void* ap_mand_data,RtU32T a_buffer_size)		
{
	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"ENTER: rtUpdateSrvData() a_udi_ref_val=%d,a_buffer_size=%d",a_udi_ref_val,a_buffer_size);	
	
	
	RtRcT l_rval	= mp_data_pool_container[a_udi_ref_val]->rtUpdateMandatoryPoolElem(mp_reg_data[a_udi_ref_val].elem_indx, ap_mand_data, a_buffer_size);
	rtPointToPoolElem(a_udi_ref_val,mp_reg_data[a_udi_ref_val].elem_indx,&(mp_reg_data[a_udi_ref_val].p_data_ptr));//FAILURE case here is unlikely

	mp_sys_agent->rtRaiseLog(RT_SH_UDI_MOD, RT_LOG_LEVEL_DEBUG, __FILE__,  __LINE__,
		"EXIT : rtUpdateSrvData(), a_udi_ref_val=%d,ret_val= %d",a_udi_ref_val,l_rval);	
	
	return l_rval;
}

#endif
