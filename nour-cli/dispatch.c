#include "dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Flag *flag_find(Flag *flags, const char *name) {
    for (i32 i = 0; flags[i].name; i++)
        if (strcmp(flags[i].name, name) == 0) return &flags[i];
    return NULL;
}

static Flag *flag_find_short(Flag *flags, char c) {
    for (i32 i = 0; flags[i].name; i++)
        if (flags[i].shorthand == c) return &flags[i];
    return NULL;
}

static Command *find_cmd(Command **cmds, const char *name) {
    for (i32 i = 0; cmds[i]; ++i) {
        if (strcmp(cmds[i]->name, name) == 0) return cmds[i];
        if (cmds[i]->alias && strcmp(cmds[i]->alias, name) == 0) return cmds[i];
    }
    return NULL;
}

static void print_flags(Flag *flags, const char *header) {
    if (!flags || !flags[0].name) return;
    printf("\n%s\n", header);
    for (i32 i = 0; flags[i].name; i++) {
        Flag *f = &flags[i];
        char sh[6] = "    ";
        if (f->shorthand) snprintf(sh, sizeof(sh), "-%c, ", f->shorthand);
        const char *th = f->type == FLAG_STR   ? " <string>" :
                         f->type == FLAG_INT   ? " <int>"    :
                         f->type == FLAG_FLOAT ? " <float>"  : "";
        printf("   %s--%-16s%s  %s", sh, f->name, th, f->usage ? f->usage : "");
        switch (f->type) {
            case FLAG_BOOL:  if (f->val.b) printf(" (default: true)"); break;
            case FLAG_STR:   if (f->val.s && *f->val.s) printf(" (default: \"%s\")", f->val.s); break;
            case FLAG_INT:   if (f->val.i) printf(" (default: %d)", f->val.i); break;
            case FLAG_FLOAT: if (f->val.f != 0.0f) printf(" (default: %.2g)", f->val.f); break;
        }
        if (f->required) printf(" [required]");
        printf("\n");
    }
}

static void print_global_flags(void) {
    printf("\nGLOBAL OPTIONS:\n");
    printf("   -h, --%-16s  show help\n",       "help");
    printf("       --%-16s  print version\n",   "version");
}

static void help_app(App *app) {
    printf("   %s - %s\n", app->name, app->description ? app->description : "");
    printf("\nUSAGE:\n   %s [global options] [command [command options]]\n", app->name);

    if (app->version) {
        printf("\nVERSION:\n   %s\n", app->version);
    }

    printf("\nCOMMANDS:\n");
    for (i32 i = 0; app->commands[i]; ++i) {
        Command *cmd = app->commands[i];
        char label[64];

        if (cmd->alias) {
            snprintf(label, sizeof(label), "%s, %s", cmd->name, cmd->alias);
        } else {
            snprintf(label, sizeof(label), "%s", cmd->name);
        }

        printf("   %-16s %s\n",
               label,
               cmd->usage ? cmd->usage : "");
    }

    print_global_flags();
}

static void help_cmd(App *app, Command *cmd) {
    printf("   %s %s - %s\n", app->name, cmd->name,
           cmd->description ? cmd->description : (cmd->usage ? cmd->usage : ""));
    printf("\nUSAGE:\n   %s %s [options]\n", app->name, cmd->name);
    if (cmd->subcommands[0]) {
        printf("\nCOMMANDS:\n");
        for (i32 i = 0; cmd->subcommands[i]; ++i)
            printf("   %-16s %s\n", cmd->subcommands[i]->name,
                   cmd->subcommands[i]->usage ? cmd->subcommands[i]->usage : "");
    }
    print_flags(cmd->flags, "OPTIONS:");
    print_global_flags();
}

static i32 parse_flags(Command *cmd, i32 argc, char **argv, i32 start, Context *ctx) {
    for (i32 i = 0; cmd->flags[i].name; i++) cmd->flags[i]._set = false;
    ctx->n_args = 0;

    for (i32 i = start; i < argc; ) {
        const char *arg = argv[i];

        if (strcmp(arg, "--") == 0) {
            i++;
            while (i < argc && ctx->n_args < MAX_ARGS)
                ctx->args[ctx->n_args++] = argv[i++];
            break;
        }
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) return -2;

        if (arg[0] == '-' && arg[1] == '-') {
            const char *key = arg + 2;
            const char *eq  = strchr(key, '=');
            char buf[128]; const char *inline_val = NULL;
            if (eq) {
                size_t len = (size_t)(eq - key);
                if (len >= sizeof(buf)) { fprintf(stderr,"error: flag name too long\n"); return -1; }
                memcpy(buf, key, len); buf[len] = '\0'; inline_val = eq + 1;
            } else { strncpy(buf, key, sizeof(buf)-1); buf[sizeof(buf)-1] = '\0'; }

            Flag *f = flag_find(cmd->flags, buf);
            if (!f) { fprintf(stderr,"error: unknown flag '--%s'\n", buf); return -1; }
            f->_set = true;
            if (f->type == FLAG_BOOL) {
                f->val.b = inline_val ? (strcmp(inline_val,"true")==0||strcmp(inline_val,"1")==0) : true;
                i++; continue;
            }
            const char *val = inline_val;
            if (!val) {
                if (i+1>=argc){fprintf(stderr,"error: '--%s' requires a value\n",buf);return -1;}
                val=argv[++i];
            }
            switch(f->type){
                case FLAG_STR:   f->val.s=val; break;
                case FLAG_INT:{  char *e; f->val.i=(i32)strtol(val,&e,10);
                                 if(*e){fprintf(stderr,"error: '--%s' expects integer\n",buf);return -1;} break;}
                case FLAG_FLOAT:{char *e; f->val.f=(f32)strtof(val,&e);
                                 if(*e){fprintf(stderr,"error: '--%s' expects number\n",buf); return -1;} break;}
                default: break;
            }
            i++;

        } else if (arg[0]=='-' && arg[1] && arg[1]!='-') {
            i32 j=1;
            while (arg[j]) {
                char sc=arg[j];
                Flag *f=flag_find_short(cmd->flags,sc);
                if(!f){fprintf(stderr,"error: unknown flag '-%c'\n",sc);return -1;}
                f->_set=true;
                if(f->type==FLAG_BOOL){f->val.b=true;j++;continue;}
                const char *val;
                if(arg[j+1]){val=arg+j+1;j=(i32)strlen(arg);}
                else{
                    if(i+1>=argc){fprintf(stderr,"error: '-%c' requires a value\n",sc);return -1;}
                    val=argv[++i];j=(i32)strlen(arg);
                }
                switch(f->type){
                    case FLAG_STR:   f->val.s=val; break;
                    case FLAG_INT:{  char *e;f->val.i=(i32)strtol(val,&e,10);
                                     if(*e){fprintf(stderr,"error: '-%c' expects integer\n",sc);return -1;}break;}
                    case FLAG_FLOAT:{char *e;f->val.f=(f32)strtof(val,&e);
                                     if(*e){fprintf(stderr,"error: '-%c' expects number\n",sc);return -1;}break;}
                    default:break;
                }
            }
            i++;
        } else {
            if(ctx->n_args>=MAX_ARGS){fprintf(stderr,"error: too many arguments\n");return -1;}
            ctx->args[ctx->n_args++]=arg; i++;
        }
    }

    for (i32 i = 0; cmd->flags[i].name; i++)
        if (cmd->flags[i].required && !cmd->flags[i]._set) {
            fprintf(stderr,"error: required flag '--%s' not provided\n",cmd->flags[i].name);
            return -1;
        }
    return 0;
}

DispatchResult dispatch(App *app, i32 argc, char **argv) {
    if (argc < 2) { help_app(app); return DISPATCH_OK; }
    if (strcmp(argv[1],"--help")==0||strcmp(argv[1],"-h")==0) { help_app(app); return DISPATCH_OK; }
    if (strcmp(argv[1],"--version")==0) {
        if (app->version) printf("%s v%s\n", app->name, app->version);
        return DISPATCH_OK;
    }

    Command  *cmd       = NULL;
    Command **search_in = app->commands;
    i32       arg_start = 1;

    while (arg_start < argc) {
        const char *tok = argv[arg_start];
        if (tok[0] == '-') break;
        Command *found = find_cmd(search_in, tok);
        if (!found) break;
        cmd = found; search_in = cmd->subcommands; arg_start++;
    }

    if (!cmd) {
        fprintf(stderr,"Unknown command: %s\n", argv[1]);
        fprintf(stderr,"Run '%s --help' for usage.\n", app->name);
        return DISPATCH_ERR;
    }
    if (!cmd->action) { help_cmd(app, cmd); return DISPATCH_OK; }

    Context ctx = { .cmd=cmd, .app=app };
    i32 rc = parse_flags(cmd, argc, argv, arg_start, &ctx);
    if (rc == -2) { help_cmd(app, cmd); return DISPATCH_OK; }
    if (rc == -1) { fprintf(stderr,"Run '%s %s --help' for usage.\n",app->name,cmd->name); return DISPATCH_ERR; }

    return cmd->action(&ctx) == 0 ? DISPATCH_OK : DISPATCH_ERR;
}

bool        flag_bool (Context *ctx, const char *n) { Flag *f=flag_find(ctx->cmd->flags,n); return f?f->val.b:false; }
const char *flag_str  (Context *ctx, const char *n) { Flag *f=flag_find(ctx->cmd->flags,n); return f?f->val.s:NULL;  }
i32         flag_int  (Context *ctx, const char *n) { Flag *f=flag_find(ctx->cmd->flags,n); return f?f->val.i:0;     }
f32         flag_float(Context *ctx, const char *n) { Flag *f=flag_find(ctx->cmd->flags,n); return f?f->val.f:0.0f;  }