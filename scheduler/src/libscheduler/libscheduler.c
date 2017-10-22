/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


// Priority queue for jobs
priqueue_t *queue;

// Scheme to use
scheme_t current_scheme;

// Numbers of cores
int num_cores;
int *available_cores;

/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements.
*/

typedef struct _job_t
{
	int job_id;				// ID of the job
	int priority;			// Priority of the job
	int core_id;			// ID of the core running the job
	int burst_time;		// How long the job runs for

	int arrival_time;	// Arrival time of the job
	int latency_time;	// How long it took to schedule the job
	int running_time;	// How long the job has been running for
	int end_time;			// When the job finished

	int finished;			// If the job has finished

} job_t;
// Comparator functions

int FCFS_comparator(const void *thing1, const void *thing2) {
	job_t *this;
	job_t *that;
	this = (job_t*)thing1;
	that = (job_t*)thing2;

	if (this->core_id < 0 && that->core_id >= 0) {
		return 1;
	} else if (that->core_id < 0 && this -> core_id >= 0) {
		return -1;
	} else {
		return (this->arrival_time - that->arrival_time);
	}
}

int SJF_comparator(const void *thing1, const void *thing2) {
	job_t *this;
	job_t *that;
	this = (job_t*)thing1;
	that = (job_t*)thing2;

	if (this->core_id < 0 && that->core_id >= 0) {
		return 1;
	} else if (that->core_id < 0 && this -> core_id >= 0) {
		return -1;
	} else {
		int this_life = this->running_time - this->running_time;
		int that_life = that->running_time - that->running_time;

		if (this_life == that_life)
			return (this->arrival_time - that->arrival_time);
		return (this_life-that_life);
	}
}

int PSJF_comparator(const void *thing1, const void *thing2) {
	job_t *this;
	job_t *that;
	this = (job_t*)thing1;
	that = (job_t*)thing2;

	int this_life = this->running_time - this->running_time;
	int that_life = that->running_time - that->running_time;

	if (this_life == that_life)
		return (this->arrival_time - that->arrival_time);
	return (this_life-that_life);
}

int PRI_comparator(const void *thing1, const void *thing2) {
	job_t *this;
	job_t *that;
	this = (job_t*)thing1;
	that = (job_t*)thing2;

	if (this->core_id < 0 && that->core_id >= 0) {
		return 1;
	} else if (that->core_id < 0 && this -> core_id >= 0) {
		return -1;
	} else {
		if (this->priority == that->priority)
			return (this->arrival_time - that->arrival_time);
		else
			return (this->priority - that->priority);
	}
}

int PPRI_comparator(const void *thing1, const void *thing2) {
	job_t *this;
	job_t *that;
	this = (job_t*)thing1;
	that = (job_t*)thing2;

	if (this->priority == that->priority)
		return (this->arrival_time - that->arrival_time);
	else
		return (this->priority - that->priority);
}

int RR_comparator(const void *thing1, const void *thing2) {
	return -1;
}

void tick(int time) {
	job_t *job;

	for (int i=0; i<priqueue_size(queue); i++) {
		job = (job_t*)priqueue_at(queue, i);
		if (job->core_id >= 0) {
			job->running_time = time - job->arrival_time;
			if (job->latency_time < 0)
				job->latency_time = time - job->arrival_time;
		} else if (job->end_time == time) {
			job->running_time = time - job->arrival_time;
		}
	}
}

int get_lowest_idle_core() {
	for (int i=0; i<num_cores; i++) {
		if (available_cores[i] < 0)
			return i;
	}
	return -1;
}

void set_next_job_nonpreemptive(int time) {
	job_t *job = NULL;
	int job_found;
	if (priqueue_size(queue) > 0) {
		int idle_core = get_lowest_idle_core();

		while (idle_core != -1) {
			job_found = 0;
			for (int i=0; i<priqueue_size(queue); i++) {
				job = (job_t*)priqueue_at(queue, i);
				if (!job->finished && job->core_id < 0) {
					job_found = 1;
					break;
				}
			}
			if (job_found) {
				job->core_id = idle_core;
				available_cores[idle_core] = job->job_id;
				idle_core = get_lowest_idle_core();
			} else {
				break;
			}
		}
	}
}

void set_next_job_preemptive(int time) {
	job_t *job = NULL;
	if (priqueue_size(queue) > 0) {
		for (int i=0; i<priqueue_size(queue); i++) {
			job = (job_t*)priqueue_at(queue, i);
			if (!job->finished && job->core_id < 0) {
				int idle_core = get_lowest_idle_core();
				if (idle_core != -1) {
					job->core_id = idle_core;
					available_cores[idle_core] = job->job_id;
				} else { // Find a job to be replaced
					job_t *running_job;
					for (int i=priqueue_size(queue)-1; i >-1; i++) {
						running_job = (job_t*)priqueue_at(queue, i);
						if (running_job->core_id > -1
							&& queue->comp(job, running_job) <= 0) {
							break;
						}
					}
					if (running_job != NULL) {
						int core_id = running_job->core_id;
						running_job->running_time = time - running_job->arrival_time;
						running_job->core_id = -1;
						if (running_job->running_time == 0)
							running_job->latency_time = -1;
						job->core_id = core_id;
						available_cores[core_id] = job->job_id;
					}
				}
			}
		}
	}
}

void set_next_job(int time) {
	switch(current_scheme) {
		case FCFS:
		case PRI:
		case SJF:
		case RR:
			set_next_job_nonpreemptive(time);
			break;
		case PPRI:
		case PSJF:
			set_next_job_preemptive(time);
			break;
	}
}

/**
  Initalizes the scheduler.

  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
	// Set number of cores and initialize cores to inactive
	num_cores = cores;
	available_cores = (int *) malloc(num_cores * sizeof(int));
	for (int i=0; i<num_cores; i++) {
		available_cores[i] = -1;
	}

	queue = (priqueue_t *)malloc(sizeof(priqueue_t));

	current_scheme = scheme;

	switch(current_scheme){
		case FCFS:
			priqueue_init(queue, FCFS_comparator);
			break;
		case SJF:
			priqueue_init(queue, SJF_comparator);
			break;
		case PSJF:
			priqueue_init(queue, PSJF_comparator);
			break;
		case PRI:
			priqueue_init(queue, PRI_comparator);
			break;
		case PPRI:
			priqueue_init(queue, PPRI_comparator);
		default:
			priqueue_init(queue, RR_comparator);
		break;
	}
}


/**
  Called when a new job arrives.

  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made.

 */
///////////////////////
// *******WIP******* //
///////////////////////
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
	// Create and initialize the job
	job_t* job = (job_t *) malloc(sizeof(job_t));
	job->job_id = job_number;
	job->priority = priority;
	job->core_id = -1;
	job->burst_time = running_time;
	job->arrival_time = time;
	job->latency_time = -1; // Set to -1 to allow for 0 latency
	job->running_time = 0;
	job->end_time = 0;
	job->finished = 0;

	priqueue_offer(queue, job);

	// Update cores
	set_next_job(time);

	// Update time
	tick(time);

	return job->core_id;
}


/**
  Called when a job has completed execution.

  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.

  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
	job_t *job;

	// Find the job
	for (int i=0; i<priqueue_size(queue); i++) {
		job_t *temp_job = priqueue_at(queue, i);
		if (temp_job->job_id == job_number) {
			job = temp_job;
			break;
		}
	}

	job->end_time = time;
	job->finished = 1;
	job->priority = 0;

	available_cores[core_id] = -1;
	job->core_id = -1;

	set_next_job(time);

	job->finished = 0;

	tick(time);

	job->finished = 1;

	return available_cores[core_id];
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.

  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
	return -1;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	job_t *job;
	int sum = 0;

	for (int i=0; i<priqueue_size(queue); i++) {
		job = (job_t *)priqueue_at(queue, i);
		sum += job->end_time - job->arrival_time - job->burst_time;
	}

	return (1.0*sum)/priqueue_size(queue);
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	job_t *job;
	int sum = 0;

	for (int i=0; i<priqueue_size(queue); i++) {
		job = (job_t *)priqueue_at(queue, i);
		sum += job->end_time - job->arrival_time;
	}

	return (1.0*sum)/priqueue_size(queue);
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	job_t *job;
	int sum = 0;

	for (int i=0; i<priqueue_size(queue); i++) {
		job = (job_t *)priqueue_at(queue, i);
		sum += job->latency_time;
	}

	return (1.0*sum)/priqueue_size(queue);
}


/**
  Free any memory associated with your scheduler.

  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
	while (priqueue_size(queue) > 0) {
		(job_t *)priqueue_poll(queue);
	}
	free(queue);
	free(available_cores);
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)

  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{

}
