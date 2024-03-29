#ifndef THREADING_H_INCLUDED
#define THREADING_H_INCLUDED


#include <boost/noncopyable.hpp>
#include <algorithm>
#include <pthread.h>


/**
 * Mutex class for the posix platform
 */
class Mutex : boost::noncopyable
{
public:
    Mutex() { pthread_mutex_init(&mMutex, NULL); }

    ~Mutex() { pthread_mutex_destroy(&mMutex); }

    void lock() { pthread_mutex_lock(&mMutex); }

    void unlock() { pthread_mutex_unlock(&mMutex); }

private:
    pthread_mutex_t mMutex;
};


/**
 * The ScopedLock keeps a Mutex object locked during its lifetime.
 */
class ScopedLock : boost::noncopyable
{
public:
    ScopedLock(Mutex & inMutex) :
        mMutex(inMutex)
    {
        mMutex.lock();
    }

    ~ScopedLock()
    {
        mMutex.unlock();
    }

private:
    Mutex & mMutex;
};


// Forward declaration.
template<class> class ScopedAccessor;


/**
 * ThreadSafe can be used to create a thread-safe object.
 * Access to the held object can be obtained by creating a ScopedAccessor object.
 */
template<class VariableT>
class ThreadSafe
{
public:
    typedef VariableT Variable;

    ThreadSafe() :
        mData(new Data(new Variable()))
    {
    }

    ThreadSafe(Variable * inVariable) :
        mData(new Data(inVariable))
    {
    }

    ThreadSafe(const ThreadSafe<Variable> & rhs) :
        mData(rhs.mData)
    {
        ++mData->mRefCount;
    }

    ThreadSafe<Variable> & operator=(const ThreadSafe<Variable> & rhs)
    {
        // Using the copy & swap idiom:
        ThreadSafe<Variable> copy(rhs);
        swap(copy);
        return *this;
    }

    ~ThreadSafe()
    {
        if (--mData->mRefCount == 0)
        {
            delete mData;
        }
    }

    void swap(ThreadSafe<Variable> & rhs)
    {
        std::swap(mData, rhs.mData);
    }

private:
    friend class ScopedAccessor<Variable>;

    Mutex & getMutex() { return mData->mMutex; }

    const Variable & getVariable() const { return *mData->mVariable; }

    Variable & getVariable() { return *mData->mVariable; }

    struct Data : boost::noncopyable
    {
        Data(Variable * inVariable) :
            mVariable(inVariable),
            mMutex(),
            mRefCount(1)
        {
        }

        ~Data()
        {
            delete mVariable;
        }

        Variable * mVariable;
        Mutex mMutex;
        unsigned mRefCount;
    };

    Data * mData;
};


/**
 * ScopedAccessor creates an atomic scope that allows access
 * to the variable held by the ThreadSafe wrapper.
 */
template<typename Variable>
class ScopedAccessor : boost::noncopyable
{
public:
    ScopedAccessor(ThreadSafe<Variable> & inThreadSafeVariable) :
        mThreadSafeVariable(inThreadSafeVariable),
        mScopedLock(inThreadSafeVariable.getMutex())
    {
    }

    const Variable & get() const { return mThreadSafeVariable.getVariable(); }

    Variable & get() { return mThreadSafeVariable.getVariable(); }

    const Variable * operator->() const { return &mThreadSafeVariable.getVariable(); }

    Variable * operator->() { return &mThreadSafeVariable.getVariable(); }

private:
    ThreadSafe<Variable> & mThreadSafeVariable;
    ScopedLock mScopedLock;
};


/**
 * Utility for creating an atomic scope.
 * Usage example:
 *
 *   ThreadSafe<Foo> foo;
 *   ATOMIC_SCOPE(Foo, foo) {
 *     // foo has type "Foo &"
 *     foo.bar();
 *   }
 */
#define FOR_BLOCK(DECL) if(bool _c_ = false) ; else for(DECL;!_c_;_c_=true)

#define ATOMIC_SCOPE(Type, name) \
    FOR_BLOCK(ScopedAccessor<Type> accessor(name)) \
        FOR_BLOCK(Type & name = accessor.get())


#endif // THREADING_H_INCLUDED
