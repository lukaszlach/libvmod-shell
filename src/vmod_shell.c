#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <ctype.h>

#include "popen_plus.c"
#include "cache/cache.h"
#include "vrt.h"
#include "vcl.h"

#include "vtim.h"
#include "vcc_shell_if.h"

struct vmod_shell_exec {
    unsigned magic;
    #define VMOD_SHELL_EXEC_MAGIC 0x12d7afe3
    struct popen_plus_process* process;
    pthread_mutex_t process_mtx;
    int is_available;
};

VCL_VOID
vmod_exec__init(VRT_CTX, struct vmod_shell_exec **shell,
		     const char *vcl_name, VCL_STRING cmd)
{
    struct vmod_shell_exec *local_shell;

    AN(cmd);
    ALLOC_OBJ(local_shell, VMOD_SHELL_EXEC_MAGIC);
    local_shell->process = popen_plus(cmd);
    AN(local_shell->process);
    VSL(SLT_Debug, 0, "Executed '%s', pid %d", cmd, local_shell->process->pid);
    setvbuf(local_shell->process->write_fp, NULL, _IONBF, 0);
    setvbuf(local_shell->process->read_fp,  NULL, _IOLBF, 1024);
    pthread_mutex_init(&local_shell->process_mtx, NULL);
    *shell = local_shell;
}

VCL_VOID
vmod_exec__fini(struct vmod_shell_exec **shell)
{
    struct vmod_shell_exec *local_shell;

    if (shell == NULL || *shell == NULL)
        return;
    local_shell = *shell;
    if (local_shell->process == NULL)
        return;
    popen_plus_kill(local_shell->process);
    popen_plus_close(local_shell->process);
    free(local_shell->process);
    pthread_mutex_destroy(&local_shell->process_mtx);
    FREE_OBJ(local_shell);
}

//

void rtrim(char * s) {
    char * p = s;
    int l = strlen(p);

    while(isspace(p[l - 1])) p[--l] = 0;
    memmove(s, p, l + 1);
}

VCL_STRING exec_read(struct vmod_shell_exec *shell)
{
    char *line = NULL;
    size_t len;
    ssize_t read;

    read = getline(&line, &len, shell->process->read_fp);
    assert(len >= 0);
    if (read == -1)
        return "";
    rtrim(line);

    return line;
}

VCL_STRING
vmod_exec_read(VRT_CTX, struct vmod_shell_exec *shell)
{
    VCL_STRING result;

    AZ(pthread_mutex_lock(&shell->process_mtx));
    result = exec_read(shell);
    AZ(pthread_mutex_unlock(&shell->process_mtx));

    return result;
}

int exec_write(struct vmod_shell_exec *shell, VCL_STRING value)
{
    if (shell->process == NULL) {
        return 0;
    }
    AN(value);
    fputs(value, shell->process->write_fp);
    return '\n' == fputc('\n', shell->process->write_fp);
}

VCL_BOOL
vmod_exec_write(VRT_CTX, struct vmod_shell_exec *shell, VCL_STRING value)
{
    int result;

    AZ(pthread_mutex_lock(&shell->process_mtx));
    result = exec_write(shell, value);
    AZ(pthread_mutex_unlock(&shell->process_mtx));

    return result;
}

VCL_STRING
vmod_exec_cmd(VRT_CTX, struct vmod_shell_exec *shell, VCL_STRING value)
{
    VCL_STRING result = NULL;

    AZ(pthread_mutex_lock(&shell->process_mtx));
    if (exec_write(shell, value))
        result = exec_read(shell);
    AZ(pthread_mutex_unlock(&shell->process_mtx));

    return result;
}

VCL_INT
vmod_exec_pid(VRT_CTX, struct vmod_shell_exec *shell)
{
    return shell->process->pid;
}

VCL_STRING vmod_exec_once(VRT_CTX, VCL_STRING cmd)
{
    FILE *fd;
    fd = popen(cmd, "r");
    if (!fd)
        return "";

    char   buffer[256];
    size_t read;
    size_t buffer_size = 256;
    size_t result_len = 0;
    char  *result = malloc(buffer_size);

    while ((read = fread(buffer, 1, sizeof(buffer), fd)) != 0) {
        if (result_len + read >= buffer_size) {
            buffer_size *= 2;
            result = realloc(result, buffer_size);
        }
        memmove(result + result_len, buffer, read);
        result_len += read;
    }
    pclose(fd);
    rtrim(result);

    return result;
}