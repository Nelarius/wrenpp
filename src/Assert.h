#ifndef ASSERT_H_INCLUDED
#define ASSERT_H_INCLUDED

#include <cstdlib>
#include <iostream>

//defined as a do-while so that a semicolon can be used
#ifdef DEBUG
    #define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion '" #condition "' failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::exit(EXIT_FAILURE); \
        } \
    } while (false) \

#else
    #define ASSERT(condition, message) do {} while (false)
#endif


#endif // ASSERT_H_INCLUDED
