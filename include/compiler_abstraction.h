#ifndef COMPILER_ABSTRACTION_H
#define COMPILER_ABSTRACTION_H

#if defined(__GNUC__)

#define COMPILER_GNU_SUPPORTED       (1u)

#else

#define COMPILER_GNU_SUPPORTED       (0u)

#endif

#if (COMPILER_GNU_SUPPORTED == 1u)

#ifndef RAM_FUNC
#define RAM_FUNC                                              \
    __attribute__((section(".code_ram"), noinline))
#endif

#ifndef RAM_CONST
#define RAM_CONST                                             \
    __attribute__((section(".ram_rodata")))
#endif

#ifndef NO_RETURN
#define NO_RETURN                                             \
    __attribute__((noreturn))
#endif

#ifndef WEAK
#define WEAK                                                  \
    __attribute__((weak))
#endif

#ifndef BOOT_ALIGNED
#define BOOT_ALIGNED(alignmentValue)                          \
    __attribute__((aligned(alignmentValue)))
#endif

#ifndef SECTION
#define SECTION(sectionName)                                  \
    __attribute__((section(sectionName)))
#endif

#ifndef USED
#define USED                                                  \
    __attribute__((used))
#endif

#ifndef UNUSED
#define UNUSED                                                \
    __attribute__((unused))
#endif

#ifndef NO_INLINE
#define NO_INLINE                                             \
    __attribute__((noinline))
#endif

#ifndef PACKED
#define PACKED                                                \
    __attribute__((packed))
#endif

#ifndef FORCE_INLINE
#define FORCE_INLINE                                          \
    static inline __attribute__((always_inline))
#endif

#else

#ifndef RAM_FUNC
#define RAM_FUNC
#endif
#ifndef RAM_CONST
#define RAM_CONST
#endif
#ifndef NO_RETURN
#define NO_RETURN
#endif
#ifndef WEAK
#define WEAK
#endif

#ifndef BOOT_ALIGNED
#define BOOT_ALIGNED(alignmentValue)
#endif
#ifndef SECTION
#define SECTION(sectionName)
#endif
#ifndef USED
#define USED
#endif
#ifndef UNUSED
#define UNUSED
#endif
#ifndef NO_INLINE
#define NO_INLINE
#endif
#ifndef PACKED
#define PACKED
#endif
#ifndef FORCE_INLINE
#define FORCE_INLINE            static inline
#endif

#endif

#ifndef COMPILER_UNUSED
#define COMPILER_UNUSED(value)                                \
    ((void)(value))
#endif

#define COMPILER_STRINGIFY_INTERNAL(value)                    \
    #value

#define COMPILER_STRINGIFY(value)                             \
    COMPILER_STRINGIFY_INTERNAL(value)

#define COMPILER_CONCAT_INTERNAL(left, right)                 \
    left##right

#define COMPILER_CONCAT(left, right)                          \
    COMPILER_CONCAT_INTERNAL(left, right)

#if (COMPILER_GNU_SUPPORTED == 1u)

#ifndef COMPILER_MEMORY_BARRIER
#define COMPILER_MEMORY_BARRIER()                             \
    __asm volatile ("" ::: "memory")
#endif

#else

#ifndef COMPILER_MEMORY_BARRIER
#define COMPILER_MEMORY_BARRIER()                             \
    do                                                        \
    {                                                         \
    } while (0)
#endif

#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)

#define BOOT_STATIC_ASSERT(condition, message)                \
    _Static_assert((condition), message)

#else

#define BOOT_STATIC_ASSERT(condition, message)                \
    typedef char COMPILER_CONCAT(                             \
        boot_static_assert_,                                  \
        __LINE__)[(condition) ? 1 : -1]

#endif

#if (COMPILER_GNU_SUPPORTED != 1u)
#error "Unsupported target compiler"
#endif

#endif /* COMPILER_ABSTRACTION_H */
