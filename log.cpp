#define LOG_BUFF_SIZE   16384

class CLog
{
        FILE *log_file;
        char *log_name, *buff, time[256];

        void refresh_time()
        {
                SYSTEMTIME st;
                GetLocalTime( &st );

                sprintf( time, "[%02d/%02d/%04d %02d:%02d:%02d.%03d]", st.wDay, st.wMonth, st.wYear,
                        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
        }

public:
        CLog( const char *name ) :
                log_name(NULL), buff(NULL)
        {
                log_file = NULL;
                if ( !name ) throw "E LG0001 Invalid file name";

                log_name = new char[strlen(name) + 1];
                if ( !log_name ) throw "E LG0002 Unable to allocate memory for log file name";
                strcpy( log_name, name );

                buff = new char[LOG_BUFF_SIZE];
                if ( !buff ) throw "E LG0003 Unable to allocate memory for log buffer";
        }

        ~CLog()
        {
                delete []log_name;
                delete []buff;
        }

        void reopen_logfile()
        {
                if ( log_file ) fclose( log_file );
                if ( log_name[0] == 0 ) return;
                log_file = fopen( log_name, "ab" );
        }

        int Log( const char *fmt, ... )
        {
                if ( !log_file ) reopen_logfile();

                va_list ap;

                va_start( ap, fmt );
                _vsnprintf ( buff, LOG_BUFF_SIZE, fmt, ap );
                va_end( ap );

                refresh_time();

                if ( log_file )
                        fprintf( log_file, "%s - %s\015\012", time, buff );

                printf( "%s - %s\015\012", time, buff );

                return 0;
        }
};
