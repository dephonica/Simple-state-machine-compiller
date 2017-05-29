class CLex
{
private:
        CInStream *ins;
        int row, col;
        char cLook, cReal;

        int saved_pointer;

public:
        CLex( CInStream *pins ) : 
                ins(pins), saved_pointer(0),
                row(1), col(1)
        {
        }
                                   
        int row_n() const                 { return row; }
        int col_n() const                 { return col; }

        inline char Look() const          { return cLook; }
        inline char Real() const          { return cReal; }

        inline void save_point()          { saved_pointer = ins->get_position(); }
        inline void restore_point() const { ins->set_position(saved_pointer); }

        inline int  eos() const           { return ins->is_eos(); }

        void get_char_low()
        {
                if ( ins->is_eos() )
                {
                        cLook = 0x20;
                        throw "A LX1001 Reached end of file";
                }

                char cPrev = cLook;
                cLook = cReal = ins->getc();

                if ( is_crlf(cLook) )
                {
                        if ( cLook == 0x0a ) row++;
                        col = 1;
                        cLook = 0x20;
                } else col++;
        }

        void get_char()
        {
                if ( ins->is_eos() )
                {
                        cLook = 0x20;
                        throw "A LX1001 Reached end of file";
                }

                cLook = cReal = ins->getc();

                if ( is_crlf(cLook) )
                {
                        row++;
                        col = 1;

                        // skip_crlf();
                        char first = cLook;
                        char side = first == 0x0a ? 0x0d : 0x0a;

                        while (1)
                        {
                                cLook = cReal = ins->getc();
                                if ( cLook == side ) continue;
                                if ( cLook == first )
                                {
                                        row++;
                                        continue;
                                }

                                break;
                        }

                } 
                
                col++;

                cLook = tolower( cLook );
        }

        inline int is_crlf( char c ) const  { return ( c == 0x0a || c == 0x0d ); }
        inline int is_white( char c ) const { return ( c == 0x20 || c == 0x09 ); }
        inline int is_alpha( char c ) const { return ( c >= 'a' && c <= 'z' ) ||
                                                     ( c >= 'A' && c <= 'Z' ) ||
                                                     ( c == '_' );               }
        inline int is_digit( char c ) const { return ( c >= '0' && c <= '9' );   }
        inline int is_alnum( char c ) const { return is_alpha(c) || is_digit(c); }
        inline int is_op( char c ) const
        {
                return c == '+' || c == '-' || c == '*' || c == '/' ||
                       c == '<' || c == '>' || c == ':' || c == '=' ||
                       c == '!' || c == '&' || c == '|';
        }

        void skip_white()              { while ( is_white(cLook) ) get_char(); }

        void skip_comma()
        {
                skip_white();
                if ( cLook == ',' )
                {
                        get_char();
                        skip_white();
                }
        }

        void skip_line()
        {
                int cur_row = row;
                while ( row == cur_row ) get_char();
                skip_white();
        }

        char *get_num( char *buff )
        {
                int ptr = 0;
                buff[0] = 0;
                if ( !is_digit(cLook) ) throw "E LX0001 Expected number";

                while ( is_digit(cLook) )
                {
                        buff[ptr++] = cLook;
                        get_char();
                }
                buff[ptr] = 0;
                skip_white();
                return buff;
        }

        char *get_string( char *buff )
        {
                int ptr = 0;
                buff[0] = 0;
                if ( cLook != '"' ) throw "E LX0002 Expected string";

                get_char();
                while ( cLook != '"' )
                {
                        buff[ptr++] = cReal;
                        get_char();
                }

                buff[ ptr ] = 0;
                skip_white();

                return buff;
        }

        char *get_name( char *buff )
        {
                int ptr = 0;
                buff[0] = 0;
                if ( !is_alpha(cLook) ) throw "E LX0003 Expected name";

                while ( is_alnum(cLook) )
                {
                        buff[ptr++] = cLook;
                        get_char();
                }

                buff[ptr] = 0;
                skip_white();

                return buff;
        }

        char *get_op( char *buff )
        {
                if ( !is_op(cLook) ) throw "E LX0004 Expected operator";

                int ptr = 0;
                buff[0] = 0;

                while ( is_op(cLook) )
                {
                        buff[ptr++] = cLook;
                        get_char();
                }
                buff[ptr] = 0;
                skip_white();
                return buff;
        }

        void match( char c )
        {                
                skip_white();
                if ( cLook != c )
                {
                        char *msg = new char[DEF_STRING_SIZE];
                        sprintf( msg, "E LX0005 Expected character '%c'", c );
                        throw msg;
                }
                get_char();
                skip_white();
        }
};
