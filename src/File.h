#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include "Assert.h"
#include <sys/stat.h>

namespace wrenly {

    /// \brief Check whether a file of the given name exists.
    inline bool FileExists( const std::string& file ) {
        struct stat buffer;
        return ( stat( file.c_str(), &buffer) == 0 );
    }

    /// \brief Get the contents of a file as a string.
    inline std::string FileToString( const std::string& file ) {
        std::ifstream fin;
        
        if ( !FileExists( file ) ) {
            throw std::runtime_error( "file not found!" );
        }
        
        fin.open( file, std::ios::in );

        std::stringstream buffer;
        buffer << fin.rdbuf();
        return buffer.str();
    }

}

#endif // FILE_H_INCLUDED
