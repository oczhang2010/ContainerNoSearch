#ifndef UTIL_ERROR_H_H
#define UTIL_ERROR_H_H

#ifdef __GNUC__
#define Error( msg ) sfu::error( sfu::Exception(msg, __func__, __FILE__, __LINE__) )
#define Error_( args ) sfu::error( sfu::Exception(sfu::format args, __func__, __FILE__, __LINE__) )
#define Assert( expr ) { if(!(expr)) sfu::error( sfu::Exception(#expr, __func__, __FILE__, __LINE__) ); }
#else
#define Error( msg , func ) sfu::error( sfu::Exception(msg, func, __FILE__, __LINE__) )
#define Error_( args , func ) sfu::error( sfu::Exception(sfu::format args, func, __FILE__, __LINE__) )
#define Assert( expr , func ) { if(!(expr)) sfu::error( sfu::Exception(#expr, func, __FILE__, __LINE__) ); }
#endif

namespace sfu {

using std::string;

class Exception
{
public:
    Exception() { line = 0; }
    Exception( const string& _err, const string& _func, const string& _file, int _line)
        : err(_err), func(_func), file(_file), line(_line) {}
    Exception(const Exception& exc)
        : err(exc.err), func(exc.func), file(exc.file), line(exc.line) {}
    Exception& operator = (const Exception& exc)
    {
        if( this != &exc )
        {
            err = exc.err; func = exc.func; file = exc.file; line = exc.line;
        }
        return *this;
    }

    string err;
    string func;
    string file;
    int line;
};

string format( const char* fmt, ... );
void error( const Exception& exc );

}

#endif