/**
 * job_state.h
 *
 *  Created on: 2011-7-15
 *      Author: auxten
 **/

#ifndef JOB_STATE_H_
#define JOB_STATE_H_

/// job init succeed
static const unsigned int   JOB_SUCC =                  101;
/// job init failed due to unsupported file type
static const unsigned int   JOB_FILE_TYPE_ERR =         102;
/// job init failed due to can't open the file
static const unsigned int   JOB_FILE_OPEN_ERR =         103;
/// job init failed due to recurse error
static const unsigned int   JOB_RECURSE_ERR =         	104;


#endif /** JOB_STATE_H_ **/
