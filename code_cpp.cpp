class CGenerateCPP
{
private:
        map<string, state_desc> &states;
        string &p_construct, &s_shutdown;

        void output_typedef(const char *pclass, FILE *out)
        {
                string s = "\n";

                // Define SM states
                char spaces[DEF_STRING_SIZE];
                memset(&spaces, 0x20, DEF_STRING_SIZE);

                int n = 0, def_len = 75;
                for ( map<string, state_desc>::iterator i = states.begin(); i != states.end(); i++, n++ )
                {
                        char buff[DEF_STRING_SIZE];
                        sprintf(buff, "#define state_%s_for_%s", i->first.c_str(), pclass);
                        s += buff;
                        
                        int pos = strlen(buff);
                        spaces[def_len - pos] = 0;
                        sprintf(buff, "%s%d\n", spaces, n);
                        spaces[def_len - pos] = 0x20;
                        s += buff;
                }                                

                s += "\n";

                for ( map<string, state_desc>::iterator i = states.begin(); i != states.end(); i++ )
                {
                        if ( i->second.s_typedef.size() )
                        {
                                s += i->second.s_typedef;
                                s += "\n";
                        }
                }

                if ( s.size() )
                        fprintf( out, "%s", s.c_str() );
        }

        void output_private(FILE *out)
        {
                string s = "int state;\n\n";

                for ( map<string, state_desc>::iterator i = states.begin(); i != states.end(); i++ )
                {
                        if ( i->second.s_private.size() )
                        {
                                s += i->second.s_private;
                                s += "\n";
                        }
                }

                if ( s.size() )
                        fprintf( out, "private:\n%s", s.c_str() );
        }

        void output_public(FILE *out)
        {
                fprintf( out, "public:\n" );

                string s;

                for ( map<string, state_desc>::iterator i = states.begin(); i != states.end(); i++ )
                {
                        if ( i->second.s_public.size() )
                        {
                                s += i->second.s_public;
                                s += "\n";
                        }
                }

                if ( s.size() )
                        fprintf( out, "%s", s.c_str() );
        }

        void output_structors(const char *pclass, const char *lclass, FILE *out)
        {
                fprintf( out, "%s(%s)\n{\n", pclass, p_construct.c_str() );
                fprintf( out, "state = state_%s_for_%s;\n\n", HEADER_STATE_NAME, lclass );

                map<string, state_desc>::iterator sd = states.find(HEADER_STATE_NAME);
                if ( sd != states.end() )
                        if ( sd->second.s_impl.size() ) 
                                fprintf( out, "%s", sd->second.s_impl.c_str() );

                fprintf( out, "}\n\n" );

                if ( s_shutdown.size() ) fprintf( out, "~%s()\n{\n%s}\n\n", pclass, s_shutdown.c_str() );
        }

        void output_switch(FILE *out, const char *lclass)
        {
                // Define states switch
                fprintf( out, "unsigned int branch;\n\n" );

                fprintf( out, "switch (state)\n{\n" );

                int w = 0;
                for ( map<string, state_desc>::iterator i = states.begin(); i != states.end(); i++, w++ )
                {
                        if ( w ) fprintf( out, "\n" );

                        fprintf( out, "case state_%s_for_%s:\n", i->first.c_str(), lclass );

                        state_desc &sd = i->second;

                        if (!i->second.outputs.size())
                        {
                                // call state implementation
                                if ( i->second.simply != i->first )
                                {
                                                fprintf( out, "h_%s();\n", i->second.simply.c_str() );
                                        fprintf( out, "state = state_%s_for_%s;\n", i->second.simply.c_str(), lclass );
                                }
                        } else
                        {
                                // calculate inputs
                                if ( i->second.inputs.size() )
                                {
                                        for ( map<string, string>::iterator k = i->second.inputs.begin();
                                                k != i->second.inputs.end(); k++ )
                                                fprintf( out, "%s_%s = %s;\n", i->first.c_str(), k->first.c_str(),
                                                        k->second.c_str() );

                                        fprintf( out, "\n" );
                                }

                                map<string, int> branch;

                                // calculate branch check
                                uint32_t v = 1;
                                fprintf( out, "branch = " );
                                for ( map<string, string>::iterator k = i->second.inputs.begin();
                                        k != i->second.inputs.end(); k++ )
                                        {
                                                char param[DEF_STRING_SIZE];
                                                sprintf( param, "%s_%s", i->first.c_str(), k->first.c_str() );

                                                if ( v > 1 ) fprintf( out, " +\n" );
                                                fprintf( out, "(%s ? %d : 0)", param, v );
                                                branch[param] = v;
                                                v <<= 1;
                                        }
                                fprintf( out, ";\n\n" );


                                // check for outputs
                                int q = 0;
                                for ( vector<goto_desc>::iterator o = i->second.outputs.begin();
                                        o != i->second.outputs.end(); o++ )
                                {
                                        if ( o->tstate != i->first )
                                        {
                                                if ( q ) fprintf( out, "else " );
                                                fprintf( out, "if (branch == " );

                                                uint32_t c = 0;
                                                for (vector<string>::iterator k = o->ins.begin(); k != o->ins.end(); k++)
                                                {
                                                        char param[DEF_STRING_SIZE];
                                                        sprintf( param, "%s_%s", i->first.c_str(), k->c_str() );

                                                        if ( branch.find(param) == branch.end() )
                                                                throw "E S0030 Not found input used in branch condition";
                                                        c += branch[param];
                                                }
                                        
                                                fprintf( out, "%d)\n{\n", c );                                        
                                                fprintf( out, "h_%s();\n", o->tstate.c_str() );
                                                fprintf( out, "state = state_%s_for_%s;\n", o->tstate.c_str(), lclass );
                                                fprintf( out, "} " );

                                                q++;
                                        }
                                }

                                if (i->second.simply.size() && i->second.simply != i->first )
                                {
                                        fprintf( out, "else\n{\n" );
                                        fprintf( out, "h_%s();\n", i->second.simply.c_str() );
                                        fprintf( out, "state = state_%s_for_%s;\n}\n", i->second.simply.c_str(), lclass );
                                }
                        }

                        fprintf( out, "break;\n" );
                }
        }

        void output_iteration(FILE *out, const char *lclass)
        {
                fprintf( out, "void iteration()\n{\n" );

                // Define input variables
                for ( map<string, state_desc>::iterator i = states.begin(); i != states.end(); i++ )
                {
                        const char *cstate = i->first.c_str();

                        if ( i->second.inputs.size() )
                        {
                                fprintf( out, "int " );

                                int c = 0;
                                for ( map<string, string>::iterator k = i->second.inputs.begin();
                                        k != i->second.inputs.end(); k++, c++ )
                                        if ( c ) fprintf( out, ", %s_%s", cstate, k->first.c_str() );
                                                else fprintf( out, "%s_%s", cstate, k->first.c_str() );

                                fprintf( out, ";\n" );
                        }
                }

                fprintf( out, "\n" );

                output_switch(out, lclass);

                fprintf( out, "}\n}\n\n" );
        }

        void output_states(FILE *out, const char *lclass)
        {
                for ( map<string, state_desc>::iterator i = states.begin(); i != states.end(); i++ )
                {
                        if ( i->first == HEADER_STATE_NAME ) continue;

                        fprintf( out, "void h_%s()\n{\n%s", i->first.c_str(), i->second.s_impl.c_str() );
                        fprintf( out, "}\n\n" );
                }
        }

public:
        CGenerateCPP(map<string, state_desc> &pstates,
                string &pp_construct, string &ps_shutdown) :
                        states(pstates), p_construct(pp_construct), s_shutdown(ps_shutdown)

        {
        }

        void output(const char *file, const char *pclass)
        {
                if (states.find(HEADER_STATE_NAME) == states.end())
                        throw "E S0028 Header state must be defined as entry point";

                FILE *out = fopen(file, "wb");

                // lower case class alias
                char lclass[DEF_STRING_SIZE];
                memset(&lclass, 0, DEF_STRING_SIZE);
                for ( int n = 0; n < strlen(pclass); n++ )
                        lclass[n] = tolower(pclass[n]);

                output_typedef(lclass, out);

                // Start class definition
                fprintf( out, "class %s\n{\n", pclass );

                output_private(out);
                output_public(out);

                output_structors(pclass, lclass, out);

                output_iteration(out, lclass);

                output_states(out, lclass);
                
                fprintf( out, "};\n" );

                fclose(out);

                const char *argv[] = {"(path to exe)", file, "-n", "-w", "-p", "-U"};
                astyle_main(sizeof(argv)/sizeof(argv[0]), (char**)&argv);
        }
};
