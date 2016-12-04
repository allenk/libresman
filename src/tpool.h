/*
libresman - a multithreaded resource data file manager.
Copyright (C) 2014-2016  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef THREADPOOL_H_
#define THREADPOOL_H_

struct thread_pool;

/* type of the function accepted as work or completion callback */
typedef void (*tpool_callback)(void*);

#ifdef __cplusplus
extern "C" {
#endif

/* if num_threads == 0, auto-detect how many threads to spawn */
struct thread_pool *tpool_create(int num_threads);
void tpool_destroy(struct thread_pool *tpool);

/* optional reference counting interface for thread pool sharing */
int tpool_addref(struct thread_pool *tpool);
int tpool_release(struct thread_pool *tpool);	/* will tpool_destroy on nref 0 */

/* if begin_batch is called before an enqueue, the worker threads will not be
 * signalled to start working until end_batch is called.
 */
void tpool_begin_batch(struct thread_pool *tpool);
void tpool_end_batch(struct thread_pool *tpool);

/* if enqueue is called without calling begin_batch first, it will immediately
 * wake up the worker threads to start working on the enqueued item
 */
int tpool_enqueue(struct thread_pool *tpool, void *data,
		tpool_callback work_func, tpool_callback done_func);
/* clear the work queue. does not cancel any currently running jobs */
void tpool_clear(struct thread_pool *tpool);

/* returns the number of queued work items */
int tpool_queued_jobs(struct thread_pool *tpool);
/* returns the number of active (working) threads */
int tpool_active_jobs(struct thread_pool *tpool);
/* returns the number of pending jobs, both in queue and active */
int tpool_pending_jobs(struct thread_pool *tpool);

/* wait for all pending jobs to be completed */
void tpool_wait(struct thread_pool *tpool);
/* wait until the pending jobs are down to the target specified
 * for example, to wait until a single job has been completed:
 *   tpool_wait_pending(tpool, tpool_pending_jobs(tpool) - 1);
 * this interface is slightly awkward to avoid race conditions. */
void tpool_wait_pending(struct thread_pool *tpool, int pending_target);
/* wait for all pending jobs to be completed for up to "timeout" milliseconds */
long tpool_timedwait(struct thread_pool *tpool, long timeout);

/* returns the number of processors on the system.
 * individual cores in multi-core processors are counted as processors.
 */
int tpool_num_processors(void);

#ifdef __cplusplus
}
#endif

#endif	/* THREADPOOL_H_ */