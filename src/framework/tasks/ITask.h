/*
 Copyright (C) 2009 Erik Hjortsberg

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef TASK_H_
#define TASK_H_

namespace Ember
{

namespace Tasks
{

class TaskExecutionContext;

/**
 * @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 * @brief Base interface for a "task".
 * A "task" is a piece of work which needs to be carried out in a separate thread. Instances of this are processed by an instance of TaskQueue, which uses a collection of concurrant TaskExecutors to execute the tasks.
 * When each task has been executed in the background threads (through the executors) it can optionally also be executed in the main thread through a call to executeTaskInMainThread().
 */
class ITask
{
public:
	/**
	 * @brief Dtor.
	 */
	virtual ~ITask()
	{
	}
	;

	/**
	 * @brief Executes the task in a background thread. This is where the bulk of the work should happen.
	 * @param context The context in which the task executes.
	 */
	virtual void executeTaskInBackgroundThread(TaskExecutionContext context) = 0;

	/**
	 * @brief Executes the task in the main thread, after executeTaskInBackgroundThread() has been called.
	 * Since this will happen in the main thread you shouldn't do any time consuming things here, since it will lock up the rendering.
	 */
	virtual void executeTaskInMainThread()
	{
	}
	;
};

}

}

#endif /* TASK_H_ */