class CInStream
{
private:
        FILE *in;

public:
        CInStream( const char *file )   { in = fopen( file, "rb" ); }
        ~CInStream()                    { fclose(in); }

        int is_eos()
        {
                if ( !in ) return true;
                return feof(in);
        }

        char getc()
        {
                if ( !in ) return 0x20;
                return fgetc(in);
        }

        int get_position()              { return ftell(in); }
        void set_position(int pos)      { if ( in ) fseek(in, pos, 0); }
};
