//
// ActiveDispatcher.h
//
// $Id: //poco/1.3/Foundation/include/Poco/ActiveDispatcher.h#2 $
//
// Library: Foundation
// Package: Threading
// Module:  ActiveObjects
//
// Definition of the ActiveDispatcher class.
//
// Copyright (c) 2006-2007, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#ifndef Foundation_ActiveDispatcher_INCLUDED
#define Foundation_ActiveDispatcher_INCLUDED


#include "Poco/Foundation.h"
#include "Poco/Runnable.h"
#include "Poco/Thread.h"
#include "Poco/ActiveStarter.h"
#include "Poco/ActiveRunnable.h"
#include "Poco/NotificationQueue.h"


namespace Poco {


class Foundation_API ActiveDispatcher: protected Runnable
	/// This class is used to implement an active object
	/// with strictly serialized method execution.
	///
	/// An active object, with is an ordinary object
	/// containing ActiveMethod members, executes all
	/// active methods in their own thread. 
	/// This behavior does not fit the "classic"
	/// definition of an active object, which serializes
	/// the execution of active methods (in other words,
	/// only one active method can be running at any given
	/// time).
	///
	/// Using this class as a base class, the serializing
	/// behavior for active objects can be implemented.
	/// 
	/// The following example shows how this is done:
	///
	///     class ActiveObject: public ActiveDispatcher
	///     {
	///     public:
	///         ActiveObject():
	///             exampleActiveMethod(this, &ActiveObject::exampleActiveMethodImpl)
	///         {
	///         }
	///
	///         ActiveMethod<std::string, std::string, ActiveObject, ActiveStarter<ActiveDispatcher> > exampleActiveMethod;
	///
	///     protected:
	///         std::string exampleActiveMethodImpl(const std::string& arg)
	///         {
	///             ...
	///         }
	///     };
	///
	/// The only things different from the example in
	/// ActiveMethod is that the ActiveObject in this case
	/// inherits from ActiveDispatcher, and that the ActiveMethod
	/// template for exampleActiveMethod has an additional parameter,
	/// specifying the specialized ActiveStarter for ActiveDispatcher.
{
public:
	ActiveDispatcher();
		/// Creates the ActiveDispatcher.

	ActiveDispatcher(Thread::Priority prio);
		/// Creates the ActiveDispatcher and sets
		/// the priority of its thread.

	virtual ~ActiveDispatcher();
		/// Destroys the ActiveDispatcher.

	void start(ActiveRunnableBase::Ptr pRunnable);
		/// Adds the Runnable to the dispatch queue.

	void cancel();
		/// Cancels all queued methods.
		
protected:
	void run();
	void stop();

private:
	Thread            _thread;
	NotificationQueue _queue;
};


template <>
class ActiveStarter<ActiveDispatcher>
	/// A specialization of ActiveStarter
	/// for ActiveDispatcher.
{
public:
	static void start(ActiveDispatcher* pOwner, ActiveRunnableBase::Ptr pRunnable)
	{
		pOwner->start(pRunnable);
	}
};


} // namespace Poco


#endif // Foundation_ActiveDispatcher_INCLUDED