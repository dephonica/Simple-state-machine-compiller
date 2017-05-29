#include "stdafx.h"

class CSyntax : public CLex
{
private:
        map<string, state_desc> states;

        string p_construct, s_shutdown;

        ///////////////////////////////////////////////////////////////////////
        /// Parser
        ///////////////////////////////////////////////////////////////////////
        void skip_comment()
        {
                // Is this comment?
                get_char();
                if ( Look() != '*' && Look() != '/' ) throw "E S0009 Invalid symbol found";

                char r_type = Look();
                get_char_low();

                while (1)
                {
                        char c = Real();

                        if ( r_type == '/' )
                        {
                                if ( is_crlf(c) )
                                {
                                        while ( is_crlf(Real()) ) get_char_low();
                                        break;
                                }

                                get_char_low();
                        } else
                        {
                                if ( c == '*' )
                                {
                                        get_char_low();
                                        if ( Real() == '/' )
                                        {
                                                get_char();
                                                break;
                                        }
                                } else get_char_low();
                        }
                }

                skip_white();
        }

        void append_cpp_code(string &target)
        {
                while ( Look() == '/' ) skip_comment();

                match('{');

                int state = STATE_PLAIN, incount = 0;
                char r_type;

                while (1)
                {
                        char c = Real();
                        if ( c == '}' && !incount && state == STATE_PLAIN ) break;

                        if ( state == STATE_PLAIN )
                        {
                                switch (c)
                                {
                                  case '{':
                                        incount++;
                                        break;

                                  case '}':
                                        if ( !incount ) throw "E S0007 Invalid cpp code parser state";
                                        incount--;
                                        break;

                                  case '/':
                                        state = STATE_MUSTBE_REM;
                                        r_type = c;
                                        break;

                                  case '"':
                                  case '\'':
                                        state = STATE_REM;
                                        r_type = c;
                                        break;
                                }
                        } else if ( state == STATE_MUSTBE_REM )
                        {
                                switch (r_type)
                                {
                                  case '/':
                                        if ( c != '*' && c != '/' ) state = STATE_PLAIN;
                                        else 
                                        {
                                                r_type = c;
                                                state = STATE_REM;
                                        }
                                        break;
                                  default:
                                        throw "E S0004 Unknown remark type";
                                }
                        } else if ( state == STATE_MUSTBE_OUTREM )
                        {
                                if ( c == '/' ) state = STATE_PLAIN;
                                        else 
                                        {
                                                state = STATE_REM;
                                                continue;
                                        }
                        } else
                        {
                                if ( (r_type == '"' || r_type == '\'') && c == r_type ) state = STATE_PLAIN;
                                else if ( r_type == '/' && is_crlf(c) ) state = STATE_PLAIN;
                                else if ( r_type == '*' && c == '*' ) state = STATE_MUSTBE_OUTREM;
                        }

                        target += c;

                        get_char_low();
                }

                match('}');
        }

        void fetch_cpp_expr(string &target, char end)
        {
                int state = STATE_PLAIN;
                char r_type;

                while (1)
                {
                        char c = Real();
                        if ( c == end && state == STATE_PLAIN ) break;

                        if ( state == STATE_PLAIN )
                        {
                                switch (c)
                                {
                                  case '/':
                                        state = STATE_MUSTBE_REM;
                                        r_type = c;
                                        break;

                                  case '"':
                                  case '\'':
                                        state = STATE_REM;
                                        r_type = c;
                                        break;
                                }
                        } else if ( state == STATE_MUSTBE_REM )
                        {
                                switch (r_type)
                                {
                                  case '/':
                                        if ( c != '*' && c != '/' ) state = STATE_PLAIN;
                                        else 
                                        {
                                                r_type = c;
                                                state = STATE_REM;
                                        }
                                        break;
                                  default:
                                        throw "E S0014 Unknown remark type";
                                }
                        } else if ( state == STATE_MUSTBE_OUTREM )
                        {
                                if ( c == '/' ) state = STATE_PLAIN;
                                        else 
                                        {
                                                state = STATE_REM;
                                                continue;
                                        }
                        } else
                        {
                                if ( (r_type == '"' || r_type == '\'') && c == r_type ) state = STATE_PLAIN;
                                else if ( r_type == '/' && is_crlf(c) ) state = STATE_PLAIN;
                                else if ( r_type == '*' && c == '*' ) state = STATE_MUSTBE_OUTREM;
                        }

                        target += c;

                        get_char_low();
                }
        }

        void fetch_cpp_params(string &target)
        {
                match('(');
                fetch_cpp_expr(target, ')');
                match(')');
        }

        void parse_inputs(const char *iname, map<string, string> &l_inputs)
        {
                match('=');

                if ( l_inputs.find(iname) != l_inputs.end() ) throw "E S0020 Input name is already used";
                
                string l_expr;
                fetch_cpp_expr(l_expr, ';');
                match(';');

                l_inputs[iname] = l_expr;
        }

        void parse_goto_code(string &simply, vector<goto_desc> &gd)
        {
                if ( Look() == '{' )
                {
                        match('{');
                        
                        while (1)
                        {
                                goto_desc gdi;

                                match('(');

                                char buff[DEF_STRING_SIZE];

                                while (1)
                                {
                                        get_name(buff);
                                        if (!strcmp(buff, "other")) break;

                                        gdi.ins.push_back(buff);

                                        if ( Look() == ')' ) break;
                                        else if ( Look() == ',' ) match(',');
                                        else throw "E S0021 Syntax error in inputs list";
                                }

                                match(')');
                                match('>');

                                if ( strcmp(buff, "other") )
                                {
                                        get_name(buff);
                                        gdi.tstate = buff;

                                        gd.push_back(gdi);
                                } else
                                {
                                        get_name(buff);
                                        simply = buff;
                                }

                                if (Look() == '}') break;
                                else if (Look() == ',') match(',');
                                else throw "E S0022 Syntax error in goto definition";
                        }

                        match('}');
                } else
                {
                        // simple state change
                        char buff[DEF_STRING_SIZE];
                        get_name(buff);
                        simply = buff;
                }

                match(';');
        }

        void state_define(const char *sname)
        {
                uint32_t h_state = hash(sname);

                map<string, string> l_inputs;
                char buff[DEF_STRING_SIZE];

                LOG( "(%s) state definition", sname );

                match('{');

                state_desc d_state;

                while (Look() != '}')
                {
                        while ( Look() == '/' ) skip_comment();

                        get_name(buff);
                        uint32_t token = hash(buff);

                        if (token == h_typedef)
                                append_cpp_code(d_state.s_typedef);
                        else if (token == h_private)
                                append_cpp_code(d_state.s_private);
                        else if (token == h_public)
                                append_cpp_code(d_state.s_public);
                        else if (token == h_impl)
                        {
                                if (Look() == '(') 
                                        if (h_state == h_header) fetch_cpp_params(p_construct);
                                        else throw "E S0025 Implementation parameters enabled in header only";

                                append_cpp_code(d_state.s_impl);
                        }
                        else if (token == h_shut)
                        {
                                if (h_state == h_header) append_cpp_code(s_shutdown);
                                else throw "E S0026 Shutdown definition enabled in header only";
                        }
                        else if (token == h_goto)
                                parse_goto_code(d_state.simply, d_state.outputs);
                        else parse_inputs(buff, d_state.inputs);
                }

                match('}');

                // Save state definition
                if (states.find(sname) != states.end())
                        throw "E S0027 State name is already used";

                // Check for goto defined
                if (!d_state.simply.size())
                        if (!d_state.outputs.size())
                                throw "E S0028 Stage branch (goto) must be strictly defined";
                        else LOG( "W S1001 Default stage branch not defined" );

                states[sname] = d_state;
        }

public:
        CSyntax( CInStream &pin ) :
                CLex(&pin)
        {
        }

        void go(const char *file, const char *pclass)
        {
                int state = STATE_OUTER;

                try
                {
                        char buff[DEF_STRING_SIZE];

                        get_char();
                        skip_white();

                        while (1)
                        {
                                while ( Look() == '/' ) skip_comment();
                                  
                                if ( !eos() )
                                {
                                        get_name(buff);
                                        state_define(buff);
                                } else break;
                        }

                        output(file, pclass);
                } catch (const char *err)
                {
                        LOG( "[%d:%d %c] Error: %s\n", row_n(), col_n() - 1, Look(), err );
                }
        }

        ///////////////////////////////////////////////////////////////////////
        /// Code generation
        ///////////////////////////////////////////////////////////////////////

        void output(const char *file, const char *pclass)
        {
                CGenerateCPP g(states, p_construct, s_shutdown);
                g.output(file, pclass);
        }
};

const char *in_file = NULL;
const char *out_file = NULL;
const char *class_name = "CStateMachine";

void print_help()
{
        printf( "Usage: ssmc.exe <input file> <output file> [class_name]\n\n" );
        exit(1);
}

void parse_args(int argc, char **argv)
{
        printf( "Simple State Machine Compiller v%d.%d build %d\n", 
                __PROG_VERSION, __PROG_SUBVERSION, __PROG_BUILD );
        printf( "(C) 1995-%d Eternal software, inc.\n\n", __BUILD_YEAR );

        if ( argc < 3 ) print_help();

        in_file = argv[1];
        out_file = argv[2];

        if ( argc == 4 ) class_name = argv[3];
}

void main(int argc, char **argv)
{
        parse_args(argc, argv);

        if (!in_file || !out_file)
        {
                printf( "Invalid command line arguments\n\n" );
                exit(2);
        }

        CInStream ins(in_file);
        CSyntax syn(ins);

        syn.go(out_file, class_name);
}
