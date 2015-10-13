#ifndef ARGUMENTLISTSTRING_H_INCLUDED
#define ARGUMENTLISTSTRING_H_INCLUDED

#include <cstdlib>
#include <initializer_list>
#include <iostream>

namespace wrenly {
namespace detail {
    
struct Any {
    Any( bool e )   : type( 'b' ) {}
    Any( double e ) : type( 'd' ) {}
    Any( float e )  : type( 'd' ) {}
    Any( int e )    : type( 'i' ) {}
    Any( const char* ) : type( 's' ) {}
    Any( const std::string& ) : type( 's' ) {} 
    template< typename T > Any( const T& t ) : type( 'v' ) {}
    
    char type;
};

/*
 * This is a fixed-size class, which takes an initializer_list of arguments,
 * and converts them into a string of argument ids for wren to consume.
 */
template< std::size_t N > 
class ArgumentListString {
    public:
        ArgumentListString( std::initializer_list<Any> l ) {
            int i{0};
            for ( auto a: l ) {
                args[ i ] = a.type;
                i++;
            }
            args[N] = '\0';
        }
        ArgumentListString()                                            = delete;
        ArgumentListString( const ArgumentListString& )                 = delete;
        ArgumentListString( ArgumentListString&& )                      = delete;
        ArgumentListString& operator=( const ArgumentListString& )      = delete;
        ArgumentListString& operator=( ArgumentListString&& )           = delete;
        ~ArgumentListString() = default;
        
        const char* getString() const {
            return args;
        }
        
    private:
        char args[N+1];
};   
}   // namespace detail
}   // namespace wrenly

#endif  // ARGUMENTLISTSTRING_H_INCLUDED