/*
 * cxa_config.h
 *
 * Created: 5/25/2015 8:55:08 PM
 *  Author: Christopher Armenio
 */ 

#ifndef CXA_CONFIG_H_
#define CXA_CONFIG_H_

#define CXA_LOGGER_TIME_ENABLE
#define CXA_LOGGER_BUFFERLEN_BYTES		32

#define CXA_ASSERT_GPIO_FLASH_ENABLE
#define CXA_ASSERT_LINE_NUM_ENABLE
#define CXA_ASSERT_MSG_ENABLE

#define CXA_STATE_MACHINE_ENABLE_TIMED_STATES
#define CXA_STATE_MACHINE_ENABLE_LOGGING

#define CXA_LINE_ENDING			"\n"

#define CXA_RPC_NODE_MAX_NAME_LEN_BYTES				36
#define CXA_RPC_MSGFACTORY_POOL_NUM_MSGS			4


#endif /* CXA_CONFIG_H_ */
