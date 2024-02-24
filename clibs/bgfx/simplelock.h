#ifndef SIMPLE_LOCK_H
#define SIMPLE_LOCK_H

#ifdef _MSC_VER

#include <windows.h>
#define inline __inline

#define atom_cas_long(ptr, oval, nval) (InterlockedCompareExchange((LONG volatile *)ptr, nval, oval) == oval)
#define atom_cas_pointer(ptr, oval, nval) (InterlockedCompareExchangePointer((PVOID volatile *)ptr, nval, oval) == oval)
#define atom_inc(ptr) InterlockedIncrement((LONG volatile *)ptr)
#define atom_dec(ptr) InterlockedDecrement((LONG volatile *)ptr)
#define atom_add(ptr, n) InterlockedAdd((LONG volatile *)ptr, n)
#define atom_sync() MemoryBarrier()
#define atom_spinlock(ptr) while (InterlockedExchange((LONG volatile *)ptr , 1)) {}
#define atom_spintrylock(ptr) (InterlockedExchange((LONG volatile *)ptr , 1) == 0)
#define atom_spinunlock(ptr) InterlockedExchange((LONG volatile *)ptr, 0)

#else

#define atom_cas_long(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)
#define atom_cas_pointer(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)
#define atom_inc(ptr) __sync_add_and_fetch(ptr, 1)
#define atom_dec(ptr) __sync_sub_and_fetch(ptr, 1)
#define atom_add(ptr, n) __sync_add_and_fetch(ptr, n)
#define atom_sync() __sync_synchronize()
#define atom_spinlock(ptr) while (__sync_lock_test_and_set(ptr,1)) {}
#define atom_spintrylock(ptr) (__sync_lock_test_and_set(ptr,1) == 0)
#define atom_spinunlock(ptr) __sync_lock_release(ptr)

#endif

typedef int spinlock_t;

/* spin lock */
#define spin_lock_init(Q) (Q)->lock = 0
#define spin_lock_destory(Q)
#define spin_lock(Q) atom_spinlock(&(Q)->lock)
#define spin_unlock(Q) atom_spinunlock(&(Q)->lock)
#define spin_trylock(Q) atom_spintrylock(&(Q)->lock)

/* read write lock */

// 定义一个结构体 rwlock，用于实现读写锁
struct rwlock {
    int write; // 标识是否有写操作正在进行，非零表示有写锁被占用
    int read;  // 记录当前持有读锁的线程数量
};

// 初始化读写锁结构体
static inline void
rwlock_init(struct rwlock *lock) {
    lock->write = 0; // 将写操作标志位初始化为 0，表示没有写操作正在进行
    lock->read = 0;  // 将读操作标志位初始化为 0，表示没有线程持有读锁
}

// 销毁读写锁结构体（实际上这个函数什么也不做）
static inline void
rwlock_destory(struct rwlock *lock) {
    (void)lock; // 忽略参数 lock，表示不使用这个参数
    // 什么也不做
}

// 获取读锁
static inline void
rwlock_rlock(struct rwlock *lock) {
    for (;;) { // 无限循环，直到成功获取读锁为止
        while(lock->write) { // 当有写锁被占用时，进入循环等待
            atom_sync(); // 调用原子同步操作，可能是一种线程同步的机制
        }
        atom_inc(&lock->read); // 增加读锁计数
        if (lock->write) { // 如果在增加读锁期间有写锁被占用
            atom_dec(&lock->read); // 减少读锁计数
        } else { // 如果没有写锁被占用
            break; // 退出循环，表示成功获取读锁
        }
    }
}

// 获取写锁
static inline void
rwlock_wlock(struct rwlock *lock) {
    atom_spinlock(&lock->write); // 调用原子自旋锁操作，锁住写标志位，防止其他线程同时获取写锁
    while(lock->read) { // 当有线程持有读锁时，进入循环等待
        atom_sync(); // 调用原子同步操作，可能是一种线程同步的机制
    }
}

// 释放写锁
static inline void
rwlock_wunlock(struct rwlock *lock) {
    atom_spinunlock(&lock->write); // 调用原子自旋解锁操作，释放写标志位
}

// 释放读锁
static inline void
rwlock_runlock(struct rwlock *lock) {
    atom_dec(&lock->read); // 原子减操作，减少读锁计数
}

#endif

// 这段 C 头文件代码定义了一个简单的自旋锁和读写锁实现，并根据不同的编译环境选择使用相应的原子操作函数。以下是其功能罗列：

// 1. 根据不同的编译环境，选择使用 Windows 或 POSIX 系统下的原子操作函数。
// 2. 定义了自旋锁的操作函数，包括初始化、销毁、加锁、解锁和尝试加锁。
// 3. 定义了读写锁的结构体 `rwlock` 和相应的操作函数，包括初始化、销毁、读锁、写锁和解锁。
// 4. 在读锁和写锁中，使用自旋锁来保证线程安全。
// 5. 代码提供了一些注释来说明每个函数的作用和实现原理。

// 综上所述，该头文件提供了一个简单的自旋锁和读写锁的实现，并提供了相应的操作函数供使用。